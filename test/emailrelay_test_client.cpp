//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// emailrelay_test_client.cpp
//
// A bare-bones smtp client for testing purposes, using blocking
// socket i/o and no event-loop.
//
// Opens multiple connections at start-up and then sends a number
// of email messages on each one in turn.
//
// usage:
//		emailrelay-test-client [-v [-v]] [<port>]
//		emailrelay-test-client [-v [-v]] <addr-ipv4> <port> [<connections> [<iterations> [<lines> [<line-length> [<messages>]]]]]
//

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "gdef.h"
#include <cstring> // std::size_t
#include <iostream>
#include <string>
#include <stdexcept>
#include <exception>
#include <vector>
#include <algorithm>
#include <cstring>
#include <stdlib.h>
#ifndef _WIN32
#include <sys/signal.h>
#endif

int cfg_verbosity = 0 ;

#ifdef _WIN32
typedef int read_size_type ;
typedef int connect_size_type ;
typedef int send_size_type ;
#else
typedef ssize_t read_size_type ;
typedef std::size_t connect_size_type ;
typedef std::size_t send_size_type ;
const int INVALID_SOCKET = -1 ;
#endif

struct Address
{
	union
	{
		sockaddr generic ;
		sockaddr_in specific ;
	} ;
	Address( const char * address , int port )
	{
		static sockaddr_in zero ;
		specific = zero ;
		specific.sin_family = AF_INET ;
		specific.sin_addr.s_addr = inet_addr( address ) ;
		specific.sin_port = htons( port ) ;
	}
	explicit Address( int port )
	{
		static sockaddr_in zero ;
		specific = zero ;
		specific.sin_family = AF_INET ;
		specific.sin_addr.s_addr = htonl( INADDR_LOOPBACK ) ;
		specific.sin_port = htons( port ) ;
	}
	sockaddr * ptr() { return &generic ; }
	const sockaddr * ptr() const { return &generic ; }
	std::size_t size() const { return sizeof(specific) ; }
} ;

struct Test
{
public:
	Test() ;
	void init( const Address & , int messages , int lines , int line_length ) ;
	bool runSome() ;
	bool done() const ;
	void close() ;

private:
	void connect( const Address & ) ;
	void send( const char * ) ;
	void send( const char * , std::size_t ) ;
	void waitline() ;
	void waitline( const char * ) ;
	void sendMessage( int , int ) ;
	static void close_( SOCKET fd ) ;
	static void shutdown( SOCKET fd ) ;
	SOCKET m_fd ;
	int m_messages ;
	int m_lines ;
	int m_line_length ;
	int m_state ;
	bool m_done ;
} ;

Test::Test() :
	m_messages(0) ,
	m_lines(0) ,
	m_line_length(0) ,
	m_state(0) ,
	m_done(false)
{
	m_fd = ::socket( PF_INET , SOCK_STREAM , 0 ) ;
	if( m_fd == INVALID_SOCKET )
		throw std::runtime_error( "invalid socket" ) ;
}

bool Test::done() const
{
	return m_done ;
}

void Test::init( const Address & a , int messages , int lines , int line_length )
{
	m_messages = messages ;
	m_lines = lines ;
	m_line_length = line_length ;
	connect( a ) ;
}

void Test::connect( const Address & a )
{
	if( cfg_verbosity ) std::cout << "connect: fd=" << m_fd << std::endl ;
	int rc = ::connect( m_fd , a.ptr() , static_cast<connect_size_type>(a.size()) ) ;
	if( rc != 0 )
		throw std::runtime_error( "connect error" ) ;
}

bool Test::runSome()
{
	if( m_state == 0 )
	{
		waitline() ; // ident line
		m_state++ ;
	}
	else if( m_state == 1 )
	{
		send( "EHLO test\r\n" ) ;
		waitline( "250 " ) ;
		m_state++ ;
	}
	else if( m_state > 1 && m_state < (m_messages+2) )
	{
		sendMessage( m_lines , m_line_length ) ;
		m_state++ ;
	}
	else
	{
		shutdown( m_fd ) ;
		m_state = 0 ;
		m_done = true ;
	}
	return m_done ;
}

void Test::sendMessage( int lines , int length )
{
	send( "MAIL FROM:<test>\r\n" ) ;
	waitline() ;
	send( "RCPT TO:<test>\r\n" ) ;
	waitline() ;
	send( "DATA\r\n" ) ;
	waitline() ;

	std::vector<char> buffer( static_cast<unsigned>(length+2) ) ;
	std::fill( buffer.begin() , buffer.end() , 't' ) ;
	buffer[buffer.size()-1U] = '\n' ;
	buffer[buffer.size()-2U] = '\r' ;
	for( int i = 0 ; i < lines ; i++ )
		send( &buffer[0] , buffer.size() ) ;

	send( ".\r\n" ) ;
	waitline() ;
}

void Test::waitline()
{
	waitline( "" ) ;
}

