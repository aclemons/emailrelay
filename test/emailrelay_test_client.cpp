//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// Optionally opens multiple connections at start-up. Sends a number
// of e-mail messages on each one in turn.
//
// usage:
//      emailrelay_test_client [options] <addr-ipv4> <port>
//         -v                 : verbose logging (can be used more than once)
//         -q                 : send "." and "QUIT" instead of "."
//         -Q                 : send "." and "QUIT" and immediately disconnect
//         --log-file <path>  : log file
//         --iterations <n>   : number of program loops (-1 for forever) (default 1)
//         --connections <n>  : number of parallel connections per loop (default 1)
//         --messages <n>     : number of messages per connection (default 1)
//         --recipients <n>   : recipients per message (default 1)
//         --lines <n>        : number of lines per message (default 1000)
//         --line-length <n>  : message line length (default 998)
//         --timeout <s>      : overall timeout (default none)
//         --utf8-domain      : use a UTF-8 domain name in e-mail addresses
//         --smtputf8         : use UTF-8 mailbox names and use SMTPUTF8 MAIL-FROM
//

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "gdef.h"
#include "gsleep.h"
#include "gconvert.h"
#include <cstring> // std::size_t
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <exception>
#include <vector>
#include <algorithm>
#include <memory>
#include <cstring> // std::strtoul()
#include <cstdlib>
#ifndef G_WINDOWS
#include <csignal>
#endif

struct Config
{
	std::string log_file ;
	int verbosity {0} ;
	bool eager_quit {false} ;
	bool eager_quit_disconnect {false} ;
	bool no_wait {false} ;
	std::string address ;
	int port {10025} ;
	int iterations {1} ; // -1 to loop forever
	int connections {1} ;
	int messages {1} ;
	int recipients {1} ;
	int lines {1000} ;
	int line_length {998} ;
	bool utf8_domain {false} ;
	bool smtputf8 {false} ;
	std::string domain {"example.com"} ;
	int timeout {0} ; // seconds until exit()
} ;

#ifdef G_WINDOWS
using read_size_type = int ;
using connect_size_type = int ;
using send_size_type = int ;
#else
using read_size_type = ssize_t ;
using connect_size_type = std::size_t ;
using send_size_type = std::size_t ;
const int INVALID_SOCKET = -1 ;
#endif

std::ostream * log_stream = &std::cout ;

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
	Test( Address , Config ) ;
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
	void sendMessage( int , int , int , std::string , bool , bool ) ;
	static void close_( SOCKET fd ) ;
	static void shutdown( SOCKET fd ) ;
	static std::string printable( std::string ) ;

private:
	SOCKET m_fd ;
	const Config m_config ;
	int m_state {0} ;
	bool m_done {false} ;
	unsigned m_wait {0} ;
} ;

Test::Test( Address address , Config config ) :
	m_config(config)
{
	m_fd = ::socket( PF_INET , SOCK_STREAM , 0 ) ;
	if( m_fd == INVALID_SOCKET )
		throw std::runtime_error( "invalid socket" ) ;
	connect( address ) ;
}

bool Test::done() const
{
	return m_done ;
}

