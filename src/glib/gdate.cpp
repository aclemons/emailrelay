//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gdate.cpp
///

#include "gdef.h"
#include "gdate.h"
#include "gstr.h"
#include "glog.h"
#include "gassert.h"
#include <ctime>

int G::Date::yearUpperLimit() noexcept
{
	//return 2035 ; // see mktime()
	return 9999 ;
}

int G::Date::yearLowerLimit() noexcept
{
	return 1970 ; // see mktime()
}

#ifndef G_LIB_SMALL
G::Date::Date()
{
	init( SystemTime::now().utc() ) ;
	//check() ;
}
#endif

#ifndef G_LIB_SMALL
G::Date::Date( SystemTime t )
{
	init( t.utc() ) ;
	//check() ;
}
#endif

#ifndef G_LIB_SMALL
G::Date::Date( SystemTime t , const LocalTime & )
{
	init( t.local() ) ;
	//check() ;
}
#endif

G::Date::Date( const BrokenDownTime & tm )
{
	init( tm ) ;
	//check() ;
}

#ifndef G_LIB_SMALL
G::Date::Date( const LocalTime & )
{
	init( SystemTime::now().local() ) ;
	//check() ;
}
#endif

#ifndef G_LIB_SMALL
G::Date::Date( int year , Date::Month month , int day_of_month , std::nothrow_t ) noexcept :
	m_day(day_of_month) ,
	m_month(static_cast<int>(month)) ,
	m_year(year)
{
	m_day = std::max( 1 , std::min(m_day,31) ) ;
	m_month = std::max( 1 , std::min(m_month,12) ) ;
	m_year = std::max( yearLowerLimit() , std::min(m_year,yearUpperLimit()) ) ;
}
#endif

#ifndef G_LIB_SMALL
G::Date::Date( int year , Date::Month month , int day_of_month ) :
	m_day(day_of_month) ,
	m_month(static_cast<int>(month)) ,
	m_year(year)
{
	check() ;
}
#endif

void G::Date::init( const BrokenDownTime & tm )
{
	m_year = tm.year() ;
	m_month = tm.month() ;
	m_day = tm.day() ;
}

void G::Date::check()
{
	bool ok =
		m_year >= yearLowerLimit() &&
		m_year <= yearUpperLimit() &&
		m_month >= 1 &&
		m_month <= 12 &&
		m_day >= 1 &&
		m_day <= 31 ;
	if( !ok )
		throw DateError( "out of range" ) ;
}

#ifndef G_LIB_SMALL
std::string G::Date::str( Format format ) const
{
	std::ostringstream ss ;
	if( format == Format::yyyy_mm_dd_slash )
	{
		ss << yyyy() << "/" << mm() << "/" << dd() ;
	}
	else if( format == Format::yyyy_mm_dd )
	{
		ss << yyyy() << mm() << dd() ;
	}
	else if( format == Format::mm_dd )
	{
		ss << mm() << dd() ;
	}
	else
	{
		G_ASSERT( !"enum error" ) ;
	}
	return ss.str() ;
}
#endif

int G::Date::monthday() const
{
	return m_day ;
}

std::string G::Date::dd() const
{
	return Str::fromInt(std::min(99,std::max(0,m_day))+100).substr(1U) ;
}

std::string G::Date::mm() const
{
	return Str::fromInt(std::min(99,std::max(0,m_month))+100).substr(1U) ;
}

std::string G::Date::yyyy() const
{
	return Str::fromInt(std::min(9999,std::max(0,m_year))+10000).substr(1U) ;
}

G::Date::Weekday G::Date::weekday() const
{
	if( ! m_weekday_set )
	{
		BrokenDownTime bdt = BrokenDownTime::midday( m_year , m_month , m_day ) ;
		const_cast<Date*>(this)->m_weekday_set = true ;
		const_cast<Date*>(this)->m_weekday = Weekday(bdt.wday()) ;
	}
	return m_weekday ;
}

