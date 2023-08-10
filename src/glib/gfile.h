//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include <cstdio> // std::remove(), std::FILE
#include <new> // std::nothrow
#include <fstream>

namespace G
{
	class File ;
	class DirectoryIteratorImp ;
}

//| \class G::File
/// A simple static class for dealing with files.
/// \see G::Path, G::FileSystem, G::Directory
///
class G::File
{
public:
	G_EXCEPTION( StatError , tx("cannot access file") ) ;
	G_EXCEPTION( CannotRemove , tx("cannot delete file") ) ;
	G_EXCEPTION( CannotRename , tx("cannot rename file") ) ;
	G_EXCEPTION( CannotCopy , tx("cannot copy file") ) ;
	G_EXCEPTION( CannotMkdir , tx("cannot create directory") ) ;
	G_EXCEPTION( CannotChmod , tx("cannot chmod file") ) ;
	G_EXCEPTION( CannotChgrp , tx("cannot chgrp file") ) ;
	G_EXCEPTION( CannotLink , tx("cannot create symlink") ) ;
	G_EXCEPTION( CannotCreate , tx("cannot create file") ) ;
	G_EXCEPTION( CannotReadLink , tx("cannot read symlink") ) ;
	G_EXCEPTION( SizeOverflow , tx("file size overflow") ) ;
	G_EXCEPTION( TimeError , tx("cannot get file modification time") ) ;
	enum class InOut { In , Out } ;
	enum class InOutAppend { In , Out , Append } ;
	enum class Seek { Start , Current , End } ;
	class Append /// An overload discriminator for G::File::open().
		{} ;
	class Text /// An overload discriminator for G::File::open().
		{} ;
	struct CreateExclusive /// An overload discriminator for G::File::open().
		{} ;
	struct Stat /// A portable 'struct stat'.
	{
		int error {0} ;
		bool enoent {false} ;
		bool eaccess {false} ;
		bool is_dir {false} ;
		bool is_link {false} ;
		bool is_executable {false} ;
		bool is_empty {false} ;
		std::time_t mtime_s {0} ;
		unsigned int mtime_us {0} ;
		unsigned long mode {0} ;
		unsigned long long size {0} ;
		unsigned long long blocks {0} ;
		uid_t uid {0} ; // unix
		gid_t gid {0} ; // unix
		bool inherit {false} ; // unix, directory group ownership passed on to new files
	} ;

	static bool remove( const Path & path , std::nothrow_t ) noexcept ;
		///< Deletes the file or directory. Returns false on error.

	static void remove( const Path & path ) ;
		///< Deletes the file or directory. Throws an exception on error.

	static bool rename( const Path & from , const Path & to , std::nothrow_t ) noexcept ;
		///< Renames the file. Whether it fails if 'to' already
		///< exists depends on the o/s (see also renameOnto()).
		///< Returns false on error.

	static void rename( const Path & from , const Path & to , bool ignore_missing = false ) ;
		///< Renames the file. Throws on error, but optionally
		///< ignores errors caused by a missing 'from' file or
		///< missing 'to' directory component.

	static bool renameOnto( const Path & from , const Path & to , std::nothrow_t ) noexcept ;
		///< Renames the file, deleting 'to' first if necessary.
		///< Returns false on error.

	static bool copy( const Path & from , const Path & to , std::nothrow_t ) ;
		///< Copies a file. Returns false on error.

	static void copy( const Path & from , const Path & to ) ;
		///< Copies a file.

	static void copy( std::istream & from , std::ostream & to ,
		std::streamsize limit = 0U , std::size_t block = 0U ) ;
			///< Copies a stream with an optional size limit.

	static bool copyInto( const Path & from , const Path & to_dir , std::nothrow_t ) ;
		///< Copies a file into a directory and does a chmodx()
		///< if necessary. Returns false on error.

