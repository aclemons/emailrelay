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
/// \file genvelope.cpp
///

#include "gdef.h"
#include "genvelope.h"
#include "gfilestore.h"
#include "gfile.h"
#include "gstr.h"
#include "gstringview.h"
#include "gxtext.h"

namespace GStore
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
		void readForwardTo( std::istream & , Envelope & ) ;
		void readForwardToAddress( std::istream & , Envelope & ) ;
		void readClientAccountSelector( std::istream & , Envelope & ) ;
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

std::size_t GStore::Envelope::write( std::ostream & stream , const GStore::Envelope & e )
{
	namespace imp = GStore::EnvelopeImp ;
	const std::string x( GStore::FileStore::x() ) ;
	G::string_view crlf = "\r\n"_sv ;

	std::streampos pos = stream.tellp() ;
	if( pos < 0 )
		stream.setstate( std::ios_base::failbit ) ;
	if( stream.fail() )
		return 0U ;

	stream << x << "Format: " << GStore::FileStore::format() << crlf ;
	stream << x << "Content: " << imp::bodyTypeName(e.body_type) << crlf ;
	stream << x << "From: " << e.from << crlf ;
	stream << x << "ToCount: " << (e.to_local.size()+e.to_remote.size()) << crlf ;
	{
		auto to_p = e.to_local.begin() ;
		for( ; to_p != e.to_local.end() ; ++to_p )
			stream << x << "To-Local: " << *to_p << crlf ;
	}
	{
		auto to_p = e.to_remote.begin() ;
		for( ; to_p != e.to_remote.end() ; ++to_p )
			stream << x << "To-Remote: " << *to_p << crlf ;
	}
	stream << x << "Authentication: " << G::Xtext::encode(e.authentication) << crlf ;
	stream << x << "Client: " << e.client_socket_address << crlf ;
	stream << x << "ClientCertificate: " << imp::folded(e.client_certificate) << crlf ;
	stream << x << "MailFromAuthIn: " << imp::xnormalise(e.from_auth_in) << crlf ;
	stream << x << "MailFromAuthOut: " << imp::xnormalise(e.from_auth_out) << crlf ;
	stream << x << "ForwardTo: " << imp::xnormalise(e.forward_to) << crlf ;
	stream << x << "ForwardToAddress: " << e.forward_to_address << crlf ;
	stream << x << "ClientAccountSelector: " << e.client_account_selector << crlf ;
	stream << x << "Utf8MailboxNames: " << (e.utf8_mailboxes?"1":"0") << crlf ;
	stream << x << "End: 1" << crlf ;
	stream.flush() ;
	return stream.fail() ? std::size_t(0U) : static_cast<std::size_t>( stream.tellp() - pos ) ;
}

void GStore::Envelope::copyExtra( std::istream & in , std::ostream & out )
{
	std::string line ;
	while( G::Str::readLine( in , line ) )
	{
		G::Str::trimRight( line , {"\r",1U} ) ;
		G::Str::trimRight( line , G::Str::ws() ) ;
		out << line << "\r\n" ;
	}
	if( in.bad() || (in.fail()&&!in.eof()) )
		throw ReadError() ;
	in.clear( std::ios_base::eofbit ) ; // clear failbit
}

void GStore::Envelope::read( std::istream & stream , GStore::Envelope & e )
{
	namespace imp = GStore::EnvelopeImp ;
	std::streampos oldpos = stream.tellg() ;
	std::string format = imp::readFormat( stream , &e.crlf ) ;
	imp::readBodyType( stream , e ) ;
	imp::readFrom( stream , e ) ;
	imp::readToList( stream , e ) ;
	imp::readAuthentication( stream , e ) ;
	imp::readClientSocketAddress( stream , e ) ;
	if( format == GStore::FileStore::format() )
	{
		imp::readClientCertificate( stream , e ) ;
		imp::readFromAuthIn( stream , e ) ;
		imp::readFromAuthOut( stream , e ) ;
		imp::readForwardTo( stream , e ) ; // 2.4
		imp::readForwardToAddress( stream , e ) ; // 2.4
		imp::readClientAccountSelector( stream , e ) ; // 2.5
		imp::readUtf8Mailboxes( stream , e ) ; // 2.5rc
	}
	else if( format == GStore::FileStore::format(-1) )
	{
		imp::readClientCertificate( stream , e ) ;
		imp::readFromAuthIn( stream , e ) ;
		imp::readFromAuthOut( stream , e ) ;
		imp::readForwardTo( stream , e ) ; // 2.4
		imp::readForwardToAddress( stream , e ) ; // 2.4
		imp::readUtf8Mailboxes( stream , e ) ; // 2.5rc
	}
	else if( format == GStore::FileStore::format(-2) )
	{
		imp::readClientCertificate( stream , e ) ;
		imp::readFromAuthIn( stream , e ) ;
		imp::readFromAuthOut( stream , e ) ;
		imp::readForwardTo( stream , e ) ; // 2.4
		imp::readForwardToAddress( stream , e ) ; // 2.4
	}
	else if( format == GStore::FileStore::format(-3) )
	{
		imp::readClientCertificate( stream , e ) ;
		imp::readFromAuthIn( stream , e ) ;
		imp::readFromAuthOut( stream , e ) ;
	}
	else if( format == GStore::FileStore::format(-4) )
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

	e.endpos = static_cast<std::size_t>(newpos-oldpos) ;
}

