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
/// \file emailrelay_test_client.cpp
///
// A bare-bones smtp client for testing purposes, using blocking
// socket i/o and no event-loop.
//
// Opens multiple connections at start-up and then sends a number
// of email messages on each one in turn.
//
// usage:
//      emailrelay_test_client [-qQ] [-v [-v]] [<port>]
//      emailrelay_test_client [-qQ] [-v [-v]] <addr-ipv4> <port> [<connections> [<iterations> [<lines> [<line-length> [<messages>]]]]]
//         -v -- verbose logging
//         -q -- send "."&"QUIT" instead of "."
//         -Q -- send "."&"QUIT" and immediately disconnect
//

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "gdef.h"
#include "gsleep.h"
#include <cstring> // std::size_t
#include <sstream>
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

struct Config
{
	int verbosity {0} ;
	bool eager_quit {false} ;
	bool eager_quit_disconnect {false} ;
	bool no_wait {false} ;
	std::string address ;
	int port {10025} ;
	int connections {1} ;
	int iterations {-1} ;
	int lines {1000} ;
	int line_length {998} ;
	int messages {2} ;
} cfg ;

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
		specific.sin_port = htons( static_cast<g_port_t>(port) ) ;
	}
	explicit Address( int port )
	{
		static sockaddr_in zero ;
		specific = zero ;
		specific.sin_family = AF_INET ;
		specific.sin_addr.s_addr = htonl( INADDR_LOOPBACK ) ;
		specific.sin_port = htons( static_cast<g_port_t>(port) ) ;
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
	void send( const std::string & ) ;
	void send( const char * , std::size_t ) ;
	void sendData( const char * , std::size_t ) ;
	void waitline() ;
	void waitline( const char * ) ;
	void sendMessage( int , int , int , bool ) ;
	static void close_( SOCKET fd ) ;
	static void shutdown( SOCKET fd ) ;
	static std::string printable( std::string ) ;

private:
	SOCKET m_fd ;
	int m_messages {0} ;
	int m_lines {0} ;
	int m_recipients {3} ;
	int m_line_length {0} ;
	int m_state {0} ;
	bool m_done {false} ;
	unsigned m_wait {0} ;
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
	if( cfg.verbosity ) std::cout << "connect: fd=" << m_fd << std::endl ;
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
		sendMessage( m_recipients , m_lines , m_line_length , m_state == (m_messages+1) ) ;
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

void Test::sendMessage( int recipients , int lines , int length , bool last )
{
	send( "MAIL FROM:<test>\r\n" ) ;
	waitline() ;
	for( int i = 0 ; i < recipients ; i++ )
	{
		send( "RCPT TO:<test>\r\n" ) ;
		waitline() ;
	}
	send( "DATA\r\n" ) ;
	waitline() ;

	std::vector<char> buffer( static_cast<std::size_t>(length)+2U ) ;
	std::fill( buffer.begin() , buffer.end() , 't' ) ;
	buffer[buffer.size()-1U] = '\n' ;
	buffer[buffer.size()-2U] = '\r' ;
	for( int i = 0 ; i < lines ; i++ )
		sendData( &buffer[0] , buffer.size() ) ;

	if( last && cfg.eager_quit )
	{
		send( ".\r\nQUIT\r\n" ) ;
		if( cfg.eager_quit_disconnect )
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
	if( cfg.no_wait )
	{
		m_wait++ ;
		return ;
	}

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
	if( cfg.verbosity )
		std::cout << m_fd << ": rx<<: [" << line << "]" << std::endl ;
}

void Test::send( const std::string & s )
{
	send( s.data() , s.size() ) ;
}

void Test::send( const char * p , std::size_t n )
{
	if( cfg.verbosity ) std::cout << m_fd << ": tx>>: [" << printable(std::string(p,n)) << "]" << std::endl ;
	::send( m_fd , p , static_cast<send_size_type>(n) , 0 ) ;
}

void Test::sendData( const char * p , std::size_t n )
{
	if( cfg.verbosity > 1 ) std::cout << m_fd << ": tx>>: [<" << n << " bytes>]" << std::endl ;
	::send( m_fd , p , static_cast<send_size_type>(n) , 0 ) ;
}

std::string Test::printable( std::string s )
{
	return s.substr( 0U , s.find_first_of("\r\n") ) ;
}

void Test::shutdown( SOCKET fd )
{
	::shutdown( fd , 1 ) ;
}

void Test::close()
{
	if( cfg.verbosity ) std::cout << "close: fd=" << m_fd << std::endl ;
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

static int to_int( const std::string & s )
{
	if( s.empty() || s.at(0) == '-' ) throw std::runtime_error( "not a number" ) ;
	const char * p = s.c_str() ;
	char * end = nullptr ;
	auto n = std::strtoul( p , &end , 10 ) ;
	if( end != (p+s.size()) ) throw std::runtime_error( "not a number" ) ;
	return static_cast<int>( n ) ;
}

std::string usage( const char * argv0 )
{
	std::ostringstream ss ;
	ss
		<< "usage: " << argv0 << " [-q | -Q] [-v [-v]] "
		<< "[--connections <connections-in-parallel>] "
		<< "[--iterations <iterations>] "
		<< "[--lines <lines-per-message>] "
		<< "[--line-length <line-length>] "
		<< "[--messages <messages-per-connection>] "
		<< "[<ipaddress>] <port>" ;
	return ss.str() ;
}

int main( int argc , char * argv [] )
{
	try
	{
		if( argc > 1 && argv[1][0] == '-' && argv[1][1] == 'h' )
		{
			std::cout << usage(argv[0]) << std::endl ;
			return 0 ;
		}

		// by default loop forever with one connection sending two large messages

		while( argc > 1 )
		{
			int remove = 0 ;
			std::string arg = argv[1] ;
			std::string value = argc > 2 ? argv[2] : "" ;
			if( arg == "-v" ) cfg.verbosity++ , remove = 1 ;
			if( arg == "-q" ) cfg.eager_quit = true , remove = 1 ;
			if( arg == "-Q" ) cfg.eager_quit = cfg.eager_quit_disconnect = true , remove = 1 ;
			if( arg == "--connections" ) cfg.connections = to_int(value) , remove = 2 ;
			if( arg == "--iterations" ) cfg.iterations = to_int(value) , remove = 2 ;
			if( arg == "--lines" ) cfg.lines = to_int(value) , remove = 2 ;
			if( arg == "--line-length" ) cfg.line_length = to_int(value) , remove = 2 ;
			if( arg == "--messages" ) cfg.messages = to_int(value) , remove = 2 ;
			if( remove == 0 ) break ;
			while( remove-- && argc > 1 )
			{
				for( int i = 1 ; (i+1) <= argc ; i++ )
					argv[i] = argv[i+1] ;
				argc-- ;
			}
		}
		if( argc == 2 )
		{
			cfg.port = to_int( argv[1] )  ;
		}
		else if( argc == 3 )
		{
			cfg.address = argv[1] , cfg.port = to_int( argv[2] ) ;
		}
		else
		{
			std::cerr << usage(argv[0]) << std::endl ;
			return 2 ;
		}

		init() ;

		Address a = cfg.address.empty() ? Address(cfg.port) : Address(cfg.address.c_str(),cfg.port) ;

		//if( cfg.verbosity )
		{
			std::cout << "connections: " << cfg.connections << std::endl ;
			std::cout << "iterations: " << cfg.iterations << std::endl ;
			std::cout << "lines: " << cfg.lines << std::endl ;
			std::cout << "line-length: " << cfg.line_length << std::endl ;
			std::cout << "messages: " << cfg.messages << std::endl ;
			std::cout << "address: " << (cfg.address.length()?cfg.address:"<default>") << std::endl ;
			std::cout << "port: " << cfg.port << std::endl ;
		}

		for( int i = 0 ; cfg.iterations < 0 || i < cfg.iterations ; i++ )
		{
			std::vector<Test> test( cfg.connections ) ;
			for( auto & t : test )
			{
				t.init( a , cfg.messages , cfg.lines , cfg.line_length ) ;
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

