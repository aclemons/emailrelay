/*
   Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
   
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
	unpack.h

	A 'C' interface for unpacking packed executables.

	This is written in "C" so that a self-extracting archive does 
	not have any dependence on the C++ runtime library.

*/

#ifndef G_UNPACK_C_H__
#define G_UNPACK_C_H__

#include "gdef.h"
#include <stdio.h>

/** An error handler type for the C unpack interface.
 */
typedef void (*UnpackErrorHandler)(const char *) ;

/** Private implementation structure for the C unpack interface.
 */
typedef struct UnpackEntry_tag
{ 
	struct UnpackEntry_tag * next ;
	char * path ; 
	unsigned long size ; 
	unsigned long offset ; 
	char * flags ; 
} UnpackEntry ;

/** A boolean type for the C unpack interface.
 */
typedef int UnpackBool ;

/** Private implementation structure for the C unpack interface.
 */
typedef struct Unpack_tag
{
	UnpackErrorHandler fn ;
	UnpackEntry * map ;
	char * path ;
	UnpackBool compressed ;
	unsigned long max_size ;
	unsigned long exe_offset ;
	unsigned long start ;
	FILE * input ;
	char * buffer ;
} Unpack ;

#if __cplusplus
extern "C" {
#endif

/** Constructor. Returns NULL on error. The handler function may be NULL.
 */
Unpack * unpack_new( const char * exe_path , UnpackErrorHandler fn ) ;

/** Returns the number of packed files. Returns zero on error. 
 */
int unpack_count( const Unpack * p ) ;

/** Returns the name or relative path of the i'th file in a heap buffer. 
 *  Returns the zero-length string on error. Free with unpack_free(). 
 */
char * unpack_name( const Unpack * p , int i ) ;

/** Returns the flags of the i'th file in a heap buffer. 
 *  Returns the zero-length string on error. Free with unpack_free(). 
 */
char * unpack_flags( const Unpack * p , int i ) ;

/** Releases the given unpack_name()/unpack_flags() buffer. 
 */
void unpack_free( char * str ) ;

/** Returns the packed size of the i'th file. 
 */
unsigned long unpack_packed_size( const Unpack * p , int i ) ;

/** Unpacks the named file under the given base directory. The name may include a relative path. 
 */
int unpack_file( Unpack * p , const char * base_dir , const char * name ) ;

/** Unpacks the named file to a specific target path. The name may include a relative path. 
 */
int unpack_file_to( Unpack * p , const char * name , const char * target_path ) ;

/** Returns the size of the original file without packing.
 */
unsigned long unpack_original_size( const Unpack * p ) ;

/** Unpacks the original file to a specific target path.
 */
int unpack_original_file( Unpack * p , const char * target_path ) ;

/** Unpacks all files.
 */
UnpackBool unpack_all( Unpack * p , const char * base_dir ) ;

/** Destructor. May be passed NULL.
 */
void unpack_delete( Unpack * p ) ;

#if __cplusplus
}
#endif

#endif
