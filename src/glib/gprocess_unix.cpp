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
		void noCloseOnExec( int fd ) noexcept ;
		void reopen( int fd , int mode ) ;
		mode_t umaskValue( G::Process::Umask::Mode mode ) ;
		bool readlink_( const char * path , std::string & value ) ;
		void setRealUserTo( Identity id ) ;
		void setEffectiveUserTo( Identity id ) ;
		bool setEffectiveUserTo( SignalSafe safe , Identity id ) noexcept ;
		void setRealGroupTo( Identity id ) ;
		void setEffectiveGroupTo( Identity id ) ;
		bool setEffectiveGroupTo( SignalSafe safe , Identity id ) noexcept ;
		void setEffectiveUserAndGroupTo( Identity id ) ;
		void setEffectiveUserAndGroupAsRootTo( Identity id ) ;
		void terminate() noexcept ;
	}
}

class G::Process::Umask::UmaskImp
{
public:
	mode_t m_old_mode ;
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

int G::Process::errno_( const G::SignalSafe & ) noexcept
{
	return errno ; // macro, not ::errno or std::errno
}

int G::Process::errno_( const G::SignalSafe & , int e_new ) noexcept
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
		int rc = ::setgroups( 0U , &dummy ) ;
		G__IGNORE_VARIABLE(int,rc) ; // (only works for root, so ignore the return code)
	}
}

G::Identity G::Process::beSpecial( Identity special_identity , bool change_group )
{
	change_group = Identity::real().isRoot() ? change_group : true ;
	Identity old_identity( Identity::effective() ) ;
	if( change_group )
		ProcessImp::setEffectiveUserAndGroupTo( special_identity ) ;
	else
		ProcessImp::setEffectiveUserTo( special_identity ) ;
	return old_identity ;
}

G::Identity G::Process::beSpecial( SignalSafe safe , Identity special_identity , bool change_group ) noexcept
{
	change_group = Identity::real().isRoot() ? change_group : true ;
	Identity old_identity( Identity::effective() ) ;
	ProcessImp::setEffectiveUserTo( safe , special_identity ) ;
	if( change_group )
		ProcessImp::setEffectiveGroupTo( safe , special_identity ) ;
	return old_identity ;
}

G::Identity G::Process::beOrdinary( Identity ordinary_id , bool change_group )
{
	Identity special_identity( Identity::effective() ) ;
	if( Identity::real().isRoot() )
		ProcessImp::setEffectiveUserAndGroupAsRootTo( ordinary_id ) ;
	else if( change_group )
		ProcessImp::setEffectiveUserAndGroupTo( Identity::real() ) ;
	else
		ProcessImp::setEffectiveUserTo( Identity::real() ) ;
	return special_identity ;
}

G::Identity G::Process::beOrdinary( SignalSafe safe , Identity ordinary_id , bool change_group ) noexcept
{
	Identity special_identity( Identity::effective() ) ;
	if( Identity::real().isRoot() )
	{
		ProcessImp::setEffectiveUserTo( safe , Identity::root() ) ;
		if( change_group )
			ProcessImp::setEffectiveGroupTo( safe , ordinary_id ) ;
		ProcessImp::setEffectiveUserTo( safe , ordinary_id ) ;
	}
	else
	{
		ProcessImp::setEffectiveUserTo( safe , Identity::real() ) ;
		if( change_group )
			ProcessImp::setEffectiveGroupTo( safe , Identity::real() ) ;
	}
	return special_identity ;
}

