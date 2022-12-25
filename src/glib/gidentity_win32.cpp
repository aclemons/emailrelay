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
/// \file gidentity_win32.cpp
///

#include "gdef.h"
#include "gidentity.h"
#include <sstream>

G::Identity::Identity() noexcept :
	m_uid(0) ,
	m_gid(0) ,
	m_h(0)
{
}

G::Identity::Identity( SignalSafe ) noexcept :
	m_uid(0) ,
	m_gid(0) ,
	m_h(0)
{
}

G::Identity::Identity( const std::string & , const std::string & ) :
	m_uid(0) ,
	m_gid(0) ,
	m_h(0)
{
}

std::pair<uid_t,gid_t> G::Identity::lookupUser( const std::string & )
{
	return { 0 , 0 } ;
}

gid_t G::Identity::lookupGroup( const std::string & )
{
	return 0 ;
}

G::Identity G::Identity::effective() noexcept
{
	return Identity() ;
}

G::Identity G::Identity::real( bool ) noexcept
{
	return Identity() ;
}

G::Identity G::Identity::invalid() noexcept
{
	return Identity() ;
}

G::Identity G::Identity::invalid( SignalSafe safe ) noexcept
{
	return Identity(safe) ;
}

G::Identity G::Identity::root() noexcept
{
	return Identity() ;
}

std::string G::Identity::str() const
{
	return "-1/-1" ;
}

uid_t G::Identity::userid() const noexcept
{
	return -1 ;
}

gid_t G::Identity::groupid() const noexcept
{
	return -1 ;
}

bool G::Identity::isRoot() const noexcept
{
	return false ;
}

bool G::Identity::operator==( const Identity & ) const noexcept
{
	return true ;
}

bool G::Identity::operator!=( const Identity & ) const noexcept
{
	return false ;
}

