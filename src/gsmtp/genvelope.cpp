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
/// \file genvelope.cpp
///

#include "gdef.h"
#include "genvelope.h"
#include "gfilestore.h"
#include "gstr.h"
#include "gstringview.h"
#include "gxtext.h"

namespace GSmtp
{
	namespace EnvelopeImp
	{
		std::string folded( const std::string & ) ;
		std::string xnormalise( const std::string & ) ;
		std::string readLine( std::istream & , bool * = nullptr ) ;
		std::string readValue( std::istream & , const std::string & , bool * = nullptr ) ;
		std::string value( const std::string & ) ;
		std::string readFormat( std::istream & stream , bool * ) ;
		void readUtf8Mailboxes( std::istream & , Envelope & ) ;
		void readBodyType( std::istream & , Envelope & ) ;
		void readFrom( std::istream & , Envelope & ) ;
		void readFromAuthIn( std::istream & , Envelope & ) ;
		void readFromAuthOut( std::istream & , Envelope & ) ;
		void readToList( std::istream & , Envelope & ) ;
		void readAuthentication( std::istream & , Envelope & ) ;
		void readClientSocketAddress( std::istream & , Envelope & ) ;
		void readClientSocketName( std::istream & , Envelope & ) ;
		void readClientCertificate( std::istream & , Envelope & ) ;
		void readEnd( std::istream & , Envelope & ) ;
		G::string_view bodyTypeName( MessageStore::BodyType ) ;
		MessageStore::BodyType parseSmtpBodyType( G::string_view , MessageStore::BodyType ) ;
		G::string_view smtpBodyType( MessageStore::BodyType ) ;
	}
}

std::size_t GSmtp::Envelope::write( std::ostream & stream , const GSmtp::Envelope & e )
{
	namespace imp = GSmtp::EnvelopeImp ;
	const std::string x( GSmtp::FileStore::x() ) ;
	G::string_view crlf { "\r\n" , 2U } ;

	std::streampos pos = stream.tellp() ;
	if( pos < 0 || stream.fail() )
		return 0U ;

	stream << x << "Format: " << GSmtp::FileStore::format() << crlf ;
	stream << x << "Content: " << imp::bodyTypeName(e.m_body_type) << crlf ;
	stream << x << "From: " << e.m_from << crlf ;
	stream << x << "ToCount: " << (e.m_to_local.size()+e.m_to_remote.size()) << crlf ;
	{
		auto to_p = e.m_to_local.begin() ;
		for( ; to_p != e.m_to_local.end() ; ++to_p )
			stream << x << "To-Local: " << *to_p << crlf ;
	}
	{
		auto to_p = e.m_to_remote.begin() ;
		for( ; to_p != e.m_to_remote.end() ; ++to_p )
			stream << x << "To-Remote: " << *to_p << crlf ;
	}
	stream << x << "Authentication: " << G::Xtext::encode(e.m_authentication) << crlf ;
	stream << x << "Client: " << e.m_client_socket_address << crlf ;
	stream << x << "ClientCertificate: " << imp::folded(e.m_client_certificate) << crlf ;
	stream << x << "MailFromAuthIn: " << imp::xnormalise(e.m_from_auth_in) << crlf ;
	stream << x << "MailFromAuthOut: " << imp::xnormalise(e.m_from_auth_out) << crlf ;
	stream << x << "Utf8MailboxNames: " << (e.m_utf8_mailboxes?"1":"0") << crlf ;
	stream << x << "End: 1" << crlf ;
	stream.flush() ;
	return stream.fail() ? std::size_t(0U) : static_cast<std::size_t>( stream.tellp() - pos ) ;
}

void GSmtp::Envelope::copy( std::istream & in , std::ostream & out )
{
	std::string line ;
	while( in.good() )
	{
		G::Str::readLineFrom( in , "\n"_sv , line ) ;
		if( in )
		{
			G::Str::trimRight( line , {"\r",1U} ) ;
			G::Str::trimRight( line , G::Str::ws() ) ;
			out << line << "\r\n" ;
		}
	}
	if( in.bad() || (in.fail()&&!in.eof()) )
		throw ReadError() ;
	in.clear( std::ios_base::eofbit ) ; // clear failbit
}