void G::Process::beOrdinaryForExec( Identity run_as_id ) noexcept
{
	using NoThrow = Identity::NoThrow ;
	if( run_as_id != Identity::invalid() )
	{
		Identity::root().setEffectiveUser( NoThrow() ) ; // for root-suid
		run_as_id.setRealGroup( NoThrow() ) ;
		run_as_id.setEffectiveGroup( NoThrow() ) ;
		run_as_id.setRealUser( NoThrow() ) ;
		run_as_id.setEffectiveUser( NoThrow() ) ;
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

void G::Process::terminate() noexcept
{
	ProcessImp::terminate() ;
}

// ===

G::Process::Id::Id() noexcept
{
	m_pid = ::getpid() ;
}

G::Process::Id::Id( SignalSafe , const char * path ) noexcept :
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

G::Process::Umask::Umask( Mode mode ) :
	m_imp(std::make_unique<UmaskImp>())
{
	m_imp->m_old_mode = ::umask( ProcessImp::umaskValue(mode) ) ;
}

G::Process::Umask::~Umask()
{
	mode_t rc = ::umask( m_imp->m_old_mode ) ; G__IGNORE_VARIABLE(mode_t,rc) ;
}

void G::Process::Umask::set( Mode mode )
{
	mode_t rc = ::umask( ProcessImp::umaskValue(mode) ) ; G__IGNORE_VARIABLE(mode_t,rc) ;
}

void G::Process::Umask::tighten()
{
	::umask( ::umask(2) | mode_t(7) ) ; // -xxxxxx---
}

// ===

void G::ProcessImp::setRealUserTo( Identity id )
{
	id.setRealUser() ;
}

void G::ProcessImp::setEffectiveUserTo( Identity id )
{
	id.setEffectiveUser() ;
}

bool G::ProcessImp::setEffectiveUserTo( SignalSafe safe , Identity id ) noexcept
{
	return id.setEffectiveUser( safe ) ;
}

void G::ProcessImp::setRealGroupTo( Identity id )
{
	id.setRealGroup() ;
}

void G::ProcessImp::setEffectiveGroupTo( Identity id )
{
	id.setEffectiveGroup() ;
}

bool G::ProcessImp::setEffectiveGroupTo( SignalSafe safe , Identity id ) noexcept
{
	return id.setEffectiveGroup( safe ) ;
}

void G::ProcessImp::setEffectiveUserAndGroupTo( Identity id )
{
	using NoThrow = Identity::NoThrow ;
	Identity old_id = Identity::effective() ;
	id.setEffectiveUser() ; // throws
	if( !id.setEffectiveGroup(NoThrow()) )
	{
		if( !old_id.setEffectiveUser(NoThrow()) ) // unwind
			terminate() ;
		throw Identity::GidError() ;
	}
}

void G::ProcessImp::setEffectiveUserAndGroupAsRootTo( Identity id )
{
	using NoThrow = Identity::NoThrow ;
	Identity old_id = Identity::effective() ;
	Identity::root().setEffectiveUser() ; // throws
	if( !id.setEffectiveGroup(NoThrow()) )
	{
		if( !old_id.setEffectiveUser(NoThrow()) ) // unwind
			terminate() ;
		throw Identity::GidError() ;
	}
	if( !id.setEffectiveUser(NoThrow()) )
	{
		if( !old_id.setEffectiveGroup(NoThrow()) || !old_id.setEffectiveUser(NoThrow()) ) // unwind
			terminate() ;
		throw Identity::UidError() ;
	}
}

void G::ProcessImp::noCloseOnExec( int fd ) noexcept
{
	::fcntl( fd , F_SETFD , 0 ) ;
}

void G::ProcessImp::reopen( int fd , int mode )
{
	int fd_null = ::open( Path::nullDevice().str().c_str() , mode ) ;
	if( fd_null < 0 ) throw std::runtime_error( "cannot open /dev/null" ) ;
	::dup2( fd_null , fd ) ;
	::close( fd_null ) ;
}

mode_t G::ProcessImp::umaskValue( G::Process::Umask::Mode mode )
{
	mode_t m = 0 ;
	if( mode == G::Process::Umask::Mode::Tightest ) m = 0177 ; // -rw-------
	if( mode == G::Process::Umask::Mode::Tighter ) m = 0117 ;  // -rw-rw----
	if( mode == G::Process::Umask::Mode::Readable ) m = 0133 ; // -rw-r--r--
	if( mode == G::Process::Umask::Mode::GroupOpen ) m = 0113 ;// -rw-rw-r--
	return m ;
}

bool G::ProcessImp::readlink_( const char * path , std::string & value )
{
	G::Path target = G::File::readlink( path , G::File::NoThrow() ) ;
	if( target != G::Path() ) value = target.str() ;
	return target != G::Path() ;
}

void G::ProcessImp::terminate() noexcept
{
	try
	{
		G_ERROR( "G::ProcessImp::terminate: failed to give up process privileges" ) ;
	}
	catch(...)
	{
	}
	std::terminate() ;
}

/// \file gprocess_unix.cpp
