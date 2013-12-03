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
// gprocess_unix.cpp
//

#include "gdef.h"
#include "glimits.h"
#include "gprocess.h"
#include "gidentity.h"
#include "gassert.h"
#include "gfs.h"
#include "glog.h"
#include <iostream>
#include <cstring> // std::strerror()
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
/// A private implementation class used by G::Process.
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
	::open( G::FileSystem::nullDevice() , O_WRONLY ) ;
	noCloseOnExec( STDERR_FILENO ) ;
}

void G::Process::closeFiles( bool keep_stderr )
{
	closeFiles( keep_stderr ? STDERR_FILENO : -1 ) ;
}

void G::Process::closeFiles( int keep )
{
	G_ASSERT( keep == -1 || keep >= STDERR_FILENO ) ;
	std::cout << std::flush ;
	std::cerr << std::flush ;

	int n = 256U ;
	long rc = ::sysconf( _SC_OPEN_MAX ) ;
	if( rc > 0L )
		n = static_cast<int>( rc ) ;

	for( int fd = 0 ; fd < n ; fd++ )
	{
		if( fd != keep )
			::close( fd ) ;
	}

	// reopen standard fds to prevent accidental use 
	// of arbitrary files or sockets as standard
	// streams
	//
	::open( G::FileSystem::nullDevice() , O_RDONLY ) ;
	::open( G::FileSystem::nullDevice() , O_WRONLY ) ;
	if( keep != STDERR_FILENO )
	{
		::open( G::FileSystem::nullDevice() , O_WRONLY ) ;
	}
	noCloseOnExec( STDIN_FILENO ) ;
	noCloseOnExec( STDOUT_FILENO ) ;
	noCloseOnExec( STDERR_FILENO ) ;
}

int G::Process::errno_()
{
	return errno ; // not ::errno or std::errno for gcc2.95
}

int G::Process::errno_( int e )
{
	int old = errno ;
	errno = e ;
	return old ;
}

std::string G::Process::strerror( int errno_ )
{
	char * p = std::strerror( errno_ ) ;
	return std::string( p ? p : "" ) ;
}

void G::Process::revokeExtraGroups()
{
	if( Identity::real().isRoot() || Identity::effective() != Identity::real() )
	{
		gid_t dummy ;
		G_IGNORE_RETURN(int) ::setgroups( 0U , &dummy ) ; // (only works for root, so ignore the return code)
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

G::Identity G::Process::beOrdinary( Identity nobody , bool change_group )
{
	Identity special_identity( Identity::effective() ) ;
	if( Identity::real().isRoot() )
	{
		setEffectiveUserTo( Identity::root() ) ;
		if( change_group ) 
		setEffectiveGroupTo( nobody ) ;
		setEffectiveUserTo( nobody ) ;
	}
	else
	{
		setEffectiveUserTo( Identity::real() ) ;
		if( change_group )
		setEffectiveGroupTo( Identity::real() ) ;
	}
	return special_identity ;
}

G::Identity G::Process::beOrdinary( SignalSafe safe , Identity nobody , bool change_group )
{
	Identity special_identity( Identity::effective() ) ;
	if( Identity::real().isRoot() )
	{
		setEffectiveUserTo( safe , Identity::root() ) ;
		if( change_group ) 
		setEffectiveGroupTo( safe , nobody ) ;
		setEffectiveUserTo( safe , nobody ) ;
	}
	else
	{
		setEffectiveUserTo( safe , Identity::real() ) ;
		if( change_group )
		setEffectiveGroupTo( safe , Identity::real() ) ;
	}
	return special_identity ;
}

void G::Process::beNobody( Identity nobody )
{
	if( Identity::real().isRoot() )
	{
		setEffectiveUserTo( Identity::root() ) ;
		setRealGroupTo( nobody ) ;
		setRealUserTo( nobody ) ;
	}
}

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
		ssize_t rc = ::read( fd , buffer , buffer_size - 1U ) ;
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
		if( mode == G::Process::Umask::Tightest ) m = 0177 ; // -rw-------
		if( mode == G::Process::Umask::Tighter ) m = 0117 ;  // -rw-rw----
		if( mode == G::Process::Umask::Readable ) m = 0133 ; // -rw-r--r--
		if( mode == G::Process::Umask::GroupOpen ) m = 0113 ;// -rw-rw-r--
		return m ;
	}
}

	/// A private implementation class used by G::Process::Umask. 
class G::Process::Umask::UmaskImp 
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
	G_IGNORE_RETURN(mode_t) ::umask( m_imp->m_old_mode ) ;
	delete m_imp ;
}

void G::Process::Umask::set( Mode mode )
{
	G_IGNORE_RETURN(mode_t) ::umask( umask_value(mode) ) ;
}

