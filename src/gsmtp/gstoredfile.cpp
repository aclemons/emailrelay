//
// Copyright (C) 2001-2021 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gstr.h"
#include "glog.h"
#include "gassert.h"
#include <fstream>

GSmtp::StoredFile::StoredFile( FileStore & store , const G::Path & envelope_path ) :
	m_store(store) ,
	m_id(MessageId::none()) ,
	m_state(State::Normal)
{
	G_ASSERT( envelope_path.basename().find(".envelope") != std::string::npos ) ; // inc .bad
	if( G::Str::tailMatch( envelope_path.basename() , ".bad" ) )
	{
		m_id = MessageId( envelope_path.withoutExtension().withoutExtension().basename() ) ;
		m_state = State::Bad ;
	}
	else
	{
		m_id = MessageId( envelope_path.withoutExtension().basename() ) ;
	}
	G_DEBUG( "GSmtp::StoredFile::ctor: id=[" << m_id.str() << "]" ) ;
}

GSmtp::StoredFile::~StoredFile()
{
	try
	{
		if( m_state == State::Locked )
		{
			// unlock
			FileWriter claim_writer ;
			G::File::rename( epath(State::Locked) , epath(State::Normal) , std::nothrow ) ;
		}
	}
	catch(...) // dtor
	{
	}
}

GSmtp::MessageId GSmtp::StoredFile::id() const
{
	return m_id ;
}

std::string GSmtp::StoredFile::location() const
{
	return cpath().str() ;
}

G::Path GSmtp::StoredFile::cpath() const
{
	return m_store.contentPath( m_id ) ;
}

G::Path GSmtp::StoredFile::epath( State state ) const
{
	if( state == State::Locked )
		return m_store.envelopePath(m_id).str().append(".busy") ;
	else if( state == State::Bad )
		return m_store.envelopePath(m_id).str().append(".bad") ;
	return m_store.envelopePath( m_id ) ;
}

int GSmtp::StoredFile::eightBit() const
{
	return m_env.m_eight_bit ;
}

void GSmtp::StoredFile::close()
{
	m_content.reset() ;
}

std::string GSmtp::StoredFile::reopen()
{
	std::string reason = "error" ;
	if( !readEnvelope(reason,true) || !openContent(reason) )
		return reason ;
	else
		return std::string() ;
}

bool GSmtp::StoredFile::readEnvelope( std::string & reason , bool check_recipients )
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

void GSmtp::StoredFile::readEnvelopeCore( bool check_recipients )
{
	std::ifstream stream ;
	{
		FileReader claim_reader ;
		G::File::open( stream , epath(m_state) ) ;
	}
	if( ! stream.good() )
		throw ReadError( epath(m_state).str() ) ;

	GSmtp::Envelope::read( stream , m_env ) ;

	if( check_recipients && m_env.m_to_remote.empty() )
		throw FormatError( "no recipients" ) ;

	if( ! stream.good() )
		throw ReadError( epath(m_state).str() ) ;
}

bool GSmtp::StoredFile::openContent( std::string & reason )
{
	try
	{
		G_DEBUG( "GSmtp::FileStore::openContent: \"" << cpath() << "\"" ) ;
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
		G_DEBUG( "GSmtp::FileStore: exception: " << e.what() ) ;
		reason = e.what() ;
		return false ;
	}
}

const std::string & GSmtp::StoredFile::eol() const
{
	static const std::string crlf( "\r\n" ) ;
	static const std::string lf( "\n" ) ;
	return m_env.m_crlf ? crlf : lf ;
}

bool GSmtp::StoredFile::lock()
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
		G_LOG( "GSmtp::StoredMessage: locking file \"" << src.basename() << "\"" ) ;
		m_state = State::Locked ;
	}
	m_store.updated() ;
	return ok ;
}

void GSmtp::StoredFile::edit( const G::StringArray & rejectees )
{
	G_ASSERT( !rejectees.empty() ) ;

	GSmtp::Envelope env_copy( m_env ) ;
	env_copy.m_to_remote = rejectees ;

	const G::Path path_in = epath( m_state ) ;
	const G::Path path_out = epath(m_state).str().append(".tmp") ;

	// create new file
	std::ofstream out ;
	int e = 0 ;
	{
		FileWriter claim_writer ;
		G::File::open( out , path_out ) ;
	}
	e = G::Process::errno_() ;
	if( !out.good() )
		throw EditError( path_in.basename() , G::Process::strerror(e) ) ;
	G::ScopeExit file_deleter( [&](){out.close();G::File::remove(path_out,std::nothrow);} ) ;

	// write new file
	std::size_t endpos = GSmtp::Envelope::write( out , env_copy ) ;
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
	GSmtp::Envelope env_check ;
	GSmtp::Envelope::read( in , env_check ) ;
	if( env_check.m_endpos != m_env.m_endpos )
		G_WARNING( "GSmtp::StoredFile::edit: unexpected change to envelope file detected: " << path_in ) ;

	// copy the existing file's tail to the new file
	in.seekg( env_check.m_endpos ) ; // NOLINT narrowing
	if( !in.good() )
		throw EditError( path_in.basename() ) ;
	GSmtp::Envelope::copy( in , out ) ;

	in.close() ;
	out.close() ;
	if( out.fail() )
		throw EditError( path_in.basename() ) ;

	// commit the file
	bool ok = false ;
	{
		FileWriter claim_writer ;
		ok = G::File::rename( path_out , path_in , std::nothrow ) ;
	}
	if( !ok )
		throw EditError( path_in.basename() ) ;
	file_deleter.release() ;

	m_env.m_crlf = true ;
	m_env.m_endpos = endpos ;
	m_env.m_to_remote = rejectees ;
}

