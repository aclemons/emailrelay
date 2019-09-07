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
// gidentity_unix.cpp
//
// Note that gcc/glibc previously required -D_BSD_SOURCE for seteuid().
//

#include "gdef.h"
#include "gidentity.h"
#include "gprocess.h"
#include "gassert.h"
#include <climits>
#include <sstream>
#include <vector>
#include <pwd.h> // getpwnam_r()
#include <unistd.h> // sysconf()

G::Identity::Identity() :
	m_uid(static_cast<uid_t>(-1)) ,
	m_gid(static_cast<gid_t>(-1)) ,
	m_h(0)
{
}

G::Identity::Identity( G::SignalSafe ) :
	m_uid(static_cast<uid_t>(-1)) ,
	m_gid(static_cast<gid_t>(-1)) ,
	m_h(0)
{
}

G::Identity::Identity( const std::string & name ) :
	m_uid(static_cast<uid_t>(-1)) ,
	m_gid(static_cast<gid_t>(-1)) ,
	m_h(0)
{
	typedef struct passwd P ;
    long n = ::sysconf( _SC_GETPW_R_SIZE_MAX ) ;
	if( n < 0 || n > INT_MAX ) n = -1L ;
    int sizes[] = { 120 , static_cast<int>(n) , 16000 , 0 } ;
    for( int * size_p = sizes ; *size_p ; ++size_p )
    {
        if( *size_p < 0 ) continue ;
        size_t buffer_size = static_cast<size_t>(*size_p) ;
        std::vector<char> buffer( buffer_size ) ;
		static P pwd_zero ;
        P pwd = pwd_zero ;
        P * result_p = nullptr ;
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

G::Identity G::Identity::invalid()
{
	return Identity() ;
}

G::Identity G::Identity::invalid( SignalSafe safe )
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

void G::Identity::setEffectiveUser( SignalSafe )
{
	m_h = 0 ; // for -Wunused-private-field
	int rc = ::seteuid(m_uid) ; G_IGNORE_VARIABLE(int,rc) ;
}

void G::Identity::setEffectiveUser( bool do_throw )
{
	if( ::seteuid(m_uid) && do_throw ) throw UidError() ;
}

void G::Identity::setRealUser( bool do_throw )
{
	if( ::setuid(m_uid) && do_throw ) throw UidError() ;
}

void G::Identity::setEffectiveGroup( bool do_throw )
{
	if( ::setegid(m_gid) && do_throw ) throw GidError() ;
}

void G::Identity::setEffectiveGroup( SignalSafe )
{
	int rc = ::setegid(m_gid) ; G_IGNORE_VARIABLE(int,rc) ;
}

void G::Identity::setRealGroup( bool do_throw )
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

void G::IdentityUser::setEffectiveUserTo( SignalSafe safe , Identity id )
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

void G::IdentityUser::setEffectiveGroupTo( SignalSafe safe , Identity id )
{
	id.setEffectiveGroup( safe ) ;
}
/// \file gidentity_unix.cpp
