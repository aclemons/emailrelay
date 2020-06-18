//
// Copyright (C) 2001-2020 Graeme Walker <graeme_walker@users.sourceforge.net>
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
//
// gfile_win32.cpp
//

#include "gdef.h"
#include "gfile.h"
#include "gprocess.h"
#include <sys/stat.h>
#include <direct.h>
#include <iomanip>
#include <sstream>
#include <share.h>

bool G::File::mkdir( const Path & dir , const NoThrow & )
{
	return 0 == ::_mkdir( dir.str().c_str() ) ;
}

bool G::File::executable( const Path & path )
{
	return exists( path , NoThrow() ) ;
}

bool G::File::empty( const Path & path )
{
	WIN32_FIND_DATAA info ;
	HANDLE h = ::FindFirstFileA( path.str().c_str() , &info ) ;
	if( h != INVALID_HANDLE_VALUE ) ::FindClose( h ) ;
	return h != INVALID_HANDLE_VALUE && info.nFileSizeHigh == 0 && info.nFileSizeLow == 0 ;
}

std::string G::File::sizeString( const Path & path )
{
	WIN32_FIND_DATAA info ;
	HANDLE h = ::FindFirstFileA( path.str().c_str() , &info ) ;
	if( h == INVALID_HANDLE_VALUE )
		return std::string() ;

	const DWORD & hi = info.nFileSizeHigh ;
	const DWORD & lo = info.nFileSizeLow ;

	::FindClose( h ) ;

	return sizeString( hi , lo ) ;
}

std::string G::File::sizeString( g_uint32_t hi , g_uint32_t lo )
{
	__int64 n = hi ;
	n <<= 32U ;
	n |= lo ;
	if( n < 0 )
		throw SizeOverflow() ;

	if( n == 0 )
		return std::string("0") ;

	std::string s ;
	while( n != 0 )
	{
		int i = static_cast<int>( n % 10 ) ;
		char c = static_cast<char>( '0' + i ) ;
		s.insert( 0U , 1U , c ) ;
		n /= 10 ;
	}
	return s ;
}

bool G::File::exists( const char * path , bool & enoent , bool & eaccess )
{
	struct _stat statbuf ;
	bool ok = 0 == ::_stat( path , &statbuf ) ;
	enoent = !ok ; // could do better
	eaccess = false ;
	return ok ;
}

bool G::File::isLink( const Path & path )
{
	// this is weak, but good enough
	struct _stat statbuf ;
	return 0 == ::_stat( path.str().c_str() , &statbuf ) && !(statbuf.st_mode & S_IFDIR) ;
}

bool G::File::isDirectory( const Path & path )
{
	struct _stat statbuf ;
	return 0 == ::_stat( path.str().c_str() , &statbuf ) && (statbuf.st_mode & S_IFDIR) ;
}

G::SystemTime G::File::time( const Path & path )
{
	struct _stat statbuf ;
	if( 0 != ::_stat( path.str().c_str() , &statbuf ) )
	{
		int e = G::Process::errno_() ;
		throw TimeError( path.str() , G::Process::strerror(e) ) ;
	}
	return SystemTime(statbuf.st_mtime) ;
}

G::SystemTime G::File::time( const Path & path , const NoThrow & )
{
	struct _stat statbuf ;
	return SystemTime( ::_stat( path.str().c_str() , &statbuf ) == 0 ? statbuf.st_mtime : 0 ) ;
}

bool G::File::chmodx( const Path & , bool )
{
	return true ; // no-op
}

G::Path G::File::readlink( const Path & )
{
	return Path() ;
}

void G::File::link( const Path & , const Path & new_link )
{
	throw CannotLink( new_link.str() , "not supported" ) ;
}

bool G::File::link( const Path & , const Path & , const NoThrow & )
{
	return false ; // not supported
}

/// \file gfile_win32.cpp