std::string G::Date::weekdayName( bool brief ) const
{
	Weekday d = weekday() ;
	const char * p = "" ;
	if( d == Weekday::sunday ) p = brief ? "Sun" : "Sunday" ;
	else if( d == Weekday::monday ) p = brief ? "Mon" : "Monday" ;
	else if( d == Weekday::tuesday ) p = brief ? "Tue" : "Tuesday" ;
	else if( d == Weekday::wednesday ) p = brief ? "Wed" : "Wednesday" ;
	else if( d == Weekday::thursday ) p= brief ? "Thu" : "Thursday" ;
	else if( d == Weekday::friday ) p = brief ? "Fri" : "Friday" ;
	else if( d == Weekday::saturday ) p = brief ? "Sat" : "Saturday" ;
	return { p } ;
}

G::Date::Month G::Date::month() const
{
	return Month(m_month) ;
}

std::string G::Date::monthName( bool brief ) const
{
	Month m = month() ;
	const char * p = "" ;
	if( m == Month::january ) p = brief ? "Jan" : "January" ;
	else if( m == Month::february ) p = brief ? "Feb" : "February" ;
	else if( m == Month::march ) p = brief ? "Mar" : "March" ;
	else if( m == Month::april ) p = brief ? "Apr" : "April" ;
	else if( m == Month::may ) p = "May" ;
	else if( m == Month::june ) p = brief ? "Jun" : "June" ;
	else if( m == Month::july ) p = brief ? "Jul" : "July" ;
	else if( m == Month::august ) p = brief ? "Aug" : "August" ;
	else if( m == Month::september ) p = brief ? "Sep" : "September" ;
	else if( m == Month::october ) p = brief ? "Oct" : "October" ;
	else if( m == Month::november ) p = brief ? "Nov" : "November" ;
	else if( m == Month::december ) p = brief ? "Dec" : "December" ;
	return { p } ;
}

int G::Date::year() const
{
	return m_year ;
}

#ifndef G_LIB_SMALL
G::Date G::Date::next() const
{
	Date d( *this ) ;
	++d ;
	return d ;
}
#endif

G::Date & G::Date::operator++()
{
	++m_day ;
	if( m_day == (lastDay(m_month,m_year)+1) )
	{
		m_day = 1 ;
		++m_month ;
		if( m_month == 13 )
		{
			m_month = 1 ;
			++m_year ;
		}
	}
	if( m_weekday_set )
	{
		if( m_weekday == Weekday::saturday )
			m_weekday = Weekday::sunday ;
		else
			m_weekday = static_cast<Weekday>(static_cast<int>(m_weekday)+1) ;
	}
	return *this ;
}

#ifndef G_LIB_SMALL
G::Date G::Date::previous() const
{
	Date d( *this ) ;
	--d ;
	return d ;
}
#endif

G::Date & G::Date::operator--()
{
	if( m_day == 1 )
	{
		if( m_month == 1 )
		{
			m_year-- ;
			m_month = 12 ;
		}
		else
		{
			m_month-- ;
		}

		m_day = lastDay( m_month , m_year ) ;
	}
	else
	{
		m_day-- ;
	}
	if( m_weekday_set )
	{
		if( m_weekday == Weekday::sunday )
			m_weekday = Weekday::saturday ;
		else
			m_weekday = static_cast<Weekday>(static_cast<int>(m_weekday)-1) ;
	}
	return *this ;
}

int G::Date::lastDay( int month , int year )
{
	int end = 30 ;
	if( month == 1 ||
		month == 3 ||
		month == 5 ||
		month == 7 ||
		month == 8 ||
		month == 10 ||
		month == 12 )
	{
		end = 31 ;
	}
	else if( month == 2 )
	{
		end = isLeapYear(year) ? 29 : 28 ;
	}
	return end ;
}

bool G::Date::isLeapYear( int y )
{
	return y >= 1800 && ( y % 400 == 0 || ( y % 100 != 0 && y % 4 == 0 ) ) ;
}

bool G::Date::operator==( const Date &other ) const
{
	return
		year() == other.year() &&
		month() == other.month() &&
		monthday() == other.monthday() ;
}

#ifndef G_LIB_SMALL
bool G::Date::operator!=( const Date &other ) const
{
	return !( other == *this ) ;
}
#endif

