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
/// \file gdatetime.h
///

#ifndef G_DATE_TIME_H
#define G_DATE_TIME_H

#include "gdef.h"
#include "gexception.h"
#include <chrono>
#include <vector>
#include <string>
#include <iostream>

namespace G
{
	class DateTime ;
	class SystemTime ;
	class TimerTime ;
	class TimeInterval ;
	class BrokenDownTime ;
	class DateTimeTest ;
}

//| \class G::BrokenDownTime
/// An encapsulation of 'struct std::tm'.
///
class G::BrokenDownTime
{
public:
	explicit BrokenDownTime( const struct std::tm & ) ;
		///< Constructor.

	BrokenDownTime( int year , int month , int day , int hh , int mm , int ss ) ;
		///< Constructor.

	static BrokenDownTime null() ;
		///< Factory function for an unusable object with bogus
		///< component values.

	static BrokenDownTime midday( int year , int month , int day ) ;
		///< Factory function for midday on the given date.

	static BrokenDownTime midnight( int year , int month , int day ) ;
		///< Factory function for midnight starting the given date.

	static BrokenDownTime local( SystemTime ) ;
		///< Factory function for the locale-dependent local broken-down
		///< time of the given epoch time. See also SystemTime::local().

	static BrokenDownTime utc( SystemTime ) ;
		///< Factory function for the utc broken-down time of the given
		///< epoch time. See also SystemTime::utc().

	bool format( char * out , std::size_t out_size , const char * fmt ) const ;
		///< Puts the formatted date, including a terminating null character,
		///< into the given output buffer. Returns false if the output buffer
		///< is too small. Only simple non-locale-dependent format specifiers
		///< are allowed, and these allowed specifiers explicitly exclude
		///< '%z' and '%Z'.

	void format( std::vector<char> & out , const char * fmt ) const ;
		///< Overload for an output vector. Throws if the vector is
		///< too small for the result (with its null terminator).

	std::string str( const char * fmt ) const ;
		///< Returns the formatted date, with the same restrictions
		///< as format().

	std::string str() const ;
		///< Returns str() using a "%F %T" format.

	int year() const ;
		///< Returns the four-digit year value.

	int month() const ;
		///< Returns the 1..12 month value.

	int day() const ;
		///< Returns the 1..31 month-day value.

	int hour() const ;
		///< Returns the 0..23 hour value.

	int min() const ;
		///< Returns the 0..59 minute value.

	int sec() const ;
		///< Returns the 0..59 or 0..60 seconds value.

	int wday() const ;
		///< Returns week day where sunday=0 and saturday=6.

	std::time_t epochTimeFromUtc() const ;
		///< Converts this utc broken-down time into epoch time.

	std::time_t epochTimeFromLocal() const ;
		///< Uses std::mktime() to convert this locale-dependent
		///< local broken-down time into epoch time.

	bool sameMinute( const BrokenDownTime & other ) const noexcept ;
		///< Returns true if this and the other broken-down
		///< times are the same, at minute resolution with
		///< no rounding.

private:
	BrokenDownTime() ;

private:
	friend class G::DateTimeTest ;
	struct std::tm m_tm{} ;
} ;

//| \class G::SystemTime
/// Represents a unix-epoch time with microsecond resolution.
///
class G::SystemTime
{
public:
	static SystemTime now() ;
		///< Factory function for the current time.

	static SystemTime zero() ;
		///< Factory function for the start of the epoch.

	explicit SystemTime( std::time_t , unsigned long us = 0UL ) noexcept ;
		///< Constructor. The first parameter should be some
		///< large positive number. The second parameter can be
		///< more than 10^6.

	bool isZero() const ;
		///< Returns true if zero().

	bool sameSecond( const SystemTime & other ) const noexcept ;
		///< Returns true if this time and the other time are the same,
		///< at second resolution.

	BrokenDownTime local() const ;
		///< Returns the locale-dependent local broken-down time.

