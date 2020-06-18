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
// gidentity_unix.cpp
//
// Note that gcc/glibc previously required -D_BSD_SOURCE for seteuid().
//

#include "gdef.h"
#include "gidentity.h"
#include "gprocess.h"
#include "gassert.h"
#include <array>
#include <climits>
#include <sstream>
#include <vector>
#include <sstream>
#include <pwd.h> // getpwnam_r()
#include <unistd.h> // sysconf()

namespace G
{
	namespace IdentityImp
	{
		int sysconf_value( int key )
		{
			long n = ::sysconf( key ) ;
			return ( n < 0 || n > INT_MAX ) ? -1 : static_cast<int>(n) ;
		}
	}
}

G::Identity::Identity() noexcept :
	m_uid(static_cast<uid_t>(-1)) ,
	m_gid(static_cast<gid_t>(-1))
{
}

G::Identity::Identity( G::SignalSafe ) noexcept :
	m_uid(static_cast<uid_t>(-1)) ,
	m_gid(static_cast<gid_t>(-1))
{
}

G::Identity::Identity( const std::string & name , const std::string & group ) :
	m_uid(static_cast<uid_t>(-1)) ,
	m_gid(static_cast<gid_t>(-1))
{
	using passwd_t = struct passwd ;
	using group_t = struct group ;

	std::array<int,3U> sizes {{ 120 , 0 , 16000 }} ;
	sizes[1] = IdentityImp::sysconf_value( _SC_GETPW_R_SIZE_MAX ) ;
	for( auto size : sizes )
	{
		if( size <= 0 ) continue ;
		auto buffer_size = static_cast<std::size_t>(size) ;
		std::vector<char> buffer( buffer_size ) ;
		passwd_t pwd {} ;
		passwd_t * result_p = nullptr ;
		int rc = ::getpwnam_r( name.c_str() , &pwd , &buffer[0] , buffer_size , &result_p ) ;
		int e = Process::errno_() ;
		if( rc == 0 && result_p )
		{
			m_uid = result_p->pw_uid ;
			m_gid = result_p->pw_gid ;
			break ;
		}
		else if( rc == 0 && name == "root" ) // in case of no /etc/passwd file
		{
			m_uid = 0 ;
			m_gid = 0 ;
			break ;
		}
		else if( rc == 0 )
		{
			throw NoSuchUser( name ) ;
		}
		else if( e != ERANGE )
		{
			throw Error( Process::strerror(e) ) ;
		}
	}

	if( !group.empty() )
	{
		sizes[1] = IdentityImp::sysconf_value( _SC_GETGR_R_SIZE_MAX ) ;
		for( auto size : sizes )
		{
			if( size <= 0 ) continue ;
			auto buffer_size = static_cast<std::size_t>(size) ;
			std::vector<char> buffer( buffer_size ) ;
			group_t grp {} ;
			group_t * result_p = nullptr ;
			int rc = ::getgrnam_r( group.c_str() , &grp , &buffer[0] , buffer_size , &result_p ) ;
			if( rc == 0 && result_p )
			{
				m_gid = result_p->gr_gid ;
				break ;
			}
			else if( rc == 0 )
			{
				throw NoSuchGroup( group ) ;
			}
		}
	}
}

G::Identity G::Identity::effective()
{
	Identity id ;
	id.m_uid = ::geteuid() ;
	id.m_gid = ::getegid() ;
	return id ;
}

G::Identity G::Identity::real()
{
	Identity id ;
	id.m_uid = ::getuid() ;
	id.m_gid = ::getgid() ;
	return id ;
}

G::Identity G::Identity::invalid() noexcept
{
	return {} ;
}

G::Identity G::Identity::invalid( SignalSafe safe ) noexcept
{
	return Identity(safe) ;
}

G::Identity G::Identity::root()
{
	Identity id ;
	id.m_uid = 0 ;
	id.m_gid = 0 ;
	return id ;
}

std::string G::Identity::str() const
{
	std::ostringstream ss ;
	ss << m_uid << "/" << m_gid ;
	return ss.str() ;
}

bool G::Identity::isRoot() const
{
	return m_uid == 0 ;
}

bool G::Identity::operator==( const Identity & other ) const
{
	return m_uid == other.m_uid && m_gid == other.m_gid ;
}

bool G::Identity::operator!=( const Identity & other ) const
{
	return ! operator==( other ) ;
}

void G::Identity::setEffectiveUser( SignalSafe ) const noexcept
{
	int rc = ::seteuid(m_uid) ; G_IGNORE_VARIABLE(int,rc) ;
}

void G::Identity::setEffectiveUser( bool do_throw ) const
{
	if( ::seteuid(m_uid) && do_throw ) throw UidError() ;
}

void G::Identity::setRealUser( bool do_throw ) const
{
	if( ::setuid(m_uid) && do_throw ) throw UidError() ;
}

void G::Identity::setEffectiveGroup( bool do_throw ) const
{
	if( ::setegid(m_gid) && do_throw )
	{
		int e = Process::errno_() ;
		std::ostringstream ss ;
		ss << m_gid ;
		throw GidError( ss.str() , Process::strerror(e) ) ;
	}
}

void G::Identity::setEffectiveGroup( SignalSafe ) const noexcept
{
	int rc = ::setegid(m_gid) ; G_IGNORE_VARIABLE(int,rc) ;
}

void G::Identity::setRealGroup( bool do_throw ) const
{
	if( ::setgid(m_gid) && do_throw ) throw GidError() ;
}

// ===

void G::IdentityUser::setRealUserTo( Identity id , bool do_throw )
{
	id.setRealUser( do_throw ) ;
}

void G::IdentityUser::setEffectiveUserTo( Identity id , bool do_throw )
{
	id.setEffectiveUser( do_throw ) ;
}

void G::IdentityUser::setEffectiveUserTo( SignalSafe safe , Identity id ) noexcept
{
	id.setEffectiveUser( safe ) ;
}

void G::IdentityUser::setRealGroupTo( Identity id , bool do_throw )
{
	id.setRealGroup( do_throw ) ;
}

void G::IdentityUser::setEffectiveGroupTo( Identity id , bool do_throw )
{
	id.setEffectiveGroup( do_throw ) ;
}

void G::IdentityUser::setEffectiveGroupTo( SignalSafe safe , Identity id ) noexcept
{
	id.setEffectiveGroup( safe ) ;
}
/// \file gidentity_unix.cpp
