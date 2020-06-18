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
#include <stdexcept>
#include <array>
#include <cstring> // std::strerror()
#include <climits> // PATH_MAX
#include <cerrno> // errno
#include <fcntl.h>

namespace G
{
	namespace ProcessImp
	{
		void noCloseOnExec( int fd )
		{
			::fcntl( fd , F_SETFD , 0 ) ;
		}
		void reopen( int fd , int mode )
		{
			int fd_null = ::open( Path::nullDevice().str().c_str() , mode ) ;
			if( fd_null < 0 ) throw std::runtime_error( "cannot open /dev/null" ) ;
			::dup2( fd_null , fd ) ;
			::close( fd_null ) ;
		}
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
	ProcessImp::reopen( STDERR_FILENO , O_WRONLY ) ;
}

void G::Process::closeFiles( bool keep_stderr )
{
	std::cout << std::flush ;
	std::cerr << std::flush ;

	ProcessImp::reopen( STDIN_FILENO , O_RDONLY ) ;
	ProcessImp::reopen( STDOUT_FILENO , O_WRONLY ) ;
	if( !keep_stderr )
		ProcessImp::reopen( STDERR_FILENO , O_WRONLY ) ;

	closeOtherFiles() ;
}

void G::Process::closeOtherFiles( int fd_keep )
{
	int n = 256U ;
	long rc = ::sysconf( _SC_OPEN_MAX ) ;
	if( rc > 0L )
		n = static_cast<int>( rc ) ;

	for( int fd = 0 ; fd < n ; fd++ )
	{
		if( fd != STDIN_FILENO && fd != STDOUT_FILENO && fd != STDERR_FILENO && fd != fd_keep )
			::close( fd ) ;
	}
	ProcessImp::noCloseOnExec( STDIN_FILENO ) ;
	ProcessImp::noCloseOnExec( STDOUT_FILENO ) ;
	ProcessImp::noCloseOnExec( STDERR_FILENO ) ;
}

int G::Process::errno_( const G::SignalSafe & )
{
	return errno ; // macro, not ::errno or std::errno
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
		// set supplementary group-ids to a zero-length list
		gid_t dummy = 0 ;
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
	std::size_t n = PATH_MAX ; n += 10U ;
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

namespace G
{
	namespace ProcessImp
	{
		bool readlink_( const char * path , std::string & value )
		{
			G::Path target = G::File::readlink( path , G::File::NoThrow() ) ;
			if( target != G::Path() ) value = target.str() ;
			return target != G::Path() ;
		}
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
		std::size_t n = static_cast<std::size_t>(rc) ;
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
	ProcessImp::readlink_( "/proc/self/exe" , result ) ||
	ProcessImp::readlink_( "/proc/curproc/file" , result ) ||
	ProcessImp::readlink_( "/proc/curproc/exe" , result ) ;
	return result ;
}
#endif

// ===

G::Process::Id::Id()
{
	m_pid = ::getpid() ;
}

G::Process::Id::Id( SignalSafe , const char * path ) :
	m_pid(0U)
{
	// signal-safe, reentrant implementation suitable for a signal handler...
	int fd = ::open( path ? path : "" , O_RDONLY ) ;
	if( fd >= 0 )
	{
		constexpr std::size_t buffer_size = 11U ;
		std::array<char,buffer_size> buffer ; // NOLINT cppcoreguidelines-pro-type-member-init
		buffer[0U] = '\0' ;
		ssize_t rc = ::read( fd , &buffer[0] , buffer_size-1U ) ;
		::close( fd ) ;
		for( const char * p = &buffer[0] ; rc > 0 && *p >= '0' && *p <= '9' ; p++ , rc-- )
		{
			m_pid *= 10 ;
			m_pid += ( *p - '0' ) ;
		}
	}
}

G::Process::Id::Id( std::istream & stream ) :
	m_pid(0U)
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

bool G::Process::Id::operator==( const Id & other ) const noexcept
{
	return m_pid == other.m_pid ;
}

// ===

namespace G
{
	namespace ProcessImp
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
	m_imp->m_old_mode = ::umask( ProcessImp::umask_value(mode) ) ;
}

G::Process::Umask::~Umask()
{
	mode_t rc = ::umask( m_imp->m_old_mode ) ; G_IGNORE_VARIABLE(mode_t,rc) ;
}

void G::Process::Umask::set( Mode mode )
{
	mode_t rc = ::umask( ProcessImp::umask_value(mode) ) ; G_IGNORE_VARIABLE(mode_t,rc) ;
}

void G::Process::Umask::tighten()
{
	::umask( ::umask(2) | mode_t(7) ) ; // -xxxxxx---
}

