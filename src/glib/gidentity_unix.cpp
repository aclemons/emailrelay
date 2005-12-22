//
// Copyright (C) 2001-2005 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// gidentity_unix.cpp
//
// Note that gcc requires -D_BSD_SOURCE for seteuid().
//

#include "gdef.h"
#include "gidentity.h"
#include <sstream>
#include <pwd.h> // getpwnam()

G::Identity::Identity() :
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
	::passwd * pw = ::getpwnam( name.c_str() ) ;
	if( pw == NULL ) throw NoSuchUser(name) ;
	m_uid = pw->pw_uid ;
	m_gid = pw->pw_gid ;
}

G::Identity G::Identity::effective()
{
	G::Identity id ;
	id.m_uid = ::geteuid() ;
	id.m_gid = ::getegid() ;
	return id ;
}

G::Identity G::Identity::real()
{
	G::Identity id ;
	id.m_uid = ::getuid() ;
	id.m_gid = ::getgid() ;
	return id ;
}

G::Identity G::Identity::invalid()
{
	return G::Identity() ;
}

G::Identity G::Identity::root() 
{
	G::Identity id ;
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

void G::IdentityUser::setRealGroupTo( Identity id , bool do_throw )
{
	id.setRealGroup( do_throw ) ;
}

void G::IdentityUser::setEffectiveGroupTo( Identity id , bool do_throw )
{
	id.setEffectiveGroup( do_throw ) ;
}

