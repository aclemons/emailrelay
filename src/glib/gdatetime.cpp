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
/// \file gdatetime.cpp
///

#include "gdef.h"
#include "gdatetime.h"
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
	}
}

G::BrokenDownTime::BrokenDownTime() :
	m_tm{}
{
	m_tm.tm_isdst = -1 ;
}

G::BrokenDownTime::BrokenDownTime( const struct std::tm & tm_in ) :
	m_tm(tm_in)
{
	// dont trust the dst flag passed in -- force mktime()
	// to do the extra work (strftime() does anyway)
	m_tm.tm_isdst = -1 ;
}

std::time_t G::BrokenDownTime::epochTimeFromLocal() const
{
	struct std::tm tm = m_tm ;
	tm.tm_isdst = -1 ;
	std::time_t t = std::mktime( &tm ) ;
	if( t == std::time_t(-1) )
		throw DateTime::Error() ;
	return t ;
}

std::time_t G::BrokenDownTime::epochTimeFromUtc() const
{
	BrokenDownTime copy( *this ) ;
	copy.m_tm.tm_sec = 0 ; // simplify for leap-seconds, add back below
	return copy.epochTimeFromUtcImp() + m_tm.tm_sec ;
}

std::time_t G::BrokenDownTime::epochTimeFromUtcImp() const
{
	static bool diff_set = false ;
	static std::time_t diff = 0 ;
	return epochTimeFromUtcImp( diff_set , diff ) ;
}

std::time_t G::BrokenDownTime::epochTimeFromUtcImp( bool & diff_set , std::time_t & diff ) const
{
	constexpr int day = 24 * 60 * 60 ;
	constexpr std::time_t dt = std::time_t(60) * 15 ; // india 30mins, nepal 45mins
	constexpr std::time_t day_and_a_bit = day + dt ;
	constexpr std::time_t t_rounding = 30 ;

	// use mktime() for a rough starting point
	std::time_t t_base = epochTimeFromLocal() ;

	// see if the previous diff result is still valid (no change of dst etc),
	if( diff_set && !sameMinute( SystemTime(t_base+diff+t_rounding).utc() ) )
		diff_set = false ;

	// find the timezone diff
	if( !diff_set )
	{
		// iterate over all possible timezones modifying the epoch time until
		// its utc broken-down-time (gmtime()) matches ours
		//
		auto t = t_base - day_and_a_bit ;
		auto end = t_base + day_and_a_bit ;
		for( diff = -day_and_a_bit ; t <= end ; t += dt , diff += dt )
		{
			if( sameMinute( SystemTime(t+t_rounding).utc() ) )
			{
				diff_set = true ;
				break ;
			}
		}
	}

	if( !diff_set )
		throw DateTime::Error( "unsupported timezone" ) ;

	return t_base + diff ;
}

G::BrokenDownTime G::BrokenDownTime::null()
{
	return {} ;
}

G::BrokenDownTime G::BrokenDownTime::local( SystemTime t )
{
	BrokenDownTime bdt ;
	std::time_t s = t.s() ;
	if( localtime_r( &s , &bdt.m_tm ) == nullptr )
		throw DateTime::Error() ;
	bdt.m_tm.tm_isdst = -1 ;
	return bdt ;
}

G::BrokenDownTime G::BrokenDownTime::utc( SystemTime t )
{
	BrokenDownTime bdt ;
	std::time_t s = t.s() ;
	if( gmtime_r( &s , &bdt.m_tm ) == nullptr )
		throw DateTime::Error() ;
	bdt.m_tm.tm_isdst = -1 ;
	return bdt ;
}

G::BrokenDownTime::BrokenDownTime( int y , int mon , int d , int h , int min , int sec ) :
	m_tm{}
{
	m_tm.tm_year = y - 1900 ;
	m_tm.tm_mon = mon - 1 ;
	m_tm.tm_mday = d ;
	m_tm.tm_hour = h ;
	m_tm.tm_min = min ;
	m_tm.tm_sec = sec ;
	m_tm.tm_isdst = -1 ;
	m_tm.tm_wday = 0 ;
	m_tm.tm_yday = 0 ;
}