	static bool mkdirs( const Path & dir , std::nothrow_t , int = 100 ) ;
		///< Creates a directory and all necessary parents.
		///< Returns false on error, but EEXIST is not
		///< an error and chmod errors are also ignored.
		///<
		///< Note that the EEXIST result is ignored even if
		///< the target path already exists as a non-directory.
		///< This is a feature not a bug because it can avoid
		///< races.

	static void mkdirs( const Path & dir , int = 100 ) ;
		///< Creates a directory and all necessary parents.
		///< Throws on error, but EEXIST is not an error.

	static bool mkdir( const Path & dir , std::nothrow_t ) ;
		///< Creates a directory. Returns false on error
		///< (including EEXIST).

	static void mkdir( const Path & dir ) ;
		///< Creates a directory. Throws on error (including
		///< EEXIST).

	static bool isEmpty( const Path & file , std::nothrow_t ) ;
		///< Returns true if the file size is zero.
		///< Returns false on error.

	static std::string sizeString( const Path & file ) ;
		///< Returns the file's size in string format.
		///< Returns the empty string on error.

	static bool exists( const Path & file ) ;
		///< Returns true if the file (directory, device etc.)
		///< exists. Symlinks are followed. Throws an exception
		///< if permission denied or too many symlinks etc.

	static bool exists( const Path & file , std::nothrow_t ) ;
		///< Returns true if the file (directory, device etc.)
		///< exists. Symlinks are followed. Returns false on error.

	static bool isExecutable( const Path & , std::nothrow_t ) ;
		///< Returns true if the path is probably executable by the
		///< calling process. Because of portability and implementation
		///< difficulties this does not return a definitive result so
		///< it should only used for generating warnings on a
		///< false return. Returns false on error.

	static bool isLink( const Path & path , std::nothrow_t ) ;
		///< Returns true if the path is an existing symlink.
		///< Returns false on error.

	static bool isDirectory( const Path & path , std::nothrow_t ) ;
		///< Returns true if the path exists() and is a directory.
		///< Symlinks are followed. Returns false on error.

	static Stat stat( const Path & path , bool read_symlink = false ) ;
		///< Returns a file status structure. Returns with
		///< the 'error' field set on error. Always fails if
		///< 'read-link' on Windows.

	static SystemTime time( const Path & file ) ;
		///< Returns the file's timestamp. Throws on error.

	static SystemTime time( const Path & file , std::nothrow_t ) ;
		///< Returns the file's timestamp. Returns SystemTime(0)
		///< on error.

	static void chmodx( const Path & file ) ;
		///< Makes the file executable. Throws on error.

	static bool chmodx( const Path & file , std::nothrow_t ) ;
		///< Makes the file executable.

	static void chmod( const Path & file , const std::string & spec ) ;
		///< Sets the file permissions. Throws on error. The
		///< spec is a simplified sub-set of the /bin/chmod
		///< command syntax. The umask is ignored.

	static void chgrp( const Path & file , const std::string & group ) ;
		///< Sets the file group ownership. Throws on error.

	static bool chgrp( const Path & file , const std::string & group , std::nothrow_t ) ;
		///< Sets the file group ownership. Returns false on error.

	static bool chgrp( const Path & file , gid_t group_id , std::nothrow_t ) ;
		///< Sets the file group ownership. Returns false on error.

	static G::Path readlink( const Path & link ) ;
		///< Reads a symlink. Throws on error.

	static G::Path readlink( const Path & link , std::nothrow_t ) ;
		///< Reads a symlink. Returns the empty path on error.

	static void link( const Path & target , const Path & new_link ) ;
		///< Creates a symlink. If the link already exists but is
		///< not not pointing at the correct target then the link
		///< is deleted and recreated. Throws on error.

	static bool link( const Path & target , const Path & new_link , std::nothrow_t ) ;
		///< Creates a symlink. Returns false on error.

	static bool hardlink( const Path & src , const Path & dst , std::nothrow_t ) ;
		///< Creates a hard link. Returns false on error or if
		///< not implemented.

