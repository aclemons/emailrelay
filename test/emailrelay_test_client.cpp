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
/// \file emailrelay_test_client.cpp
///
// A bare-bones smtp client for testing purposes, using blocking
// socket i/o and no event-loop.
//
// Opens multiple connections at start-up and then sends a number
// of email messages on each one in turn.
//
// usage:
//      emailrelay-test-client [-qQ] [-v [-v]] [<port>]
//      emailrelay-test-client [-qQ] [-v [-v]] <addr-ipv4> <port> [<connections> [<iterations> [<lines> [<line-length> [<messages>]]]]]
//         -v -- verbose logging
//         -q -- send "."&"QUIT" instead of "."
//         -Q -- send "."&"QUIT" and immediately disconnect
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
#include <cstring> // std::strtoul()
#include <cstdlib>
#ifndef _WIN32
#include <csignal>
#endif

int cfg_verbosity = 0 ;
bool cfg_eager_quit = false ;
bool cfg_eager_quit_disconnect = false ;

#ifdef _WIN32
using read_size_type = int ;
using connect_size_type = int ;
using send_size_type = int ;
#else
using read_size_type = ssize_t ;
using connect_size_type = std::size_t ;
using send_size_type = std::size_t ;
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
		if( specific.sin_addr.s_addr == INADDR_NONE )
			throw std::runtime_error( std::string("invalid ipv4 address: ") + address ) ;
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
	void sendMessage( int , int , bool ) ;
	static void close_( SOCKET fd ) ;
	static void shutdown( SOCKET fd ) ;
	SOCKET m_fd ;
	int m_messages{0} ;
	int m_lines{0} ;
	int m_line_length{0} ;
	int m_state{0} ;
	bool m_done{false} ;
} ;

Test::Test()
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
		sendMessage( m_lines , m_line_length , m_state == (m_messages+1) ) ;
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

void Test::sendMessage( int lines , int length , bool last )
{
	send( "MAIL FROM:<test>\r\n" ) ;
	waitline() ;
	send( "RCPT TO:<test>\r\n" ) ;
	waitline() ;
	send( "DATA\r\n" ) ;
	waitline() ;

	std::vector<char> buffer( static_cast<std::size_t>(length)+2U ) ;
	std::fill( buffer.begin() , buffer.end() , 't' ) ;
	buffer[buffer.size()-1U] = '\n' ;
	buffer[buffer.size()-2U] = '\r' ;
	for( int i = 0 ; i < lines ; i++ )
		send( &buffer[0] , buffer.size() ) ;

	if( last && cfg_eager_quit )
	{
		send( ".\r\nQUIT\r\n" ) ;
		if( cfg_eager_quit_disconnect )
		{
			close() ;
			return ;
		}
		waitline() ;
		waitline() ; // again
	}
	else
	{
		send( ".\r\n" ) ;
		waitline() ;
	}
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
	return static_cast<int>( std::strtoul(p,nullptr,10) ) ;
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

		while( argc > 1 && argv[1][0] == '-' && (argv[1][1] == 'v' || argv[1][1] == 'q' || argv[1][1] == 'Q') )
		{
			char c = argv[1][1] ;
			if( c == 'v' ) cfg_verbosity++ ;
			if( c == 'q' ) cfg_eager_quit = true ;
			if( c == 'Q' ) cfg_eager_quit = cfg_eager_quit_disconnect = true ;
			for( int i = 1 ; (i+1) <= argc ; i++ )
				argv[i] = argv[i+1] ;
			argc-- ;
		}

		// by default loop forever with one connection sending two large messages
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

		init() ;

		Address a = arg_address ? Address(arg_address,port) : Address(port) ;

		if( cfg_verbosity )
		{
			std::cout << "address: " << (arg_address?arg_address:"<default>") << std::endl ;
			std::cout << "port: " << port << std::endl ;
			std::cout << "connections-in-parallel: " << connections << std::endl ;
			std::cout << "iterations: " << iterations << std::endl ;
			std::cout << "lines-per-message: " << lines << std::endl ;
			std::cout << "line-length: " << line_length << std::endl ;
			std::cout << "messages-per-connection: " << messages << std::endl ;
		}

		for( int i = 0 ; iterations < 0 || i < iterations ; i++ )
		{
			std::vector<Test> test( connections ) ;
			for( auto & t : test )
			{
				t.init( a , messages , lines , line_length ) ;
			}
			for( unsigned done_count = 0 ; done_count < test.size() ; )
			{
				for( auto & t : test )
				{
					if( !t.done() && t.runSome() )
						done_count++ ;
				}
			}
			for( auto & t : test )
			{
				t.close() ;
			}
		}

		return 0 ;
	}
	catch( std::exception & e )
	{
		std::cerr << "exception: " << e.what() << std::endl ;
	}
	return 1 ;
}

