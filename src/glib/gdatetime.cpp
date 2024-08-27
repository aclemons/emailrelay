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
/// \file gdatetime.cpp
///

#include "gdef.h"
#include "gdatetime.h"
#include "goptional.h"
#include "gstr.h"
#include "gassert.h"
#include <sstream>
#include <iomanip>
#include <utility>
#include <vector>
#include <type_traits>

namespace G
{
	namespace DateTimeImp
	{
		static constexpr const char * good_format = "%ntYyCGgmUWVjdwuHIMSDFRT" ;
		static constexpr unsigned int million = 1000000U ;

		template <typename Tp> TimeInterval interval( Tp start , Tp end )
		{
			using namespace std::chrono ;
			if( end <= start )
				return TimeInterval::zero() ;
			auto d = end - start ;

			auto s = (duration_cast<seconds>(d)).count() ; G_ASSERT( s >= 0 ) ;
			typename std::make_unsigned<decltype(s)>::type su = s ;
			if( sizeof(su) > sizeof(TimeInterval::s_type) &&
				su > std::numeric_limits<TimeInterval::s_type>::max() )
					return TimeInterval::limit() ;

			auto us = (duration_cast<microseconds>(d) % seconds(1)).count() ; G_ASSERT( us >= 0 ) ;
			typename std::make_unsigned<decltype(us)>::type usu = us ;
			G_ASSERT( us <= std::numeric_limits<TimeInterval::us_type>::max() ) ;

			return TimeInterval( static_cast<TimeInterval::s_type>(su) , static_cast<TimeInterval::us_type>(usu) ) ;
		}
		bool operator<( const std::tm & a , const std::tm & b ) noexcept
		{
			if( a.tm_year < b.tm_year ) return true ;
			if( a.tm_year > b.tm_year ) return false ;
			if( a.tm_mon < b.tm_mon ) return true ;
			if( a.tm_mon > b.tm_mon ) return false ;
			if( a.tm_mday < b.tm_mday ) return true ;
			if( a.tm_mday > b.tm_mday ) return false ;
			if( a.tm_hour < b.tm_hour ) return true ;
			if( a.tm_hour > b.tm_hour ) return false ;
			if( a.tm_min < b.tm_min ) return true ;
			if( a.tm_min > b.tm_min ) return false ;
			return a.tm_sec < b.tm_sec ;
		}
		bool sameMinute( const std::tm & a , const std::tm & b ) noexcept
		{
			return
				a.tm_year == b.tm_year &&
				a.tm_mon == b.tm_mon &&
				a.tm_mday == b.tm_mday &&
				a.tm_hour == b.tm_hour &&
				a.tm_min == b.tm_min ;
		}
		bool sameSecond( const std::tm & a , const std::tm & b ) noexcept
		{
			return sameMinute( a , b ) && a.tm_sec == b.tm_sec ;
		}
		void localtime_( std::tm & tm_out , std::time_t t_in )
		{
			if( localtime_r( &t_in , &tm_out ) == nullptr )
				throw DateTime::Error() ;
			tm_out.tm_isdst = -1 ;
		}
		void gmtime_( std::tm & tm_out , std::time_t t_in )
		{
			if( gmtime_r( &t_in , &tm_out ) == nullptr )
				throw DateTime::Error() ;
			tm_out.tm_isdst = -1 ;
		}
		std::time_t mktime_( std::tm & tm )
		{
			tm.tm_isdst = -1 ;
			std::time_t t = std::mktime( &tm ) ;
			if( t == std::time_t(-1) )
				throw DateTime::Error() ;
			return t ;
		}
		std::time_t mktimelocal( const std::tm & local_tm_in )
		{
			struct std::tm tm = local_tm_in ;
			return mktime_( tm ) ;
		}
		std::time_t mktimeutc( const std::tm & utc_tm_in , std::time_t begin , std::time_t end )
		{
			// returns 't' such that std::gmtime(t) gives the target broken-down time -- does
			// a binary search over the time_t range down to one second resolution
			std::time_t count = end - begin ;
			std::time_t t = begin ;
			while( count > 0 )
			{
				std::time_t i = t ;
				std::time_t step = count / 2 ;
				i += step ;
				std::tm tm {} ;
				gmtime_( tm , i ) ;
				if( tm < utc_tm_in )
				{
					t = ++i ;
					count -= step + 1 ;
				}
				else
				{
					count = step ;
				}
			}
			return t ;
		}
	}
}

