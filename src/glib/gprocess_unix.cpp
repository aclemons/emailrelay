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
// gprocess_unix.cpp
//

#include "gdef.h"
#include "gprocess.h"
#include "gidentity.h"
#include "gstr.h"
#include "gfile.h"
#include "gpath.h"
#include "glog.h"
#include <iostream>
#include <cstring> // std::strerror()
#include <limits.h>
#include <fcntl.h>
#include <errno.h>

namespace
{
	void noCloseOnExec( int fd )
	{
		::fcntl( fd , F_SETFD , 0 ) ;
	}
}

/// \class G::Process::IdImp
/// A private implementation class used by G::Process that wraps
/// a process id and hides the pid_t type.
///
class G::Process::IdImp
{
public:
	pid_t m_pid ;
} ;

// ===

void G::Process::cd( const Path & dir )
{
	if( ! cd(dir,NoThrow()) )
		throw CannotChangeDirectory( dir.str() ) ;
}

bool G::Process::cd( const Path & dir , NoThrow )
{
	return 0 == ::chdir( dir.str().c_str() ) ;
}

void G::Process::closeStderr()
{
	::close( STDERR_FILENO ) ;
	::open( Path::nullDevice().str().c_str() , O_WRONLY ) ;
	noCloseOnExec( STDERR_FILENO ) ;
}

void G::Process::closeFiles( bool keep_stderr )
{
	closeFilesExcept( keep_stderr ? STDERR_FILENO : -1 ) ;
}

void G::Process::closeFilesExcept( int keep_1 , int keep_2 )
{
	// flush
	std::cout << std::flush ;
	std::cerr << std::flush ;

	// find how many to close
	int n = 256U ;
	long rc = ::sysconf( _SC_OPEN_MAX ) ;
	if( rc > 0L )
		n = static_cast<int>( rc ) ;

	// close everything except those given
	for( int fd = 0 ; fd < n ; fd++ )
	{
		if( fd != keep_1 && fd != keep_2 )
			::close( fd ) ;
	}

	// reopen standard fds to /dev/null
	Path dev_null = Path::nullDevice() ;
	if( keep_1 != STDIN_FILENO && keep_2 != STDIN_FILENO )
	{
		::open( dev_null.str().c_str() , O_RDONLY ) ;
	}
	if( keep_1 != STDOUT_FILENO && keep_2 != STDOUT_FILENO )
	{
		::open( dev_null.str().c_str() , O_WRONLY ) ;
	}
	if( keep_1 != STDERR_FILENO && keep_2 != STDERR_FILENO )
	{
		::open( dev_null.str().c_str() , O_WRONLY ) ;
	}

	// make sure standard fds stay open across exec()
	noCloseOnExec( STDIN_FILENO ) ;
	noCloseOnExec( STDOUT_FILENO ) ;
	noCloseOnExec( STDERR_FILENO ) ;
}

int G::Process::errno_( const G::SignalSafe & )
{
	return errno ; // not ::errno or std::errno
}

int G::Process::errno_( const G::SignalSafe & , int e_new )
{
	int e_old = errno ;
	errno = e_new ;
	return e_old ;
}

std::string G::Process::strerror( int errno_ )
{
	char * p = std::strerror( errno_ ) ;
	std::string s( p ? p : "" ) ;
	if( s.empty() ) s = "unknown error" ;
	return Str::isPrintableAscii(s) ? Str::lower(s) : s ;
}

void G::Process::revokeExtraGroups()
{
	if( Identity::real().isRoot() || Identity::effective() != Identity::real() )
	{
		gid_t dummy ;
		int rc = ::setgroups( 0U , &dummy ) ; G_IGNORE_VARIABLE(int,rc) ; // (only works for root, so ignore the return code)
	}
}

G::Identity G::Process::beSpecial( Identity special_identity , bool change_group )
{
	change_group = Identity::real().isRoot() ? change_group : true ;
	Identity old_identity( Identity::effective() ) ;
	setEffectiveUserTo( special_identity ) ;
	if( change_group )
	setEffectiveGroupTo( special_identity ) ;
	return old_identity ;
}

G::Identity G::Process::beSpecial( SignalSafe safe , Identity special_identity , bool change_group )
{
	change_group = Identity::real().isRoot() ? change_group : true ;
	Identity old_identity( Identity::effective() ) ;
	setEffectiveUserTo( safe , special_identity ) ;
	if( change_group )
	setEffectiveGroupTo( safe , special_identity ) ;
	return old_identity ;
}

G::Identity G::Process::beOrdinary( Identity ordinary_id , bool change_group )
{
	Identity special_identity( Identity::effective() ) ;
	if( Identity::real().isRoot() )
	{
		setEffectiveUserTo( Identity::root() ) ;
		if( change_group )
		setEffectiveGroupTo( ordinary_id ) ;
		setEffectiveUserTo( ordinary_id ) ;
	}
	else
	{
		setEffectiveUserTo( Identity::real() ) ;
		if( change_group )
		setEffectiveGroupTo( Identity::real() ) ;
	}
	return special_identity ;
}