void Test::waitline( const char * match )
{
	std::string line ;
	for(;;)
	{
		char c = '\0' ;
		read_size_type rc = ::recv( m_fd , &c , 1U , 0 ) ;
		if( rc <= 0 )
			throw std::runtime_error( "read error" ) ;

		if( c == '\n' && ( *match == '\0' || line.find(match) != std::string::npos ) )
			break ;

		if( c == '\r' ) line.append( "\\r" ) ;
		else if( c == '\n' ) line.append( "\\n" ) ;
		else line.append( 1U , c ) ;
	}
	if( cfg_verbosity )
		std::cout << m_fd << ": rx<<: [" << line << "]" << std::endl ;
}

void Test::send( const char * p )
{
	if( cfg_verbosity ) std::cout << m_fd << ": tx>>: [" << std::string(p,strcspn(p,"\r\n")) << "]" << std::endl ;
	::send( m_fd , p , static_cast<send_size_type>(std::strlen(p)) , 0 ) ;
}

void Test::send( const char * p , std::size_t n )
{
	if( cfg_verbosity > 1 ) std::cout << m_fd << ": tx>>: [<" << n << " bytes>]" << std::endl ;
	::send( m_fd , p , static_cast<send_size_type>(n) , 0 ) ;
}

void Test::shutdown( SOCKET fd )
{
	::shutdown( fd , 1 ) ;
}

void Test::close()
{
	if( cfg_verbosity ) std::cout << "close: fd=" << m_fd << std::endl ;
	close_( m_fd ) ;
}

void Test::close_( SOCKET fd )
{
	#ifdef _WIN32
		::closesocket( fd ) ;
	#else
		::close( fd ) ;
	#endif
}

void init()
{
	#ifdef _WIN32
		WSADATA info ;
		if( ::WSAStartup( MAKEWORD(2,2) , &info ) )
			throw std::runtime_error( "WSAStartup failed" ) ;
	#else
		signal( SIGPIPE , SIG_IGN ) ;
	#endif
}

static int to_int( const char * p )
{
	if( *p == '\0' ) return 0 ;
	if( *p == '-' ) return -1 ;
	return static_cast<int>( ::strtoul(p,nullptr,10) ) ;
}

int main( int argc , char * argv [] )
{
	try
	{
		if( argc > 1 && argv[1][0] == '-' && argv[1][1] == 'h' )
		{
			std::cout
				<< "usage: " << argv[0] << " [-v [-v]] "
				<< "<ipaddress> <port> "
				<< "<connections-in-parallel> "
				<< "<iterations> "
				<< "<lines-per-message> <line-length> "
				<< "<messages-per-connection>"
				<< std::endl ;
			return 0 ;
		}

        while( argc > 1 && argv[1][0] == '-' && argv[1][1] == 'v' )
        {
            cfg_verbosity++ ;
            for( int i = 1 ; (i+1) <= argc ; i++ )
                argv[i] = argv[i+1] ;
            argc-- ;
		}

		const char * arg_address = nullptr ;
		const char * arg_port = "10025" ;
		const char * arg_connections = "1" ;
		const char * arg_iterations = "-1" ;
		const char * arg_lines = "1000" ;
		const char * arg_line_length = "998" ;
		const char * arg_messages = "2" ;
		if( argc == 2 ) arg_port = argv[1] ;
		if( argc > 2 ) arg_address = argv[1] , arg_port = argv[2] ;
		if( argc > 3 ) arg_connections = argv[3] ;
		if( argc > 4 ) arg_iterations = argv[4] ;
		if( argc > 5 ) arg_lines = argv[5] ;
		if( argc > 6 ) arg_line_length = argv[6] ;
		if( argc > 7 ) arg_messages = argv[7] ;
		int port = to_int( arg_port ) ;
		int connections = to_int( arg_connections ) ;
		int iterations = to_int( arg_iterations ) ;
		int lines = to_int( arg_lines ) ;
		int line_length = to_int( arg_line_length ) ;
		int messages = to_int( arg_messages ) ;

		if( cfg_verbosity )
		{
			std::cout << "port: " << port << std::endl ;
			std::cout << "connections-in-parallel: " << connections << std::endl ;
			std::cout << "iterations: " << iterations << std::endl ;
			std::cout << "lines-per-message: " << lines << std::endl ;
			std::cout << "line-length: " << line_length << std::endl ;
			std::cout << "messages-per-connection: " << messages << std::endl ;
		}

		init() ;

		Address a = arg_address ? Address(arg_address,port) : Address(port) ;

		for( int i = 0 ; iterations < 0 || i < iterations ; i++ )
		{
			std::vector<Test> test( connections ) ;
			{ for( size_t t = 0 ; t < test.size() ; t++ ) test[t].init( a , messages , lines , line_length ) ; }
			for( unsigned done_count = 0 ; done_count < test.size() ; )
			{
				for( size_t t = 0 ; t < test.size() ; t++ )
				{
					if( !test[t].done() && test[t].runSome() )
						done_count++ ;
				}
			}
			{ for( size_t t = 0 ; t < test.size() ; t++ ) test[t].close() ; }
		}

		return 0 ;
	}
	catch( std::exception & e )
	{
		std::cerr << "exception: " << e.what() << std::endl ;
	}
	return 1 ;
}

/// \file emailrelay_test_client.cpp
