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
/// \file gdate.cpp
///

#include "gdef.h"
#include "gdate.h"
#include "glog.h"
#include "gassert.h"
#include <ctime>
#include <iomanip>
#include <sstream>

int G::Date::yearUpperLimit()
{
	return 2035 ; // see mktime()
}

int G::Date::yearLowerLimit()
{
	return 1970 ; // see mktime()
}

G::Date::Date()
{
	init( SystemTime::now().utc() ) ;
}

G::Date::Date( SystemTime t )
{
	init( t.utc() ) ;
}

G::Date::Date( SystemTime t , const LocalTime & )
{
	init( t.local() ) ;
}

G::Date::Date( const BrokenDownTime & tm )
{
	init( tm ) ;
}

G::Date::Date( const LocalTime & )
{
	init( SystemTime::now().local() ) ;
}

G::Date::Date( int year , Date::Month month , int day_of_month ) :
	m_day(day_of_month) ,
	m_month(static_cast<int>(month)) ,
	m_year(year)
{
	G_ASSERT( year >= yearLowerLimit() ) ;
	G_ASSERT( year <= yearUpperLimit() ) ;
	G_ASSERT( day_of_month > 0 ) ;
	G_ASSERT( day_of_month < 32 ) ;
	G_ASSERT( static_cast<int>(month) >= 1 ) ;
	G_ASSERT( static_cast<int>(month) <= 12 ) ;
}

void G::Date::init( const BrokenDownTime & tm )
{
	m_year = tm.year() ;
	m_month = tm.month() ;
	m_day = tm.day() ;
}

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

int G::Date::monthday() const
{
	return m_day ;
}

std::string G::Date::dd() const
{
	std::ostringstream ss ;
	ss << std::setw(2) << std::setfill('0') << m_day ;
	return ss.str() ;
}

std::string G::Date::mm() const
{
	std::ostringstream ss ;
	ss << std::setw(2) << std::setfill('0') << m_month ;
	return ss.str() ;
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
	if( weekday() == Weekday::sunday ) return brief ? "Sun" : "Sunday" ;
	if( weekday() == Weekday::monday ) return brief ? "Mon" : "Monday" ;
	if( weekday() == Weekday::tuesday ) return brief ? "Tue" : "Tuesday" ;
	if( weekday() == Weekday::wednesday ) return brief ? "Wed" : "Wednesday" ;
	if( weekday() == Weekday::thursday ) return brief ? "Thu" : "Thursday" ;
	if( weekday() == Weekday::friday ) return brief ? "Fri" : "Friday" ;
	if( weekday() == Weekday::saturday ) return brief ? "Sat" : "Saturday" ;
	return "" ;
}

G::Date::Month G::Date::month() const
{
	return Month(m_month) ;
}

std::string G::Date::monthName( bool brief ) const
{
	if( month() == Month::january ) return brief ? "Jan" : "January" ;
	if( month() == Month::february ) return brief ? "Feb" : "February" ;
	if( month() == Month::march ) return brief ? "Mar" : "March" ;
	if( month() == Month::april ) return brief ? "Apr" : "April" ;
	if( month() == Month::may ) return "May" ;
	if( month() == Month::june ) return brief ? "Jun" : "June" ;
	if( month() == Month::july ) return brief ? "Jul" : "July" ;
	if( month() == Month::august ) return brief ? "Aug" : "August" ;
	if( month() == Month::september ) return brief ? "Sep" : "September" ;
	if( month() == Month::october ) return brief ? "Oct" : "October" ;
	if( month() == Month::november ) return brief ? "Nov" : "November" ;
	if( month() == Month::december ) return brief ? "Dec" : "December" ;
	return "" ;
}

int G::Date::year() const
{
	return m_year ;
}

std::string G::Date::yyyy() const
{
	std::ostringstream ss ;
	ss << std::setw(4) << std::setfill('0') << m_year ;
	return ss.str() ;
}

G::Date G::Date::next() const
{
	Date d( *this ) ;
	++d ;
	return d ;
}

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

G::Date G::Date::previous() const
{
	Date d( *this ) ;
	--d ;
	return d ;
}

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

bool G::Date::operator!=( const Date &other ) const
{
	return !( other == *this ) ;
}