G::BrokenDownTime::BrokenDownTime() :
	m_tm{}
{
	m_tm.tm_isdst = -1 ;
}

#ifndef G_LIB_SMALL
G::BrokenDownTime::BrokenDownTime( const struct std::tm & tm_in ) :
	m_tm(tm_in)
{
	// dont trust the dst flag passed in -- force mktime()
	// to do the extra work (strftime() does anyway)
	m_tm.tm_isdst = -1 ;
}
#endif

G::BrokenDownTime::BrokenDownTime( int y , int mon , int d , int h , int min , int s ) :
	m_tm{}
{
	m_tm.tm_year = y - 1900 ;
	m_tm.tm_mon = mon - 1 ;
	m_tm.tm_mday = d ;
	m_tm.tm_hour = h ;
	m_tm.tm_min = min ;
	m_tm.tm_sec = s ;
	m_tm.tm_isdst = -1 ;
	m_tm.tm_wday = 0 ;
	m_tm.tm_yday = 0 ;
}

#ifndef G_LIB_SMALL
std::time_t G::BrokenDownTime::epochTimeFromLocal() const
{
	return DateTimeImp::mktimelocal( m_tm ) ;
}
#endif

std::time_t G::BrokenDownTime::epochTimeFromUtc() const
{
	std::time_t t0 = DateTimeImp::mktimelocal( m_tm ) ;

	static std::optional<std::time_t> memo ;
	if( memo.has_value() )
	{
		std::tm tm {} ;
		DateTimeImp::gmtime_( tm , t0+memo.value() ) ;
		if( DateTimeImp::sameSecond(tm,m_tm) )
			return t0 + memo.value() ;
	}

	std::time_t dt = 25 * 3600 + 10 ;
	std::time_t begin = std::max(dt,t0) - dt ;
	std::time_t end = t0 + dt ;
	std::time_t t = DateTimeImp::mktimeutc( m_tm , begin , end ) ;
	if( t == begin || t == end )
		throw DateTime::Error( "timezone error" ) ;

	memo = t - t0 ;
	return t ;
}

#ifndef G_LIB_SMALL
G::BrokenDownTime G::BrokenDownTime::null()
{
	return {} ;
}
#endif

G::BrokenDownTime G::BrokenDownTime::local( SystemTime t )
{
	BrokenDownTime bdt ;
	DateTimeImp::localtime_( bdt.m_tm , t.s() ) ;
	return bdt ;
}

G::BrokenDownTime G::BrokenDownTime::utc( SystemTime t )
{
	BrokenDownTime bdt ;
	DateTimeImp::gmtime_( bdt.m_tm , t.s() ) ;
	return bdt ;
}

G::BrokenDownTime G::BrokenDownTime::midday( int year , int month , int day )
{
	return { year , month , day , 12 , 0 , 0 } ;
}

#ifndef G_LIB_SMALL
G::BrokenDownTime G::BrokenDownTime::midnight( int year , int month , int day )
{
	return { year , month , day , 0 , 0 , 0 } ;
}
#endif

