/*
   Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
   
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later
   version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
   
*/

/*
   poke.c
 
   This is a small program that connects to the
   specified port on the local machine, sends
   a fixed string, and prints out the first
   bit of what it gets sent back.

   In daemon mode it detaches from the terminal,
   writes a pid-file, and then sends the fixed string 
   periodically, discarding any responses.
   
   Its purpose is to provide a low-overhead
   mechanism for stimulating the E-MailRelay
   daemon to send its queued-up messages to
   the remote smtp server.

   If there is an error no output is generated,
   but the exit code is non-zero.

   usage: poke [-d] [<port> [<send-string>]]

*/  

#if defined(G_WINDOWS) || defined(G_WIN32) || defined(_WIN32) || defined(WIN32)
 #ifndef G_WIN32
  #define G_WIN32
 #endif
 #include <windows.h>
 #include <io.h>
 #include <stdlib.h>
 #include <string.h>
 #define write _write
 #define STDOUT_FILENO 1
 void sleep( unsigned int s ) { Sleep( s * 1000UL ) ; }
#else
 #include <unistd.h>
 #include <sys/socket.h>
 #include <netinet/in.h>
 #include <netdb.h>
 #include <arpa/inet.h>
 #include <sys/types.h>
 #include <sys/stat.h>
 #include <fcntl.h>
 #include <stdlib.h>
 #include <string.h>
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

static void detach( void )
{
  #ifndef G_WIN32
	if( fork() ) exit( EXIT_SUCCESS ) ;
	chdir( "/" ) ;
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
		char * p = buffer + sizeof(buffer) - 1 ;
		pid_t pid = getpid() ;
		for( *p = '\0' ; pid != 0 ; pid /= 10 )
			*--p = '0' + (pid%10) ;
		write( fd , p , strlen(p) ) ;
		write( fd , "\n" , 1 ) ;
		close( fd ) ;
	}
  #endif
}

static BOOL poke( int argc , char * argv [] )
{
	const char * const host = "127.0.0.1" ;
	unsigned short port = 10025U ; /* --admin port */
	char buffer[160U] = "FLUSH" ;
	struct sockaddr_in address ;
	int fd , rc ;

	/* parse the command line -- port number */
	if( argc > 1 ) 
	{
		port = atoi(argv[1]) ;
	}

	/* parse the command line -- send string */
	if( argc > 2 )
	{
		buffer[0] = '\0' ;
		strncat( buffer , argv[2] , sizeof(buffer)-5U ) ;
	}
	strcat( buffer , "\015\012" ) ;

	/* open the socket */
	fd = socket( AF_INET , SOCK_STREAM , 0 ) ;
	if( fd < 0 ) 
		return FALSE ;

	/* prepare the address */
	memset( &address , 0 , sizeof(address) ) ;
	address.sin_family = AF_INET ;
	address.sin_port = htons( port ) ;
	address.sin_addr.s_addr = inet_addr( host ) ;

	/* connect */
	rc = connect( fd , (const struct sockaddr*)&address , sizeof(address) ) ;
	if( rc < 0 ) 
	{
		close( fd ) ;
		return FALSE ;
	}

	/* send the string */
	rc = send( fd , buffer , strlen(buffer) , 0 ) ;
	if( rc != (int)strlen(buffer) ) 
	{
		close( fd ) ;
		return FALSE ;
	}

	/* read the reply */
	rc = recv( fd , buffer , sizeof(buffer)-1U , 0 ) ;
	if( rc <= 0 ) 
	{
		close( fd ) ;
		return FALSE ;
	}
	close( fd ) ;

	/* print the reply */
	write( STDOUT_FILENO , buffer , rc ) ;
	buffer[0U] = '\n' ;
	buffer[1U] = '\0' ;
	write( STDOUT_FILENO , buffer , strlen(buffer) ) ;

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