void GSmtp::StoredFile::fail( const std::string & reason , int reason_code )
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
		G_LOG_S( "GSmtp::StoredMessage: failing file: "
			<< "\"" << epath(m_state).basename() << "\" -> "
			<< "\"" << bad_path.basename() << "\"" ) ;

		FileWriter claim_writer ;
		G::File::rename( epath(m_state) , bad_path , std::nothrow ) ;
		m_state = State::Bad ;
	}
}

void GSmtp::StoredFile::unfail()
{
	G_DEBUG( "GSmtp::StoredMessage: unfailing file: " << epath(m_state) ) ;
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
			G_LOG( "GSmtp::StoredMessage: unfailed file: "
				<< "\"" << src.basename() << "\" -> "
				<< "\"" << dst.basename() << "\"" ) ;
			m_state = State::Normal ;
		}
		else
		{
			G_WARNING( "GSmtp::StoredMessage: failed to unfail file: \"" << src << "\"" ) ;
		}
	}
}

void GSmtp::StoredFile::addReason( const G::Path & path , const std::string & reason , int reason_code ) const
{
	std::ofstream file ;
	{
		FileWriter claim_writer ;
		G::File::open( file , path , G::File::Append() ) ;
	}
	if( !file.is_open() )
		G_ERROR( "GSmtp::StoredFile::addReason: cannot re-open envelope file to append the failure reason: " << path ) ;
	file << FileStore::x() << "Reason: " << G::Str::toPrintableAscii(reason) << eol() ;
	file << FileStore::x() << "ReasonCode:" ; if( reason_code ) file << " " << reason_code ; file << eol() ;
}

void GSmtp::StoredFile::destroy()
{
	G_LOG( "GSmtp::StoredMessage: deleting file: \"" << epath(m_state).basename() << "\"" ) ;
	int e = 0 ;
	{
		FileWriter claim_writer ;
		if( !G::File::remove( epath(m_state) , std::nothrow ) )
			e = G::Process::errno_() ;
	}
	if( e )
		G_WARNING( "GSmtp::StoredFile::destroy: failed to delete envelope file: " << G::Process::strerror(e) ) ;

	G_LOG( "GSmtp::StoredMessage: deleting file: \"" << cpath().basename() << "\"" ) ;
	m_content.reset() ; // close it before deleting
	e = 0 ;
	{
		FileWriter claim_writer ;
		if( !G::File::remove( cpath() , std::nothrow ) )
			e = G::Process::errno_() ;
	}
	if( e )
		G_WARNING( "GSmtp::StoredFile::destroy: failed to delete content file: " << G::Process::strerror(e) ) ;
}

std::string GSmtp::StoredFile::from() const
{
	return m_env.m_from ;
}

std::string GSmtp::StoredFile::to( std::size_t i ) const
{
	return i < m_env.m_to_remote.size() ? m_env.m_to_remote[i] : std::string() ;
}

std::size_t GSmtp::StoredFile::toCount() const
{
	return m_env.m_to_remote.size() ;
}

std::istream & GSmtp::StoredFile::contentStream()
{
	if( m_content == nullptr )
		m_content = std::make_unique<Stream>() ;
	return *m_content ;
}

std::string GSmtp::StoredFile::authentication() const
{
	return m_env.m_authentication ;
}

std::string GSmtp::StoredFile::fromAuthIn() const
{
	return m_env.m_from_auth_in ;
}

std::string GSmtp::StoredFile::fromAuthOut() const
{
	return m_env.m_from_auth_out ;
}

// ==

GSmtp::StoredFile::Stream::Stream() :
	StreamBuf(&G::File::read,&G::File::write,&G::File::close),
	std::istream(static_cast<StreamBuf*>(this))
{
	// (because on windows we want _O_NOINHERIT and _SH_DENYNO)
}

void GSmtp::StoredFile::Stream::open( const G::Path & path )
{
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

