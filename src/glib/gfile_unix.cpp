//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gfile_unix.cpp
///

#include "gdef.h"
#include "gfile.h"
#include "gstr.h"
#include "gprocess.h"
#include "glog.h"
#include "gassert.h"
#include <vector>
#include <sstream>
#include <cerrno> // ENOENT etc
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

namespace G
{
	namespace FileImp
	{
		bool removeImp( const char * path , int * e ) noexcept ;
		std::pair<bool,mode_t> newmode( mode_t , const std::string & ) ;
		std::pair<std::time_t,unsigned int> mtime( struct stat & statbuf ) noexcept
		{
			#if GCONFIG_HAVE_STATBUF_TIMESPEC
				return { statbuf.st_mtimespec.tv_sec , statbuf.st_mtimespec.tv_nsec/1000U } ;
			#else
				#if GCONFIG_HAVE_STATBUF_NSEC
					return { statbuf.st_mtime , statbuf.st_mtim.tv_nsec/1000U } ;
				#else
					return { statbuf.st_mtime , 0U } ;
				#endif
			#endif
		}
	}
}

void G::File::open( std::ofstream & ofstream , const Path & path )
{
	ofstream.open( path.cstr() , std::ios_base::out | std::ios_base::binary ) ;
}

void G::File::open( std::ofstream & ofstream , const Path & path , Text )
{
	ofstream.open( path.cstr() , std::ios_base::out ) ;
}

void G::File::open( std::ofstream & ofstream , const Path & path , Append )
{
	ofstream.open( path.cstr() , std::ios_base::app | std::ios_base::binary ) ;
}

void G::File::open( std::ifstream & ifstream , const Path & path )
{
	ifstream.open( path.cstr() , std::ios_base::in | std::ios_base::binary ) ;
}

#ifndef G_LIB_SMALL
void G::File::open( std::ifstream & ifstream , const Path & path , Text )
{
	ifstream.open( path.cstr() , std::ios_base::in ) ;
}
#endif

std::filebuf * G::File::open( std::filebuf & fb , const Path & path , InOut inout )
{
	return
		inout == InOut::In ?
			fb.open( path.cstr() , std::ios_base::in | std::ios_base::binary ) :
			fb.open( path.cstr() , std::ios_base::out | std::ios_base::binary ) ;
}

int G::File::open( const Path & path , InOutAppend mode , bool ) noexcept
{
	static_assert( noexcept(path.cstr()) , "" ) ;
	const char * path_cstr = path.cstr() ;
	if( mode == InOutAppend::In )
		return ::open( path_cstr , O_RDONLY ) ; // NOLINT
	else if( mode == InOutAppend::Out )
		return ::open( path_cstr , O_WRONLY|O_CREAT|O_TRUNC , 0666 ) ; // NOLINT
	else if( mode == InOutAppend::OutNoCreate )
		return ::open( path_cstr , O_WRONLY , 0666 ) ; // NOLINT
	else
		return ::open( path_cstr , O_WRONLY|O_CREAT|O_APPEND , 0666 ) ; // NOLINT
}

#ifndef G_LIB_SMALL
int G::File::open( const Path & path , CreateExclusive ) noexcept
{
	static_assert( noexcept(path.cstr()) , "" ) ;
	return ::open( path.cstr() , O_WRONLY|O_CREAT|O_EXCL , 0666 ) ; // NOLINT
}
#endif

#ifndef G_LIB_SMALL
std::FILE * G::File::fopen( const Path & path , const char * mode ) noexcept
{
	return std::fopen( path.cstr() , mode ) ;
}
#endif

bool G::File::probe( const Path & path ) noexcept
{
	static_assert( noexcept(path.cstr()) , "" ) ;
	static_assert( noexcept(File::remove(path,std::nothrow)) , "" ) ;

	int fd = ::open( path.cstr() , O_WRONLY|O_CREAT|O_EXCL , 0666 ) ; // NOLINT
	if( fd < 0 )
		return false ;

	File::remove( path , std::nothrow ) ;
	::close( fd ) ;
	return true ;
}

