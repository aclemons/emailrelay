//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gfile.h
///

#ifndef G_FILE_H
#define G_FILE_H

#include "gdef.h"
#include "gpath.h"
#include "gexception.h"
#include "gdatetime.h"
#include <cstdio> // std::remove()
#include <fstream>

namespace G
{
	class File ;
	class DirectoryIteratorImp ;
}

/// \class G::File
/// A simple static class for dealing with files.
/// \see G::Path, G::FileSystem, G::Directory
///
class G::File
{
public:
	G_EXCEPTION( StatError , "cannot access file" ) ;
	G_EXCEPTION( CannotRemove , "cannot delete file" ) ;
	G_EXCEPTION( CannotRename , "cannot rename file" ) ;
	G_EXCEPTION( CannotCopy , "cannot copy file" ) ;
	G_EXCEPTION( CannotMkdir , "cannot mkdir" ) ;
	G_EXCEPTION( CannotChmod , "cannot chmod file" ) ;
	G_EXCEPTION( CannotLink , "cannot create symlink" ) ;
	G_EXCEPTION( CannotCreate , "cannot create empty file" ) ;
	G_EXCEPTION( CannotReadLink , "cannot read symlink" ) ;
	G_EXCEPTION( SizeOverflow , "file size overflow" ) ;
	G_EXCEPTION( TimeError , "cannot get file modification time" ) ;
	class NoThrow /// An overload discriminator class for File methods.
		{} ;

	static bool remove( const Path & path , const NoThrow & ) ;
		///< Deletes the file or directory. Returns false on error.

	static void remove( const Path & path ) ;
		///< Deletes the file or directory. Throws an exception on error.

	static bool rename( const Path & from , const Path & to , const NoThrow & ) ;
		///< Renames the file. Returns false on error.

	static void rename( const Path & from , const Path & to , bool ignore_missing = false ) ;
		///< Renames the file. Throws on error, but optionally
		///< ignores errors caused by a missing 'from' file or
		///< missing 'to' directory component.

	static bool copy( const Path & from , const Path & to , const NoThrow & ) ;
		///< Copies a file. Returns false on error.

	static void copy( const Path & from , const Path & to ) ;
		///< Copies a file.

	static void copy( std::istream & from , std::ostream & to ,
		std::streamsize limit = 0U , std::string::size_type block = 0U ) ;
			///< Copies a stream with an optional size limit.

	static bool mkdirs( const Path & dir , const NoThrow & , int = 100 ) ;
		///< Creates a directory and all necessary parents. Returns false on error.
		///< Does chmodx() on all created directories.

	static void mkdirs( const Path & dir , int = 100 ) ;
		///< Creates a directory and all necessary parents.
		///< Does chmodx() on all created directories.

	static bool mkdir( const Path & dir , const NoThrow & ) ;
		///< Creates a directory. Returns false on error.

	static void mkdir( const Path & dir ) ;
		///< Creates a directory.

	static bool empty( const Path & file ) ;
		///< Returns true if the file size is zero.

	static std::string sizeString( const Path & file ) ;
		///< Returns the file's size in string format.
		///< Returns the empty string on error.

	static bool exists( const Path & file ) ;
		///< Returns true if the file (directory, device etc.)
		///< exists. Symlinks are followed. Throws an exception
		///< if permission denied or too many symlinks etc.

	static bool exists( const Path & file , const NoThrow & ) ;
		///< Returns true if the file (directory, device etc.)
		///< exists. Symlinks are followed. Returns false on error.

	static bool isLink( const Path & path ) ;
		///< Returns true if the path is an existing symlink.

	static bool isDirectory( const Path & path ) ;
		///< Returns true if the path exists() and is a directory.
		///< Symlinks are followed.

	static EpochTime time( const Path & file ) ;
		///< Returns the file's timestamp. Throws on error.

	static EpochTime time( const Path & file , const NoThrow & ) ;
		///< Returns the file's timestamp. Returns EpochTime(0)
		///< on error.

	static void chmodx( const Path & file ) ;
		///< Makes the file executable. Throws on error.

	static bool chmodx( const Path & file , const NoThrow & ) ;
		///< Makes the file executable.

	static G::Path readlink( const Path & link ) ;
		///< Reads a symlink. Throws on error.

	static G::Path readlink( const Path & link , const NoThrow & ) ;
		///< Reads a symlink. Returns the empty path on error.

	static void link( const Path & target , const Path & new_link ) ;
		///< Creates a symlink. If the link already exists but is
		///< not not pointing at the correct target then the link
		///< is deleted and recreated. Throws on error.

	static bool link( const Path & target , const Path & new_link , const NoThrow & ) ;
		///< Creates a symlink. Returns false on error.

	static bool executable( const Path & ) ;
		///< Returns true if the path is probably executable.
		///< Because of portability and implementation difficulties
		///< this does not return a definitive result so it should
		///< only used for generating warnings on a false return.

	static void create( const Path & ) ;
		///< Creates an empty file. Throws on error.

	static int compare( const Path & , const Path & , const NoThrow & ) ;
		///< Compares the contents of the two files. Returns 0, 1 or -1.

	static void open( std::ofstream & , const Path & ) ;
		///< Calls open() on the given output file stream, with a 'out-binary' output
		///< openmode (but no 'app' or 'ate'). Uses SH_DENYNO on windows.

	static void open( std::ofstream & , const Path & , std::ios_base::openmode ) ;
		///< Calls open() on the given output file stream, with 'out-binary'
		///< added to the given openmode. Uses SH_DENYNO on windows.

	static void open( std::ifstream & , const Path & ) ;
		///< Calls open() on the given input file stream, with a 'in-binary' input
		///< openmode. Uses SH_DENYNO on windows.

	static void open( std::ifstream & , const Path & , std::ios_base::openmode ) ;
		///< Calls open() on the given input file stream, with 'in-binary'
		///< added to the given openmode. Uses SH_DENYNO on windows.

	static std::filebuf * open( std::filebuf & , const Path & , std::ios_base::openmode ) ;
		///< Calls open() on the given filebuf, with 'binary'
		///< added to the given openmode. Uses SH_DENYNO on windows.
		///< Returns the address of the given filebuf, or nullptr
		///< on failure.

private:
	friend class G::DirectoryIteratorImp ;
	File() g__eq_delete ;
	static std::string copy( const Path & , const Path & , int ) ;
	static std::string sizeString( g_uint32_t hi , g_uint32_t lo ) ; // win32
	static bool exists( const Path & , bool , bool ) ;
	static bool exists( const char * , bool & , bool & ) ; // o/s-specific
	static bool rename( const std::string & from , const std::string & to , bool & enoent ) ;
	static bool chmodx( const Path & file , bool ) ;
	static int link( const char * , const char * ) ;
	static bool linked( const Path & , const Path & ) ;
	static bool remove( const std::string & path , bool , bool ) ;
} ;

#endif
