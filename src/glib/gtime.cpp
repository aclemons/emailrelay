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
/// \file gtime.cpp
///

#include "gdef.h"
#include "gtime.h"
#include "gdatetime.h"
#include <sstream>
#include <algorithm>

G::Time::Time( int hh , int mm , int ss ) :
	m_hh(std::min(23,std::max(0,hh))) ,
	m_mm(std::min(59,std::max(0,mm))) ,
	m_ss(std::min((hh==23&&mm==59)?60:59,std::max(0,ss)))
{
}

G::Time::Time( const BrokenDownTime & tm ) :
	m_hh(tm.hour()) ,
	m_mm(tm.min()) ,
	m_ss(tm.sec())
{
}

G::Time::Time() :
	Time(SystemTime::now().utc())
{
}

G::Time::Time( SystemTime t ) :
	Time(t.utc())
{
}

G::Time::Time( const LocalTime & ) :
	Time(SystemTime::now().local())
{
}

G::Time::Time( SystemTime t , const LocalTime & ) :
	Time(t.local())
{
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
	if( sep == nullptr ) sep = "" ;
	std::ostringstream ss ;
	ss << (m_hh/10) << (m_hh%10) << sep << (m_mm/10) << (m_mm%10) << sep << (m_ss/10) << (m_ss%10) ;
	return ss.str() ;
}

std::string G::Time::hhmm( const char * sep ) const
{
	if( sep == nullptr ) sep = "" ;
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

unsigned int G::Time::value() const
{
	return
		static_cast<unsigned int>(std::max(0,std::min(23,m_hh))) * 3600U +
		static_cast<unsigned int>(std::max(0,std::min(59,m_mm))) * 60U +
		static_cast<unsigned int>(std::max(0,std::min(59,m_ss))) ; // ignore leap seconds here
}

G::Time G::Time::at( unsigned int s )
{
	unsigned int hh = s / 3600U ;
	unsigned int mm_ss = s - (hh*3600U) ;
	return {
		std::max(0,std::min(23,static_cast<int>(hh))) ,
		std::max(0,std::min(59,static_cast<int>(mm_ss/60U))) ,
		std::max(0,std::min(59,static_cast<int>(mm_ss%60U))) } ;
}

bool G::Time::operator==( const Time & other ) const
{
	return m_hh == other.m_hh && m_mm == other.m_mm && m_ss == other.m_ss ;
}

bool G::Time::operator!=( const Time & other ) const
{
	return !(*this==other) ;
}

