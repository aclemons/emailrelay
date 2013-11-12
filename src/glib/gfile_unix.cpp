//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gfile_unix.cpp
//
	
#include "gdef.h"
#include "glimits.h"
#include "gfile.h"
#include "gprocess.h"
#include "gdebug.h"
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

bool G::File::executable( const Path & path )
{
	struct stat statbuf ;
	if( 0 == ::stat( path.str().c_str() , &statbuf ) )
	{
		bool x = !!( statbuf.st_mode & S_IXUSR ) ;
		bool r = 
			( statbuf.st_mode & S_IFMT ) == S_IFREG ||
			( statbuf.st_mode & S_IFMT ) == S_IFLNK ;
		return x && r ; // indicitive only
	}
	else
	{
		return false ;
	}
}

std::string G::File::sizeString( const Path & path )
{
	struct stat statbuf ;
	if( 0 != ::stat( path.str().c_str() , &statbuf ) )
		return std::string() ;

	std::ostringstream ss ;
	ss << statbuf.st_size ;
	return ss.str() ;
}

G::File::time_type G::File::time( const Path & path )
{
	struct stat statbuf ;
	if( 0 != ::stat( path.str().c_str() , &statbuf ) )
		throw TimeError( path.str() ) ;
	return statbuf.st_mtime ;
}

G::File::time_type G::File::time( const Path & path , const NoThrow & )
{
	struct stat statbuf ;
	return ::stat( path.str().c_str() , &statbuf ) == 0 ? statbuf.st_mtime : 0 ;
}

bool G::File::chmodx( const Path & path , bool do_throw )
{
	mode_t mode = S_IRUSR | S_IWUSR | S_IXUSR ;
	struct stat statbuf ;
	if( 0 == ::stat( path.str().c_str() , &statbuf ) )
	{
		G_DEBUG( "G::File::chmodx: old: " << statbuf.st_mode ) ;
		mode = statbuf.st_mode | S_IXUSR ;
		if( mode & S_IRGRP ) mode |= S_IXGRP ;
		if( mode & S_IROTH ) mode |= S_IXOTH ;
		G_DEBUG( "G::File::chmodx: new: " << mode ) ;
	}

	bool ok = 0 == ::chmod( path.str().c_str() , mode ) ;
	if( !ok && do_throw )
		throw CannotChmod( path.str() ) ;
	return ok ;
}

void G::File::link( const Path & target , const Path & new_link )
{
	if( !link(target,new_link,NoThrow()) )
	{
		int error = G::Process::errno_() ; // keep first
		std::ostringstream ss ;
		ss << "[" << new_link << "] -> [" << target << "] " "(" << error << ")" ;
		throw CannotLink( ss.str() ) ;
	}
}

bool G::File::link( const Path & target , const Path & new_link , const NoThrow & )
{
	// optimisation
	char buffer[limits::path] ;
	ssize_t rc = ::readlink( new_link.str().c_str() , buffer , sizeof(buffer) ) ;
	size_t n = rc < 0 ? size_t(0U) : static_cast<size_t>(rc) ;
	if( rc > 0 && n != sizeof(buffer) )
	{
		std::string old_target( buffer , n ) ;
		if( target.str() == old_target )
			return true ;
	}

	if( exists(new_link) )
		remove( new_link , NoThrow() ) ;

	rc = ::symlink( target.str().c_str() , new_link.str().c_str() ) ;
	// dont put anything here (preserve errno)
	return rc == 0 ;
}

/// \file gfile_unix.cpp
