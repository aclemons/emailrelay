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
///
/// \file gdatetime.h
///

#ifndef G_DATE_TIME_H
#define G_DATE_TIME_H

#include "gdef.h"
#include "gexception.h"
#include <ctime>
#include <string>
#include <iostream>

namespace G
{
	class DateTime ;
	class EpochTime ;
	class TimeInterval ;
}

/// \class G::EpochTime
/// A subsecond-resolution timestamp based on a time_t.
///
class G::EpochTime
{
public:
	typedef std::time_t seconds_type ;

	explicit EpochTime( std::time_t ) ;
		///< Constructor.

	EpochTime( std::time_t , unsigned long us ) ;
		///< Constructor. The first parameter should be some
		///< large positive number. The second parameter can be
		///< more than 10^6.

	void streamOut( std::ostream & ) const ;
		///< Used by operator<<().

private:
	void normalise() ;

public:
	std::time_t s ;
	unsigned int us ;
} ;

/// \class G::TimeInterval
/// An interval between two G::EpochTime values.
///
class G::TimeInterval
{
public:
	typedef unsigned int seconds_type ;

	TimeInterval( unsigned int s , unsigned int us = 0U ) ;
		///< Constructor.

	TimeInterval( const EpochTime & start , const EpochTime & end ) ;
		///< Constructor. Constructs a zero interval if 'end' is before
		///< 'start', and the limit() interval if 'end' is too far
		///< ahead of 'start' for the underlying type.

	static TimeInterval limit() ;
		///< Returns the maximum valid interval.

	void streamOut( std::ostream & ) const ;
		///< Used by operator<<().

private:
	void normalise() ;

public:
	unsigned int s ;
	unsigned int us ;
} ;

inline
G::TimeInterval::TimeInterval( unsigned int s_ , unsigned int us_ ) :
	s(s_) ,
	us(us_)
{
	if( us > 1000000U )
		normalise() ;
}

namespace G
{
	EpochTime operator+( EpochTime et , std::time_t interval ) ;
	EpochTime operator+( EpochTime base , TimeInterval interval ) ;
	TimeInterval operator-( EpochTime end , EpochTime start ) ;
	TimeInterval operator-( TimeInterval end , TimeInterval start ) ;
	bool operator<( EpochTime lhs , EpochTime rhs ) ;
	bool operator<( TimeInterval lhs , TimeInterval rhs ) ;
	bool operator==( EpochTime lhs , EpochTime rhs ) ;
	bool operator==( TimeInterval lhs , TimeInterval rhs ) ;
	bool operator!=( EpochTime lhs , EpochTime rhs ) ;
	bool operator!=( TimeInterval lhs , TimeInterval rhs ) ;
	bool operator<=( EpochTime lhs , EpochTime rhs ) ;
	bool operator<=( TimeInterval lhs , TimeInterval rhs ) ;
	bool operator>=( EpochTime lhs , EpochTime rhs ) ;
	bool operator>=( TimeInterval lhs , TimeInterval rhs ) ;
	bool operator>( EpochTime lhs , EpochTime rhs ) ;
	bool operator>( TimeInterval lhs , TimeInterval rhs ) ;
	std::ostream & operator<<( std::ostream & s , const EpochTime & et ) ;
	std::ostream & operator<<( std::ostream & s , const TimeInterval & ti ) ;
}

/// \class G::DateTime
/// A low-level static class used by Date and Time.
///
class G::DateTime
{
public:
	G_EXCEPTION( Error , "date/time error" ) ;
	typedef struct std::tm BrokenDownTime ;
	typedef std::pair<bool,unsigned int> Offset ;

	static EpochTime now() ;
		///< Returns the current epoch time.

	static EpochTime epochTime( const BrokenDownTime & broken_down_time , bool optimise = true ) ;
		///< Converts from UTC broken-down-time to epoch time.

	static BrokenDownTime utc( EpochTime epoch_time ) ;
		///< Converts from epoch time to UTC broken-down-time.