G::Identity G::Process::beOrdinary( SignalSafe safe , Identity ordinary_id , bool change_group )
{
	Identity special_identity( Identity::effective() ) ;
	if( Identity::real().isRoot() )
	{
		setEffectiveUserTo( safe , Identity::root() ) ;
		if( change_group )
		setEffectiveGroupTo( safe , ordinary_id ) ;
		setEffectiveUserTo( safe , ordinary_id ) ;
	}
	else
	{
		setEffectiveUserTo( safe , Identity::real() ) ;
		if( change_group )
		setEffectiveGroupTo( safe , Identity::real() ) ;
	}
	return special_identity ;
}

void G::Process::beOrdinaryForExec( Identity run_as_id )
{
	if( run_as_id != Identity::invalid() )
	{
		setEffectiveUserTo( Identity::root() , false ) ; // for root-suid
		setRealGroupTo( run_as_id , false ) ;
		setEffectiveGroupTo( run_as_id , false ) ;
		setRealUserTo( run_as_id , false ) ;
		setEffectiveUserTo( run_as_id , false ) ;
	}
}

std::string G::Process::cwd( bool no_throw )
{
	size_t n = PATH_MAX ; n += 10U ;
	std::vector<char> buffer( n ) ;
	for( int i = 0 ; i < 1000 ; i++ )
	{
		char * p = getcwd( &buffer[0] , buffer.size() ) ;
		int error = errno_() ;
		if( p == nullptr && error == ERANGE )
			buffer.resize( buffer.size() + n ) ;
		else if( p == nullptr && !no_throw )
			throw std::runtime_error( "getcwd() failed" ) ;
		else
			break ;
	}
	buffer.push_back( '\0' ) ;
	return std::string( &buffer[0] ) ;
}

namespace
{
	bool readlink_( const char * path , std::string & value )
	{
		G::Path target = G::File::readlink( path , G::File::NoThrow() ) ;
		if( target != G::Path() ) value = target.str() ;
		return target != G::Path() ;
	}
}

#ifdef G_UNIX_MAC
#include <libproc.h>
std::string G::Process::exe()
{
	// (see also _NSGetExecutablePath())
	std::vector<char> buffer( std::max(100,PROC_PIDPATHINFO_MAXSIZE) ) ;
	buffer[0] = '\0' ;
	int rc = proc_pidpath( getpid() , &buffer[0] , buffer.size() ) ;
	if( rc > 0 )
	{
		size_t n = static_cast<size_t>(rc) ;
		if( n > buffer.size() ) n = buffer.size() ;
		return std::string( &buffer[0] , n ) ;
	}
	else
	{
		return std::string() ;
	}
}
#else
std::string G::Process::exe()
{
	// best effort, not guaranteed
	std::string result ;
	readlink_( "/proc/self/exe" , result ) ||
	readlink_( "/proc/curproc/file" , result ) ||
	readlink_( "/proc/curproc/exe" , result ) ;
	return result ;
}
#endif

// ===

G::Process::Id::Id()
{
	m_pid = ::getpid() ;
}

G::Process::Id::Id( SignalSafe , const char * path ) :
	m_pid(0)
{
	// signal-safe, reentrant implementation suitable for a signal handler...
	int fd = ::open( path ? path : "" , O_RDONLY ) ;
	if( fd >= 0 )
	{
		const size_t buffer_size = 11U ;
		char buffer[buffer_size] ;
		buffer[0U] = '\0' ;
		ssize_t rc = ::read( fd , buffer , buffer_size-1U ) ;
		::close( fd ) ;
		for( const char * p = buffer ; rc > 0 && *p >= '0' && *p <= '9' ; p++ , rc-- )
		{
			m_pid *= 10 ;
			m_pid += ( *p - '0' ) ;
		}
	}
}

G::Process::Id::Id( std::istream & stream )
{
	stream >> m_pid ;
	if( !stream.good() )
		throw Process::InvalidId() ;
}

std::string G::Process::Id::str() const
{
	std::ostringstream ss ;
	ss << m_pid ;
	return ss.str() ;
}

bool G::Process::Id::operator==( const Id & other ) const
{
	return m_pid == other.m_pid ;
}

// ===

namespace
{
	mode_t umask_value( G::Process::Umask::Mode mode )
	{
		mode_t m = 0 ;
		if( mode == G::Process::Umask::Mode::Tightest ) m = 0177 ; // -rw-------
		if( mode == G::Process::Umask::Mode::Tighter ) m = 0117 ;  // -rw-rw----
		if( mode == G::Process::Umask::Mode::Readable ) m = 0133 ; // -rw-r--r--
		if( mode == G::Process::Umask::Mode::GroupOpen ) m = 0113 ;// -rw-rw-r--
		return m ;
	}
}

/// \class G::Process::UmaskImp
/// A pimple-pattern implementation class used by G::Process::Umask.
///
class G::Process::UmaskImp
{
public:
	mode_t m_old_mode ;
} ;

G::Process::Umask::Umask( Mode mode ) :
	m_imp(new UmaskImp)
{
	m_imp->m_old_mode = ::umask( umask_value(mode) ) ;
}

G::Process::Umask::~Umask()
{
	mode_t rc = ::umask( m_imp->m_old_mode ) ; G_IGNORE_VARIABLE(mode_t,rc) ;
}

void G::Process::Umask::set( Mode mode )
{
	mode_t rc = ::umask( umask_value(mode) ) ; G_IGNORE_VARIABLE(mode_t,rc) ;
}

void G::Process::Umask::tighten()
{
	::umask( ::umask(2) | mode_t(7) ) ; // -xxxxxx---
}

