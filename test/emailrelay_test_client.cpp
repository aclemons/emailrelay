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
// emailrelay_test_client.cpp
//
// A bare-bones smtp client for testing purposes, using blocking
// socket i/o, no event-loop and very little error checking.
//
// usage: emailrelay-test-client { <ip-address> <port> | <port> }
//

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "gdef.h"
#include <iostream>
#include <stdexcept>
#include <exception>
#include <vector>
#include <algorithm>
#include <cstring>
#include <stdlib.h>

#ifdef _WIN32
typedef int size_type ;
#else
typedef size_t size_type ;
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
	size_t size() const { return sizeof(specific) ; }
} ;

struct Test
{
	explicit Test( SOCKET fd ) : m_fd(fd) {}
	void run() ;
	void send( const char * ) ;
	void send( const char * , size_t ) ;
	void sendMessage() ;
	static void sleep_( int s ) ;
	static void close_( SOCKET fd ) ;
	static void shutdown( SOCKET fd ) ;
	SOCKET m_fd ;
} ;

void Test::run()
{
	send( "EHLO test\r\n" ) ;
	for( int i = 0 ; i < 2 ; i++ )
		sendMessage() ;
	shutdown( m_fd ) ;
}

void Test::sendMessage()
{
	send( "MAIL FROM:<test>\r\n" ) ;
	send( "RCPT TO:<test>\r\n" ) ;
	send( "DATA\r\n" ) ;

	std::vector<char> buffer( 1000U ) ;
	std::fill( buffer.begin() , buffer.end() , 't' ) ;
	buffer[buffer.size()-1U] = '\n' ;
	buffer[buffer.size()-2U] = '\r' ;
	for( int i = 0 ; i < 100000 ; i++ )
		send( &buffer[0] , buffer.size() ) ;

	send( ".\r\n" ) ;
	sleep_( 1 ) ;
}

void Test::send( const char * p )
{
	::send( m_fd , p , static_cast<size_type>(std::strlen(p)) , 0 ) ;
	sleep_( 1 ) ;
}

void Test::send( const char * p , size_t n )
{
	::send( m_fd , p , static_cast<size_type>(n) , 0 ) ;
}

void Test::shutdown( SOCKET fd )
{
	::shutdown( fd , 1 ) ;
}

void Test::close_( SOCKET fd )
{
	#ifdef _WIN32
		::closesocket( fd ) ;
	#else
		::close( fd ) ;
	#endif
}

void Test::sleep_( int s )
{
	#ifdef _WIN32
		::Sleep( s * 1000 ) ;
	#else
		::sleep( s ) ;
	#endif
}

void check_valid( SOCKET fd )
{
	if( fd == INVALID_SOCKET )
		throw std::runtime_error( "invalid socket" ) ;
}

void check_zero( int rc )
{
	if( rc != 0 )
		throw std::runtime_error( "failed" ) ;
}

void init()
{
	#ifdef _WIN32
		WSADATA info ;
		::WSAStartup( MAKEWORD(2,2) , &info ) ;
	#endif
}

int main( int argc , char * argv [] )
{
	try
	{
		const char * arg_address = nullptr ;
		const char * arg_port = "10025" ;
		if( argc > 2 )
		{
			arg_address = argv[1] ;
			arg_port = argv[2] ;
		}
		else if( argc > 1 )
		{
			arg_port = argv[1] ;
		}
		int port = static_cast<int>( ::strtoul(arg_port,nullptr,10) ) ;

		init() ;

		Address a = arg_address ? Address(arg_address,port) : Address(port) ;

		for(;;)
		{
			SOCKET fd = ::socket( PF_INET , SOCK_STREAM , 0 ) ;
			check_valid( fd ) ;

			int rc = ::connect( fd , a.ptr() , static_cast<size_type>(a.size()) ) ;
			check_zero( rc ) ;

			Test test( fd ) ;
			test.run() ;

			Test::close_( fd ) ;
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