void Test::connect( const Address & a )
{
	if( m_config.verbosity ) std::cout << "connect: fd=" << m_fd << std::endl ;
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
	else if( m_state > 1 && m_state < (m_config.messages+2) )
	{
		sendMessage( m_config.recipients , m_config.lines , m_config.line_length , m_config.domain , m_config.smtputf8 , m_state == (m_config.messages+1) ) ;
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

void Test::sendMessage( int recipients , int lines , int length , std::string domain , bool smtputf8 , bool last )
{
	const std::string a { '\xC4' , '\x80' } ; // u8"\u0100" (u8"" is c++17)
	const std::string b { '\xC6' , '\x80' } ; // u8"\u0180"
	std::string alice = smtputf8 ? (a+"lice") : "alice" ;
	std::string bob = smtputf8 ? (b+"ob") : "bob" ;

	send( "MAIL FROM:<"+alice+"@"+domain+">" + (smtputf8?" SMTPUTF8":"") + "\r\n" ) ;
	waitline() ;
	for( int i = 0 ; i < recipients ; i++ )
	{
		send( "RCPT TO:<"+bob+(recipients>1?std::to_string(i):std::string())+"@"+domain+">\r\n" ) ;
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

	if( last && m_config.eager_quit )
	{
		send( ".\r\nQUIT\r\n" ) ;
		if( m_config.eager_quit_disconnect )
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
	if( m_config.no_wait )
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
	if( m_config.verbosity )
		*log_stream << "fd" << m_fd << ": rx<<: [" << line << "]" << std::endl ;
}

void Test::send( const std::string & s )
{
	send( s.data() , s.size() ) ;
}

void Test::send( const char * p , std::size_t n )
{
	if( m_config.verbosity ) *log_stream << "fd" << m_fd << ": tx>>: [" << printable(std::string(p,n)) << "]" << std::endl ;
	::send( m_fd , p , static_cast<send_size_type>(n) , 0 ) ;
}

void Test::sendData( const char * p , std::size_t n )
{
	if( m_config.verbosity > 1 ) *log_stream << "fd" << m_fd << ": tx>>: [<" << n << " bytes>]" << std::endl ;
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
	if( m_config.verbosity ) *log_stream << "close: fd=" << m_fd << std::endl ;
	close_( m_fd ) ;
}

void Test::close_( SOCKET fd )
{
	#ifdef G_WINDOWS
		::closesocket( fd ) ;
	#else
		::close( fd ) ;
	#endif
}

void init()
{
	#ifdef G_WINDOWS
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
		<< "[--log-file <path>] "
		<< "[--iterations <iterations>] "
		<< "[--connections <connections-in-parallel>] "
		<< "[--messages <messages-per-connection>] "
		<< "[--recipients <recipients-per-message>] "
		<< "[--lines <lines-per-message>] "
		<< "[--line-length <line-length>] "
		<< "[--timeout <seconds>] "
		<< "[--utf8-domain] [--smtputf8] "
		<< "[<ipaddress>] <port>" ;
	return ss.str() ;
}

int main( int argc , char * argv [] )
{
	if( argc > 1 && argv[1][0] == '-' && argv[1][1] == 'h' )
	{
		std::cout << usage(argv[0]) << std::endl ;
		return 0 ;
	}

	Config config ;
	while( argc > 1 )
	{
		int remove = 0 ;
		std::string arg = argv[1] ;
		std::string value = argc > 2 ? argv[2] : "" ;
		if( arg == "-v" ) config.verbosity++ , remove = 1 ;
		if( arg == "-q" ) config.eager_quit = true , remove = 1 ;
		if( arg == "-Q" ) config.eager_quit = config.eager_quit_disconnect = true , remove = 1 ;
		if( arg == "--log-file" ) config.log_file = value , remove = 2 ;
		if( arg == "--iterations" ) config.iterations = to_int(value) , remove = 2 ;
		if( arg == "--connections" ) config.connections = to_int(value) , remove = 2 ;
		if( arg == "--recipients" ) config.recipients = to_int(value) , remove = 2 ;
		if( arg == "--messages" ) config.messages = to_int(value) , remove = 2 ;
		if( arg == "--lines" ) config.lines = to_int(value) , remove = 2 ;
		if( arg == "--line-length" ) config.line_length = to_int(value) , remove = 2 ;
		if( arg == "--utf8-domain" ) config.utf8_domain = true , remove = 1 ;
		if( arg == "--smtputf8" ) config.smtputf8 = true , remove = 1 ;
		if( arg == "--timeout" ) config.timeout = to_int(value) , remove = 2 ;
		if( remove == 0 ) break ;
		while( remove-- && argc > 1 )
		{
			for( int i = 1 ; (i+1) <= argc ; i++ )
				argv[i] = argv[i+1] ;
			argc-- ;
		}
		std::string u { '\xC3' , '\xBC' } ; // u8"\u00FC"
		config.domain = config.utf8_domain ? std::string("b"+u+"cher.example.com") : std::string("example.com") ;
		if( !config.log_file.empty() )
		{
			log_stream = new std::ofstream( config.log_file.c_str() ) ;
			if( !log_stream->good() )
				throw std::runtime_error( "cannot open log file [" + config.log_file + "]" ) ;
		}
	}
	if( argc == 2 )
	{
		config.port = to_int( argv[1] )  ;
	}
	else if( argc == 3 )
	{
		config.address = argv[1] , config.port = to_int( argv[2] ) ;
	}
	else
	{
		std::cerr << usage(argv[0]) << std::endl ;
		return 2 ;
	}

	init() ;

	G::threading::thread_type timer_thread ;
	bool thread_stop = false ;
	try
	{
		if( config.timeout )
		{
			#ifdef G_WINDOWS
			int timeout = config.timeout ;
			timer_thread = std::thread( [&thread_stop,timeout]() {
					for( int i = 0 ; i < timeout ; i++ )
					{
						sleep( 1 ) ;
						if( thread_stop )
							break ;
					}
					if( !thread_stop )
					{
						*log_stream << "timed out" << std::endl ;
						exit( 1 ) ;
					}
				}) ;
			#else
			thread_stop = !thread_stop ;
			alarm( config.timeout ) ; // SIGALRM
			#endif
		}

		Address address = config.address.empty() ? Address(config.port) : Address(config.address.c_str(),config.port) ;

		if( config.verbosity )
		{
			std::cout << "address: " << (config.address.length()?config.address:"<default>") << std::endl ;
			std::cout << "port: " << config.port << std::endl ;
			std::cout << "iterations: " << config.iterations << std::endl ;
			std::cout << "connections: " << config.connections << std::endl ;
			std::cout << "messages: " << config.messages << std::endl ;
			std::cout << "recipients: " << config.recipients << std::endl ;
			std::cout << "lines: " << config.lines << std::endl ;
			std::cout << "line-length: " << config.line_length << std::endl ;
		}

		for( int i = 0 ; config.iterations < 0 || i < config.iterations ; i++ )
		{
			std::vector<std::shared_ptr<Test>> tests ;
			for( int t = 0 ; t < config.connections ; t++ )
				tests.push_back( std::make_shared<Test>(address,config) ) ;
			for( unsigned done_count = 0 ; done_count < tests.size() ; )
			{
				for( auto & t : tests )
				{
					if( !t->done() && t->runSome() )
						done_count++ ;
				}
			}
			for( auto & t : tests )
			{
				t->close() ;
			}
		}

		thread_stop = true ;
		if( timer_thread.joinable() ) timer_thread.join() ;
		return 0 ;
	}
	catch( std::exception & e )
	{
		std::cerr << "exception: " << e.what() << std::endl ;
	}
	thread_stop = true ;
	if( timer_thread.joinable() ) timer_thread.join() ;
	return 1 ;
}

