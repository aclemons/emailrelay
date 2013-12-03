//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gsmtp.h"
#include "gfilestore.h"
#include "gstoredfile.h"
#include "gmemory.h"
#include "gxtext.h"
#include "gfile.h"
#include "gstr.h"
#include "glog.h"
#include "gassert.h"
#include <fstream>

GSmtp::StoredFile::StoredFile( FileStore & store , const G::Path & envelope_path ) :
	m_store(store) ,
	m_envelope_path(envelope_path) ,
	m_eight_bit(false) ,
	m_errors(0U) ,
	m_locked(false)
{
	m_name = m_envelope_path.basename() ;
	size_t pos = m_name.rfind(".envelope") ;
	if( pos != std::string::npos ) m_name.erase( pos ) ;
	G_DEBUG( "StoredFile: \"" << m_name << "\"" ) ;
}

GSmtp::StoredFile::~StoredFile() 
{ 
	try
	{
		unlock() ;
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

bool GSmtp::StoredFile::eightBit() const
{
	return m_eight_bit ;
}

void GSmtp::StoredFile::sync()
{
	readEnvelopeCore( true ) ;
}

bool GSmtp::StoredFile::readEnvelope( std::string & reason , bool check )
{
	try
	{
		readEnvelopeCore( check ) ;
		return true ;
	}
	catch( std::exception & e ) // invalid file in store
	{
		reason = e.what() ;
		return false ;
	}
}

void GSmtp::StoredFile::readEnvelopeCore( bool check )
{
	FileReader claim_reader ;
	std::ifstream stream( m_envelope_path.str().c_str() , std::ios_base::binary | std::ios_base::in ) ;
	if( ! stream.good() )
		throw OpenError( m_envelope_path.str() ) ;

	readFormat( stream ) ;
	readFlag( stream ) ;
	readFrom( stream ) ;
	readToList( stream ) ;
	readAuthentication( stream ) ;
	readClientSocketAddress( stream ) ;
	if( m_format == FileStore::format() )
	{
		readClientSocketName( stream ) ;
		readClientCertificate( stream ) ;
	}
	readEnd( stream ) ;

	if( check && m_to_remote.size() == 0U )
		throw NoRecipients() ;

	if( ! stream.good() )
		throw StreamError() ;

	readReasons( stream ) ;
}

void GSmtp::StoredFile::readFormat( std::istream & stream )
{
	std::string format_line = getline(stream) ;
	m_format = value(format_line,"Format") ;
	if( ! FileStore::knownFormat(m_format) )
		throw InvalidFormat( m_format ) ;
}

void GSmtp::StoredFile::readFlag( std::istream & stream )
{
	std::string content_line = getline(stream) ;
	m_eight_bit = value(content_line,"Content") == "8bit" ;
}

void GSmtp::StoredFile::readFrom( std::istream & stream )
{
	m_from = value(getline(stream),"From") ;
	G_DEBUG( "GSmtp::StoredFile::readFrom: from \"" << m_from << "\"" ) ;
}

void GSmtp::StoredFile::readToList( std::istream & stream )
{
	m_to_local.clear() ;
	m_to_remote.clear() ;

	std::string to_count_line = getline(stream) ;
	unsigned int to_count = G::Str::toUInt( value(to_count_line,"ToCount") ) ;

	for( unsigned int i = 0U ; i < to_count ; i++ )
	{
		std::string to_line = getline(stream) ;
		bool is_local = to_line.find(FileStore::x()+"To-Local") == 0U ;
		bool is_remote = to_line.find(FileStore::x()+"To-Remote") == 0U ;
		if( ! is_local && ! is_remote )
			throw InvalidTo(to_line) ;
			
		G_DEBUG( "GSmtp::StoredFile::readToList: to "
			"[" << (i+1U) << "/" << to_count << "] "
			"(" << (is_local?"local":"remote") << ") "
			<< "\"" << value(to_line) << "\"" ) ;

		if( is_local )
			m_to_local.push_back( value(to_line) ) ;
		else
			m_to_remote.push_back( value(to_line) ) ;
	}
}

void GSmtp::StoredFile::readAuthentication( std::istream & stream )
{
	m_authentication = G::Xtext::decode(value(getline(stream),"Authentication")) ;
}

void GSmtp::StoredFile::readClientSocketAddress( std::istream & stream )
{
	m_client_socket_address = value(getline(stream),"Client") ;
}

void GSmtp::StoredFile::readClientSocketName( std::istream & stream )
{
	m_client_socket_name = G::Xtext::decode(value(getline(stream),"ClientName")) ;
}

void GSmtp::StoredFile::readClientCertificate( std::istream & stream )
{
	m_client_certificate = G::Xtext::decode(value(getline(stream),"ClientCertificate")) ;
}

void GSmtp::StoredFile::readEnd( std::istream & stream )
{
	std::string end = getline(stream) ;
	if( end.find(FileStore::x()+"End") != 0U )
		throw NoEnd() ;
}

void GSmtp::StoredFile::readReasons( std::istream & stream )
{
	m_errors = 0U ;
	while( stream.good() )
	{
		std::string reason = getline(stream) ;
		if( reason.find(FileStore::x()+"Reason") == 0U )
			m_errors++ ;
	}
}

bool GSmtp::StoredFile::openContent( std::string & reason )
{
	try
	{
		FileReader claim_reader ;
		G::Path content_path = contentPath() ;
		G_DEBUG( "GSmtp::FileStore::openContent: \"" << content_path << "\"" ) ;
		std::auto_ptr<std::istream> stream( new std::ifstream( 
			content_path.str().c_str() , std::ios_base::in | std::ios_base::binary ) ) ;
		if( !stream->good() )
		{
			reason = "cannot open content file" ;
			return false ;
		}

		G_DEBUG( "GSmtp::MessageStore: processing envelope \"" << m_envelope_path.basename() << "\"" ) ;
		G_DEBUG( "GSmtp::MessageStore: processing content \"" << content_path.basename() << "\"" ) ;

		m_content = stream ;
		return true ;
	}
	catch( std::exception & e ) // invalid file in store
	{
		G_DEBUG( "GSmtp::FileStore: exception: " << e.what() ) ;
		reason = e.what() ;
		return false ;
	}
}

std::string GSmtp::StoredFile::getline( std::istream & stream ) const
{
	return G::Str::readLineFrom( stream , crlf() ) ;
}

std::string GSmtp::StoredFile::value( const std::string & s , const std::string & key ) const
{
	size_t pos = s.find(":") ;
	if( pos == std::string::npos )
		throw MessageStore::FormatError(key) ;

	if( !key.empty() )
	{
		size_t key_pos = s.find(key) ;
		if( key_pos == std::string::npos || (key_pos+key.length()) != pos )
			throw MessageStore::FormatError(key) ;
	}

	return s.substr(pos+2U) ;
}

const std::string & GSmtp::StoredFile::crlf()
{
	static const std::string s( "\015\012" ) ;
	return s ;
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

void GSmtp::StoredFile::unlock()
{
	if( m_locked )
	{
		G_LOG( "GSmtp::StoredMessage: unlocking file \"" << m_envelope_path.basename() << "\"" ) ;
		{
			FileWriter claim_writer ;
			G::File::rename( m_envelope_path , m_old_envelope_path ) ;
		}
		m_envelope_path = m_old_envelope_path ;
		m_locked = false ;
		m_store.updated() ;
	}
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
		G::Path dst = m_envelope_path ;
		dst.removeExtension() ;
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

void GSmtp::StoredFile::addReason( const G::Path & path , const std::string & reason , int reason_code )
{
	FileWriter claim_writer ;
	std::ofstream file( path.str().c_str() , 
		std::ios_base::binary | std::ios_base::app ) ; // "app", not "ate", for win32
	file << FileStore::x() << "Reason: " << reason << crlf() ;
	file << FileStore::x() << "ReasonCode:" ; if( reason_code ) file << " " << reason_code ; file << crlf() ;
}

G::Path GSmtp::StoredFile::badPath( G::Path busy_path )
{
	busy_path.removeExtension() ; // "foo.envelope.busy" -> "foo.envelope"
	return G::Path( busy_path.str() + ".bad" ) ; // "foo.envelope.bad"
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
	m_content <<= 0 ; // close it first (but not much good if it's been extracted already)
	{
		FileWriter claim_writer ;
		G::File::remove( content_path , G::File::NoThrow() ) ;
	}
}

const std::string & GSmtp::StoredFile::from() const 
{ 
	return m_from ; 
}

const G::Strings & GSmtp::StoredFile::to() const 
{ 
	return m_to_remote ; 
}

std::auto_ptr<std::istream> GSmtp::StoredFile::extractContentStream() 
{ 
	G_ASSERT( m_content.get() != NULL ) ; // (a stronger assertion would be that we own it too)
	return m_content ;
}

G::Path GSmtp::StoredFile::contentPath() const
{
	std::string s = m_envelope_path.str() ;
	std::string e( ".envelope" ) ; // also works for ".envelope.bad" etc.
	size_t pos = s.rfind( e ) ;
	if( pos == std::string::npos ) throw InvalidFilename(s) ;
	s.erase( pos ) ;
	s.append( ".content" ) ;
	return G::Path( s ) ;
}

size_t GSmtp::StoredFile::remoteRecipientCount() const
{
	return m_to_remote.size() ;
}

size_t GSmtp::StoredFile::errorCount() const
{
	return m_errors ;
}

std::string GSmtp::StoredFile::authentication() const
{
	return m_authentication ;
}

/// \file gstoredfile.cpp
