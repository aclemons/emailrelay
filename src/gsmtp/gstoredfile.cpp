//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
	m_locked(false) ,
	m_crlf(false)
{
	m_name = m_envelope_path.basename() ;
	size_t pos = m_name.rfind(".envelope") ;
	if( pos != std::string::npos ) m_name.erase( pos ) ;
	G_DEBUG( "GSmtp::StoredFile::ctor: name=[" << m_name << "]" ) ;
}

GSmtp::StoredFile::~StoredFile()
{
	try
	{
		if( m_locked )
		{
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

bool GSmtp::StoredFile::eightBit() const
{
	return m_eight_bit ;
}

void GSmtp::StoredFile::sync( bool )
{
	readEnvelopeCore( true ) ;
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
	FileReader claim_reader ;
	std::ifstream stream( m_envelope_path.str().c_str() , std::ios_base::binary | std::ios_base::in ) ;
	if( ! stream.good() )
		throw ReadError( m_envelope_path.str() ) ;

	m_crlf = false ;
	readFormat( stream ) ;
	readFlag( stream ) ;
	readFrom( stream ) ;
	readToList( stream ) ;
	readAuthentication( stream ) ;
	readClientSocketAddress( stream ) ;
	if( m_format == FileStore::format() )
	{
		readClientCertificate( stream ) ;
		readFromAuthIn( stream ) ;
		readFromAuthOut( stream ) ;
	}
	else if( m_format == FileStore::format(-1) )
	{
		readClientSocketName( stream ) ;
		readClientCertificate( stream ) ;
	}
	readEnd( stream ) ;

	if( check_recipients && m_to_remote.size() == 0U )
		throw FormatError( "no recipients" ) ;

	if( ! stream.good() )
		throw ReadError( m_envelope_path.str() ) ;

	readReasons( stream ) ;
}

void GSmtp::StoredFile::readFormat( std::istream & stream )
{
	m_format = readValue( stream , "Format" ) ;
	if( ! FileStore::knownFormat(m_format) )
		throw FormatError( "unknown format id" , m_format ) ;
}

void GSmtp::StoredFile::readFlag( std::istream & stream )
{
	m_eight_bit = readValue(stream,"Content") == "8bit" ;
}

void GSmtp::StoredFile::readFrom( std::istream & stream )
{
	m_from = readValue( stream , "From" ) ;
	G_DEBUG( "GSmtp::StoredFile::readFrom: from \"" << m_from << "\"" ) ;
}

void GSmtp::StoredFile::readFromAuthIn( std::istream & stream )
{
	m_from_auth_in = readValue( stream , "MailFromAuthIn" ) ;
	if( !m_from_auth_in.empty() && m_from_auth_in != "+" && !G::Xtext::valid(m_from_auth_in) )
		throw FormatError( "invalid mail-from-auth-in encoding" ) ;
}

void GSmtp::StoredFile::readFromAuthOut( std::istream & stream )
{
	m_from_auth_out = readValue( stream , "MailFromAuthOut" ) ;
	if( !m_from_auth_out.empty() && m_from_auth_out != "+" && !G::Xtext::valid(m_from_auth_out) )
		throw FormatError( "invalid mail-from-auth-out encoding" ) ;
}

void GSmtp::StoredFile::readToList( std::istream & stream )
{
	m_to_local.clear() ;
	m_to_remote.clear() ;

	unsigned int to_count = G::Str::toUInt( readValue(stream,"ToCount") ) ;

	for( unsigned int i = 0U ; i < to_count ; i++ )
	{
		std::string to_line = readLine( stream ) ;
		bool is_local = to_line.find(FileStore::x()+"To-Local: ") == 0U ;
		bool is_remote = to_line.find(FileStore::x()+"To-Remote: ") == 0U ;
		if( ! is_local && ! is_remote )
			throw FormatError( "bad 'to' line" ) ;

		if( is_local )
			m_to_local.push_back( value(to_line) ) ;
		else
			m_to_remote.push_back( value(to_line) ) ;
	}
}

void GSmtp::StoredFile::readAuthentication( std::istream & stream )
{
	m_authentication = G::Xtext::decode( readValue(stream,"Authentication") ) ;
}

void GSmtp::StoredFile::readClientSocketAddress( std::istream & stream )
{
	m_client_socket_address = readValue( stream , "Client" ) ;
}

void GSmtp::StoredFile::readClientSocketName( std::istream & stream )
{
	m_client_socket_name = G::Xtext::decode( readValue(stream,"ClientName") ) ;
}

void GSmtp::StoredFile::readClientCertificate( std::istream & stream )
{
	m_client_certificate = readValue( stream , "ClientCertificate" ) ;
}

void GSmtp::StoredFile::readEnd( std::istream & stream )
{
	std::string end = readLine( stream ) ;
	if( end.find(FileStore::x()+"End") != 0U )
		throw FormatError( "no end line" ) ;
}

void GSmtp::StoredFile::readReasons( std::istream & stream )
{
	m_errors = 0U ;
	while( stream.good() )
	{
		std::string reason = readLine( stream ) ;
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
		unique_ptr<std::istream> stream( new std::ifstream(
			content_path.str().c_str() , std::ios_base::in | std::ios_base::binary ) ) ;
		if( !stream->good() )
		{
			reason = "cannot open content file" ;
			return false ;
		}

		G_DEBUG( "GSmtp::MessageStore: processing envelope \"" << m_envelope_path.basename() << "\"" ) ;
		G_DEBUG( "GSmtp::MessageStore: processing content \"" << content_path.basename() << "\"" ) ;

		m_content.reset( stream.release() ) ;
		return true ;
	}
	catch( std::exception & e ) // invalid file in store
	{
		G_DEBUG( "GSmtp::FileStore: exception: " << e.what() ) ;
		reason = e.what() ;
		return false ;
	}
}

std::string GSmtp::StoredFile::readLine( std::istream & stream )
{
	std::string line = G::Str::readLineFrom( stream ) ;

	// allow lf or crlf and stay consistent when appending
	if( !line.empty() && line.at(line.size()-1U) == '\r' )
		m_crlf = true ;
	G::Str::trimRight( line , "\r" ) ;

	return line ;
}

std::string GSmtp::StoredFile::readValue( std::istream & stream , const std::string & expected_key )
{
	std::string line = readLine( stream ) ;

	std::string prefix = FileStore::x() + expected_key + ":" ;
	if( line == prefix )
		return std::string() ;

	prefix.append( 1U , ' ' ) ;
	size_t pos = line.find( prefix  ) ;
	if( pos != 0U )
		throw FormatError( "expected \"" + FileStore::x() + expected_key + ":\"" ) ;

	// RFC-2822 unfolding
	for(;;)
	{
		char c = stream.peek() ;
		if( c == ' ' || c == '\t' )
			line.append( readLine(stream) ) ;
		else
			break ;
	}

	return value( line ) ;
}

std::string GSmtp::StoredFile::value( const std::string & line ) const
{
	return G::Str::trimmed( G::Str::tail( line , line.find(":") , std::string() ) , G::Str::ws() ) ;
}

const std::string & GSmtp::StoredFile::eol() const
{
	static const std::string crlf( "\r\n" ) ;
	static const std::string lf( "\n" ) ;
	return m_crlf ? crlf : lf ;
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
		{
			FileWriter claim_writer ;
			G::File::rename( m_envelope_path , m_old_envelope_path ) ;
		}
		G_LOG( "GSmtp::StoredMessage: unlocking file \"" << m_envelope_path.basename() << "\"" ) ;
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
	FileWriter claim_writer ;
	std::ofstream file( path.str().c_str() ,
		std::ios_base::binary | std::ios_base::app ) ; // "app", not "ate", for win32
	file << FileStore::x() << "Reason: " << reason << eol() ;
	file << FileStore::x() << "ReasonCode:" ; if( reason_code ) file << " " << reason_code ; file << eol() ;
}

G::Path GSmtp::StoredFile::badPath( G::Path busy_path )
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
	m_content.reset() ; // close it first (but not much good if it's been extracted already)
	{
		FileWriter claim_writer ;
		G::File::remove( content_path , G::File::NoThrow() ) ;
	}
}

const std::string & GSmtp::StoredFile::from() const
{
	return m_from ;
}

const G::StringArray & GSmtp::StoredFile::to() const
{
	return m_to_remote ;
}

unique_ptr<std::istream> GSmtp::StoredFile::extractContentStream()
{
	G_ASSERT( m_content.get() != nullptr ) ;
	return unique_ptr<std::istream>( m_content.release() ) ;
}

G::Path GSmtp::StoredFile::contentPath() const
{
	std::string filename = m_envelope_path.str() ;

	size_t pos = filename.rfind( ".envelope" ) ; // also works for ".envelope.bad" etc.
	if( pos == std::string::npos )
		throw FilenameError( filename ) ;

	return G::Path( G::Str::head(filename,pos,std::string()) + ".content" ) ;
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

std::string GSmtp::StoredFile::fromAuthIn() const
{
	return m_from_auth_in ;
}

std::string GSmtp::StoredFile::fromAuthOut() const
{
	return m_from_auth_out ;
}

/// \file gstoredfile.cpp
