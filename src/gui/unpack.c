/*
   Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
   
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
	unpack.c
*/

#include "gdef.h"
#include "unpack.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef int bool ;
static const bool true = 1 ;
static const bool false = 0 ;

#define ignore( p ) { p++ ; }
#define check_that( b , c ) { check_m( m , b , c ) ; if(!(b)) return false ; }

typedef void (*Fn)( const char * ) ;
typedef struct Entry_tag
{ 
	struct Entry_tag * next ;
	char * path ; 
	unsigned long size ; 
	unsigned long offset ; 
} Entry ;
typedef struct M_tag
{
	Fn fn ;
	unsigned long max_size ;
	Entry * map ;
	char * path ;
	FILE * input ;
	unsigned long start ;
	char * buffer ;
} M ;

#ifdef _WIN32
static const char * r = "rb" ;
static const char * w = "wb" ;
#else
static const char * r = "r" ;
static const char * w = "w" ;
#endif

static void unpack_delete_( M * m ) ;
static bool unpack_init( M * m ) ;

/* still compile if no zlib but exit at run-time */
#ifdef HAVE_ZLIB_H
 #include <zlib.h>
#else
 #pragma warning("no zlib")
 typedef char Bytef ;
 char * const Z_NULL = 0 ;
 int Z_SYNC_FLUSH = 0 ;
 int Z_STREAM_END = 0 ;
 int Z_OK = 0 ;
 typedef struct z_stream_tag {
 const char * next_in ; unsigned avail_in ; const char * next_out ; 
 unsigned avail_out ; const char * zalloc ; const char * zfree ; const char * opaque ; } z_stream ;
 static int inflateInit( z_stream * s ) 
 { 
  ignore(s) ;
  fprintf( stderr , "%s\n" , "no zlib available at compile-time" ) ;
  exit(1) ;
 }
 static int inflate( z_stream * s , int i ) { ignore(s) ; ignore(i) ; return 0 ; }
 static int inflateEnd( z_stream * s ) { ignore(s) ; return 0 ; }
#endif

static char * strdup_( const char * src )
{
	char * dst = src ? malloc(strlen(src)+1) : NULL ;
	if( dst && src )
		strcpy( dst , src ) ;
	return dst ;
}

static Bytef * zptr( char * p )
{
	return (Bytef*)p ;
}

static M * unpack_new_( const char * path , Fn fn )
{
	if( path == NULL ) return NULL ;

	M * m = (M*) malloc( sizeof(M) ) ;
	if( m == NULL ) return NULL ;

	m->fn = fn ;
	m->max_size = 0UL ;
	m->map = NULL ;
	m->path = strdup_( path ) ;
	m->input = NULL ;
	m->start = 0 ;
	m->buffer = NULL ;

	if( ! unpack_init(m) )
	{
		unpack_delete_(m) ;
		m = NULL ;
	}

	return m ;
}

static void unpack_delete_( M * m )
{
	if( !m ) return ;
	if( m->path ) free( m->path ) ;
	if( m->input ) fclose( m->input ) ;
	if( m->buffer ) free( m->buffer ) ;
	if( m->map )
	{
		Entry * p = m->map ;
		while( p != NULL )
		{
			Entry * temp = p ;
			if( p->path ) free( p->path ) ;
			p = p->next ;
			free( temp ) ;
		}
	}
	if( m ) free( m ) ;
}

static int unpack_count_( const M * m )
{
	const Entry * p = NULL ;
	if( m == NULL ) return 0 ;
	int n = 0 ;
	for( p = m->map ; p != NULL ; p = p->next , n++ )
		/* no-op */ ;
	return n ;
}

static const Entry * unpack_entry( const M * m , int n )
{
	const Entry * p = NULL ;
	if( m != NULL && n >= 0 ) 
	{
		int i = 0 ;
		for( p = m->map ; p != NULL ; p = p->next , i++ )
		{
			if( i == n ) break ;
		}
	}
	return p ;
}

static char * unpack_name_( const M * m , int n )
{
	const Entry * p = unpack_entry( m , n ) ;
	const char * name = p ? p->path : "" ;
	return strdup_(name) ;
}

static unsigned long unpack_packed_size_( const M * m , int n )
{
	const Entry * p = unpack_entry( m , n ) ;
	return p ? p->size : 0UL ;
}

