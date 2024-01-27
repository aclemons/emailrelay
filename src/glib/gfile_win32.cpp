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
/// \file gfile_win32.cpp
///

#include "gdef.h"
#include "gfile.h"
#include "gassert.h"
#include "gprocess.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <io.h>
#include <cstdio>
#include <direct.h>
#include <share.h>
#include <iomanip>
#include <sstream>
#include <streambuf>
#include <limits>
#include <array>

namespace G
{
	namespace FileImp
	{
		template <typename T>
		void open( T & io , const char * path , std::ios_base::openmode mode )
		{
			#if GCONFIG_HAVE_EXTENDED_OPEN
				io.open( path , mode , _SH_DENYNO ) ; // _fsopen()
			#else
				io.open( path , mode ) ;
			#endif
		}
		int open( const char * path , int flags , int pmode ) noexcept
		{
			#if GCONFIG_HAVE_SOPEN_S
				_set_errno( 0 ) ; // mingw bug
				int fd = -1 ;
				errno_t rc = _sopen_s( &fd , path , flags | _O_NOINHERIT , _SH_DENYNO , pmode ) ;
				return rc == 0 ? fd : -1 ;
			#else
				#if GCONFIG_HAVE_SOPEN
					return _sopen( path , flags | _O_NOINHERIT , _SH_DENYNO , pmode ) ;
				#else
					return _open( path , flags | _O_NOINHERIT , pmode ) ;
				#endif
			#endif
		}
		void uninherited( HANDLE h )
		{
			if( h )
				SetHandleInformation( h , HANDLE_FLAG_INHERIT , 0 ) ;
		}
		HANDLE handle( int fd )
		{
			return fd >= 0 ? reinterpret_cast<HANDLE>( _get_osfhandle(fd) ) : HNULL ;
		}
		int fd( std::FILE * fp )
		{
			return fp ? _fileno( fp ) : -1 ;
		}
		std::FILE * fopen( const char * path , const char * mode ) noexcept
		{
			std::FILE * fp = nullptr ;
			#if GCONFIG_HAVE_FSOPEN
				fp = _fsopen( path , mode , _SH_DENYNO ) ;
			#else
				#if GCONFIG_HAVE_FOPEN_S
					errno_t e = fopen_s( &fp , path , mode ) ;
					if( e )
						fp = nullptr ;
				#else
					fp = std::fopen( path , mode ) ;
				#endif
			#endif
			uninherited( handle(fd(fp)) ) ; // or add "N" to mode
			return fp ;
		}
	}
}

void G::File::open( std::ofstream & ofstream , const Path & path )
{
	FileImp::open( ofstream , path.cstr() , std::ios_base::out | std::ios_base::binary ) ;
}

void G::File::open( std::ofstream & ofstream , const Path & path , Text )
{
	FileImp::open( ofstream , path.cstr() , std::ios_base::out ) ;
}

void G::File::open( std::ofstream & ofstream , const Path & path , Append )
{
	FileImp::open( ofstream , path.cstr() , std::ios_base::app | std::ios_base::binary ) ;
}

void G::File::open( std::ifstream & ifstream , const Path & path )
{
	FileImp::open( ifstream , path.cstr() , std::ios_base::in | std::ios_base::binary ) ;
}

void G::File::open( std::ifstream & ifstream , const Path & path , Text )
{
	FileImp::open( ifstream , path.cstr() , std::ios_base::in ) ;
}

std::filebuf * G::File::open( std::filebuf & fb , const Path & path , InOut inout )
{
	inout == InOut::In ?
		FileImp::open( fb , path.cstr() , std::ios_base::in | std::ios_base::binary ) :
		FileImp::open( fb , path.cstr() , std::ios_base::out | std::ios_base::binary ) ;
	return fb.is_open() ? &fb : nullptr ;
}

int G::File::open( const char * path , InOutAppend mode ) noexcept
{
	int pmode = _S_IREAD | _S_IWRITE ;
	if( mode == InOutAppend::In )
		return FileImp::open( path , _O_RDONLY|_O_BINARY , pmode ) ;
	else if( mode == InOutAppend::Out )
		return FileImp::open( path , _O_WRONLY|_O_CREAT|_O_TRUNC|_O_BINARY , pmode ) ;
	else
		return FileImp::open( path , _O_WRONLY|_O_CREAT|_O_APPEND|_O_BINARY , pmode ) ;
}

int G::File::open( const char * path , CreateExclusive ) noexcept
{
	int pmode = _S_IREAD | _S_IWRITE ;
	return FileImp::open( path , _O_WRONLY|_O_CREAT|_O_EXCL|_O_BINARY , pmode ) ;
}

std::FILE * G::File::fopen( const char * path , const char * mode ) noexcept
{
	G_ASSERT( path && mode ) ;
	return FileImp::fopen( path , mode ) ;
}