	BrokenDownTime utc() const ;
		///< Returns the utc broken-down time.

	unsigned int ms() const ;
		///< Returns the millisecond fraction.

	unsigned int us() const ;
		///< Returns the microsecond fraction.

	std::time_t s() const noexcept ;
		///< Returns the number of seconds since the start of the epoch.

	bool operator<( const SystemTime & ) const ;
		///< Comparison operator.

	bool operator<=( const SystemTime & ) const ;
		///< Comparison operator.

	bool operator==( const SystemTime & ) const ;
		///< Comparison operator.

	bool operator!=( const SystemTime & ) const ;
		///< Comparison operator.

	bool operator>( const SystemTime & ) const ;
		///< Comparison operator.

	bool operator>=( const SystemTime & ) const ;
		///< Comparison operator.

	void operator+=( TimeInterval ) ;
		///< Adds the given interval. Throws on overflow.

	SystemTime operator+( TimeInterval ) const ;
		///< Returns this time with given interval added.
		///< Throws on overflow.

	TimeInterval operator-( const SystemTime & start ) const ;
		///< Returns the given start time's interval() compared
		///< to this end time. Returns TimeInterval::zero() on
		///< underflow or TimeInterval::limit() on overflow of
		///< TimeInterval::s_type.

	TimeInterval interval( const SystemTime & end ) const ;
		///< Returns the interval between this time and the given
		///< end time. Returns TimeInterval::zero() on underflow or
		///< TimeInterval::limit() on overflow of TimeInterval::s_type.

	void streamOut( std::ostream & ) const ;
		///< Streams out the time comprised of the s() value, a decimal
		///< point, and then the six-digit us() value.

private:
	friend class G::DateTimeTest ;
	using time_point_type = std::chrono::time_point<std::chrono::system_clock> ;
	using duration_type = time_point_type::duration ;
	explicit SystemTime( time_point_type ) ;
	SystemTime & add( unsigned long us ) ;

private:
	time_point_type m_tp ;
} ;

//| \class G::TimerTime
/// A monotonically increasing subsecond-resolution timestamp, notionally
/// unrelated to time_t.
///
class G::TimerTime
{
public:
	using time_point_type = std::chrono::time_point<std::chrono::steady_clock> ;

	static TimerTime now() ;
		///< Factory function for the current steady-clock time.

	static TimerTime zero() ;
		///< Factory function for the start of the epoch, guaranteed
		///< to be less than any now().

	bool isZero() const noexcept ;
		///< Returns true if zero().

	bool sameSecond( const TimerTime & other ) const ;
		///< Returns true if this time and the other time are the same,
		///< at second resolution.

	static constexpr bool less_noexcept = noexcept(time_point_type() < time_point_type()) ; // NOLINT bogus cert-err58-cpp

	static bool less( const TimerTime & , const TimerTime & ) noexcept(less_noexcept) ;
		///< Comparison operator.

	bool operator<=( const TimerTime & ) const ;
		///< Comparison operator.

	bool operator==( const TimerTime & ) const ;
		///< Comparison operator.

	bool operator!=( const TimerTime & ) const ;
		///< Comparison operator.

	bool operator>( const TimerTime & ) const ;
		///< Comparison operator.

	bool operator>=( const TimerTime & ) const ;
		///< Comparison operator.

	TimerTime operator+( const TimeInterval & ) const ;
		///< Returns this time with given interval added.

	void operator+=( TimeInterval ) ;
		///< Adds an interval.

	TimeInterval operator-( const TimerTime & start ) const ;
		///< Returns the given start time's interval() compared
		///< to this end time. Returns TimeInterval::zero() on
		///< underflow or TimeInterval::limit() if the
		///< TimeInterval::s_type value overflows.

