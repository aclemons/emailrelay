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
	Time(tm.hour(),tm.min(),tm.sec())
{
}

#ifndef G_LIB_SMALL
G::Time::Time() :
	Time(SystemTime::now().utc())
{
}
#endif

#ifndef G_LIB_SMALL
G::Time::Time( SystemTime t ) :
	Time(t.utc())
{
}
#endif

#ifndef G_LIB_SMALL
G::Time::Time( const LocalTime & ) :
	Time(SystemTime::now().local())
{
}
#endif

#ifndef G_LIB_SMALL
G::Time::Time( SystemTime t , const LocalTime & ) :
	Time(t.local())
{
}
#endif

#ifndef G_LIB_SMALL
int G::Time::hours() const
{
	return m_hh ;
}
#endif

#ifndef G_LIB_SMALL
int G::Time::minutes() const
{
	return m_mm ;
}
#endif

#ifndef G_LIB_SMALL
int G::Time::seconds() const
{
	return m_ss ;
}
#endif

std::string G::Time::xx( int n )
{
	return std::string(1U,char('0'+n/10)).append(1U,char('0'+n%10)) ;
}

std::string G::Time::hhmmss( const char * sep ) const
{
	if( sep == nullptr ) sep = "" ;
	return std::string(xx(m_hh)).append(sep).append(xx(m_mm)).append(sep).append(xx(m_ss)) ;
}

#ifndef G_LIB_SMALL
std::string G::Time::hhmm( const char * sep ) const
{
	if( sep == nullptr ) sep = "" ;
	return std::string(xx(m_hh)).append(sep).append(xx(m_mm)) ;
}
#endif

#ifndef G_LIB_SMALL
std::string G::Time::ss() const
{
	return xx( m_ss ) ;
}
#endif

#ifndef G_LIB_SMALL
unsigned int G::Time::value() const
{
	return
		static_cast<unsigned int>(std::max(0,std::min(23,m_hh))) * 3600U +
		static_cast<unsigned int>(std::max(0,std::min(59,m_mm))) * 60U +
		static_cast<unsigned int>(std::max(0,std::min(59,m_ss))) ; // ignore leap seconds here
}
#endif

#ifndef G_LIB_SMALL
G::Time G::Time::at( unsigned int s )
{
	unsigned int hh = s / 3600U ;
	unsigned int mm_ss = s - (hh*3600U) ;
	return {
		std::max(0,std::min(23,static_cast<int>(hh))) ,
		std::max(0,std::min(59,static_cast<int>(mm_ss/60U))) ,
		std::max(0,std::min(59,static_cast<int>(mm_ss%60U))) } ;
}
#endif

bool G::Time::operator==( const Time & other ) const
{
	return m_hh == other.m_hh && m_mm == other.m_mm && m_ss == other.m_ss ;
}

#ifndef G_LIB_SMALL
bool G::Time::operator!=( const Time & other ) const
{
	return !(*this==other) ;
}
#endif