bool G::File::probe( const char * path ) noexcept
{
	int pmode = _S_IREAD | _S_IWRITE ;
	int fd = FileImp::open( path , _O_WRONLY|_O_CREAT|_O_EXCL|O_TEMPORARY|_O_BINARY , pmode ) ;
	if( fd >= 0 )
		_close( fd ) ; // also deletes
	return fd >= 0 ;
}

void G::File::create( const Path & path )
{
	int fd = FileImp::open( path.cstr() , _O_RDONLY|_O_CREAT , _S_IREAD|_S_IWRITE ) ;
	if( fd < 0 )
		throw CannotCreate( path.str() ) ;
	_close( fd ) ;
}

bool G::File::renameOnto( const Path & from , const Path & to , std::nothrow_t ) noexcept
{
	bool ok = 0 == std::rename( from.cstr() , to.cstr() ) ;
	int error = Process::errno_() ;
	if( !ok && error == EEXIST ) // MS documentation says EACCES :-<
	{
		std::remove( to.cstr() ) ;
		ok = 0 == std::rename( from.cstr() , to.cstr() ) ;
	}
	return ok ;
}

ssize_t G::File::read( int fd , char * p , std::size_t n ) noexcept
{
	constexpr std::size_t limit = std::numeric_limits<unsigned int>::max() ;
	unsigned int un = static_cast<unsigned int>(std::min(limit,n)) ;
	return _read( fd , p , un ) ;
}

ssize_t G::File::write( int fd , const char * p , std::size_t n ) noexcept
{
	constexpr std::size_t limit = std::numeric_limits<unsigned int>::max() ;
	unsigned int un = static_cast<unsigned int>(std::min(limit,n)) ;
	return _write( fd , p , un ) ;
}

void G::File::close( int fd ) noexcept
{
	_close( fd ) ;
}

int G::File::mkdirImp( const Path & dir ) noexcept
{
	int rc = _mkdir( dir.cstr() ) ;
	if( rc == 0 )
	{
		return 0 ;
	}
	else
	{
		int e = G::Process::errno_() ;
		if( e == 0 ) e = EINVAL ;
		return e ;
	}
}

G::File::Stat G::File::statImp( const char * path , bool ) noexcept
{
	Stat s ;
	struct _stat64 statbuf {} ;
	if( 0 == _stat64( path , &statbuf ) )
	{
		s.error = 0 ;
		s.enoent = false ;
		s.eaccess = false ;
		s.is_dir = (statbuf.st_mode & S_IFDIR) ;
		s.is_link = !s.is_dir ; // good enough for now
		s.is_executable = (statbuf.st_mode & _S_IEXEC) ; // based on filename extension
		s.is_empty = statbuf.st_size == 0 ;
		s.mtime_s = static_cast<std::time_t>(statbuf.st_mtime) ;
		s.mtime_us = 0 ;
		s.mode = static_cast<unsigned long>( statbuf.st_mode & 07777 ) ;
		s.size = static_cast<unsigned long long>( statbuf.st_size ) ;
		s.blocks = static_cast<unsigned long long>( statbuf.st_size >> 24 ) ;
	}
	else
	{
		int error = Process::errno_() ;
		s.error = error ? error : EINVAL ;
		s.enoent = true ; // could do better
		s.eaccess = false ;
	}
	return s ;
}

bool G::File::existsImp( const char * path , bool & enoent , bool & eaccess ) noexcept
{
	Stat s = statImp( path ) ;
	if( s.error )
	{
		enoent = s.enoent ;
		eaccess = s.eaccess ;
	}
	return s.error == 0 ;
}

bool G::File::chmodx( const Path & , bool )
{
	return true ; // no-op
}

void G::File::chmod( const Path & , const std::string & )
{
}

void G::File::chgrp( const Path & , const std::string & )
{
}

bool G::File::chgrp( const Path & , const std::string & , std::nothrow_t )
{
	return true ; // no-op
}

bool G::File::chgrp( const Path & , gid_t , std::nothrow_t )
{
	return true ; // no-op
}

bool G::File::hardlink( const Path & , const Path & , std::nothrow_t )
{
	return false ;
}

G::Path G::File::readlink( const Path & )
{
	return Path() ;
}

void G::File::link( const Path & , const Path & new_link )
{
	throw CannotLink( new_link.str() , "not supported" ) ;
}

bool G::File::link( const Path & , const Path & , std::nothrow_t )
{
	return false ; // not supported
}

std::streamoff G::File::seek( int fd , std::streamoff offset , Seek origin ) noexcept
{
	off_t rc = _lseek( fd , static_cast<off_t>(offset) ,
		origin == Seek::Start ? SEEK_SET : ( origin == Seek::End ? SEEK_END : SEEK_CUR ) ) ;
	return static_cast<std::streamoff>(rc) ;
}

