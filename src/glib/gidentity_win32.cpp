//
// Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gidentity_win32.cpp
//

#include "gdef.h"
#include "gidentity.h"
#include <sstream>

G::Identity::Identity() :
	m_uid(0) ,
	m_gid(0)
{
}

G::Identity::Identity( const std::string & ) :
	m_uid(0) ,
	m_gid(0)
{
}

G::Identity G::Identity::effective()
{
	return G::Identity() ;
}

G::Identity G::Identity::real()
{
	return G::Identity() ;
}

G::Identity G::Identity::invalid()
{
	return G::Identity() ;
}

G::Identity G::Identity::root() 
{
	return G::Identity() ;
}

std::string G::Identity::str() const
{
	return "-1/-1" ;
}

bool G::Identity::isRoot() const
{
	return false ;
}

bool G::Identity::operator==( const Identity & other ) const
{
	return true ;
}

bool G::Identity::operator!=( const Identity & other ) const
{
	return false ;
}

void G::Identity::setEffectiveUser( bool do_throw )
{
}

void G::Identity::setRealUser( bool do_throw )
{
}

void G::Identity::setEffectiveGroup( bool do_throw )
{
}

void G::Identity::setRealGroup( bool do_throw )
{
}

// ===

void G::IdentityUser::setRealUserTo( Identity , bool )
{
}

void G::IdentityUser::setEffectiveUserTo( Identity , bool )
{
}

void G::IdentityUser::setRealGroupTo( Identity , bool )
{
}

void G::IdentityUser::setEffectiveGroupTo( Identity , bool )
{
}

