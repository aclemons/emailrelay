//
// Copyright (C) 2001-2004 Graeme Walker <graeme_walker@users.sourceforge.net>
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
//
// gfile.h
//
	
#ifndef G_FILE_H
#define G_FILE_H

#include "gdef.h"
#include "gpath.h"
#include "gexception.h"
#include <cstdio> // std::remove()

namespace G
{
	class File ;
	class DirectoryIteratorImp ;
}

// Class: G::File
// Description: A simple static class for dealing with files.
// See also: Path, FileSystem, Directory
//
class G::File 
{
public:
	G_EXCEPTION( StatError , "cannot stat() file" ) ;
	G_EXCEPTION( CannotRemove , "cannot delete file" ) ;
	G_EXCEPTION( CannotRename , "cannot rename file" ) ;
	G_EXCEPTION( CannotCopy , "cannot copy file" ) ;
	G_EXCEPTION( CannotMkdir , "cannot mkdir" ) ;
	G_EXCEPTION( SizeOverflow , "file size overflow" ) ;
	class NoThrow // An overload discriminator class for File methods.
		{} ;

	static bool remove( const Path & path , const NoThrow & ) ;
		// Deletes the file or directory. Returns false on error.

	static void remove( const Path & path ) ;
		// Deletes the file or directory. Throws an exception on error.

	static bool rename( const Path & from , const Path & to , const NoThrow & ) ;
		// Renames the file. Returns false on error.

	static void rename( const Path & from , const Path & to ) ;
		// Renames the file.

	static bool copy( const Path & from , const Path & to , const NoThrow & ) ;
		// Copies a file. Returns false on error.

	static void copy( const Path & from , const Path & to ) ;
		// Copies a file.

	static bool mkdir( const Path & dir , const NoThrow & ) ;
		// Creates a directory. Returns false on error.

	static void mkdir( const Path & dir ) ;
		// Creates a directory.

	static std::string sizeString( const Path & file ) ;
		// Returns the file's size in string format.
		// Returns the empty string on error.

	static bool exists( const Path & file ) ;
		// Returns true if the file (or link or device etc.)
		// exists. Throws an exception if permission denied
		// or too many symlinks etc.

	static bool exists( const Path & file , const NoThrow & ) ;
		// Returns true if the file (or link or device etc.)
		// exists. Returns false on error.

private:
	friend class G::DirectoryIteratorImp ;
	static std::string sizeString( g_uint32_t hi , g_uint32_t lo ) ; // win32
	static bool exists( const Path & , bool , bool ) ;
	static bool exists( const char * , bool & ) ; // o/s-specific
} ;

#endif
