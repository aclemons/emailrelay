/*
   Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
   poke.c

   This is a small program that connects to the specified port on the local
   machine, sends a fixed string, and prints out the first bit of what it
   gets sent back.

   In daemon mode it detaches from the terminal, writes a pid-file, and then
   sends the fixed string periodically, discarding any responses.

   Its purpose is to provide a low-overhead mechanism for stimulating the
   E-MailRelay daemon to send its queued-up messages to the remote smtp server.

   If there is an error no output is generated, but the exit code is non-zero.

   usage: poke [-d] [<port> [<send-string>]]

*/

#include "gdef.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef G_UNIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
typedef int socket_t ;
#else
#include <io.h>
#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#endif
#define sleep( x ) Sleep( (x) * 1000 )
typedef SOCKET socket_t ;
#endif

#ifndef TRUE
 #define TRUE 1
#endif
#ifndef FALSE
 #define FALSE 0
#endif
#ifndef BOOL
 #define BOOL int
#endif

static ssize_t write_( socket_t fd , const void * p , size_t n )
{
 #ifdef G_WIN32
	return _write( (int)fd , p , (unsigned int)n ) ;
 #else
	return write( fd , p , n ) ;
 #endif
}

static void close_( socket_t fd )
{
 #ifdef G_WIN32
	_close( (int)fd ) ;
 #else
	close( fd ) ;
 #endif
}

static void init( void )
{
  #ifdef G_WIN32
	static WSADATA info ;
	if( 0 != WSAStartup( MAKEWORD(1,1) , &info ) )
		; /* error */
  #endif
}

static void detach( void )
{
  #ifndef G_WIN32
	if( fork() ) exit( EXIT_SUCCESS ) ;
	{ int rc = chdir( "/" ) ; (void) rc ; }
	setsid() ;
	if( fork() ) exit( EXIT_SUCCESS ) ;
	close( STDIN_FILENO ) ;
	close( STDOUT_FILENO ) ;
	close( STDERR_FILENO ) ;
  #endif
}

static void pidfile( void )
{
  #ifndef G_WIN32
	int fd = open( "/var/run/emailrelay-poke.pid" , O_CREAT | O_WRONLY | O_TRUNC , 0666 ) ;
	if( fd >= 0 )
	{
		char buffer[30] ;
		ssize_t rc ;
		char * p = buffer + sizeof(buffer) - 1 ;
		pid_t pid = getpid() ;
		for( *p = '\0' ; pid != 0 ; pid /= 10 )
			*--p = '0' + (pid%10) ;
		rc = write_( fd , p , strlen(p) ) ;
		rc += write_( fd , "\n" , 1 ) ;
		close( fd ) ;
	}
  #endif
}

static BOOL poke( int argc , char * argv [] )
{
	const char * const host = "127.0.0.1" ;
	unsigned short port = 10025U ; /* --admin port */
	char buffer[160U] = "FLUSH" ;
	union {
		struct sockaddr_in specific ;
		struct sockaddr generic ;
	} address ;
	socket_t fd ;
	int rc ;
	ssize_t n ;

	/* parse the command line -- port number */
	if( argc > 1 )
	{
		port = (unsigned short)atoi(argv[1]) ;
	}

	/* parse the command line -- send string */
	if( argc > 2 )
	{
		buffer[0] = '\0' ;
		#ifdef G_WIN32
			strncat_s( buffer , sizeof(buffer) , argv[2] , _TRUNCATE ) ;
		#else
			strncat( buffer , argv[2] , sizeof(buffer)-5U ) ;
		#endif
	}
	#ifdef G_WIN32
		strcat_s( buffer , sizeof(buffer) , "\015\012" ) ;
	#else
		strcat( buffer , "\015\012" ) ;
	#endif

	/* open the socket */
	fd = socket( AF_INET , SOCK_STREAM , 0 ) ;
	if( fd < 0 )
	{
		return FALSE ;
	}

	/* prepare the address */
	memset( &address , 0 , sizeof(address.specific) ) ;
	address.specific.sin_family = AF_INET ;
	address.specific.sin_port = htons( port ) ;
	if( 1 != inet_pton( AF_INET , host , &address.specific.sin_addr ) )
		return FALSE ;

	/* connect */
	rc = connect( fd , &address.generic , (socklen_t)(sizeof(address.generic)) ) ;
	if( rc < 0 )
	{
		close_( fd ) ;
		return FALSE ;
	}

	/* send the string */
	n = send( fd , buffer , (socklen_t)strlen(buffer) , 0 ) ;
	if( n < 0 || (size_t)n != strlen(buffer) )
	{
		close_( fd ) ;
		return FALSE ;
	}

	/* read the reply */
	n = recv( fd , buffer , (socklen_t)(sizeof(buffer)-1U) , 0 ) ;
	if( n <= 0 )
	{
		close_( fd ) ;
		return FALSE ;
	}
	close_( fd ) ;

	/* print the reply */
	n = write_( STDOUT_FILENO , buffer , (size_t)n ) ;
	buffer[0U] = '\n' ;
	buffer[1U] = '\0' ;
	n += write_( STDOUT_FILENO , buffer , strlen(buffer) ) ;

	return TRUE ;
}

int main( int argc , char * argv [] )
{
	/* parse the command line -- daemon switch */
	BOOL daemon = FALSE ;
	if( argc > 1 && strcmp(argv[1],"-d") == 0 )
	{
		daemon = TRUE ;
		argc-- ;
		argv++ ;
	}

	/* run once, or in a loop */
	init() ;
	if( daemon )
	{
		detach() ;
		pidfile() ;
		for(;;)
		{
			poke( argc , argv ) ;
			sleep( 60 ) ;
		}
		return EXIT_FAILURE ;
	}
	else
	{
		return poke( argc , argv ) ? EXIT_SUCCESS : EXIT_FAILURE ;
	}
}
