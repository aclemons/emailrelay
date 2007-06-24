//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gfile_unix.cpp
//
	
#include "gdef.h"
#include "gfile.h"
#include "gprocess.h"
#include <errno.h>
#include <sys/stat.h>
#include <sstream>

bool G::File::mkdir( const Path & dir , const NoThrow & )
{
	return 0 == ::mkdir( dir.str().c_str() , S_IRUSR | S_IWUSR | S_IXUSR ) ;
}

bool G::File::exists( const char * path , bool & enoent )
{
	struct stat statbuf ;
	if( 0 == ::stat( path , &statbuf ) )
	{
		return true ;
	}
	else
	{
		int error = G::Process::errno_() ;
		enoent = error == ENOENT || error == ENOTDIR ;
		return false ;
	}
}

std::string G::File::sizeString( const Path & path )
{
	struct stat statbuf ;
	if( 0 != ::stat( path.pathCstr() , &statbuf ) )
		return std::string() ;

	std::ostringstream ss ;
	ss << statbuf.st_size ;
	return ss.str() ;
}

G::File::time_type G::File::time( const Path & path )
{
	struct stat statbuf ;
	if( 0 != ::stat( path.pathCstr() , &statbuf ) )
		throw TimeError( path.str() ) ;
	return statbuf.st_mtime ;
}

G::File::time_type G::File::time( const Path & path , const NoThrow & )
{
	struct stat statbuf ;
	return ::stat( path.pathCstr() , &statbuf ) == 0 ? statbuf.st_mtime : 0 ;
}

bool G::File::chmodx( const Path & path , bool do_throw )
{
	bool ok = 0 == ::chmod( path.str().c_str() , S_IRUSR | S_IWUSR | S_IXUSR ) ;
	if( !ok && do_throw )
		throw CannotChmod( path.str() ) ;
	return ok ;
}

void G::File::link( const Path & target , const Path & new_link )
{
	if( !link(target,new_link,NoThrow()) )
	{
		int error = G::Process::errno_() ; // keep first
		CannotLink e( new_link.str() ) ;
		std::ostringstream ss ;
		ss << "(" << error << ")" ;
		e.append( ss.str() ) ;
		throw e ;
	}
}

bool G::File::link( const Path & target , const Path & new_link , const NoThrow & )
{
	if( exists(target) )
		remove( target , NoThrow() ) ;
	int rc = ::symlink( target.str().c_str() , new_link.str().c_str() ) ;
	// dont put anything here
	return rc == 0 ;
}

/// \file gfile_unix.cpp
