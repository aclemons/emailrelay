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
		void readEightBitFlag( std::istream & , Envelope & ) ;
		void readFrom( std::istream & , Envelope & ) ;
		void readFromAuthIn( std::istream & , Envelope & ) ;
		void readFromAuthOut( std::istream & , Envelope & ) ;
		void readForwardTo( std::istream & , Envelope & ) ;
		void readForwardToAddress( std::istream & , Envelope & ) ;
		void readToList( std::istream & , Envelope & ) ;
		void readAuthentication( std::istream & , Envelope & ) ;
		void readClientSocketAddress( std::istream & , Envelope & ) ;
		void readClientSocketName( std::istream & , Envelope & ) ;
		void readClientCertificate( std::istream & , Envelope & ) ;
		void readEnd( std::istream & , Envelope & ) ;
	}
}

std::size_t GSmtp::Envelope::write( std::ostream & stream , const GSmtp::Envelope & e )
{
	namespace imp = GSmtp::EnvelopeImp ;
	const std::string x( GSmtp::FileStore::x() ) ;
	const char * crlf = "\r\n" ;

	std::streampos pos = stream.tellp() ;
	if( pos < 0 || stream.fail() )
		return 0U ;

	stream << x << "Format: " << GSmtp::FileStore::format() << crlf ;
	stream << x << "Content: " << (e.m_eight_bit==1?"8bit":(e.m_eight_bit==0?"7bit":"unknown")) << crlf ;
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
	stream << x << "ForwardTo: " << imp::xnormalise(e.m_forward_to) << crlf ;
	stream << x << "ForwardToAddress: " << e.m_forward_to_address << crlf ;
	stream << x << "End: 1" << crlf ;
	stream.flush() ;
	return stream.fail() ? std::size_t(0U) : static_cast<std::size_t>( stream.tellp() - pos ) ;
}

void GSmtp::Envelope::copy( std::istream & in , std::ostream & out )
{
	std::string line ;
	while( in.good() )
	{
		G::Str::readLineFrom( in , "\n" , line ) ;
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
	imp::readEightBitFlag( stream , e ) ;
	imp::readFrom( stream , e ) ;
	imp::readToList( stream , e ) ;
	imp::readAuthentication( stream , e ) ;
	imp::readClientSocketAddress( stream , e ) ;
	if( format == GSmtp::FileStore::format() )
	{
		imp::readClientCertificate( stream , e ) ;
		imp::readFromAuthIn( stream , e ) ;
		imp::readFromAuthOut( stream , e ) ;
		imp::readForwardTo( stream , e ) ; // 2.4
		imp::readForwardToAddress( stream , e ) ; // 2.4
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

void GSmtp::EnvelopeImp::readEightBitFlag( std::istream & stream , Envelope & e )
{
	std::string content = readValue( stream , "Content" ) ;
	e.m_eight_bit = content == "8bit" ? 1 : ( content == "7bit" ? 0 : -1 ) ;
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

void GSmtp::EnvelopeImp::readForwardTo( std::istream & stream , Envelope & e )
{
	e.m_forward_to = readValue( stream , "ForwardTo" ) ;
}

void GSmtp::EnvelopeImp::readForwardToAddress( std::istream & stream , Envelope & e )
{
	e.m_forward_to_address = readValue( stream , "ForwardToAddress" ) ;
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