#ifndef G_LIB_SMALL
void G::File::create( const Path & path )
{
	int fd = ::open( path.cstr() , O_RDONLY|O_CREAT , 0666 ) ; // NOLINT
	if( fd < 0 )
		throw CannotCreate( path.str() ) ;
	::close( fd ) ;
}
#endif

bool G::File::renameOnto( const Path & from , const Path & to , std::nothrow_t ) noexcept
{
	return 0 == std::rename( from.cstr() , to.cstr() ) ; // overwrites 'to'
}

ssize_t G::File::read( int fd , char * p , std::size_t n ) noexcept
{
	return ::read( fd , p , n ) ;
}

ssize_t G::File::write( int fd , const char * p , std::size_t n ) noexcept
{
	return ::write( fd , p , n ) ;
}

void G::File::close( int fd ) noexcept
{
	::close( fd ) ;
}

bool G::FileImp::removeImp( const char * path , int * e ) noexcept
{
	bool ok = path && 0 == std::remove( path ) ;
	if( e )
		*e = ok ? 0 : ( path ? Process::errno_() : EINVAL ) ;
	return ok ;
}

bool G::File::cleanup( const Cleanup::Arg & arg ) noexcept
{
	return FileImp::removeImp( arg.str() , nullptr ) ;
}

bool G::File::remove( const Path & path , std::nothrow_t ) noexcept
{
	static_assert( noexcept(path.cstr()) , "" ) ;
	return FileImp::removeImp( path.cstr() , nullptr ) ;
}

void G::File::remove( const Path & path )
{
	int e = 0 ;
	bool ok = FileImp::removeImp( path.cstr() , &e ) ;
	if( !ok )
	{
		G_WARNING( "G::File::remove: cannot delete file [" << path << "]: " << Process::strerror(e) ) ;
		throw CannotRemove( path.str() , Process::strerror(e) ) ;
	}
}

