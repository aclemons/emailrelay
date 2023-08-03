//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ===
///
/// \file gstoredfile.cpp
///

#include "gdef.h"
#include "gfilestore.h"
#include "gstoredfile.h"
#include "gscope.h"
#include "gfile.h"
#include "gfbuf.h"
#include "gstr.h"
#include "glog.h"
#include "gassert.h"
#include <fstream>
#include <type_traits>
#include <limits>
#include <cstdio> // BUFSIZ
#include <utility>

GStore::StoredFile::StoredFile( FileStore & store , const MessageId & id , State state ) :
	m_store(store) ,
	m_id(id) ,
	m_state(state) ,
	m_unlock(false)
{
}

GStore::StoredFile::~StoredFile()
{
	try
	{
		if( m_unlock && m_state == State::Locked )
		{
			G_DEBUG( "GStore::StoredFile::dtor: unlocking envelope [" << epath(State::Locked).basename() << "]" ) ;
			FileOp::rename( epath(State::Locked) , epath(State::Normal) ) ;
			static_cast<MessageStore&>(m_store).updated() ;
		}
	}
	catch(...) // dtor
	{
	}
}

void GStore::StoredFile::noUnlock()
{
	m_unlock = false ;
}

GStore::MessageId GStore::StoredFile::id() const
{
	return m_id ;
}

std::string GStore::StoredFile::location() const
{
	return cpath().str() ;
}

G::Path GStore::StoredFile::cpath() const
{
	return m_store.contentPath( m_id ) ;
}

G::Path GStore::StoredFile::epath( State state ) const
{
	return m_store.envelopePath( m_id , state ) ;
}

GStore::MessageStore::BodyType GStore::StoredFile::bodyType() const
{
	return m_env.body_type ;
}

void GStore::StoredFile::close()
{
	m_content.reset() ;
}

std::string GStore::StoredFile::reopen()
{
	std::string reason = "error" ;
	if( !readEnvelope(reason) || !openContent(reason) )
		return reason ;
	else
		return std::string() ;
}

bool GStore::StoredFile::readEnvelope( std::string & reason )
{
	try
	{
		m_env = FileStore::readEnvelope( epath(m_state) ) ;
		return true ;
	}
	catch( std::exception & e ) // invalid file in store
	{
		reason = e.what() ;
		return false ;
	}
}

bool GStore::StoredFile::openContent( std::string & reason )
{
	try
	{
		G_DEBUG( "GStore::FileStore::openContent: reading content [" << cpath().basename() << "]" ) ;
		auto stream = std::make_unique<Stream>( cpath() ) ;
		if( !stream )
		{
			reason = "cannot open content file" ;
			return false ;
		}
		stream->exceptions( std::ios_base::badbit ) ;
		m_content = std::move( stream ) ;
		return true ;
	}
	catch( std::exception & e ) // invalid file in store
	{
		G_DEBUG( "GStore::FileStore: exception: " << e.what() ) ;
		reason = e.what() ;
		return false ;
	}
}

const std::string & GStore::StoredFile::eol() const
{
	static const std::string crlf( "\r\n" ) ;
	static const std::string lf( "\n" ) ;
	return m_env.crlf ? crlf : lf ;
}

bool GStore::StoredFile::lock()
{
	G_DEBUG( "GStore::StoredFile::lock: locking envelope [" << epath(m_state).basename() << "]" ) ;
	const G::Path src = epath( m_state ) ;
	const G::Path dst = epath( State::Locked ) ;
	bool ok = FileOp::rename( src , dst ) ;
	if( ok )
	{
		m_state = State::Locked ;
		m_unlock = true ;
	}
	else
	{
		G_DEBUG( "GStore::StoredFile::lock: failed to lock envelope "
			"[" << src.basename() << "] (" << G::Process::strerror(FileOp::errno_()) << ")" ) ;
	}
	static_cast<MessageStore&>(m_store).updated() ;
	return ok ;
}

void GStore::StoredFile::editRecipients( const G::StringArray & recipients )
{
	editEnvelope( [&recipients](Envelope &env_){env_.to_remote=recipients;} ) ;
}