GStore::MessageStore::BodyType GStore::Envelope::parseSmtpBodyType( const std::string & s , MessageStore::BodyType default_ )
{
	namespace imp = GStore::EnvelopeImp ;
	return imp::parseSmtpBodyType( {s.data(),s.size()} , default_ ) ;
}

#ifndef G_LIB_SMALL
std::string GStore::Envelope::smtpBodyType( MessageStore::BodyType type )
{
	namespace imp = GStore::EnvelopeImp ;
	return G::sv_to_string( imp::smtpBodyType( type ) ) ;
}
#endif

// ==

std::string GStore::EnvelopeImp::folded( const std::string & s_in )
{
	std::string s = s_in ;
	G::Str::trim( s , G::Str::ws() ) ;
	G::Str::replaceAll( s , "\r" , "" ) ;
	G::Str::replaceAll( s , "\n" , "\r\n " ) ; // RFC-2822 folding
	return s ;
}

std::string GStore::EnvelopeImp::xnormalise( const std::string & s )
{
	return G::Xtext::encode( G::Xtext::decode(s) ) ;
}

std::string GStore::EnvelopeImp::readFormat( std::istream & stream , bool * crlf )
{
	std::string format = readValue( stream , "Format" , crlf ) ;
	if( ! FileStore::knownFormat(format) )
		throw Envelope::ReadError( "unknown format id" , format ) ;
	return format ;
}

void GStore::EnvelopeImp::readUtf8Mailboxes( std::istream & stream , Envelope & e )
{
	e.utf8_mailboxes = readValue(stream,"Utf8MailboxNames") == "1" ;
}

void GStore::EnvelopeImp::readBodyType( std::istream & stream , Envelope & e )
{
	std::string body_type = readValue( stream , "Content" ) ;
	if( body_type == bodyTypeName(MessageStore::BodyType::SevenBit) )
		e.body_type = MessageStore::BodyType::SevenBit ;
	else if( body_type == bodyTypeName(MessageStore::BodyType::EightBitMime) )
		e.body_type = MessageStore::BodyType::EightBitMime ;
	else if( body_type == bodyTypeName(MessageStore::BodyType::BinaryMime) )
		e.body_type = MessageStore::BodyType::BinaryMime ;
	else
		e.body_type = MessageStore::BodyType::Unknown ;
}

void GStore::EnvelopeImp::readFrom( std::istream & stream , Envelope & e )
{
	e.from = readValue( stream , "From" ) ;
}

void GStore::EnvelopeImp::readFromAuthIn( std::istream & stream , Envelope & e )
{
	e.from_auth_in = readValue( stream , "MailFromAuthIn" ) ;
	if( !e.from_auth_in.empty() && e.from_auth_in != "+" && !G::Xtext::valid(e.from_auth_in) )
		throw Envelope::ReadError( "invalid mail-from-auth-in encoding" ) ;
}

void GStore::EnvelopeImp::readFromAuthOut( std::istream & stream , Envelope & e )
{
	e.from_auth_out = readValue( stream , "MailFromAuthOut" ) ;
	if( !e.from_auth_out.empty() && e.from_auth_out != "+" && !G::Xtext::valid(e.from_auth_out) )
		throw Envelope::ReadError( "invalid mail-from-auth-out encoding" ) ;
}