void GSmtp::Envelope::read( std::istream & stream , GSmtp::Envelope & e )
{
	namespace imp = GSmtp::EnvelopeImp ;
	std::streampos oldpos = stream.tellg() ;
	std::string format = imp::readFormat( stream , &e.m_crlf ) ;
	imp::readBodyType( stream , e ) ;
	imp::readFrom( stream , e ) ;
	imp::readToList( stream , e ) ;
	imp::readAuthentication( stream , e ) ;
	imp::readClientSocketAddress( stream , e ) ;
	if( format == GSmtp::FileStore::format() )
	{
		imp::readClientCertificate( stream , e ) ;
		imp::readFromAuthIn( stream , e ) ;
		imp::readFromAuthOut( stream , e ) ;
		imp::readUtf8Mailboxes( stream , e ) ;
	}
	else if( format == GSmtp::FileStore::format(-1) )
	{
		imp::readClientCertificate( stream , e ) ;
		imp::readFromAuthIn( stream , e ) ;
		imp::readFromAuthOut( stream , e ) ;
	}
	else if( format == GSmtp::FileStore::format(-2) )
	{
		imp::readClientSocketName( stream , e ) ;
		imp::readClientCertificate( stream , e ) ;
	}
	imp::readEnd( stream , e ) ;

	if( stream.bad() )
		throw ReadError() ;
	else if( stream.fail() && stream.eof() )
		stream.clear( std::ios_base::eofbit ) ; // clear failbit -- see tellg()

	std::streampos newpos = stream.tellg() ;
	if( newpos <= 0 || newpos < oldpos )
		throw ReadError() ; // never gets here

	e.m_endpos = static_cast<std::size_t>(newpos-oldpos) ;
}

GSmtp::MessageStore::BodyType GSmtp::Envelope::parseSmtpBodyType( const std::string & s , MessageStore::BodyType default_ )
{
	namespace imp = GSmtp::EnvelopeImp ;
	return imp::parseSmtpBodyType( {s.data(),s.size()} , default_ ) ;
}

std::string GSmtp::Envelope::smtpBodyType( MessageStore::BodyType type )
{
	namespace imp = GSmtp::EnvelopeImp ;
	return G::sv_to_string( imp::smtpBodyType( type ) ) ;
}

// ==

std::string GSmtp::EnvelopeImp::folded( const std::string & s_in )
{
	std::string s = s_in ;
	G::Str::trim( s , G::Str::ws() ) ;
	G::Str::replaceAll( s , "\r" , "" ) ;
	G::Str::replaceAll( s , "\n" , "\r\n " ) ; // RFC-2822 folding
	return s ;
}

std::string GSmtp::EnvelopeImp::xnormalise( const std::string & s )
{
	return G::Xtext::encode( G::Xtext::decode(s) ) ;
}

std::string GSmtp::EnvelopeImp::readFormat( std::istream & stream , bool * crlf )
{
	std::string format = readValue( stream , "Format" , crlf ) ;
	if( ! FileStore::knownFormat(format) )
		throw Envelope::ReadError( "unknown format id" , format ) ;
	return format ;
}

void GSmtp::EnvelopeImp::readUtf8Mailboxes( std::istream & stream , Envelope & e )
{
	e.m_utf8_mailboxes = readValue(stream,"Utf8MailboxNames") == "1" ;
}

void GSmtp::EnvelopeImp::readBodyType( std::istream & stream , Envelope & e )
{
	std::string body_type = readValue( stream , "Content" ) ;
	if( body_type == bodyTypeName(MessageStore::BodyType::SevenBit) )
		e.m_body_type = MessageStore::BodyType::SevenBit ;
	else if( body_type == bodyTypeName(MessageStore::BodyType::EightBitMime) )
		e.m_body_type = MessageStore::BodyType::EightBitMime ;
	else if( body_type == bodyTypeName(MessageStore::BodyType::BinaryMime) )
		e.m_body_type = MessageStore::BodyType::BinaryMime ;
	else
		e.m_body_type = MessageStore::BodyType::Unknown ;
}

void GSmtp::EnvelopeImp::readFrom( std::istream & stream , Envelope & e )
{
	e.m_from = readValue( stream , "From" ) ;
	G_DEBUG( "GSmtp::EnvelopeImp::readFrom: from \"" << e.m_from << "\"" ) ;
}

void GSmtp::EnvelopeImp::readFromAuthIn( std::istream & stream , Envelope & e )
{
	e.m_from_auth_in = readValue( stream , "MailFromAuthIn" ) ;
	if( !e.m_from_auth_in.empty() && e.m_from_auth_in != "+" && !G::Xtext::valid(e.m_from_auth_in) )
		throw Envelope::ReadError( "invalid mail-from-auth-in encoding" ) ;
}

void GSmtp::EnvelopeImp::readFromAuthOut( std::istream & stream , Envelope & e )
{
	e.m_from_auth_out = readValue( stream , "MailFromAuthOut" ) ;
	if( !e.m_from_auth_out.empty() && e.m_from_auth_out != "+" && !G::Xtext::valid(e.m_from_auth_out) )
		throw Envelope::ReadError( "invalid mail-from-auth-out encoding" ) ;
}