G::BrokenDownTime G::BrokenDownTime::midday( int year , int month , int day )
{
	return { year , month , day , 12 , 0 , 0 } ;
}

G::BrokenDownTime G::BrokenDownTime::midnight( int year , int month , int day )
{
	return { year , month , day , 0 , 0 , 0 } ;
}

void G::BrokenDownTime::format( std::vector<char> & out , const char * fmt ) const
{
	for( const char * p = std::strchr(fmt,'%') ; p && p[1] ; p = std::strchr(p+1,'%') )
	{
		if( std::strchr(DateTimeImp::good_format,p[1]) == nullptr )
			throw DateTime::Error("bad format string") ;
	}

	std::tm tm_copy = m_tm ;
	tm_copy.tm_isdst = -1 ;
	(void) mktime( &tm_copy ) ; // fill in isdst, wday, yday

	if( std::strftime( &out[0] , out.size() , fmt , &tm_copy ) == 0U )
		throw DateTime::Error() ;
}

std::string G::BrokenDownTime::str() const
{
	return str( "%F %T" ) ;
}

std::string G::BrokenDownTime::str( const char * fmt ) const
{
	std::size_t n = std::strlen( fmt ) ;
	for( const char * p = std::strchr(fmt,'%') ; p && p[1] ; p = std::strchr(p+1,'%') )
		n += 10U ; // biggest allowed format is eg. %F -> "2001-12-31"

	std::vector<char> buffer( n ) ;
	format( buffer , fmt ) ;
	buffer.at(buffer.size()-1U) = '\0' ; // just in case
	return { &buffer[0] } ;
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
	return m_tm.tm_wday ;
}

bool G::BrokenDownTime::sameMinute( const BrokenDownTime & other ) const
{
	return
		year() == other.year() &&
		month() == other.month() &&
		day() == other.day() &&
		hour() == other.hour() &&
		min() == other.min() ;
}

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

G::SystemTime & G::SystemTime::add( unsigned long us )
{
	m_tp += std::chrono::microseconds( us ) ;
	return *this ;
}

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

unsigned int G::SystemTime::ms() const
{
	using namespace std::chrono ;
	return static_cast<unsigned int>((duration_cast<milliseconds>(m_tp.time_since_epoch()) % seconds(1)).count()) ;
}

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

G::SystemTime G::SystemTime::zero()
{
	duration_type zero{0} ;
	G_ASSERT( SystemTime(time_point_type(zero)).s() == 0 )  ; // assert 1970 epoch as per c++17
	return SystemTime( time_point_type(zero) ) ;
}

bool G::SystemTime::operator<( const SystemTime & other ) const
{
	return m_tp < other.m_tp ;
}

bool G::SystemTime::operator<=( const SystemTime & other ) const
{
	return m_tp <= other.m_tp ;
}

bool G::SystemTime::operator==( const SystemTime & other ) const
{
	return m_tp == other.m_tp ;
}

bool G::SystemTime::operator!=( const SystemTime & other ) const
{
	return !( *this == other ) ;
}

bool G::SystemTime::operator>( const SystemTime & other ) const
{
	return m_tp > other.m_tp ;
}

bool G::SystemTime::operator>=( const SystemTime & other ) const
{
	return m_tp >= other.m_tp ;
}

G::SystemTime G::SystemTime::operator+( TimeInterval interval ) const
{
	SystemTime t( *this ) ;
	t += interval ;
	return t ;
}

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

G::TimerTime G::TimerTime::now()
{
	return { std::chrono::steady_clock::now() , false } ;
}

G::TimerTime G::TimerTime::zero()
{
	duration_type zero_duration{0} ;
	return { time_point_type(zero_duration) , true } ;
}

G::TimerTime::TimerTime( time_point_type tp , bool is_zero ) :
	m_is_zero(is_zero) ,
	m_tp(tp)
{
}

G::TimerTime G::TimerTime::test( int s , int us )
{
	return { time_point_type(std::chrono::seconds(s)+std::chrono::microseconds(us)) , s==0 && us==0 } ;
}

unsigned long G::TimerTime::s() const
{
	using namespace std::chrono ;
	return static_cast<unsigned long>(duration_cast<seconds>(m_tp.time_since_epoch()).count()) ;
}