void GStore::StoredFile::editEnvelope( std::function<void(Envelope&)> edit_fn , std::istream * headers_stream )
{
	// re-read the envelope (disregard m_env because we need the stream)
	G::Path envelope_path = epath(m_state) ;
	std::ifstream envelope_stream ;
	Envelope envelope = FileStore::readEnvelope( envelope_path , &envelope_stream ) ;
	envelope_stream.seekg( envelope.endpos ) ; // NOLINT narrowing

	// edit the envelope as required
	edit_fn( envelope ) ;

	// write the envelope to a temporary file
	const G::Path envelope_path_tmp = epath(m_state).str().append(".tmp") ;
	G::ScopeExit file_cleanup( [envelope_path_tmp](){FileOp::remove(envelope_path_tmp);} ) ;
	std::ofstream envelope_stream_tmp ;
	envelope.endpos = writeEnvelopeImp( envelope , envelope_path_tmp , envelope_stream_tmp ) ;
	envelope.crlf = true ;

	// copy trailing headers (see StoredMessage::fail(), MessageDelivery::deliver(), etc)
	GStore::Envelope::copyExtra( envelope_stream , envelope_stream_tmp ) ;

	// add more trailing headers
	if( headers_stream )
		GStore::Envelope::copyExtra( *headers_stream , envelope_stream_tmp ) ;

	// close
	envelope_stream.close() ;
	envelope_stream_tmp.close() ;
	if( envelope_stream_tmp.fail() )
		throw EditError( envelope_path.basename() ) ;

	// commit
	replaceEnvelope( envelope_path , envelope_path_tmp ) ;
	file_cleanup.release() ;
	m_env = envelope ;
	static_cast<MessageStore&>(m_store).updated() ;
}

void GStore::StoredFile::replaceEnvelope( const G::Path & envelope_path , const G::Path & envelope_path_tmp )
{
	G_DEBUG( "GStore::StoredFile::replaceEnvelope: renaming envelope "
		"[" << envelope_path.basename() << "] -> [" << envelope_path_tmp.basename() << "]" ) ;

	if( !FileOp::renameOnto( envelope_path_tmp , envelope_path ) )
		throw EditError( "renaming" , envelope_path.basename() , G::Process::strerror(FileOp::errno_()) ) ;
}

std::size_t GStore::StoredFile::writeEnvelopeImp( const Envelope & envelope , const G::Path & envelope_path , std::ofstream & stream )
{
	if( !FileOp::openOut( stream , envelope_path ) )
		throw EditError( "creating" , envelope_path.basename() ) ;

	std::size_t endpos = GStore::Envelope::write( stream , envelope ) ;
	if( endpos == 0U )
		throw EditError( envelope_path.basename() ) ;
	return endpos ;
}

void GStore::StoredFile::fail( const std::string & reason , int reason_code )
{
	if( FileOp::exists( epath(m_state) ) ) // client-side preprocessing may have removed it
	{
		addReason( epath(m_state) , reason , reason_code ) ;

		G::Path bad_path = epath( State::Bad ) ;
		G_LOG_S( "GStore::StoredFile::fail: failing envelope [" << epath(m_state).basename() << "] "
			<< "-> [" << bad_path.basename() << "]" ) ;

		FileOp::rename( epath(m_state) , bad_path ) ;
		m_state = State::Bad ;
	}
	else
	{
		G_DEBUG( "GStore::StoredFile::fail: cannot fail envelope [" << epath(m_state).basename() << "]" ) ;
	}
	m_unlock = false ;
	static_cast<MessageStore&>(m_store).updated() ;
}

void GStore::StoredFile::addReason( const G::Path & path , const std::string & reason , int reason_code ) const
{
	std::ofstream stream ;
	if( !FileOp::openAppend( stream , path ) )
		G_ERROR( "GStore::StoredFile::addReason: cannot re-open envelope file to append the failure reason: "
			<< "[" << path.basename() << "] (" << G::Process::strerror(FileOp::errno_()) << ")" ) ;

	stream << FileStore::x() << "Reason: " << G::Str::toPrintableAscii(reason) << eol() ;
	stream << FileStore::x() << "ReasonCode:" ; if( reason_code ) stream << " " << reason_code ; stream << eol() ;
}

