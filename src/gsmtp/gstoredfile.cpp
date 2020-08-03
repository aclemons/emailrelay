//
// Copyright (C) 2001-2020 Graeme Walker <graeme_walker@users.sourceforge.net>
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
//
// gstoredfile.cpp
//

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
	m_content(new std::ifstream) ,
	m_envelope_path(envelope_path) ,
	m_locked(false)
{
	m_name = m_envelope_path.basename() ;
	std::size_t pos = m_name.rfind(".envelope") ;
	if( pos != std::string::npos ) m_name.erase( pos ) ;
	G_DEBUG( "GSmtp::StoredFile::ctor: name=[" << m_name << "]" ) ;
}

GSmtp::StoredFile::~StoredFile()
{
	try
	{
		if( m_locked )
		{
			// unlock
			FileWriter claim_writer ;
			G::File::rename( m_envelope_path , m_old_envelope_path , G::File::NoThrow() ) ;
		}
	}
	catch(...) // dtor
	{
	}
}

std::string GSmtp::StoredFile::name() const
{
	return m_name ;
}

std::string GSmtp::StoredFile::location() const
{
	return contentPath().str() ;
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
		G::File::open( stream , m_envelope_path ) ;
	}
	if( ! stream.good() )
		throw ReadError( m_envelope_path.str() ) ;

	GSmtp::Envelope::read( stream , m_env ) ;

	if( check_recipients && m_env.m_to_remote.empty() )
		throw FormatError( "no recipients" ) ;

	if( ! stream.good() )
		throw ReadError( m_envelope_path.str() ) ;
}