unsigned long G::TimerTime::us() const
{
	using namespace std::chrono ;
	return static_cast<unsigned long>((duration_cast<microseconds>(m_tp.time_since_epoch()) % seconds(1)).count()) ;
}

std::string G::TimerTime::str() const
{
	std::ostringstream ss ;
	ss << s() << '.' << std::setw(6) << std::setfill('0') << us() ;
	return ss.str() ;
}

G::TimerTime G::TimerTime::operator+( const TimeInterval & interval ) const
{
	TimerTime t( *this ) ;
	t += interval ;
	t.m_is_zero = m_is_zero && interval.s() == 0U && interval.us() == 0U ;
	return t ;
}

void G::TimerTime::operator+=( TimeInterval i )
{
	using namespace std::chrono ;
	m_tp += seconds(i.s()) ;
	m_tp += microseconds(i.us()) ;
	m_is_zero = m_is_zero && i.s() == 0U && i.us() == 0U ;
}

G::TimeInterval G::TimerTime::operator-( const TimerTime & start ) const
{
	return start.interval( *this ) ;
}

G::TimeInterval G::TimerTime::interval( const TimerTime & end ) const
{
	return DateTimeImp::interval( m_tp , end.m_tp ) ;
}

bool G::TimerTime::sameSecond( const TimerTime & t ) const
{
	using namespace std::chrono ;
	return
		duration_cast<seconds>(m_tp.time_since_epoch()) ==
		duration_cast<seconds>(t.m_tp.time_since_epoch()) ;
}

bool G::TimerTime::operator<( const TimerTime & other ) const
{
	return m_tp < other.m_tp ;
}

bool G::TimerTime::operator<=( const TimerTime & other ) const
{
	return m_tp <= other.m_tp ;
}

bool G::TimerTime::operator==( const TimerTime & other ) const
{
	return m_tp == other.m_tp ;
}

bool G::TimerTime::operator!=( const TimerTime & other ) const
{
	return m_tp != other.m_tp ;
}

bool G::TimerTime::operator>( const TimerTime & other ) const
{
	return m_tp > other.m_tp ;
}

bool G::TimerTime::operator>=( const TimerTime & other ) const
{
	return m_tp >= other.m_tp ;
}

// ==

G::TimeInterval::TimeInterval( s_type s , us_type us ) :
	m_s(s) ,
	m_us(us)
{
	normalise() ;
}

G::TimeInterval::TimeInterval( const SystemTime & start , const SystemTime & end ) :
	m_s(0) ,
	m_us(0)
{
	TimeInterval i = start.interval( end ) ;
	m_s = i.m_s ;
	m_us = i.m_us ;
	normalise() ;
}

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

bool G::TimeInterval::operator!=( const TimeInterval & other ) const
{
	return !( *this == other ) ;
}

bool G::TimeInterval::operator<( const TimeInterval & other ) const
{
	return m_s < other.m_s || ( m_s == other.m_s && m_us < other.m_us ) ;
}

bool G::TimeInterval::operator<=( const TimeInterval & other ) const
{
	return *this == other || *this < other ;
}

bool G::TimeInterval::operator>( const TimeInterval & other ) const
{
	return m_s > other.m_s || ( m_s == other.m_s && m_us > other.m_us ) ;
}

bool G::TimeInterval::operator>=( const TimeInterval & other ) const
{
	return *this == other || *this > other ;
}

G::TimeInterval G::TimeInterval::operator+( const TimeInterval & other ) const
{
	TimeInterval t( *this ) ;
	t += other ;
	return t ;
}

G::TimeInterval G::TimeInterval::operator-( const TimeInterval & other ) const
{
	TimeInterval t( *this ) ;
	t -= other ;
	return t ;
}

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
	unsigned int hh = (offset.second+30U) / 3600U ;
	unsigned int mm = ((offset.second+30U) / 60U) % 60 ;

	std::ostringstream ss ;
	char sign = (offset.first || (hh==0&&mm==0)) ? '+' : '-' ;
	ss << sign << (hh/10U) << (hh%10U) << (mm/10) << (mm%10) ;
	return ss.str() ;
}

