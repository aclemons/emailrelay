//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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

GStore::StoredFile::StoredFile( FileStore & store , const G::Path & path ) :
	m_store(store) ,
	m_id(MessageId::none()) ,
	m_state(State::Normal)
{
	G_ASSERT( path.basename().find(".envelope") != std::string::npos ) ; // inc .bad
	if( G::Str::tailMatch( path.basename() , ".bad" ) )
	{
		m_id = MessageId( path.withoutExtension().withoutExtension().basename() ) ;
		m_state = State::Bad ;
	}
	else
	{
		m_id = MessageId( path.withoutExtension().basename() ) ;
	}
	G_DEBUG( "GStore::StoredFile::ctor: id=[" << m_id.str() << "]" ) ;
}

GStore::StoredFile::~StoredFile()
{
	try
	{
		if( m_state == State::Locked )
		{
			FileWriter claim_writer ;
			G::File::rename( epath(State::Locked) , epath(State::Normal) , std::nothrow ) ;
		}
	}
	catch(...) // dtor
	{
	}
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
	if( state == State::Locked )
		return m_store.envelopePath(m_id).str().append(".busy") ;
	else if( state == State::Bad )
		return m_store.envelopePath(m_id).str().append(".bad") ;
	return m_store.envelopePath( m_id ) ;
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
	if( !readEnvelope(reason,true) || !openContent(reason) )
		return reason ;
	else
		return std::string() ;
}

bool GStore::StoredFile::readEnvelope( std::string & reason , bool check_recipients )
{
	try
	{
		readEnvelopeCore( check_recipients ) ;
		return true ;
	}
	catch( std::exception & e ) // invalid file in store
	{
		reason = e.what() ;
		return false ;
	}
}

void GStore::StoredFile::readEnvelopeCore( bool check_recipients )
{
	std::ifstream stream ;
	{
		FileReader claim_reader ;
		G::File::open( stream , epath(m_state) ) ;
	}
	if( ! stream.good() )
		throw ReadError( epath(m_state).str() ) ;

	GStore::Envelope::read( stream , m_env ) ;

	if( check_recipients && m_env.to_remote.empty() )
		throw FormatError( "no recipients" ) ;

	if( ! stream.good() )
		throw ReadError( epath(m_state).str() ) ;
}