static unsigned long file_size( const char * path )
{
	static struct stat zero ;
	if( path == NULL ) return 0UL ;
	struct stat buf = zero ;
	int rc = stat( path , &buf ) ;
	if( rc != 0 ) buf.st_size = 0 ;
	return buf.st_size ;
}

static void check_m( M * m , bool ok , const char * message )
{
	if( !ok )
		if( m && m->fn ) (*(m->fn))(message) ;
}

static bool unpack_init( M * m )
{
	unsigned long exe_size = 0UL ;
	unsigned long exe_offset = 0UL ;
	unsigned long file_offset = 0UL ;
	int rc = 0 ;
	Entry * tail = NULL ;

	if( m == NULL ) 
		return false ;

	/* check the file exists */
	m->input = fopen( m->path , r ) ;
	check_that( m->input != NULL , "open error" ) ;
	fclose( m->input ) ;
	m->input = NULL ;

	/* get the file size */
	exe_size = file_size( m->path ) ;
	check_that( exe_size > 12UL , "invalid exe size" ) ;

	/* seek to near the end */
	m->input = fopen( m->path , r ) ;
	check_that( m->input != NULL , "open error" ) ;
	rc = fseek( m->input , exe_size-12UL , SEEK_SET ) ;
	check_that( rc == 0 , "seek error" ) ;

	/* read the original size */
	exe_offset = 0UL ;
	rc = fscanf( m->input , "%lu" , &exe_offset ) ;
	check_that( rc == 1 , "offset format error" ) ;
	check_that( exe_offset != 0UL , "invalid offset" ) ;
	check_that( (exe_offset+12U) < exe_size , "invalid offset" ) ;
	check_that( !feof(m->input) && !ferror(m->input) , "offset read error" ) ;

	/* seek to the directory */
	rc = fseek( m->input , exe_offset , SEEK_SET ) ;
	check_that( !feof(m->input) && !ferror(m->input) , "table seek error" ) ;

	/* read the directory */
	file_offset = 0UL ;
	for(;;)
	{
		char * file_path = NULL ;
		unsigned long file_size = 0UL ;
		char big_buffer[10001] ;

		rc = fscanf( m->input , "%lu" , &file_size ) ;
		check_that( rc == 1 , "table entry size error" ) ;
		rc = fscanf( m->input , "%10000s" , big_buffer ) ; /* TODO */
		check_that( rc == 1 , "table entry read error" ) ;
		big_buffer[sizeof(big_buffer)-1] = '\0' ;
		file_path = strdup_(big_buffer) ;
		check_that( file_path != NULL , "out of memory" ) ;

		if( file_size == 0U ) 
		{
			check_that( strcmp(file_path,"end") == 0 , "invalid internal directory" ) ;
			free( file_path ) ;
			break ;
		}

		Entry * entry = malloc( sizeof(Entry) ) ;
		check_that( entry != NULL , "out of memory" ) ;
		entry->next = NULL ;
		entry->path = file_path ;
		entry->size = file_size ;
		entry->offset = file_offset ;

		if( tail == NULL )
			m->map = entry ;
		else
			tail->next = entry ;
		tail = entry ;

		file_offset += file_size ;
		if( file_size > m->max_size )
			m->max_size = file_size ;
	}

	/* reserve a buffer */
	check_that( m->max_size < 100000000UL , "too big" ) ; /* sanity limit */
	m->buffer = malloc( m->max_size + 1UL ) ;
	check_that( m->buffer != NULL , "out of memory" ) ;

	/* eat the newline */
	rc = fgetc( m->input ) ;
	check_that( rc != EOF , "file-map read error" ) ;
	m->start = ftell( m->input ) ;

	return true ;
}

