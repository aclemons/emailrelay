//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gidentity_unix.cpp
///

#include "gdef.h"
#include "gidentity.h"
#include "gprocess.h"
#include "gassert.h"
#include <array>
#include <climits>
#include <vector>
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

G::Identity::Identity( SignalSafe ) noexcept :
	m_uid(static_cast<uid_t>(-1)) ,
	m_gid(static_cast<gid_t>(-1))
{
}

G::Identity::Identity( const std::string & name , const std::string & group ) :
	m_uid(static_cast<uid_t>(-1)) ,
	m_gid(static_cast<gid_t>(-1))
{
	auto pair = lookupUser( name ) ;
	m_uid = pair.first ;
	m_gid = pair.second ;

	if( !group.empty() )
		m_gid = lookupGroup( group ) ;
}

std::pair<uid_t,gid_t> G::Identity::lookupUser( const std::string & name )
{
	using passwd_t = struct passwd ;
	std::pair<uid_t,gid_t> result( 0 , 0 ) ;
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
			result.first = result_p->pw_uid ;
			result.second = result_p->pw_gid ;
			break ;
		}
		else if( rc == 0 && name == "root" ) // in case of no /etc/passwd file
		{
			result.first = 0 ;
			result.second = 0 ;
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
	return result ;
}

gid_t G::Identity::lookupGroup( const std::string & group )
{
	using group_t = struct group ;
	gid_t result = 0 ;
	std::array<int,3U> sizes {{ 120 , 0 , 16000 }} ;
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
			result = result_p->gr_gid ;
			break ;
		}
		else if( rc == 0 )
		{
			throw NoSuchGroup( group ) ;
		}
	}
	return result ;
}

G::Identity G::Identity::effective() noexcept
{
	Identity id ;
	id.m_uid = ::geteuid() ;
	id.m_gid = ::getegid() ;
	id.m_h = 0 ; // pacifies -Wunused-private-field
	return id ;
}

G::Identity G::Identity::real( bool with_cache ) noexcept
{
	static bool first = true ;
	static uid_t u = 0 ;
	static gid_t g = 0 ;
	if( first )
	{
		first = false ;
		u = ::getuid() ;
		g = ::getgid() ;
	}
	Identity id ;
	id.m_uid = (first||with_cache) ? u : ::getuid() ;
	id.m_gid = (first||with_cache) ? g : ::getgid() ;
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

G::Identity G::Identity::root() noexcept
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

uid_t G::Identity::userid() const noexcept
{
	return m_uid ;
}

gid_t G::Identity::groupid() const noexcept
{
	return m_gid ;
}

bool G::Identity::isRoot() const noexcept
{
	return m_uid == 0 ;
}

bool G::Identity::operator==( const Identity & other ) const noexcept
{
	return m_uid == other.m_uid && m_gid == other.m_gid ;
}

bool G::Identity::operator!=( const Identity & other ) const noexcept
{
	return !operator==( other ) ;
}