bool G::BrokenDownTime::format( char * out , std::size_t out_size , const char * fmt ) const
{
	for( const char * p = std::strchr(fmt,'%') ; p && p[1] ; p = std::strchr(p+1,'%') )
	{
		if( std::strchr(DateTimeImp::good_format,p[1]) == nullptr )
			throw DateTime::Error( "bad format string" ) ;
	}

	std::tm tm_copy = m_tm ;
	DateTimeImp::mktime_( tm_copy ) ; // fill in isdst, wday, yday

	return std::strftime( out , out_size , fmt , &tm_copy ) > 0U ;
}

void G::BrokenDownTime::format( std::vector<char> & out , const char * fmt ) const
{
	if( !format( out.data() , out.size() , fmt ) )
		throw DateTime::Error() ;
}

#ifndef G_LIB_SMALL
std::string G::BrokenDownTime::str() const
{
	return str( "%F %T" ) ;
}
#endif

std::string G::BrokenDownTime::str( const char * fmt ) const
{
	std::size_t n = std::strlen( fmt ) + 1U ;
	for( const char * p = std::strchr(fmt,'%') ; p && p[1] ; p = std::strchr(p+1,'%') )
		n += 10U ; // biggest allowed format is eg. %F -> "2001-12-31"

	std::vector<char> buffer( n ) ;
	format( buffer , fmt ) ;
	buffer.at(buffer.size()-1U) = '\0' ; // just in case
	return { buffer.data() } ;
}

int G::BrokenDownTime::hour() const
{
	return m_tm.tm_hour ;
}

int G::BrokenDownTime::min() const
{
	return m_tm.tm_min ;
}

int G::BrokenDownTime::sec() const
{
	return m_tm.tm_sec ;
}

int G::BrokenDownTime::year() const
{
	return m_tm.tm_year + 1900 ;
}

int G::BrokenDownTime::month() const
{
	return m_tm.tm_mon + 1 ;
}

int G::BrokenDownTime::day() const
{
	return m_tm.tm_mday ;
}

int G::BrokenDownTime::wday() const
{
	std::tm tm_copy = m_tm ;
	DateTimeImp::mktime_( tm_copy ) ;
	return tm_copy.tm_wday ;
}

#ifndef G_LIB_SMALL
bool G::BrokenDownTime::sameMinute( const BrokenDownTime & other ) const noexcept
{
	return DateTimeImp::sameMinute( m_tm , other.m_tm ) ;
}
#endif

// ==

G::SystemTime::SystemTime( time_point_type tp ) :
	m_tp(tp)
{
}

G::SystemTime::SystemTime( std::time_t t , unsigned long us ) noexcept
{
	m_tp = std::chrono::system_clock::from_time_t(t) ;
	m_tp += std::chrono::microseconds( us ) ;
}

G::SystemTime G::SystemTime::now()
{
	return SystemTime( std::chrono::system_clock::now() ) ;
}

G::TimeInterval G::SystemTime::operator-( const SystemTime & start ) const
{
	return start.interval( *this ) ;
}

G::TimeInterval G::SystemTime::interval( const SystemTime & end ) const
{
	return DateTimeImp::interval( m_tp , end.m_tp ) ;
}

#ifndef G_LIB_SMALL
G::SystemTime & G::SystemTime::add( unsigned long us )
{
	m_tp += std::chrono::microseconds( us ) ;
	return *this ;
}
#endif

bool G::SystemTime::sameSecond( const SystemTime & t ) const noexcept
{
	return s() == t.s() ;
}

G::BrokenDownTime G::SystemTime::local() const
{
	return BrokenDownTime::local( *this ) ;
}

G::BrokenDownTime G::SystemTime::utc() const
{
	return BrokenDownTime::utc( *this ) ;
}

#ifndef G_LIB_SMALL
unsigned int G::SystemTime::ms() const
{
	using namespace std::chrono ;
	return static_cast<unsigned int>((duration_cast<milliseconds>(m_tp.time_since_epoch()) % seconds(1)).count()) ;
}
#endif

unsigned int G::SystemTime::us() const
{
	using namespace std::chrono ;
	return static_cast<unsigned int>((duration_cast<microseconds>(m_tp.time_since_epoch()) % seconds(1)).count()) ;
}