void GStore::StoredFile::destroy()
{
	G_LOG( "GStore::StoredFile::destroy: deleting envelope [" << epath(m_state).basename() << "]" ) ;
	if( !FileOp::remove( epath(m_state) ) )
		G_WARNING( "GStore::StoredFile::destroy: failed to delete envelope file "
			<< "[" << epath(m_state).basename() << "] (" << G::Process::strerror(FileOp::errno_()) << ")" ) ;

	G_LOG( "GStore::StoredFile::destroy: deleting content [" << cpath().basename() << "]" ) ;
	m_content.reset() ; // close it before deleting
	if( !FileOp::remove( cpath() ) )
		G_WARNING( "GStore::StoredFile::destroy: failed to delete content file "
			<< "[" << cpath().basename() << "] (" << G::Process::strerror(FileOp::errno_()) << "]" ) ;

	m_unlock = false ;
	static_cast<MessageStore&>(m_store).updated() ;
}

std::string GStore::StoredFile::from() const
{
	return m_env.from ;
}

std::string GStore::StoredFile::to( std::size_t i ) const
{
	return i < m_env.to_remote.size() ? m_env.to_remote[i] : std::string() ;
}

std::size_t GStore::StoredFile::toCount() const
{
	return m_env.to_remote.size() ;
}

std::size_t GStore::StoredFile::contentSize() const
{
	std::streamoff size = m_content ? m_content->size() : 0U ;
	if( static_cast<std::make_unsigned<std::streamoff>::type>(size) > std::numeric_limits<std::size_t>::max() )
		throw SizeError( "too big" ) ;
	return static_cast<std::size_t>(size) ;
}

std::istream & GStore::StoredFile::contentStream()
{
	if( m_content == nullptr )
		m_content = std::make_unique<Stream>() ;
	return *m_content ;
}

std::string GStore::StoredFile::authentication() const
{
	return m_env.authentication ;
}

std::string GStore::StoredFile::fromAuthIn() const
{
	return m_env.from_auth_in ;
}

std::string GStore::StoredFile::forwardTo() const
{
	return m_env.forward_to ;
}

std::string GStore::StoredFile::forwardToAddress() const
{
	return m_env.forward_to_address ;
}

std::string GStore::StoredFile::clientAccountSelector() const
{
	return m_env.client_account_selector ;
}

bool GStore::StoredFile::utf8Mailboxes() const
{
	return m_env.utf8_mailboxes ;
}

std::string GStore::StoredFile::fromAuthOut() const
{
	return m_env.from_auth_out ;
}

// ==

GStore::StoredFile::Stream::Stream() :
	StreamBuf(&G::File::read,&G::File::write,&G::File::close),
	std::istream(static_cast<StreamBuf*>(this))
{
}

GStore::StoredFile::Stream::Stream( const G::Path & path ) :
	StreamBuf(&G::File::read,&G::File::write,&G::File::close),
	std::istream(static_cast<StreamBuf*>(this))
{
	open( path ) ;
}

void GStore::StoredFile::Stream::open( const G::Path & path )
{
	// (because on windows we want _O_NOINHERIT and _SH_DENYNO)
	int fd = FileOp::fdopen( path.cstr() ) ;
	if( fd >= 0 )
	{
		StreamBuf::open( fd ) ;
		clear() ;
	}
	else
	{
		clear( std::ios_base::failbit ) ;
	}
}

std::streamoff GStore::StoredFile::Stream::size() const
{
	// (G::fbuf is not seekable)
	int fd = file() ;
	auto old = G::File::seek( fd , 0 , G::File::Seek::Current ) ;
	auto end_ = G::File::seek( fd , 0 , G::File::Seek::End ) ;
	auto new_ = G::File::seek( fd , old , G::File::Seek::Start ) ;
	if( old < 0 || old != new_ )
		throw StoredFile::SizeError() ;
	return end_ ;
}

