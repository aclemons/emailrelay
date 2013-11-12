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
// gtime.cpp
//

#include "gdef.h"
#include "gtime.h"
#include "gdatetime.h"
#include "gassert.h"

G::Time::Time( const G::DateTime::BrokenDownTime & tm )
{
	m_hh = tm.tm_hour ;
	m_mm = tm.tm_min ;
	m_ss = tm.tm_sec ;
}

G::Time::Time()
{
	G::DateTime::BrokenDownTime tm = G::DateTime::utc( G::DateTime::now() ) ;
	m_hh = tm.tm_hour ;
	m_mm = tm.tm_min ;
	m_ss = tm.tm_sec ;
}

G::Time::Time( G::DateTime::EpochTime t )
{
	G::DateTime::BrokenDownTime tm = G::DateTime::utc( t ) ;
	m_hh = tm.tm_hour ;
	m_mm = tm.tm_min ;
	m_ss = tm.tm_sec ;
}

G::Time::Time( const LocalTime & )
{
	G::DateTime::BrokenDownTime tm = G::DateTime::local( G::DateTime::now() ) ;
	m_hh = tm.tm_hour ;
	m_mm = tm.tm_min ;
	m_ss = tm.tm_sec ;
}

G::Time::Time( G::DateTime::EpochTime t , const LocalTime & )
{
	G::DateTime::BrokenDownTime tm = G::DateTime::local( t ) ;
	m_hh = tm.tm_hour ;
	m_mm = tm.tm_min ;
	m_ss = tm.tm_sec ;
}

int G::Time::hours() const
{
	return m_hh ;
}

int G::Time::minutes() const
{
	return m_mm ;
}

int G::Time::seconds() const
{
	return m_ss ;
}

std::string G::Time::hhmmss( const char * sep ) const
{
	if( sep == NULL ) sep = "" ;
	std::ostringstream ss ;
	ss << (m_hh/10) << (m_hh%10) << sep << (m_mm/10) << (m_mm%10) << sep << (m_ss/10) << (m_ss%10) ;
	return ss.str() ;
}

std::string G::Time::hhmm( const char * sep ) const
{
	if( sep == NULL ) sep = "" ;
	std::ostringstream ss ;
	ss << (m_hh/10) << (m_hh%10) << sep << (m_mm/10) << (m_mm%10) ;
	return ss.str() ;
}

std::string G::Time::ss() const
{
	std::ostringstream ss ;
	ss << (m_ss/10) << (m_ss%10) ;
	return ss.str() ;
}


/// \file gtime.cpp
