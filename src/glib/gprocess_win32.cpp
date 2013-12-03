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
// gprocess_win32.cpp
//

#include "gdef.h"
#include "glimits.h"
#include "gprocess.h"
#include "gexception.h"
#include "gstr.h"
#include "gfs.h"
#include "glog.h"
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <process.h>
#include <io.h>
#include <fcntl.h>

class G::Process::IdImp 
{
public: 
	unsigned int m_pid ;
} ;

// ===

G::Process::Id::Id() 
{
	m_pid = static_cast<unsigned int>(::_getpid()) ; // or ::GetCurrentProcessId()
}

G::Process::Id::Id( SignalSafe , const char * path ) :
	m_pid(0)
{
	std::ifstream file( path ? path : "" ) ;
	file >> m_pid ;
	if( !file.good() )
		m_pid = 0 ;
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

bool G::Process::Id::operator==( const Id & rhs ) const
{
	return m_pid == rhs.m_pid ;
}

// not implemented...
//G::Process::Id::Id( const char * pid_file_path ) {} 

// ===

void G::Process::closeFiles( bool keep_stderr )
{
	std::cout << std::flush ;
	std::cerr << std::flush ;
	// old versions of this code closed files 
	// but it's not really needed
}

void G::Process::closeStderr()
{
}

void G::Process::cd( const Path & dir )
{
	if( !cd(dir,NoThrow()) )
		throw CannotChangeDirectory( dir.str() ) ;
}

bool G::Process::cd( const Path & dir , NoThrow )
{
	return 0 == ::_chdir( dir.str().c_str() ) ;
}

int G::Process::errno_()
{
	return errno ;
}

int G::Process::errno_( int e )
{
	int old = errno ;
	errno = e ;
	return old ;
}

std::string G::Process::strerror( int errno_ )
{
	std::ostringstream ss ;
	ss << errno_ ; // could do better
	return ss.str() ;
}

G::Identity G::Process::beOrdinary( Identity identity , bool )
{
	// not implemented -- see also ImpersonateLoggedOnUser()
	return identity ;
}

G::Identity G::Process::beOrdinary( SignalSafe , Identity identity , bool )
{
	// not implemented -- see also ImpersonateLoggedOnUser()
	return identity ;
}

G::Identity G::Process::beSpecial( Identity identity , bool )
{
	// not implemented -- see also RevertToSelf()
	return identity ;
}

G::Identity G::Process::beSpecial( SignalSafe , Identity identity , bool )
{
	// not implemented -- see also RevertToSelf()
	return identity ;
}

void G::Process::revokeExtraGroups()
{
	// not implemented
}

// not implemented...
// Who G::Process::fork() {}
// Who G::Process::fork( Id & child ) {}
// void G::Process::exec( const Path & exe , const std::string & arg ) {}
// int G::Process::wait( const Id & child ) {}
// int G::Process::wait( const Id & child , int error_return ) {}

// ===

G::Process::Umask::Umask( G::Process::Umask::Mode ) :
	m_imp(0)
{
}

G::Process::Umask::~Umask()
{
}

void G::Process::Umask::set( G::Process::Umask::Mode )
{
	// not implemented
}

/// \file gprocess_win32.cpp