bool GSmtp::StoredFile::openContent( std::string & reason )
{
	try
	{
		G::Path content_path = contentPath() ;
		G_DEBUG( "GSmtp::FileStore::openContent: \"" << content_path << "\"" ) ;
		auto stream = std::make_unique<std::ifstream>() ;
		{
			FileReader claim_reader ;
			G::File::open( *stream , content_path ) ;
		}
		if( !stream->good() )
		{
			reason = "cannot open content file" ;
			return false ;
		}
		stream->exceptions( std::ios_base::badbit ) ; // (new)
		m_content.reset( stream.release() ) ; // NOLINT upcast from ifstream to stream
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
	const G::Path src = m_envelope_path ;
	const G::Path dst( src.str() + ".busy" ) ;
	bool ok = false ;
	{
		FileWriter claim_writer ;
		ok = G::File::rename( src , dst , G::File::NoThrow() ) ;
	}
	if( ok )
	{
		G_LOG( "GSmtp::StoredMessage: locking file \"" << src.basename() << "\"" ) ;
		m_envelope_path = dst ;
		m_old_envelope_path = src ;
		m_locked = true ;
	}
	m_store.updated() ;
	return ok ;
}

void GSmtp::StoredFile::edit( const G::StringArray & rejectees )
{
	G_ASSERT( !rejectees.empty() ) ;

	GSmtp::Envelope env_copy( m_env ) ;
	env_copy.m_to_remote = rejectees ;

	G::Path path_in( m_envelope_path.str() ) ;
	G::Path path_out( m_envelope_path.str() + ".tmp" ) ;

	// create new file
	std::ofstream out ;
	{
		FileWriter claim_writer ;
		G::File::open( out , path_out ) ;
	}
	if( !out.good() )
		throw EditError( path_in.str() ) ;
	G::ScopeExit file_deleter( [=](){G::File::remove(path_out,G::File::NoThrow());} ) ;

	// write new file
	std::size_t endpos = GSmtp::Envelope::write( out , env_copy ) ;
	if( endpos == 0U )
		throw EditError( path_in.str() ) ;

	// open existing file
	std::ifstream in ;
	{
		FileReader claim_reader ;
		G::File::open( in , path_in ) ;
	}
	if( !in.good() )
		throw EditError( path_in.str() ) ;

	// re-read the existing file's endpos, just in case
	GSmtp::Envelope env_check ;
	GSmtp::Envelope::read( in , env_check ) ;
	if( env_check.m_endpos != m_env.m_endpos )
		G_WARNING( "GSmtp::StoredFile::edit: unexpected change to envelope file detected: " << path_in ) ;

	// copy the existing file's tail to the new file
	in.seekg( env_check.m_endpos ) ;
	if( !in.good() )
		throw EditError( path_in.str() ) ;
	GSmtp::Envelope::copy( in , out ) ;

	in.close() ;
	out.close() ;
	if( out.fail() )
		throw EditError( path_in.str() ) ;

	// commit the file
	bool ok = false ;
	{
		FileWriter claim_writer ;
		ok = G::File::rename( path_out , path_in , G::File::NoThrow() ) ;
	}
	if( !ok )
		throw EditError( path_in.str() ) ;
	file_deleter.release() ;

	m_env.m_crlf = true ;
	m_env.m_endpos = endpos ;
	m_env.m_to_remote = rejectees ;
}

void GSmtp::StoredFile::fail( const std::string & reason , int reason_code )
{
	if( G::File::exists(m_envelope_path) ) // client-side preprocessing may have removed it
	{
		addReason( m_envelope_path , reason , reason_code ) ;

		G::Path bad_path = badPath( m_envelope_path ) ;
		G_LOG_S( "GSmtp::StoredMessage: failing file: "
			<< "\"" << m_envelope_path.basename() << "\" -> "
			<< "\"" << bad_path.basename() << "\"" ) ;

		FileWriter claim_writer ;
		G::File::rename( m_envelope_path , bad_path , G::File::NoThrow() ) ;
	}
}

void GSmtp::StoredFile::unfail()
{
	G_DEBUG( "GSmtp::StoredMessage: unfailing file: " << m_envelope_path ) ;
	if( m_envelope_path.extension() == "bad" )
	{
		G::Path dst = m_envelope_path.withoutExtension() ; // "foo.envelope.bad" -> "foo.envelope"
		bool ok = false ;
		{
			FileWriter claim_writer ;
			ok = G::File::rename( m_envelope_path , dst , G::File::NoThrow() ) ;
		}
		if( ok )
		{
			G_LOG( "GSmtp::StoredMessage: unfailed file: "
				<< "\"" << m_envelope_path.basename() << "\" -> "
				<< "\"" << dst.basename() << "\"" ) ;
			m_envelope_path = dst ;
		}
		else
		{
			G_WARNING( "GSmtp::StoredMessage: failed to unfail file: \"" << m_envelope_path << "\"" ) ;
		}
	}
}

void GSmtp::StoredFile::addReason( const G::Path & path , const std::string & reason , int reason_code ) const
{
	std::ofstream file ;
	{
		FileWriter claim_writer ;
		G::File::open( file , path , std::ios_base::app ) ; // "app", not "ate", for win32
	}
	file << FileStore::x() << "Reason: " << G::Str::toPrintableAscii(reason) << eol() ;
	file << FileStore::x() << "ReasonCode:" ; if( reason_code ) file << " " << reason_code ; file << eol() ;
}

G::Path GSmtp::StoredFile::badPath( const G::Path & busy_path )
{
	return busy_path.withExtension("bad") ; ; // "foo.envelope.busy" -> "foo.envelope.bad"
}

void GSmtp::StoredFile::destroy()
{
	G_LOG( "GSmtp::StoredMessage: deleting file: \"" << m_envelope_path.basename() << "\"" ) ;
	{
		FileWriter claim_writer ;
		G::File::remove( m_envelope_path , G::File::NoThrow() ) ;
	}

	G::Path content_path = contentPath() ;
	G_LOG( "GSmtp::StoredMessage: deleting file: \"" << content_path.basename() << "\"" ) ;
	m_content.reset() ; // close it before deleting
	{
		FileWriter claim_writer ;
		G::File::remove( content_path , G::File::NoThrow() ) ;
	}
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
	G_ASSERT( m_content != nullptr ) ;
	if( m_content == nullptr )
		m_content.reset( new std::ifstream ) ; // upcast

	return *m_content ;
}

G::Path GSmtp::StoredFile::contentPath() const
{
	std::string filename = m_envelope_path.str() ;

	std::size_t pos = filename.rfind( ".envelope" ) ; // also works for ".envelope.bad" etc.
	if( pos == std::string::npos )
		throw FilenameError( filename ) ;

	return G::Path( G::Str::head(filename,pos,std::string()) + ".content" ) ;
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

/// \file gstoredfile.cpp
