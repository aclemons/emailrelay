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
/// \file gprocess_win32.cpp
///

#include "gdef.h"
#include "gnowide.h"
#include "gprocess.h"
#include "gexception.h"
#include "gconvert.h"
#include "gstr.h"
#include "glog.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <process.h>
#include <io.h>
#include <fcntl.h>
#include <direct.h> // _getcwd()

G::Process::Id::Id() noexcept
{
	m_pid = static_cast<unsigned int>(::_getpid()) ; // or ::GetCurrentProcessId()
}

std::string G::Process::Id::str() const
{
	std::ostringstream ss ;
	ss << m_pid ;
	return ss.str() ;
}

bool G::Process::Id::operator==( const Id & rhs ) const noexcept
{
	return m_pid == rhs.m_pid ;
}

bool G::Process::Id::operator!=( const Id & rhs ) const noexcept
{
	return m_pid != rhs.m_pid ;
}

// ===

void G::Process::closeFiles( bool /*keep_stderr*/ )
{
	std::cout << std::flush ;
	std::cerr << std::flush ;
}

void G::Process::closeOtherFiles( int )
{
}

void G::Process::inheritStandardFiles()
{
}

void G::Process::closeStderr()
{
}

void G::Process::cd( const Path & dir )
{
	if( !cd(dir,std::nothrow) )
		throw CannotChangeDirectory( dir.str() ) ;
}

bool G::Process::cd( const Path & dir , std::nothrow_t )
{
	return 0 == ::_chdir( dir.cstr() ) ;
}

int G::Process::errno_( const SignalSafe & ) noexcept
{
	int e = EINVAL ;
	if( _get_errno( &e ) )
		e = EINVAL ;
	return e ;
}

void G::Process::errno_( int e ) noexcept
{
	_set_errno( e ) ;
}

int G::Process::errno_( const SignalSafe & , int e ) noexcept
{
	int old = errno_( SignalSafe() ) ;
	_set_errno( e ) ;
	return old ;
}

std::string G::Process::strerror( int errno_ )
{
	std::string s = nowide::strerror( errno_ ) ;
	return Str::isPrintableAscii(s) ? Str::lower(s) : s ;
}

std::string G::Process::errorMessage( DWORD e )
{
	std::string s = nowide::formatMessage( e ) ;
	G::Str::trimRight( s , ".\r\n" ) ;
	if( s.empty() )
		return std::string("error ").append( std::to_string(e) ) ;
	else
		return Str::isPrintableAscii(s) ? Str::lower(s) : s ;
}

std::pair<G::Identity,G::Identity> G::Process::beOrdinaryAtStartup( const std::string & , bool )
{
	// identity switching not implemented for windows
	return { Identity::invalid() , Identity::invalid() } ;
}

void G::Process::beOrdinary( Identity , bool )
{
}

void G::Process::beOrdinaryForExec( Identity ) noexcept
{
}

void G::Process::beSpecial( Identity , bool )
{
}

void G::Process::beSpecialForExit( SignalSafe , Identity ) noexcept
{
}

void G::Process::setEffectiveUser( Identity )
{
}

void G::Process::setEffectiveGroup( Identity )
{
}

G::Path G::Process::exe()
{
	return nowide::exe() ;
}

G::Path G::Process::cwd()
{
	G::Path result = nowide::cwd() ;
	if( result.empty() )
		throw Process::GetCwdError() ;
	return result ;
}

G::Path G::Process::cwd( std::nothrow_t )
{
	return nowide::cwd() ;
}

// ===

class G::Process::UmaskImp
{
} ;

G::Process::Umask::Umask( Process::Umask::Mode )
{
}

G::Process::Umask::~Umask()
= default ;

void G::Process::Umask::set( Process::Umask::Mode )
{
	// not implemented
}

void G::Process::Umask::tightenOther()
{
	// not implemented
}

void G::Process::Umask::loosenGroup()
{
	// not implemented
}