	static BrokenDownTime local( EpochTime epoch_time ) ;
		///< Converts from epoch time to local broken-down-time.

	static Offset offset( EpochTime epoch_time , bool optimise = true ) ;
		///< Returns the offset between UTC and localtime as at
		///< 'epoch_time'. The returned pair has 'first' set to
		///< true if localtime is ahead of (ie. east of) UTC.
		///<
		///< (Note that this may be a relatively expensive
		///< operation.)

	static std::string offsetString( Offset offset ) ;
		///< Converts the given utc/localtime offset into a five-character
		///< "+/-hhmm" string.
		///< See also RFC-2822.

	static std::string offsetString( int hh ) ;
		///< Overload for a signed integer timezone.

private:
	DateTime() g__eq_delete ;
	static bool equivalent( EpochTime , const BrokenDownTime & ) ;
	static bool equivalent( const BrokenDownTime & , const BrokenDownTime & ) ;
	static std::tm * gmtime_imp( const std::time_t * , std::tm * ) ;
	static std::tm * localtime_imp( const std::time_t * , std::tm * ) ;
	static EpochTime epochTime( const BrokenDownTime & broken_down_time , bool & , std::time_t & ) ;
} ;

inline
G::EpochTime::EpochTime( std::time_t t ) :
	s(t) ,
	us(0UL)
{
}

inline
G::EpochTime G::operator+( G::EpochTime et , std::time_t interval )
{
	et.s += interval ;
	return et ;
}

inline
G::TimeInterval G::operator-( G::EpochTime end , G::EpochTime start )
{
	return TimeInterval( start , end ) ;
}

inline
bool G::operator<( G::EpochTime lhs , G::EpochTime rhs )
{
	return lhs.s < rhs.s || ( lhs.s == rhs.s && lhs.us < rhs.us ) ;
}

inline
bool G::operator<( G::TimeInterval lhs , G::TimeInterval rhs )
{
	return lhs.s < rhs.s || ( lhs.s == rhs.s && lhs.us < rhs.us ) ;
}

inline
bool G::operator==( G::EpochTime lhs , G::EpochTime rhs )
{
	return lhs.s == rhs.s && lhs.us == rhs.us ;
}

inline
bool G::operator==( G::TimeInterval lhs , G::TimeInterval rhs )
{
	return lhs.s == rhs.s && lhs.us == rhs.us ;
}

inline
bool G::operator!=( G::EpochTime lhs , G::EpochTime rhs )
{
	return !( lhs == rhs ) ;
}

inline
bool G::operator!=( G::TimeInterval lhs , G::TimeInterval rhs )
{
	return !( lhs == rhs ) ;
}

inline
bool G::operator<=( G::EpochTime lhs , G::EpochTime rhs )
{
	return lhs == rhs || lhs < rhs ;
}

inline
bool G::operator<=( G::TimeInterval lhs , G::TimeInterval rhs )
{
	return lhs == rhs || lhs < rhs ;
}

inline
bool G::operator>=( G::EpochTime lhs , G::EpochTime rhs )
{
	return !( lhs < rhs ) ;
}

inline
bool G::operator>=( G::TimeInterval lhs , G::TimeInterval rhs )
{
	return !( lhs < rhs ) ;
}

inline
bool G::operator>( G::EpochTime lhs , G::EpochTime rhs )
{
	return !( lhs == rhs ) && !( lhs < rhs ) ;
}

inline
bool G::operator>( G::TimeInterval lhs , G::TimeInterval rhs )
{
	return !( lhs == rhs ) && !( lhs < rhs ) ;
}

inline
std::ostream & G::operator<<( std::ostream & s , const G::EpochTime & t )
{
	t.streamOut( s ) ;
	return s ;
}

inline
std::ostream & G::operator<<( std::ostream & s , const G::TimeInterval & ti )
{
	ti.streamOut( s ) ;
	return s ;
}

#endif