std::time_t G::SystemTime::s() const noexcept
{
	using namespace std::chrono ;
	G_ASSERT( duration_cast<seconds>(m_tp.time_since_epoch()).count() == system_clock::to_time_t(m_tp) ) ; // as per c++17
	return system_clock::to_time_t( m_tp ) ;
}

#ifndef G_LIB_SMALL
G::SystemTime G::SystemTime::zero()
{
	duration_type zero{0} ;
	G_ASSERT( SystemTime(time_point_type(zero)).s() == 0 )  ; // assert 1970 epoch as per c++17
	return SystemTime( time_point_type(zero) ) ;
}
#endif

#ifndef G_LIB_SMALL
bool G::SystemTime::isZero() const
{
	return m_tp == time_point_type( duration_type(0) ) ;
}
#endif

bool G::SystemTime::operator<( const SystemTime & other ) const
{
	return m_tp < other.m_tp ;
}

#ifndef G_LIB_SMALL
bool G::SystemTime::operator<=( const SystemTime & other ) const
{
	return m_tp <= other.m_tp ;
}
#endif

bool G::SystemTime::operator==( const SystemTime & other ) const
{
	return m_tp == other.m_tp ;
}

bool G::SystemTime::operator!=( const SystemTime & other ) const
{
	return !( *this == other ) ;
}

#ifndef G_LIB_SMALL
bool G::SystemTime::operator>( const SystemTime & other ) const
{
	return m_tp > other.m_tp ;
}
#endif

#ifndef G_LIB_SMALL
bool G::SystemTime::operator>=( const SystemTime & other ) const
{
	return m_tp >= other.m_tp ;
}
#endif

#ifndef G_LIB_SMALL
G::SystemTime G::SystemTime::operator+( TimeInterval interval ) const
{
	SystemTime t( *this ) ;
	t += interval ;
	return t ;
}
#endif

void G::SystemTime::operator+=( TimeInterval i )
{
	using namespace std::chrono ;
	m_tp += seconds(i.s()) ;
	m_tp += microseconds(i.us()) ;
}

void G::SystemTime::streamOut( std::ostream & stream ) const
{
	int w = static_cast<int>( stream.width() ) ;
	char c = stream.fill() ;
	stream
		<< s() << "."
		<< std::setw(6) << std::setfill('0')
		<< us()
		<< std::setw(w) << std::setfill(c) ;
}

std::ostream & G::operator<<( std::ostream & stream , const SystemTime & t )
{
	t.streamOut( stream ) ;
	return stream ;
}

// ==

G::TimerTime::TimerTime( time_point_type tp ) :
	m_tp(tp)
{
}

G::TimerTime G::TimerTime::now()
{
	time_point_type tp = std::chrono::steady_clock::now() ;
	if( tp == time_point_type() ) tp += duration_type(1) ;
	return TimerTime( tp ) ;
}

G::TimerTime G::TimerTime::zero()
{
	return TimerTime( time_point_type( duration_type(0) ) ) ;
}

bool G::TimerTime::isZero() const noexcept
{
	return m_tp == time_point_type( duration_type(0) ) ;
}

#ifndef G_LIB_SMALL
G::TimerTime G::TimerTime::test( int s , int us )
{
	using namespace std::chrono ;
	return TimerTime( time_point_type( seconds(s) + microseconds(us) ) ) ;
}
#endif

unsigned long G::TimerTime::s() const
{
	using namespace std::chrono ;
	return static_cast<unsigned long>( duration_cast<seconds>(m_tp.time_since_epoch()).count() ) ;
}

unsigned long G::TimerTime::us() const
{
	using namespace std::chrono ;
	return static_cast<unsigned long>( (duration_cast<microseconds>(m_tp.time_since_epoch()) % seconds(1)).count() ) ;
}