	TimeInterval interval( const TimerTime & end ) const ;
		///< Returns the interval between this time and the given
		///< end time. Returns TimeInterval::zero() on underflow or
		///< TimeInterval::limit() if the TimeInterval::s_type
		///< value overflows.

private:
	friend class G::DateTimeTest ;
	using duration_type = time_point_type::duration ;
	explicit TimerTime( time_point_type ) ;
	static TimerTime test( int , int ) ;
	unsigned long s() const ; // DateTimeTest
	unsigned long us() const ; // DateTimeTest
	std::string str() const ; // DateTimeTest

private:
	time_point_type m_tp ;
} ;

//| \class G::TimeInterval
/// An interval between two G::SystemTime values or two G::TimerTime
/// values.
///
class G::TimeInterval
{
public:
	using s_type = unsigned int ;
	using us_type = unsigned int ;

	explicit TimeInterval( unsigned int s , unsigned int us = 0U ) ;
		///< Constructor.

	TimeInterval( const SystemTime & start , const SystemTime & end ) ;
		///< Constructor. Constructs a zero interval if 'end' is before
		///< 'start', and the limit() interval if 'end' is too far
		///< ahead of 'start' for the underlying type.

	TimeInterval( const TimerTime & start , const TimerTime & end ) ;
		///< Constructor. Overload for TimerTime.

	static TimeInterval zero() ;
		///< Factory function for the zero interval.

	static TimeInterval limit() ;
		///< Factory function for the maximum valid interval.

	unsigned int s() const ;
		///< Returns the number of seconds.

	unsigned int us() const ;
		///< Returns the fractional microseconds part.

	void streamOut( std::ostream & ) const ;
		///< Streams out the interval.

	bool operator<( const TimeInterval & ) const ;
		///< Comparison operator.

	bool operator<=( const TimeInterval & ) const ;
		///< Comparison operator.

	bool operator==( const TimeInterval & ) const ;
		///< Comparison operator.

	bool operator!=( const TimeInterval & ) const ;
		///< Comparison operator.

	bool operator>( const TimeInterval & ) const ;
		///< Comparison operator.

	bool operator>=( const TimeInterval & ) const ;
		///< Comparison operator.

	TimeInterval operator+( const TimeInterval & ) const ;
		///< Returns the combined interval. Throws on overflow.

	TimeInterval operator-( const TimeInterval & ) const ;
		///< Returns the interval difference. Throws on underflow.

	void operator+=( TimeInterval ) ;
		///< Adds the given interval. Throws on overflow.

	void operator-=( TimeInterval ) ;
		///< Subtracts the given interval. Throws on underflow.

private:
	void normalise() ;
	static void increase( unsigned int & s , unsigned int ds = 1U ) ;
	static void decrease( unsigned int & s , unsigned int ds = 1U ) ;

private:
	unsigned int m_s ;
	unsigned int m_us ;
} ;

//| \class G::DateTime
/// A static class that knows about timezone offsets.
///
class G::DateTime
{
public:
	G_EXCEPTION_CLASS( Error , tx("date/time error") ) ;
	using Offset = std::pair<bool,unsigned int> ;

	static Offset offset( SystemTime ) ;
		///< Returns the offset in seconds between UTC and localtime
		///< as at the given system time. The returned pair has 'first'
		///< set to true if localtime is ahead of (ie. east of) UTC.

	static std::string offsetString( Offset offset ) ;
		///< Converts the given utc/localtime offset into a five-character
		///< "+/-hhmm" string.
		///< See also RFC-2822.

	static std::string offsetString( int hh ) ;
		///< Overload for a signed integer timezone.

public:
	DateTime() = delete ;
} ;

namespace G
{
	std::ostream & operator<<( std::ostream & , const SystemTime & ) ;
	std::ostream & operator<<( std::ostream & , const TimeInterval & ) ;
	inline bool operator<( const TimerTime & a , const TimerTime & b ) noexcept(TimerTime::less_noexcept)
	{
		return TimerTime::less( a , b ) ;
	}
}

inline bool G::TimerTime::less( const TimerTime & a , const TimerTime & b ) noexcept(less_noexcept)
{
	return a.m_tp < b.m_tp ;
}

#endif