	static void create( const Path & ) ;
		///< Creates the file if it does not exist. Leaves it
		///< alone if it does. Throws on error.

	static int compare( const Path & , const Path & , bool ignore_whitespace = false ) ;
		///< Compares the contents of the two files. Returns 0, 1 or -1.

	static void open( std::ofstream & , const Path & ) ;
		///< Calls open() on the given output file stream.
		///< Uses SH_DENYNO and O_BINARY on windows.

	static void open( std::ofstream & , const Path & , Append ) ;
		///< Calls open() on the given output file stream.
		///< This overload is for append-on-every-write mode.
		///< Uses SH_DENYNO and O_BINARY on windows.

	static void open( std::ofstream & , const Path & , Text ) ;
		///< Calls open() on the given output file stream.
		///< This overload is for native end-of-line conversion.
		///< Uses SH_DENYNO and O_BINARY on windows.

	static void open( std::ifstream & , const Path & ) ;
		///< Calls open() on the given input file stream.
		///< Uses SH_DENYNO and O_BINARY on windows.

	static void open( std::ifstream & , const Path & , Text ) ;
		///< Calls open() on the given input file stream.
		///< This overload is for native end-of-line conversion.
		///< Uses SH_DENYNO and O_BINARY on windows.

	static std::filebuf * open( std::filebuf & , const Path & , InOut ) ;
		///< Calls open() on the given filebuf. Returns the address
		///< of the given filebuf, or nullptr on failure.
		///< Uses SH_DENYNO and O_BINARY on windows.

	static int open( const char * , InOutAppend ) noexcept ;
		///< Opens a file descriptor. Returns -1 on error.
		///< Uses SH_DENYNO and O_BINARY on windows.

	static int open( const char * , CreateExclusive ) noexcept ;
		///< Creates a file and returns a writable file descriptor.
		///< Fails if the file already exists. Returns -1 on error.
		///< Uses SH_DENYNO and O_BINARY on windows.

	static std::FILE * fopen( const char * , const char * mode ) noexcept ;
		///< Calls std::fopen().

	static bool probe( const char * ) noexcept ;
		///< Creates and deletes a temporary probe file. Fails if
		///< the file already exists. Returns false on error.

	static ssize_t read( int fd , char * , std::size_t ) noexcept ;
		///< Calls ::read() or equivalent.

	static ssize_t write( int fd , const char * , std::size_t ) noexcept ;
		///< Calls ::write() or equivalent.

	static void close( int fd ) noexcept ;
		///< Calls ::close() or equivalent.

	static std::streamoff seek( int fd , std::streamoff offset , Seek ) noexcept ;
		///< Does ::lseek() or equivalent.

	static void setNonBlocking( int fd ) noexcept ;
		///< Sets the file descriptor to non-blocking mode.

public:
	File() = delete ;

private:
	static const int rdonly = 1<<0 ;
	static const int wronly = 1<<1 ;
	static const int rdwr = 1<<2 ;
	static const int trunc = 1<<3 ;
	static const int creat = 1<<4 ;
	static const int append = 1<<5 ;
	friend class G::DirectoryIteratorImp ;
	static std::string copy( const Path & , const Path & , int ) ;
	static bool exists( const Path & , bool , bool ) ;
	static bool existsImp( const char * , bool & , bool & ) noexcept ;
	static Stat statImp( const char * , bool = false ) noexcept ;
	static bool rename( const char * , const char * to , bool & enoent ) noexcept ;
	static bool chmodx( const Path & file , bool ) ;
	static int linkImp( const char * , const char * ) ;
	static bool linked( const Path & , const Path & ) ;
	static int mkdirImp( const Path & dir ) noexcept ;
	static bool mkdirsr( const Path & dir , int & , int & ) ;
	static bool chmod( const Path & , const std::string & , std::nothrow_t ) ;
} ;

#endif