#ifndef G_LIB_SMALL
std::string G::TimerTime::str() const
{
	std::ostringstream ss ;
	ss << s() << '.' << std::setw(6) << std::setfill('0') << us() ;
	return ss.str() ;
}
#endif

G::TimerTime G::TimerTime::operator+( const TimeInterval & interval ) const
{
	TimerTime t( *this ) ;
	t += interval ;
	return t ;
}

void G::TimerTime::operator+=( TimeInterval i )
{
	using namespace std::chrono ;
	m_tp += seconds(i.s()) ;
	m_tp += microseconds(i.us()) ;
}

#ifndef G_LIB_SMALL
G::TimeInterval G::TimerTime::operator-( const TimerTime & start ) const
{
	return start.interval( *this ) ;
}
#endif

G::TimeInterval G::TimerTime::interval( const TimerTime & end ) const
{
	return DateTimeImp::interval( m_tp , end.m_tp ) ;
}

#ifndef G_LIB_SMALL
bool G::TimerTime::sameSecond( const TimerTime & t ) const
{
	using namespace std::chrono ;
	return
		duration_cast<seconds>(m_tp.time_since_epoch()) ==
		duration_cast<seconds>(t.m_tp.time_since_epoch()) ;
}
#endif

bool G::TimerTime::operator<=( const TimerTime & other ) const
{
	return m_tp <= other.m_tp ;
}

bool G::TimerTime::operator==( const TimerTime & other ) const
{
	return m_tp == other.m_tp ;
}

#ifndef G_LIB_SMALL
bool G::TimerTime::operator!=( const TimerTime & other ) const
{
	return m_tp != other.m_tp ;
}
#endif

#ifndef G_LIB_SMALL
bool G::TimerTime::operator>( const TimerTime & other ) const
{
	return m_tp > other.m_tp ;
}
#endif

#ifndef G_LIB_SMALL
bool G::TimerTime::operator>=( const TimerTime & other ) const
{
	return m_tp >= other.m_tp ;
}
#endif

// ==

G::TimeInterval::TimeInterval( s_type s , us_type us ) :
	m_s(s) ,
	m_us(us)
{
	normalise() ;
}

#ifndef G_LIB_SMALL
G::TimeInterval::TimeInterval( const SystemTime & start , const SystemTime & end ) :
	m_s(0) ,
	m_us(0)
{
	TimeInterval i = start.interval( end ) ;
	m_s = i.m_s ;
	m_us = i.m_us ;
	normalise() ;
}
#endif

G::TimeInterval::TimeInterval( const TimerTime & start , const TimerTime & end ) :
	m_s(0) ,
	m_us(0)
{
	TimeInterval i = start.interval( end ) ;
	m_s = i.m_s ;
	m_us = i.m_us ;
	normalise() ;
}

void G::TimeInterval::normalise()
{
	using namespace G::DateTimeImp ;
	if( m_us >= million )
	{
		m_us -= million ;
		increase( m_s ) ;
		if( m_us >= million ) // still
		{
			increase( m_s , m_us / million ) ;
			m_us = m_us % million ;
		}
	}
}

G::TimeInterval G::TimeInterval::limit()
{
	using namespace G::DateTimeImp ;
	return TimeInterval( std::numeric_limits<s_type>::max() , million-1U ) ;
}

G::TimeInterval G::TimeInterval::zero()
{
	return TimeInterval( 0UL , 0U ) ;
}

G::TimeInterval::s_type G::TimeInterval::s() const
{
	return m_s ;
}

G::TimeInterval::us_type G::TimeInterval::us() const
{
	return m_us ;
}

bool G::TimeInterval::operator==( const TimeInterval & other ) const
{
	return m_s == other.m_s && m_us == other.m_us ;
}

#ifndef G_LIB_SMALL
bool G::TimeInterval::operator!=( const TimeInterval & other ) const
{
	return !( *this == other ) ;
}
#endif