static bool unpack_imp( M * m , const char * to_dir , const Entry * entry )
{
	int rc = 0 ;
	FILE * output = NULL ;

	/* belt-and-braces */
	if( m == NULL ) return false ;
	check_that( to_dir != NULL , "usage error" ) ;
	check_that( entry != NULL , "internal error" ) ;

	/* sync up */
	rc = fseek( m->input , m->start + entry->offset , SEEK_SET ) ;
	check_that( rc == 0 , "seek error" ) ;

	/* read file data */
	{
		unsigned long i = 0UL ;
		char * p = m->buffer ;
		for( ; i < entry->size && !ferror(m->input) ; i++ , p++ )
		{
			int c = fgetc( m->input ) ;
			if( c == EOF || feof(m->input) )
				break ;
			*p = (char)c ;
		}
		check_that( !ferror(m->input) , "read error" ) ;
		check_that( i == entry->size , "read error (2)" ) ;
	}

	/* open the output */
	output = NULL ;
	{
		char * to_path = NULL ;

		to_path = malloc( strlen(to_dir) + 1 + strlen(entry->path) + 1 ) ;
		check_that( to_path != NULL , "out of memory" ) ;
		strcpy( to_path , to_dir ) ;
		strcat( to_path , "/" ) ;
		strcat( to_path , entry->path ) ;

		output = fopen( to_path , w ) ;
		free( to_path ) ;
		to_path = NULL ;
		check_that( output != NULL , "cannot open output" ) ;
	}

	/* unzip the buffer into the output file */
	{
		char buffer_out[1024U*8U] ;
		z_stream z ;
		z.next_in = zptr(m->buffer) ;
		z.avail_in = entry->size ;
		z.next_out = zptr(buffer_out) ;
		z.avail_out = sizeof(buffer_out) ;
		z.zalloc = Z_NULL ;
		z.zfree = Z_NULL ;
		z.opaque = 0 ;
		rc = inflateInit( &z ) ;
		if( rc != Z_OK ) fclose(output) ;
		check_that( rc == Z_OK , "inflateInit() error" ) ;
		for(;;)
		{
			unsigned int n = 0U ;
			size_t n_written = 0U ;
			z.next_out = zptr(buffer_out) ;
			z.avail_out = sizeof(buffer_out) ;
			rc = inflate( &z , Z_SYNC_FLUSH ) ;
			if( rc != Z_OK && rc != Z_STREAM_END )
			{
				fclose(output) ;
				inflateEnd( &z ) ;
				check_that( false , "inflate() error" ) ;
			}
			n = sizeof(buffer_out) - z.avail_out ;
			n_written = fwrite( buffer_out , 1 , n , output ) ;
			if( n != n_written ) fclose(output) ;
			check_that( n == n_written , "write error" ) ;
			if( rc == Z_STREAM_END ) break ;
		}
		rc = inflateEnd( &z ) ;
		check_that( rc == Z_OK , "inflateEnd() error" ) ;
	}

	check_that( !ferror(output) , "write error" ) ;
	rc = fclose( output ) ;
	check_that( rc == 0 , "close error" ) ;
	return true ;
}

static bool unpack_all_( M * m , const char * to_dir )
{
	const Entry * p = NULL ;
	if( m == NULL ) return false ;
	for( p = m->map ; p != NULL ; p = p->next )
	{
		bool ok = unpack_imp( m , to_dir , p ) ;
		if( !ok )
			return false ;
	}
	return true ;
}

static bool unpack_file_( M * m , const char * to_dir , const char * name )
{
	const Entry * p = NULL ;
	bool ok = false ;
	if( m == NULL ) return false ;
	for( p = m->map ; p != NULL ; p = p->next )
	{
		if( 0 == strcmp(p->path,name) )
		{
			ok = unpack_imp( m , to_dir , p ) ;
			break ;
		}
	}
	return ok ;
}

/* 
== 
*/

Unpack * unpack_new( const char * exe_path , UnpackErrorHandler fn )
{
	return (Unpack*) unpack_new_( exe_path , (Fn)fn ) ;
}

int unpack_count( const Unpack * p )
{
	return unpack_count_( (const M*)p ) ;
}

char * unpack_name( const Unpack * p , int i )
{
	return unpack_name_( (const M*)p , i ) ;
}

void unpack_free( char * p )
{
	free( p ) ;
}

unsigned long unpack_packed_size( const Unpack * p , int i )
{
	return unpack_packed_size_( (const M*)p , i ) ;
}

int unpack_all( Unpack * p , const char * to_dir )
{
	return unpack_all_( (M*)p , to_dir ) ;
}

int unpack_file( Unpack * p , const char * to_dir , const char * name )
{
	return unpack_file_( (M*)p , to_dir , name ) ;
}

void unpack_delete( Unpack * p )
{
	unpack_delete_( (M*)p ) ;
}

