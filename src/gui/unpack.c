/*
   Copyright (C) 2001-2008 Graeme Walker <graeme_walker@users.sourceforge.net>
   
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
	char * flags ; 
} Entry ;
typedef struct M_tag
{
	Fn fn ;
	unsigned long max_size ;
	bool compressed ;
	Entry * map ;
	char * path ;
	FILE * input ;
	unsigned long start ;
	char * buffer ;
} M ;

#ifdef _WIN32
static const char * r = "rb" ;
static const char * w = "wb" ;
static int os_mkdir( const char * p , int mode ) { return _mkdir( p ) ; }
 #ifndef G_MINGW
  static int access( const char * p , int mode ) { return _access( p , mode ) ; }
 #endif
#else
static int os_mkdir( const char * p , int mode ) { return mkdir( p , mode ) ; }
static const char * r = "r" ;
static const char * w = "w" ;
#endif

static void unpack_delete_( M * m ) ;
static bool unpack_init( M * m ) ;

/* still compile if no zlib but exit at run-time */
#ifdef NO_ZLIB
 #ifdef HAVE_ZLIB_H
  #undef HAVE_ZLIB_H
 #endif
#endif
#ifdef HAVE_ZLIB_H
 #include <zlib.h>
 static bool have_zlib( void ) { return true ; }
#else
 static bool have_zlib( void ) { return false ; }
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

static void replace_all( char * p , size_t n , char c1 , char c2 )
{
	for( ; p && *p && n ; p++ , n-- )
	{
		if( *p == c1 )
			*p = c2 ;
	}
}

static char * first_slash( char * p )
{
	char * p1 = p ? strchr( p , '/' ) : NULL ;
	char * p2 = p ? strchr( p , '\\' ) : NULL ;
	return ( p1 && p2 ) ? ( p1 > p2 ? p1 : p2 ) : ( p1 ? p1 : p2 ) ;
}

static char * last_slash( char * p )
{
	char * p1 = p ? strrchr( p , '/' ) : NULL ;
	char * p2 = p ? strrchr( p , '\\' ) : NULL ;
	return ( p1 && p2 ) ? ( p1 > p2 ? p1 : p2 ) : ( p1 ? p1 : p2 ) ;
}

static char * after( char * p )
{
	return p ? (p+1) : p ;
}

static bool is_slash( const char * p )
{
	return p && ( *p == '/' || *p == '\\' ) ;
}

static char * chomp( char * path )
{
	char * nul ;
	bool windows_has_drive = strlen(path) > 2U && path[1] == ':' ;
	bool windows_is_unc = strlen(path) > 2U && is_slash(path+0) && is_slash(path+1) ;
	char * unc_root = first_slash( after(first_slash(path+2)) ) ;
	char * p = last_slash( path ) ;
	bool is_root = ( p == path ) || ( windows_has_drive && p == (path+2) ) || ( windows_is_unc && p == unc_root ) ;
	p = (p && !is_root) ? p : NULL ;
	nul = p ? p : (path+strlen(path)) ;
	*nul = '\0' ;
	return p ;
}

static bool unchomp( char * p , const char * ref )
{
	if( p == NULL || ref == NULL || strlen(p) == strlen(ref) )
	{
		return false ;
	}
	else
	{
		char c = ref[strlen(p)] ;
		p[strlen(p)] = c ;
		return true ;
	}
}

static bool mkdir_for( const char * path_in )
{
	bool ok = true ;
	char * path = strdup_( path_in ) ;
	if( path && chomp(path) )
	{
		char * dir = strdup_( path ) ;
		char * p = NULL ;
		while( ( p = chomp(path) ) ) ;
		do
		{
			ok = ( 0 == access(path,F_OK) ) || ( 0 == os_mkdir(path,0777) ) ;
			if( !ok ) break ;
		} while( unchomp(path,dir) ) ;
		if( dir ) free( dir ) ;
	}
	if( path ) free( path ) ;
	return ok ;
}

static Bytef * zptr( char * p )
{
	return (Bytef*)p ;
}

static M * unpack_new_( const char * path , Fn fn )
{
	M * m ;
	if( path == NULL ) return NULL ;

	m = (M*) malloc( sizeof(M) ) ;
	if( m == NULL ) return NULL ;

	m->fn = fn ;
	m->max_size = 0UL ;
	m->compressed = 0 ;
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
			if( p->flags ) free( p->flags ) ;
			p = p->next ;
			free( temp ) ;
		}
	}
	if( m ) free( m ) ;
}