bool G::TimeInterval::operator<( const TimeInterval & other ) const
{
	return m_s < other.m_s || ( m_s == other.m_s && m_us < other.m_us ) ;
}

#ifndef G_LIB_SMALL
bool G::TimeInterval::operator<=( const TimeInterval & other ) const
{
	return *this == other || *this < other ;
}
#endif

bool G::TimeInterval::operator>( const TimeInterval & other ) const
{
	return m_s > other.m_s || ( m_s == other.m_s && m_us > other.m_us ) ;
}

#ifndef G_LIB_SMALL
bool G::TimeInterval::operator>=( const TimeInterval & other ) const
{
	return *this == other || *this > other ;
}
#endif

#ifndef G_LIB_SMALL
G::TimeInterval G::TimeInterval::operator+( const TimeInterval & other ) const
{
	TimeInterval t( *this ) ;
	t += other ;
	return t ;
}
#endif

#ifndef G_LIB_SMALL
G::TimeInterval G::TimeInterval::operator-( const TimeInterval & other ) const
{
	TimeInterval t( *this ) ;
	t -= other ;
	return t ;
}
#endif

void G::TimeInterval::increase( unsigned int & s , unsigned int ds )
{
	const auto old = s ;
	s += ds ;
	const bool overflow = s < old ;
	if( overflow )
		throw DateTime::Error( "overflow" ) ;
}

void G::TimeInterval::operator+=( TimeInterval i )
{
	using namespace G::DateTimeImp ;
	m_us += i.m_us ;
	if( m_us >= million )
	{
		m_us -= million ;
		increase( m_s ) ;
	}
	increase( m_s , i.m_s ) ;
}

void G::TimeInterval::decrease( unsigned int & s , unsigned int ds )
{
	if( s < ds )
		throw DateTime::Error( "underflow" ) ;
	s -= ds ;
}

void G::TimeInterval::operator-=( TimeInterval i )
{
	using namespace G::DateTimeImp ;
	if( m_us < i.m_us )
	{
		decrease( m_s ) ;
		m_us += million ;
	}
	m_us -= i.m_us ;
	decrease( m_s , i.m_s ) ;
}

void G::TimeInterval::streamOut( std::ostream & stream ) const
{
	int w = static_cast<int>( stream.width() ) ;
	char c = stream.fill() ;
	stream
		<< s() << "."
		<< std::setw(6) << std::setfill('0')
		<< us()
		<< std::setw(w) << std::setfill(c) ;
}

std::ostream & G::operator<<( std::ostream & stream , const TimeInterval & ti )
{
	ti.streamOut( stream ) ;
	return stream ;
}

// ==

G::DateTime::Offset G::DateTime::offset( SystemTime t_in )
{
	G_ASSERT( !(t_in == SystemTime::zero()) ) ;
	SystemTime t_zone( BrokenDownTime::local(t_in).epochTimeFromUtc() ) ;
	bool ahead = t_in < t_zone ; // ie. east-of
	TimeInterval i = ahead ? (t_zone-t_in) : (t_in-t_zone) ;
	return Offset{ ahead , i.s() } ;
}

#ifndef G_LIB_SMALL
std::string G::DateTime::offsetString( int tz )
{
	std::ostringstream ss ;
	ss << ( tz < 0 ? "-" : "+" ) ;
	if( tz < 0 ) tz = -tz ;
	ss << (tz/10) << (tz%10) << "00" ;
	return ss.str() ;
}
#endif

std::string G::DateTime::offsetString( Offset offset )
{
	unsigned int hh = (offset.second+30U) / 3600U ;
	unsigned int mm = ((offset.second+30U) / 60U) % 60 ;

	std::ostringstream ss ;
	char sign = (offset.first || (hh==0&&mm==0)) ? '+' : '-' ;
	ss << sign << (hh/10U) << (hh%10U) << (mm/10) << (mm%10) ;
	return ss.str() ;
}

