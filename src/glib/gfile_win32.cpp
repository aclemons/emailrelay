//
// Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gfile_win32.cpp
//
	
#include "gdef.h"
#include "gfile.h"
#include <sys/stat.h>
#include <direct.h>
#include <iomanip>
#include <sstream>

bool G::File::mkdir( const Path & dir , const NoThrow & )
{
	return 0 == ::_mkdir( dir.str().c_str() ) ;
}

std::string G::File::sizeString( const Path & path )
{
	WIN32_FIND_DATA info ;
	HANDLE h = ::FindFirstFile( path.str().c_str() , &info ) ;
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

bool G::File::exists( const char * path , bool & enoent )
{
	struct _stat statbuf ;
	bool ok = 0 == ::_stat( path , &statbuf ) ;
	enoent = !ok ;
	return ok ;
}

