//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "genvironment.h"
#include "gstr.h"
#include "glog.h"
#include "gassert.h"
#include <sstream>
#include <iomanip>

namespace
{
	const std::time_t minute = 60 ;
	const std::time_t hour = 60 * minute ;
	const std::time_t day = 24 * hour ;
	const unsigned int million = 1000000U ;
}

G::EpochTime::EpochTime( std::time_t s_ , unsigned long us_ ) :
	s(s_) ,
	us(static_cast<unsigned int>(us_))
{
	if( us_ >= million )
	{
		us_ -= million ;
		s_++ ;
		if( us_ >= million ) // still
		{
			s_ += static_cast<std::time_t>( us_ / million ) ;
			us_ = static_cast<unsigned int>( us_ % million ) ;
		}
		s = s_ ;
		us = static_cast<unsigned int>(us_) ;
	}
}

G::EpochTime G::operator+( EpochTime t , TimeInterval interval )
{
	t.s += interval.s ;
	t.us += interval.us ;
	if( t.us > million ) // carry
	{
		t.s++ ;
		t.us -= million ;
	}
	return t ;
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

G::TimeInterval::TimeInterval( const EpochTime & start , const EpochTime & end ) :
    s(0U) ,
    us(0U)
{
    if( end > start )
    {
        EpochTime t = end ;
		{
			t.s -= start.s ;
			if( t.us < start.us ) // borrow
			{
				G_ASSERT( t.s != 0 ) ;
				t.s-- ;
				t.us += million ;
			}
			G_ASSERT( t.us >= start.us ) ;
			t.us -= start.us ;
		}
        G_ASSERT( t.s >= 0 && t.us < million ) ;
		{
    		typedef std::make_unsigned<std::time_t>::type utime_t ;
        	utime_t ts = static_cast<utime_t>( t.s ) ;

        	us = t.us ;
        	s = static_cast<unsigned int>(ts) ; // possibly narrowing utime_t->uint
        	bool overflow = static_cast<utime_t>(s) != ts ;
        	if( overflow )
			{
            	s = ~0U ;
				us = million-1U ;
			}
		}
    }
}

G::TimeInterval G::TimeInterval::limit()
{
	return TimeInterval( ~0U , million-1U ) ;
}

void G::TimeInterval::normalise()
{
	if( us >= million )
	{
		us -= million ;
		s++ ;
	}
	if( us >= million ) // still
	{
		s += ( us / million ) ;
		us = us % million ;
	}
}

void G::TimeInterval::streamOut( std::ostream & stream ) const
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

G::EpochTime G::DateTime::epochTime( const BrokenDownTime & bdt , bool optimise )
{
	static std::time_t diff = 0 ;
	static bool diff_set = false ;
	std::time_t diff_no = 0 ;
	bool diff_set_no = false ;
	return epochTime( bdt , optimise?diff_set:diff_set_no , optimise?diff:diff_no ) ;
}

G::EpochTime G::DateTime::epochTime( const BrokenDownTime & bdt , bool & diff_set , std::time_t & diff )
{
	// we are given a UTC brokendown time, but there is no UTC
	// equivalent of mktime() -- timegm(3) is deprecated in favour
	// of nasty fiddling with the TZ environment variable

	// use mktime() for a rough starting point
	BrokenDownTime bdt_simple( bdt ) ;
	const bool leap_second = bdt.tm_sec == 60 ;
	if( leap_second ) bdt_simple.tm_sec = 59 ;
	EpochTime tlocal( std::mktime(&bdt_simple) , 0U ) ; // localtime

	// re-check the difference
	if( diff_set && !equivalent( EpochTime(tlocal.s+diff) , bdt ) )
		diff_set = false ; // optimisation failure eg. change of dst

	// find the timezone difference
	if( !diff_set )
	{
		// iterate over all possible timezones modifying the epoch time until
		// its utc broken-down-time (gmtime()) matches the broken-down-time
		// passed in (to some tolerance)
		//
		const std::time_t dt = minute * 15 ; // india 30mins, nepal 45mins
		const std::time_t day_and_a_bit = day + dt ;
		std::time_t di = 0 ;
		EpochTime ti( tlocal.s - day_and_a_bit ) ;
		const EpochTime end( tlocal.s + day_and_a_bit ) ;
		for( di = -day_and_a_bit ; ti <= end ; ti.s += dt , di += dt )
		{
			if( equivalent( ti , bdt ) )
			{
				diff = di ;
				diff_set = true ;
				break ;
			}
		}
	}

	if( !diff_set )
		throw Error( "unsupported timezone" , G::Environment::get("TZ","") ) ;

	return EpochTime( tlocal.s + diff + (leap_second?1:0) ) ;
}

G::DateTime::BrokenDownTime G::DateTime::utc( EpochTime epoch_time )
{
	static BrokenDownTime zero ;
	BrokenDownTime result = zero ;
	DateTime::gmtime_imp( &epoch_time.s , &result ) ;
	return result ;
}

G::DateTime::BrokenDownTime G::DateTime::local( EpochTime epoch_time )
{
	static BrokenDownTime zero ;
	BrokenDownTime bdt_local = zero ;
	DateTime::localtime_imp( &epoch_time.s , &bdt_local ) ;
	return bdt_local ;
}

G::DateTime::Offset G::DateTime::offset( EpochTime utc , bool optimise )
{
	BrokenDownTime bdt_local = local( utc ) ;
	EpochTime local = epochTime( bdt_local , optimise ) ;
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
	BrokenDownTime bdt_test = utc( t ) ;
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
