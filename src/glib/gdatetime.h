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
}

/// \class G::EpochTime
/// A subsecond-resolution timestamp based on a time_t.
///
class G::EpochTime
{
public:
	explicit EpochTime( std::time_t ) ;
		///< Constructor.

	EpochTime( std::time_t , unsigned long us ) ;
		///< Constructor. The 'us' parameter can be more than 10^6.

	void streamOut( std::ostream & ) const ;
		///< Used by operator<<.

public:
	std::time_t s ;
	unsigned int us ;
} ;

namespace G
{
	EpochTime operator+( EpochTime et , std::time_t s ) ;
	EpochTime operator+( EpochTime lhs , EpochTime rhs ) ;
	EpochTime operator-( EpochTime big , EpochTime small_ ) ;
	bool operator<( EpochTime lhs , EpochTime rhs ) ;
	bool operator==( EpochTime lhs , EpochTime rhs ) ;
	bool operator!=( EpochTime lhs , EpochTime rhs ) ;
	bool operator<=( EpochTime lhs , EpochTime rhs ) ;
	bool operator>=( EpochTime lhs , EpochTime rhs ) ;
	bool operator>( EpochTime lhs , EpochTime rhs ) ;
	std::ostream & operator<<( std::ostream & s , const EpochTime & et ) ;
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

	static EpochTime epochTime( const BrokenDownTime & broken_down_time ) ;
		///< Converts from UTC broken-down-time to epoch time.

	static BrokenDownTime utc( EpochTime epoch_time ) ;
		///< Converts from epoch time to UTC broken-down-time.

	static BrokenDownTime local( EpochTime epoch_time ) ;
		///< Converts from epoch time to local broken-down-time.

	static Offset offset( EpochTime epoch_time ) ;
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
	static bool equivalent( EpochTime , const BrokenDownTime & ) ;
	static bool equivalent( const BrokenDownTime & , const BrokenDownTime & ) ;
	static std::tm * gmtime_imp( const std::time_t * , std::tm * ) ;
	static std::tm * localtime_imp( const std::time_t * , std::tm * ) ;
	DateTime() ; // not implemented
} ;

inline
G::EpochTime::EpochTime( std::time_t t ) :
	s(t) ,
	us(0UL)
{
}

inline
G::EpochTime G::operator+( G::EpochTime et , std::time_t s )
{
	et.s += s ;
	return et ;
}

inline
bool G::operator<( G::EpochTime lhs , G::EpochTime rhs )
{
	return lhs.s < rhs.s || (lhs.s == rhs.s && lhs.us < rhs.us ) ;
}

inline
bool G::operator==( G::EpochTime lhs , G::EpochTime rhs )
{
	return lhs.s == rhs.s && lhs.us == rhs.us ;
}

inline
bool G::operator!=( G::EpochTime lhs , G::EpochTime rhs )
{
	return !(lhs == rhs) ;
}

inline
bool G::operator<=( G::EpochTime lhs , G::EpochTime rhs )
{
	return lhs == rhs || lhs < rhs ;
}

inline
bool G::operator>=( G::EpochTime lhs , G::EpochTime rhs )
{
	return !(lhs < rhs) ;
}

inline
bool G::operator>( G::EpochTime lhs , G::EpochTime rhs )
{
	return lhs >= rhs && lhs != rhs ;
}

inline
std::ostream & G::operator<<( std::ostream & s , const G::EpochTime & t )
{
	t.streamOut( s ) ;
	return s ;
}

#endif
