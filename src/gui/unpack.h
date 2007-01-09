//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
/*
	unpack.h

	A 'C' interface for unpacking packed executables.
*/

#ifndef G_UNPACK_C_H__
#define G_UNPACK_C_H__

#include "gdef.h"

#if __cplusplus
extern "C" {
#endif

/** An opaque type for the unpack interface.
 */
typedef struct Unpack_tag {} Unpack ;

/** An error handler type for the unpack interface.
 */
typedef void (*UnpackErrorHandler)(const char *) ;

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

/** Releases the given unpack_name() buffer. 
 */
void unpack_free( char * name ) ;

/** Returns the packed size of the i'th file. 
 */
unsigned long unpack_packed_size( const Unpack * p , int i ) ;

/** Unpacks the named file. The name may include a relative path. 
 */
int unpack_file( Unpack * p , const char * base_dir , const char * name ) ;

/** Unpacks all files.
 */
int unpack_all( Unpack * p , const char * base_dir ) ;

/** Destructor. May be passed NULL.
 */
void unpack_delete( Unpack * p ) ;

#if __cplusplus
}
#endif

#endif