void GStore::EnvelopeImp::readForwardTo( std::istream & stream , Envelope & e )
{
	e.forward_to = readValue( stream , "ForwardTo" ) ;
}

void GStore::EnvelopeImp::readForwardToAddress( std::istream & stream , Envelope & e )
{
	e.forward_to_address = readValue( stream , "ForwardToAddress" ) ;
}

void GStore::EnvelopeImp::readToList( std::istream & stream , Envelope & e )
{
	e.to_local.clear() ;
	e.to_remote.clear() ;

	unsigned int to_count = G::Str::toUInt( readValue(stream,"ToCount") ) ;

	for( unsigned int i = 0U ; i < to_count ; i++ )
	{
		std::string to_line = readLine( stream ) ;
		bool is_local = to_line.find(FileStore::x().append("To-Local: ")) == 0U ;
		bool is_remote = to_line.find(FileStore::x().append("To-Remote: ")) == 0U ;
		if( ! is_local && ! is_remote )
			throw Envelope::ReadError( "bad 'to' line" ) ;

		if( is_local )
			e.to_local.push_back( value(to_line) ) ;
		else
			e.to_remote.push_back( value(to_line) ) ;
	}
}

void GStore::EnvelopeImp::readAuthentication( std::istream & stream , Envelope & e )
{
	e.authentication = G::Xtext::decode( readValue(stream,"Authentication") ) ;
}

void GStore::EnvelopeImp::readClientAccountSelector( std::istream & stream , Envelope & e )
{
	e.client_account_selector = readValue( stream , "ClientAccountSelector" ) ;
}

void GStore::EnvelopeImp::readClientSocketAddress( std::istream & stream , Envelope & e )
{
	e.client_socket_address = readValue( stream , "Client" ) ;
}

void GStore::EnvelopeImp::readClientSocketName( std::istream & stream , Envelope & )
{
	G::Xtext::decode( readValue(stream,"ClientName") ) ;
}

void GStore::EnvelopeImp::readClientCertificate( std::istream & stream , Envelope & e )
{
	e.client_certificate = readValue( stream , "ClientCertificate" ) ;
}

void GStore::EnvelopeImp::readEnd( std::istream & stream , Envelope & )
{
	std::string end = readLine( stream ) ;
	if( end.find(FileStore::x()+"End") != 0U )
		throw Envelope::ReadError( "no end line" ) ;
}

std::string GStore::EnvelopeImp::readValue( std::istream & stream , const std::string & expected_key , bool * crlf )
{
	std::string line = readLine( stream , crlf ) ;

	std::string prefix = FileStore::x().append(expected_key).append(1U,':') ;
	if( line == prefix )
		return std::string() ;

	prefix.append( 1U , ' ' ) ;
	std::size_t pos = line.find( prefix  ) ;
	if( pos != 0U )
		throw Envelope::ReadError( std::string("expected \"").append(FileStore::x()).append(expected_key).append(":\"") ) ;

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

std::string GStore::EnvelopeImp::readLine( std::istream & stream , bool * crlf )
{
	std::string line = G::Str::readLineFrom( stream ) ;

	if( crlf && !line.empty() )
		*crlf = line.at(line.size()-1U) == '\r' ;

	G::Str::trimRight( line , {"\r",1U} ) ;
	return line ;
}

std::string GStore::EnvelopeImp::value( const std::string & line )
{
	return G::Str::trimmed( G::Str::tail( line , line.find(':') , std::string() ) , G::Str::ws() ) ;
}

G::string_view GStore::EnvelopeImp::bodyTypeName( MessageStore::BodyType type )
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

GStore::MessageStore::BodyType GStore::EnvelopeImp::parseSmtpBodyType( G::string_view s , MessageStore::BodyType default_ )
{
	if( s.empty() )
		return default_ ;
	else if( G::Str::imatch( "7BIT"_sv , s ) )
		return MessageStore::BodyType::SevenBit ;
	else if( G::Str::imatch( "8BITMIME"_sv , s ) )
		return MessageStore::BodyType::EightBitMime ;
	else if( G::Str::imatch( "BINARYMIME"_sv , s ) )
		return MessageStore::BodyType::BinaryMime ;
	else
		return MessageStore::BodyType::Unknown ;
}

G::string_view GStore::EnvelopeImp::smtpBodyType( MessageStore::BodyType type )
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