bool GStore::StoredFile::openContent( std::string & reason )
{
	try
	{
		G_DEBUG( "GStore::FileStore::openContent: \"" << cpath() << "\"" ) ;
		auto stream = std::make_unique<Stream>() ;
		{
			FileReader claim_reader ;
			stream->open( cpath() ) ;
		}
		if( !stream->good() )
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
	const G::Path src = epath( m_state ) ;
	const G::Path dst = epath( State::Locked ) ;
	bool ok = false ;
	{
		FileWriter claim_writer ;
		ok = G::File::rename( src , dst , std::nothrow ) ;
	}
	if( ok )
	{
		G_LOG( "GStore::StoredFile::lock: locking file \"" << src.basename() << "\"" ) ;
		m_state = State::Locked ;
	}
	static_cast<MessageStore&>(m_store).updated() ;
	return ok ;
}

void GStore::StoredFile::edit( const G::StringArray & rejectees )
{
	G_ASSERT( !rejectees.empty() ) ;

	GStore::Envelope env_copy( m_env ) ;
	env_copy.to_remote = rejectees ;

	const G::Path path_in = epath( m_state ) ;
	const G::Path path_out = epath(m_state).str().append(".tmp") ;

	// create new file
	std::ofstream out ;
	{
		FileWriter claim_writer ;
		G::File::open( out , path_out ) ;
	}
	if( !out.good() )
		throw EditError( "creating" , path_out.basename() ) ;
	G::ScopeExit file_deleter( [&](){out.close();G::File::remove(path_out,std::nothrow);} ) ;

	// write new file
	std::size_t endpos = GStore::Envelope::write( out , env_copy ) ;
	if( endpos == 0U )
		throw EditError( path_in.basename() ) ;

	// open existing file
	std::ifstream in ;
	{
		FileReader claim_reader ;
		G::File::open( in , path_in ) ;
	}
	if( !in.good() )
		throw EditError( path_in.basename() ) ;

	// re-read the existing file's endpos, just in case
	GStore::Envelope env_check ;
	GStore::Envelope::read( in , env_check ) ;
	if( env_check.endpos != m_env.endpos )
		G_WARNING( "GStore::StoredFile::edit: unexpected change to envelope file detected: " << path_in ) ;

	// copy the existing file's tail to the new file
	in.seekg( env_check.endpos ) ; // NOLINT narrowing
	if( !in.good() )
		throw EditError( path_in.basename() ) ;
	GStore::Envelope::copy( in , out ) ;

	in.close() ;
	out.close() ;
	if( out.fail() )
		throw EditError( path_in.basename() ) ;

	// commit the file
	bool ok = false ;
	int e = 0 ;
	{
		FileWriter claim_writer ;
		ok = G::File::remove( path_in , std::nothrow ) && G::File::rename( path_out , path_in , std::nothrow ) ;
		e = G::Process::errno_() ;
	}
	if( !ok )
		throw EditError( "renaming" , path_in.basename() , G::Process::strerror(e) ) ;
	file_deleter.release() ;

	m_env.crlf = true ;
	m_env.endpos = endpos ;
	m_env.to_remote = rejectees ;
}

void GStore::StoredFile::fail( const std::string & reason , int reason_code )
{
	bool exists = false ;
	{
		FileReader claim_reader ;
		exists = G::File::exists( epath(m_state) ) ;
	}
	if( exists ) // client-side preprocessing may have removed it
	{
		addReason( epath(m_state) , reason , reason_code ) ;

		G::Path bad_path = epath( State::Bad ) ;
		G_LOG_S( "GStore::StoredFile::fail: failing file: "
			<< "\"" << epath(m_state).basename() << "\" -> "
			<< "\"" << bad_path.basename() << "\"" ) ;

		FileWriter claim_writer ;
		G::File::rename( epath(m_state) , bad_path , std::nothrow ) ;
		m_state = State::Bad ;
	}
}

void GStore::StoredFile::unfail()
{
	G_DEBUG( "GStore::StoredFile::unfail: unfailing file: " << epath(m_state) ) ;
	if( m_state == State::Bad )
	{
		const G::Path src = epath( m_state ) ;
		const G::Path dst = epath( State::Normal ) ;
		bool ok = false ;
		{
			FileWriter claim_writer ;
			ok = G::File::rename( src , dst , std::nothrow ) ;
		}
		if( ok )
		{
			G_LOG( "GStore::StoredFile::unfail: unfailed file: "
				<< "\"" << src.basename() << "\" -> "
				<< "\"" << dst.basename() << "\"" ) ;
			m_state = State::Normal ;
		}
		else
		{
			G_WARNING( "GStore::StoredFile::unfail: failed to unfail file: \"" << src << "\"" ) ;
		}
	}
}

void GStore::StoredFile::addReason( const G::Path & path , const std::string & reason , int reason_code ) const
{
	std::ofstream file ;
	{
		FileWriter claim_writer ;
		G::File::open( file , path , G::File::Append() ) ;
	}
	if( !file.is_open() )
		G_ERROR( "GStore::StoredFile::addReason: cannot re-open envelope file to append the failure reason: " << path ) ;

	file << FileStore::x() << "Reason: " << G::Str::toPrintableAscii(reason) << eol() ;
	file << FileStore::x() << "ReasonCode:" ; if( reason_code ) file << " " << reason_code ; file << eol() ;
}

void GStore::StoredFile::destroy()
{
	G_LOG( "GStore::StoredFile::destroy: deleting file: \"" << epath(m_state).basename() << "\"" ) ;
	int e = 0 ;
	{
		FileWriter claim_writer ;
		if( !G::File::remove( epath(m_state) , std::nothrow ) )
			e = G::Process::errno_() ;
	}
	if( e )
		G_WARNING( "GStore::StoredFile::destroy: failed to delete envelope file: " << G::Process::strerror(e) ) ;

	G_LOG( "GStore::StoredFile::destroy: deleting file: \"" << cpath().basename() << "\"" ) ;
	m_content.reset() ; // close it before deleting
	e = 0 ;
	{
		FileWriter claim_writer ;
		if( !G::File::remove( cpath() , std::nothrow ) )
			e = G::Process::errno_() ;
	}
	if( e )
		G_WARNING( "GStore::StoredFile::destroy: failed to delete content file: " << G::Process::strerror(e) ) ;
}

std::string GStore::StoredFile::from() const
{
	return m_env.from ;
}

std::string GStore::StoredFile::to( std::size_t i ) const
{
	return i < m_env.to_remote.empty() ? std::string() : m_env.to_remote[i] ;
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

void GStore::StoredFile::Stream::open( const G::Path & path )
{
	// (because on windows we want _O_NOINHERIT and _SH_DENYNO)
	int fd = G::File::open( path.cstr() , G::File::InOutAppend::In ) ;
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

