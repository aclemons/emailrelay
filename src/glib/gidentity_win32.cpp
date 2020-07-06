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
// gidentity_win32.cpp
//

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

G::Identity G::Identity::effective() noexcept
{
	return Identity() ;
}

G::Identity G::Identity::real() noexcept
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

bool G::Identity::isRoot() const noexcept
{
	return false ;
}

bool G::Identity::operator==( const Identity & other ) const noexcept
{
	return true ;
}

bool G::Identity::operator!=( const Identity & other ) const noexcept
{
	return false ;
}

void G::Identity::setRealUser() const
{
}

bool G::Identity::setRealUser( NoThrow ) const noexcept
{
	return true ;
}

void G::Identity::setEffectiveUser() const
{
}

bool G::Identity::setEffectiveUser( NoThrow ) const noexcept
{
	return true ;
}

bool G::Identity::setEffectiveUser( SignalSafe ) const noexcept
{
	return true ;
}

void G::Identity::setRealGroup() const
{
}

bool G::Identity::setRealGroup( NoThrow ) const noexcept
{
	return true ;
}

void G::Identity::setEffectiveGroup() const
{
}

bool G::Identity::setEffectiveGroup( NoThrow ) const noexcept
{
	return true ;
}

bool G::Identity::setEffectiveGroup( SignalSafe ) const noexcept
{
	return true ;
}

/// \file gidentity_win32.cpp
