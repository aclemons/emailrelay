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
//
// gfile_unix.cpp
//

#include "gdef.h"
#include "gfile.h"
#include "gprocess.h"
#include "glog.h"
#include "gassert.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <vector>
#include <sstream>

namespace
{
	G::EpochTime mtime( struct stat & statbuf )
	{
		#if GCONFIG_HAVE_STATBUF_NSEC
		return G::EpochTime( statbuf.st_mtime , statbuf.st_mtim.tv_nsec/1000U ) ;
		#else
		return G::EpochTime( statbuf.st_mtime ) ;
		#endif
	}
}

bool G::File::mkdir( const Path & dir , const NoThrow & )
{
	// open permissions, but limited by umask
	return 0 == ::mkdir( dir.str().c_str() , 0777 ) ;
}

bool G::File::exists( const char * path , bool & enoent , bool & eaccess )
{
	struct stat statbuf ;
	if( 0 == ::stat( path , &statbuf ) )
	{
		return true ;
	}
	else
	{
		int error = Process::errno_() ;
		enoent = error == ENOENT || error == ENOTDIR ;
		eaccess = error == EACCES ;
		return false ;
	}
}

bool G::File::isLink( const Path & path )
{
	struct stat statbuf ;
	return 0 == ::stat( path.str().c_str() , &statbuf ) && (statbuf.st_mode & S_IFLNK) ;
}

bool G::File::isDirectory( const Path & path )
{
	struct stat statbuf ;
	return 0 == ::stat( path.str().c_str() , &statbuf ) && (statbuf.st_mode & S_IFDIR) ;
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

bool G::File::empty( const Path & path )
{
	struct stat statbuf ;
	return 0 == ::stat( path.str().c_str() , &statbuf ) && statbuf.st_size == 0 ;
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

G::EpochTime G::File::time( const Path & path )
{
	struct stat statbuf ;
	if( 0 != ::stat( path.str().c_str() , &statbuf ) )
		throw TimeError( path.str() ) ;
	return mtime( statbuf ) ;
}

G::EpochTime G::File::time( const Path & path , const NoThrow & )
{
	struct stat statbuf ;
	if( ::stat( path.str().c_str() , &statbuf ) != 0 )
		return EpochTime( 0 ) ;
	return mtime( statbuf ) ;
}

bool G::File::chmodx( const Path & path , bool do_throw )
{
	struct stat statbuf ;
	mode_t mode =
		0 == ::stat( path.str().c_str() , &statbuf ) ?
			statbuf.st_mode :
			mode_t(0777) ; // default to open permissions, but limited by umask

	mode |= ( S_IRUSR | S_IXUSR ) ; // add user-read and user-executable
	if( mode & S_IRGRP ) mode |= S_IXGRP ; // add group-executable iff group-read
	if( mode & S_IROTH ) mode |= S_IXOTH ; // add world-executable iff world-read

	// apply the current umask
	mode_t mask = ::umask( ::umask(0) ) ;
	mode &= ~mask ;

	bool ok = 0 == ::chmod( path.str().c_str() , mode ) ;
	if( !ok && do_throw )
		throw CannotChmod( path.str() ) ;
	return ok ;
}

void G::File::link( const Path & target , const Path & new_link )
{
	if( linked(target,new_link) ) // optimisation
		return ;

	if( exists(new_link) )
		remove( new_link , NoThrow() ) ;

	int error = link( target.str().c_str() , new_link.str().c_str() ) ;

	if( error != 0 )
	{
		std::ostringstream ss ;
		ss << "[" << new_link << "] -> [" << target << "] " "(" << error << ")" ;
		throw CannotLink( ss.str() ) ;
	}
}

bool G::File::link( const Path & target , const Path & new_link , const NoThrow & )
{
	if( linked(target,new_link) ) // optimisation
		return true ;

	if( exists(new_link) )
		remove( new_link , NoThrow() ) ;

	return 0 == link( target.str().c_str() , new_link.str().c_str() ) ;
}

int G::File::link( const char * target , const char * new_link )
{
	int rc = ::symlink( target , new_link ) ;
	int error = Process::errno_() ;
	return rc == 0 ? 0 : (error?error:EINVAL) ;
}

G::Path G::File::readlink( const Path & link )
{
	Path result = readlink( link , NoThrow() ) ;
	if( result == Path() )
		throw CannotReadLink( link.str() ) ;
	return result ;
}

G::Path G::File::readlink( const Path & link , const NoThrow & )
{
	Path result ;
	struct stat statbuf ;
	int rc = ::lstat( link.str().c_str() , &statbuf ) ;
	if( rc == 0 )
	{
		size_t buffer_size = statbuf.st_size ? (statbuf.st_size+1U) : 1024U ;
		std::vector<char> buffer( buffer_size , '\0' ) ;
		ssize_t rc = ::readlink( link.str().c_str() , &buffer[0] , buffer.size() ) ;
		if( rc > 0 && static_cast<size_t>(rc) < buffer.size() ) // filesystem race can cause trucation -- treat as an error
		{
			G_ASSERT( buffer.at(static_cast<size_t>(rc-1)) != '\0' ) ; // readlink does not null-terminate
			result = Path( std::string( &buffer[0] , static_cast<size_t>(rc) ) ) ;
		}
	}
	return result ;
}

bool G::File::linked( const Path & target , const Path & new_link )
{
	// see if already linked correctly - errors and overflows are not fatal
	return readlink(new_link,NoThrow()) == target ;
}

/// \file gfile_unix.cpp
