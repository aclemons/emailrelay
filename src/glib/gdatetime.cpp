//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gdatetime.cpp
//

#include "gdef.h"
#include "gdatetime.h"
#include "gstr.h"
#include "gassert.h"
#include <sstream>
#include <iomanip>

namespace
{
	const std::time_t minute = 60 ;
	const std::time_t hour = 60 * minute ;
	const std::time_t day = 24 * hour ;
}

G::EpochTime::EpochTime( std::time_t s_ , unsigned long us_ ) :
	s(s_) ,
	us(static_cast<unsigned int>(us_))
{
	if( us_ >= 1000000UL )
	{
		s += static_cast<std::time_t>( us_ / 1000000UL ) ;
		us = static_cast<unsigned int>( us_ % 1000000UL ) ;
	}
}

G::EpochTime G::operator+( G::EpochTime lhs , G::EpochTime rhs )
{
	lhs.s += rhs.s ;
	lhs.us += rhs.us ;
	if( lhs.us > 1000000UL )
	{
		lhs.s++ ;
		lhs.us -= 1000000UL ;
	}
	return lhs ;
}

G::EpochTime G::operator-( G::EpochTime big , G::EpochTime small_ )
{
	big.s -= small_.s ;
	if( small_.us > big.us )
	{
		big.s-- ;
		big.us += 1000000U ;
	}
	big.us -= small_.us ;
	return big ;
}

void G::EpochTime::streamOut( std::ostream & stream ) const
{
	std::streamsize w = stream.width() ;
	char c = stream.fill() ;
	stream
		<< s << "."
		<< std::setw(6) << std::setfill('0')
		<< us
		<< std::setw(w) << std::setfill(c) ;
}

// ==

G::EpochTime G::DateTime::epochTime( const BrokenDownTime & bdt_in )
{
	// get a rough starting point -- mktime() works with localtime,
	// including daylight saving, but there is no UTC equivalent --
	// timegm(3) is deprecated in favour of nasty fiddling with the
	// TZ environment variable
	//
	BrokenDownTime bdt( bdt_in ) ;
	const bool leap_second = bdt.tm_sec == 60 ;
	if( leap_second ) bdt.tm_sec = 59 ;
	EpochTime tlocal( std::mktime(&bdt) , 0U ) ; // localtime

	// optimisation
	static std::time_t diff = 0 ;
	static bool diff_set = false ;
	if( diff_set )
	{
		EpochTime t( tlocal.s + diff ) ;
		if( equivalent( t , bdt_in ) )
			return leap_second ? (t+1) : t ;
		diff_set = false ; // optimisation failure eg. change of dst
	}

	// iterate over all possible timezones
	//
	const std::time_t dt = minute * 30 ;
	const std::time_t day_and_a_bit = day + dt ;
	EpochTime t( tlocal.s - day_and_a_bit ) ;
	const EpochTime end( tlocal.s + day_and_a_bit ) ;
	for( diff = -day_and_a_bit ; t <= end ; t.s += dt , diff += dt )
	{
		if( equivalent( t , bdt_in ) )
		{
			diff_set = true ;
			return leap_second ? (t+1) : t ;
		}
	}
	throw Error() ;
}

G::DateTime::BrokenDownTime G::DateTime::utc( EpochTime epoch_time )
{
	static BrokenDownTime zero ;
	BrokenDownTime result = zero ;
	G::DateTime::gmtime_imp( &epoch_time.s , &result ) ;
	return result ;
}

G::DateTime::BrokenDownTime G::DateTime::local( EpochTime epoch_time )
{
	static BrokenDownTime zero ;
	BrokenDownTime bdt_local = zero ;
	G::DateTime::localtime_imp( &epoch_time.s , &bdt_local ) ;
	return bdt_local ;
}

G::DateTime::Offset G::DateTime::offset( EpochTime utc )
{
	BrokenDownTime bdt_local = local(utc) ;

	EpochTime local = epochTime(bdt_local) ;
	bool ahead = local.s >= utc.s ; // ie. east-of
	EpochTime n( ahead ? (local.s-utc.s) : (utc.s-local.s) ) ;
	return Offset( ahead , static_cast<unsigned int>(n.s) ) ;
}

std::string G::DateTime::offsetString( int tz )
{
	std::ostringstream ss ;
	ss << ( tz < 0 ? "-" : "+" ) ;
	if( tz < 0 ) tz = -tz ;
	ss << (tz/10) << (tz%10) << "00" ;
	return ss.str() ;
}

std::string G::DateTime::offsetString( Offset offset )
{
	unsigned int hh = offset.second / 3600U ;
	unsigned int mm = (offset.second / 60U) % 60 ;

	std::ostringstream ss ;
	char sign = (offset.first || (hh==0&&mm==0)) ? '+' : '-' ;
	ss << sign << (hh/10U) << (hh%10U) << (mm/10) << (mm%10) ;
	return ss.str() ;
}

bool G::DateTime::equivalent( EpochTime t , const BrokenDownTime & bdt_in )
{
	BrokenDownTime bdt_test = utc(t) ;
	return equivalent( bdt_test , bdt_in ) ;
}

bool G::DateTime::equivalent( const BrokenDownTime & bdt1 , const BrokenDownTime & bdt2 )
{
	return
		bdt1.tm_mday == bdt2.tm_mday &&
		bdt1.tm_hour == bdt2.tm_hour &&
		bdt1.tm_min == bdt2.tm_min ;
}

/// \file gdatetime.cpp