int G::File::mkdirImp( const Path & dir ) noexcept
{
	int rc = ::mkdir( dir.cstr() , 0777 ) ; // open permissions, but limited by umask
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

G::File::Stat G::File::statImp( const char * path , bool symlink_nofollow ) noexcept
{
	Stat s ;
	struct stat statbuf {} ;
	if( 0 == ( symlink_nofollow ? (::lstat(path,&statbuf)) : (::stat(path,&statbuf)) ) )
	{
		s.error = 0 ;
		s.enoent = false ;
		s.eaccess = false ;
		s.is_link = (statbuf.st_mode & S_IFMT) == S_IFLNK ; // NOLINT
		s.is_dir = (statbuf.st_mode & S_IFMT) == S_IFDIR ; // NOLINT
		s.is_executable = !!(statbuf.st_mode & S_IXUSR) && !!(statbuf.st_mode & S_IRUSR) ; // indicitive // NOLINT
		s.is_empty = statbuf.st_size == 0 ;
		s.mtime_s = FileImp::mtime(statbuf).first ;
		s.mtime_us = FileImp::mtime(statbuf).second ;
		s.mode = static_cast<unsigned long>( statbuf.st_mode & mode_t(07777) ) ; // NOLINT
		s.size = static_cast<unsigned long long>( statbuf.st_size ) ;
		s.blocks = static_cast<unsigned long long>(statbuf.st_size) >> 24U ;
		s.uid = statbuf.st_uid ;
		s.gid = statbuf.st_gid ;
		s.inherit = s.is_dir && ( G::is_bsd() || ( statbuf.st_mode & S_ISGID ) ) ;
	}
	else
	{
		int error = Process::errno_() ;
		s.error = error ? error : EINVAL ;
		s.enoent = error == ENOENT || error == ENOTDIR ;
		s.eaccess = error == EACCES ;
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

bool G::File::chmodx( const Path & path , bool do_throw )
{
	Stat s = statImp( path.cstr() ) ;
	mode_t mode = s.error ? mode_t(0777) : mode_t(s.mode) ;

	mode |= ( S_IRUSR | S_IXUSR ) ; // add user-read and user-executable // NOLINT
	if( mode & S_IRGRP ) mode |= S_IXGRP ; // add group-executable iff group-read // NOLINT
	if( mode & S_IROTH ) mode |= S_IXOTH ; // add world-executable iff world-read // NOLINT

	// apply the current umask
	mode_t mask = ::umask( 0 ) ; ::umask( mask ) ;
	mode &= ~mask ;

	bool ok = 0 == ::chmod( path.cstr() , mode ) ;
	if( !ok && do_throw )
		throw CannotChmod( path.str() ) ;
	return ok ;
}

#ifndef G_LIB_SMALL
void G::File::chmod( const Path & path , const std::string & spec )
{
	if( !chmod( path , spec , std::nothrow ) )
		throw CannotChmod( path.str() ) ;
}
#endif

bool G::File::chmod( const Path & path , const std::string & spec , std::nothrow_t )
{
	if( spec.empty() )
	{
		return false ;
	}
	else if( spec.find_first_not_of("01234567") == std::string::npos )
	{
		mode_t mode = static_cast<mode_t>( strtoul( spec.c_str() , nullptr , 8 ) ) ;
		return mode <= 07777 && 0 == ::chmod( path.cstr() , mode ) ;
	}
	else
	{
		Stat s = statImp( path.cstr() ) ;
		if( s.error )
			return false ;
		std::pair<bool,mode_t> pair = FileImp::newmode( s.mode , spec ) ;
		return pair.first && 0 == ::chmod( path.cstr() , pair.second ) ;
	}
}

std::pair<bool,mode_t> G::FileImp::newmode( mode_t mode , const std::string & spec_in )
{
	mode &= mode_t(07777) ;
	G::StringArray spec_list = G::Str::splitIntoFields( spec_in , ',' ) ;
	bool ok = !spec_list.empty() ;
	for( auto spec : spec_list )
	{
		if( spec.size() >= 2U &&
			( spec.at(0U) == '+' || spec.at(0U) == '-' || spec.at(0U) == '=' ) )
		{
			spec.insert( 0U , "a" ) ;
		}
		if( spec.size() >= 3U &&
			( spec.at(0U) == 'u' || spec.at(0U) == 'g' || spec.at(0U) == 'o' || spec.at(0U) == 'a' ) &&
			( spec.at(1U) == '+' || spec.at(1U) == '-' || spec.at(1U) == '=' ) )
		{
			mode_t part = 0 ;
			mode_t special = 0 ;
			for( const char * p = spec.c_str()+2 ; *p ; p++ )
			{
				if( *p == 'r' )
					part |= mode_t(4) ;
				else if( *p == 'w' )
					part |= mode_t(2) ;
				else if( *p == 'x' )
					part |= mode_t(1) ;
				else if( *p == 's' && spec[0] == 'u' )
					special |= S_ISUID ; // NOLINT
				else if( *p == 's' && spec[0] == 'g' )
					special |= S_ISGID ; // NOLINT
				else if( *p == 't' && spec[0] == 'o' )
					special |= S_ISVTX ; // NOLINT
				else
					ok = false ;
			}
			unsigned int shift = spec[0]=='u' ? 6U : (spec[0]=='g'?3U:0U) ;
			if( spec[0] == 'a' )
			{
				mode_t mask = umask(0) ; umask( mask ) ;
				part = ( ((part<<6U)|(part<<3U)|part) & ~mask ) ;
			}
			if( spec[1] == '=' && spec[0] == 'a' )
			{
				mode = part ;
			}
			else if( spec[1] == '=' )
			{
				mode_t clearbits = (mode_t(7)<<shift) | (spec[0]=='u'?mode_t(S_ISUID):(spec[0]=='g'?mode_t(S_ISGID):mode_t(S_ISVTX))) ;
				mode &= ~clearbits ;
				mode |= (part<<shift) ;
				mode |= special ;
			}
			else if( spec[1] == '+' )
			{
				mode |= ( (part<<shift) | special ) ;
			}
			else
			{
				mode &= ~( (part<<shift) | special ) ;
			}
		}
		else
		{
			ok = false ;
		}
	}
	return { ok , mode } ;
}

#ifndef G_LIB_SMALL
void G::File::chgrp( const Path & path , const std::string & group )
{
	bool ok = 0 == ::chown( path.cstr() , -1 , Identity::lookupGroup(group) ) ;
	if( !ok )
		throw CannotChgrp( path.str() ) ;
}
#endif

#ifndef G_LIB_SMALL
bool G::File::chgrp( const Path & path , const std::string & group , std::nothrow_t )
{
	return 0 == ::chown( path.cstr() , -1 , Identity::lookupGroup(group) ) ;
}
#endif

bool G::File::chgrp( const Path & path , gid_t group_id , std::nothrow_t )
{
	return 0 == ::chown( path.cstr() , -1 , group_id ) ;
}

bool G::File::hardlink( const Path & src , const Path & dst , std::nothrow_t )
{
	return 0 == ::link( src.cstr() , dst.cstr() ) ;
}

#ifndef G_LIB_SMALL
void G::File::link( const Path & target , const Path & new_link )
{
	if( linked(target,new_link) ) // optimisation
		return ;

	if( exists(new_link) )
		File::remove( new_link , std::nothrow ) ;

	int error = linkImp( target.cstr() , new_link.cstr() ) ;

	if( error != 0 )
	{
		std::ostringstream ss ;
		ss << "[" << new_link << "] -> [" << target << "] " "(" << error << ")" ;
		throw CannotLink( ss.str() ) ;
	}
}
#endif

#ifndef G_LIB_SMALL
bool G::File::link( const Path & target , const Path & new_link , std::nothrow_t )
{
	if( linked(target,new_link) ) // optimisation
		return true ;

	if( exists(new_link) )
		File::remove( new_link , std::nothrow ) ;

	return 0 == linkImp( target.cstr() , new_link.cstr() ) ;
}
#endif

int G::File::linkImp( const char * target , const char * new_link )
{
	int rc = ::symlink( target , new_link ) ;
	int error = Process::errno_() ;
	return rc == 0 ? 0 : (error?error:EINVAL) ;
}

#ifndef G_LIB_SMALL
G::Path G::File::readlink( const Path & link )
{
	Path result = readlink( link , std::nothrow ) ;
	if( result.empty() )
		throw CannotReadLink( link.str() ) ;
	return result ;
}
#endif

G::Path G::File::readlink( const Path & link , std::nothrow_t )
{
	Path result ;
	struct stat statbuf {} ;
	int rc = ::lstat( link.cstr() , &statbuf ) ;
	if( rc == 0 )
	{
		std::size_t buffer_size = statbuf.st_size ? (statbuf.st_size+1U) : 1024U ;
		std::vector<char> buffer( buffer_size , '\0' ) ;
		ssize_t nread = ::readlink( link.cstr() , buffer.data() , buffer.size() ) ;

		// (filesystem race can cause truncation -- treat as an error)
		if( nread > 0 && static_cast<std::size_t>(nread) < buffer.size() )
		{
			G_ASSERT( buffer.at(static_cast<std::size_t>(nread-1)) != '\0' ) ; // readlink does not null-terminate
			result = Path( std::string( buffer.data() , static_cast<std::size_t>(nread) ) ) ;
		}
	}
	return result ;
}

bool G::File::linked( const Path & target , const Path & new_link )
{
	// see if already linked correctly - errors and overflows are not fatal
	return readlink(new_link,std::nothrow) == target ;
}

std::streamoff G::File::seek( int fd , std::streamoff offset , Seek origin ) noexcept
{
	off_t rc = ::lseek( fd , static_cast<off_t>(offset) ,
		origin == Seek::Start ? SEEK_SET : ( origin == Seek::End ? SEEK_END : SEEK_CUR ) ) ;
	return static_cast<std::streamoff>(rc) ;
}

#ifndef G_LIB_SMALL
void G::File::setNonBlocking( int fd ) noexcept
{
	int flags = ::fcntl( fd , F_GETFL ) ; // NOLINT
	if( flags != -1 )
	{
		flags |= O_NONBLOCK ; // NOLINT
		GDEF_IGNORE_RETURN ::fcntl( fd , F_SETFL , flags ) ; // NOLINT
	}
}
#endif

