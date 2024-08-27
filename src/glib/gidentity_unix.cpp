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
/// \file gidentity_unix.cpp
///

#include "gdef.h"
#include "gidentity.h"
#include "gprocess.h"
#include "grange.h"
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
		bool lookupUser( std::string & name , uid_t & uid , gid_t & gid ) ;
		bool lookupGroup( const std::string & group , gid_t & gid ) ;
		int sysconf_( int key ) ;
	}
}

G::Identity::Identity( uid_t uid , gid_t gid ) :
	m_uid(uid) ,
	m_gid(gid)
{
}

G::Identity::Identity() noexcept : // invalid()
	m_uid(static_cast<uid_t>(-1)) ,
	m_gid(static_cast<gid_t>(-1))
{
}

G::Identity::Identity( SignalSafe ) noexcept : // invalid()
	Identity()
{
}

G::Identity::Identity( const std::string & name_in , const std::string & group ) :
	Identity()
{
	std::string name = name_in ;
	if( !IdentityImp::lookupUser( name , m_uid , m_gid ) )
		throw NoSuchUser( name_in ) ;

	if( !group.empty() && !IdentityImp::lookupGroup( group , m_gid ) )
		throw NoSuchGroup( group ) ;
}

G::Identity G::Identity::effective() noexcept
{
	return { ::geteuid() , ::getegid() } ;
}

G::Identity G::Identity::real() noexcept
{
	return { ::getuid() , ::getgid() } ;
}

G::Identity G::Identity::invalid() noexcept
{
	return {} ;
}

#ifndef G_LIB_SMALL
G::Identity G::Identity::invalid( SignalSafe safe ) noexcept
{
	return Identity( safe ) ;
}
#endif

G::Identity G::Identity::root() noexcept
{
	return { 0 , 0 } ;
}

#ifndef G_LIB_SMALL
std::string G::Identity::str() const
{
	std::ostringstream ss ;
	ss << static_cast<int>(m_uid) << "/" << static_cast<int>(m_gid) ;
	return ss.str() ;
}
#endif

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

#ifndef G_LIB_SMALL
std::pair<G::Identity,std::string> G::Identity::lookup( std::string_view name_in )
{
	Identity result ;
	std::string name = sv_to_string( name_in ) ;
	if( !IdentityImp::lookupUser( name , result.m_uid , result.m_gid ) )
		throw NoSuchUser( name_in ) ;
	return std::make_pair( result , name ) ;
}
#endif

std::pair<G::Identity,std::string> G::Identity::lookup( std::string_view name_in , std::nothrow_t )
{
	Identity result ;
	std::string name = sv_to_string( name_in ) ;
	IdentityImp::lookupUser( name , result.m_uid , result.m_gid ) ;
	return std::make_pair( result , name ) ;
}

gid_t G::Identity::lookupGroup( const std::string & group )
{
	gid_t result = static_cast<gid_t>(-1) ;
	IdentityImp::lookupGroup( group , result ) ;
	return result ;
}

bool G::Identity::match( std::pair<int,int> uid_range ) const
{
	return G::Range::within( uid_range , static_cast<int>(m_uid) ) ;
}

// ==

bool G::IdentityImp::lookupUser( std::string & name , uid_t & uid , gid_t & gid )
{
	using passwd_t = struct passwd ;
	std::array<int,3U> sizes {{ 120 , 0 , 16000 }} ;
	sizes[1] = IdentityImp::sysconf_( _SC_GETPW_R_SIZE_MAX ) ;
	for( std::size_t i = 0U ; i < sizes.size() ; i++ )
	{
		int size = sizes[i] ;
		if( size <= 0 ) continue ;
		auto buffer_size = static_cast<std::size_t>(size) ;
		std::vector<char> buffer( buffer_size ) ;
		passwd_t pwd {} ;
		passwd_t * result_p = nullptr ;
		int rc = ::getpwnam_r( name.c_str() , &pwd , buffer.data() , buffer_size , &result_p ) ;
		int e = Process::errno_() ;
		if( rc != 0 && e == ERANGE && (i+1U) < sizes.size() )
		{
			continue ; // try again with with bigger buffer
		}
		else if( rc == 0 && result_p )
		{
			name = result_p->pw_name ;
			uid = result_p->pw_uid ;
			gid = result_p->pw_gid ;
			return true ;
		}
		else if( rc == 0 && name == "root" ) // in case of no /etc/passwd file
		{
			uid = 0 ;
			gid = 0 ;
			return true ;
		}
		else if( rc == 0 )
		{
			break ; // no such user
		}
		else
		{
			throw Identity::Error( Process::strerror(e) ) ;
		}
	}
	return false ;
}

bool G::IdentityImp::lookupGroup( const std::string & group , gid_t & gid )
{
	using group_t = struct group ;
	std::array<int,3U> sizes {{ 120 , 0 , 16000 }} ;
	sizes[1] = IdentityImp::sysconf_( _SC_GETGR_R_SIZE_MAX ) ;
	for( auto size : sizes )
	{
		if( size <= 0 ) continue ;
		auto buffer_size = static_cast<std::size_t>(size) ;
		std::vector<char> buffer( buffer_size ) ;
		group_t grp {} ;
		group_t * result_p = nullptr ;
		int rc = ::getgrnam_r( group.c_str() , &grp , buffer.data() , buffer_size , &result_p ) ;
		if( rc == 0 && result_p )
		{
			gid = result_p->gr_gid ;
			return true ;
		}
		else if( rc == 0 )
		{
			return false ;
		}
	}
	return false ;
}

int G::IdentityImp::sysconf_( int key )
{
	long n = ::sysconf( key ) ;
	return ( n < 0 || n > INT_MAX ) ? -1 : static_cast<int>(n) ;
}