static int unpack_count_( const M * m )
{
	int n = 0 ;
	const Entry * p = NULL ;
	if( m == NULL ) return 0 ;
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

static char * unpack_flags_( const M * m , int n )
{
	const Entry * p = unpack_entry( m , n ) ;
	const char * flags = p ? p->flags : "" ;
	return strdup_(flags) ;
}

static unsigned long unpack_packed_size_( const M * m , int n )
{
	const Entry * p = unpack_entry( m , n ) ;
	return p ? p->size : 0UL ;
}

static unsigned long file_size( const char * path )
{
	static struct stat zero ;
	struct stat buf = zero ;
	int rc ;
	if( path == NULL ) return 0UL ;
	rc = stat( path , &buf ) ;
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
	int c = 0 ;
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

	/* read the compression flag */
	rc = fseek( m->input , exe_offset , SEEK_SET ) ;
	check_that( !feof(m->input) && !ferror(m->input) , "table seek error" ) ;
	c = fgetc( m->input ) ;
	check_that( ( c == '1' || c == '0' ) && c != EOF && !feof(m->input) && !ferror(m->input) , "format error" ) ;
	m->compressed = c == '1' ;
	check_that( !m->compressed || have_zlib() , "cannot decompress: not built with zlib" ) ;

	/* seek to the directory */
	rc = fseek( m->input , exe_offset + 2UL , SEEK_SET ) ;
	check_that( !feof(m->input) && !ferror(m->input) , "table seek error" ) ;

	/* read the directory */
	file_offset = 0UL ;
	for(;;)
	{
		char * file_flags = NULL ;
		char * file_path = NULL ;
		unsigned long file_size = 0UL ;
		char big_buffer[10001] ;

		rc = fscanf( m->input , "%lu" , &file_size ) ; /* size */
		check_that( rc == 1 , "table entry size error" ) ;
		rc = fscanf( m->input , "%10000s" , big_buffer ) ; /* flags */
		check_that( rc == 1 , "table entry read error" ) ;
		big_buffer[sizeof(big_buffer)-1] = '\0' ;
		file_flags = strdup_(big_buffer) ;
		check_that( file_flags != NULL , "out of memory" ) ;
		rc = fscanf( m->input , "%10000s" , big_buffer ) ; /* path */
		check_that( rc == 1 , "table entry read error" ) ;
		big_buffer[sizeof(big_buffer)-1] = '\0' ;
		replace_all( big_buffer , sizeof(big_buffer) , '\001' , ' ' ) ;
		file_path = strdup_(big_buffer) ;
		check_that( file_path != NULL , "out of memory" ) ;

		if( file_size == 0U ) 
		{
			check_that( strcmp(file_path,"end") == 0 , "invalid internal directory" ) ;
			free( file_path ) ;
			free( file_flags ) ;
			break ;
		}

		{
			Entry * entry = malloc( sizeof(Entry) ) ;
			check_that( entry != NULL , "out of memory" ) ;
			entry->next = NULL ;
			entry->path = file_path ;
			entry->size = file_size ;
			entry->offset = file_offset ;
			entry->flags = file_flags ;

			if( tail == NULL )
				m->map = entry ;
			else
				tail->next = entry ;
			tail = entry ;
		}

		file_offset += file_size ;
		if( file_size > m->max_size )
			m->max_size = file_size ;
	}

	/* eat the newline */
	rc = fgetc( m->input ) ;
	check_that( rc != EOF , "file-map read error" ) ;
	m->start = ftell( m->input ) ;

	/* reserve a buffer */
	check_that( m->max_size < 100000000UL , "too big" ) ; /* sanity limit */
	m->buffer = malloc( m->max_size + 1UL ) ;
	check_that( m->buffer != NULL , "out of memory" ) ;

	return true ;
}

static bool unpack_imp( M * m , const char * base_dir , const Entry * entry )
{
	int rc = 0 ;
	FILE * output = NULL ;

	/* belt-and-braces */
	if( m == NULL ) return false ;
	check_that( base_dir != NULL , "usage error" ) ;
	check_that( entry != NULL , "internal error" ) ;

	/* sync up */
	rc = fseek( m->input , m->start + entry->offset , SEEK_SET ) ;
	check_that( rc == 0 , "seek error" ) ;

	/* read file data into the buffer */
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
		bool ok = false ;
		char * to_path = NULL ;

		to_path = malloc( strlen(base_dir) + 1 + strlen(entry->path) + 1 ) ;
		check_that( to_path != NULL , "out of memory" ) ;
		strcpy( to_path , base_dir ) ;
		strcat( to_path , "/" ) ;
		strcat( to_path , entry->path ) ;

		ok = mkdir_for( to_path ) ;
		check_that( ok , "cannot create output directory" ) ;

		output = fopen( to_path , w ) ;
		free( to_path ) ;
		to_path = NULL ;
		check_that( output != NULL , "cannot open output" ) ;
	}

	/* unzip the buffer into the output file */
	if( m->compressed )
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
	else
	{
		size_t n_written = fwrite( m->buffer , 1 , entry->size , output ) ;
		check_that( entry->size == n_written , "write error" ) ;
	}

	check_that( !ferror(output) , "write error" ) ;
	rc = fclose( output ) ;
	check_that( rc == 0 , "close error" ) ;
	return true ;
}

static bool unpack_all_( M * m , const char * base_dir )
{
	const Entry * p = NULL ;
	if( m == NULL ) return false ;
	for( p = m->map ; p != NULL ; p = p->next )
	{
		bool ok = unpack_imp( m , base_dir , p ) ;
		if( !ok )
			return false ;
	}
	return true ;
}

static bool unpack_file_( M * m , const char * base_dir , const char * name )
{
	const Entry * p = NULL ;
	bool ok = false ;
	if( m == NULL ) return false ;
	for( p = m->map ; p != NULL ; p = p->next )
	{
		if( 0 == strcmp(p->path,name) )
		{
			ok = unpack_imp( m , base_dir , p ) ;
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

char * unpack_flags( const Unpack * p , int i )
{
	return unpack_flags_( (const M*)p , i ) ;
}

void unpack_free( char * p )
{
	free( p ) ;
}

unsigned long unpack_packed_size( const Unpack * p , int i )
{
	return unpack_packed_size_( (const M*)p , i ) ;
}

int unpack_all( Unpack * p , const char * base_dir )
{
	return unpack_all_( (M*)p , base_dir ) ;
}

int unpack_file( Unpack * p , const char * base_dir , const char * name )
{
	return unpack_file_( (M*)p , base_dir , name ) ;
}

void unpack_delete( Unpack * p )
{
	unpack_delete_( (M*)p ) ;
}