void GSmtp::EnvelopeImp::readToList( std::istream & stream , Envelope & e )
{
	e.m_to_local.clear() ;
	e.m_to_remote.clear() ;

	unsigned int to_count = G::Str::toUInt( readValue(stream,"ToCount") ) ;

	for( unsigned int i = 0U ; i < to_count ; i++ )
	{
		std::string to_line = readLine( stream ) ;
		bool is_local = to_line.find(FileStore::x()+"To-Local: ") == 0U ;
		bool is_remote = to_line.find(FileStore::x()+"To-Remote: ") == 0U ;
		if( ! is_local && ! is_remote )
			throw Envelope::ReadError( "bad 'to' line" ) ;

		if( is_local )
			e.m_to_local.push_back( value(to_line) ) ;
		else
			e.m_to_remote.push_back( value(to_line) ) ;
	}
}

void GSmtp::EnvelopeImp::readAuthentication( std::istream & stream , Envelope & e )
{
	e.m_authentication = G::Xtext::decode( readValue(stream,"Authentication") ) ;
}

void GSmtp::EnvelopeImp::readClientSocketAddress( std::istream & stream , Envelope & e )
{
	e.m_client_socket_address = readValue( stream , "Client" ) ;
}

void GSmtp::EnvelopeImp::readClientSocketName( std::istream & stream , Envelope & )
{
	G::Xtext::decode( readValue(stream,"ClientName") ) ;
}

void GSmtp::EnvelopeImp::readClientCertificate( std::istream & stream , Envelope & e )
{
	e.m_client_certificate = readValue( stream , "ClientCertificate" ) ;
}

void GSmtp::EnvelopeImp::readEnd( std::istream & stream , Envelope & )
{
	std::string end = readLine( stream ) ;
	if( end.find(FileStore::x()+"End") != 0U )
		throw Envelope::ReadError( "no end line" ) ;
}

std::string GSmtp::EnvelopeImp::readValue( std::istream & stream , const std::string & expected_key , bool * crlf )
{
	std::string line = readLine( stream , crlf ) ;

	std::string prefix = FileStore::x() + expected_key + ":" ;
	if( line == prefix )
		return std::string() ;

	prefix.append( 1U , ' ' ) ;
	std::size_t pos = line.find( prefix  ) ;
	if( pos != 0U )
		throw Envelope::ReadError( "expected \"" + FileStore::x() + expected_key + ":\"" ) ;

	// RFC-2822 unfolding
	for(;;)
	{
		int c = stream.peek() ;
		if( c == ' ' || c == '\t' )
		{
			std::string next_line = readLine( stream ) ;
			if( next_line.empty() || (next_line[0]!=' '&&next_line[0]!='\t') ) // just in case
				throw Envelope::ReadError() ;
			next_line[0] = '\n' ;
			line.append( next_line ) ;
		}
		else
			break ;
	}

	return value( line ) ;
}

std::string GSmtp::EnvelopeImp::readLine( std::istream & stream , bool * crlf )
{
	std::string line = G::Str::readLineFrom( stream ) ;

	if( crlf && !line.empty() )
		*crlf = line.at(line.size()-1U) == '\r' ;

	G::Str::trimRight( line , {"\r",1U} ) ;
	return line ;
}

std::string GSmtp::EnvelopeImp::value( const std::string & line )
{
	return G::Str::trimmed( G::Str::tail( line , line.find(':') , std::string() ) , G::Str::ws() ) ;
}

G::string_view GSmtp::EnvelopeImp::bodyTypeName( MessageStore::BodyType type )
{
	if( type == MessageStore::BodyType::EightBitMime )
		return "8bit"_sv ;
	else if( type == MessageStore::BodyType::SevenBit )
		return "7bit"_sv ;
	else if( type == MessageStore::BodyType::BinaryMime )
		return "binarymime"_sv ;
	else
		return "unknown"_sv ;
}

GSmtp::MessageStore::BodyType GSmtp::EnvelopeImp::parseSmtpBodyType( G::string_view s , MessageStore::BodyType default_ )
{
	if( s.empty() )
		return default_ ;
	else if( G::Str::imatch( "7BIT" , 4U , s ) )
		return MessageStore::BodyType::SevenBit ;
	else if( G::Str::imatch( "8BITMIME" , 8U , s ) )
		return MessageStore::BodyType::EightBitMime ;
	else if( G::Str::imatch( "BINARYMIME" , 10U , s ) )
		return MessageStore::BodyType::BinaryMime ;
	else
		return MessageStore::BodyType::Unknown ;
}

G::string_view GSmtp::EnvelopeImp::smtpBodyType( MessageStore::BodyType type )
{
	if( type == MessageStore::BodyType::EightBitMime )
		return "8BITMIME"_sv ;
	else if( type == MessageStore::BodyType::SevenBit )
		return "7BIT"_sv ;
	else if( type == MessageStore::BodyType::BinaryMime )
		return "BINARYMIME"_sv ;
	else
		return {} ;
}

