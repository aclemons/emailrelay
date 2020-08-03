//
// Copyright (C) 2001-2020 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gstr.cpp
//

#include "gdef.h"
#include "gstr.h"
#include "glog.h"
#include "gassert.h"
#include <algorithm>
#include <type_traits> // std::make_unsigned
#include <iterator>
#include <limits>
#include <functional>
#include <iomanip>
#include <string>
#include <sstream>
#include <cerrno>

namespace G
{
	namespace StrImp /// An implementation namespace for G::Str.
	{
		void escape( std::string & s , char c_escape , const char * specials_in , const char * specials_out , bool with_nul ) ;
		bool isDigit( char c ) ;
		bool isHex( char c ) ;
		bool isPrintableAscii( char c ) ;
		char toLower( char c )  ;
		char toUpper( char c )  ;
		void readLineFrom( std::istream & stream , const std::string & eol , std::string & line ) ;
		template <typename T> void splitIntoTokens( const std::string & in , T & out , const std::string & ws ) ;
		template <typename T> void splitIntoFields( const std::string & in , T & out , const std::string & ws ) ;
		template <typename T> void splitIntoFields( const std::string & in_in , T & out , const std::string & ws ,
			char escape , bool remove_escapes ) ;
		template <typename Tchar = char , typename Tuchar = typename std::make_unsigned<Tchar>::type> struct PrintableAppender ;
		template <typename T> struct Joiner ;
		template <typename T1, typename T2, typename P> bool equal4( T1 p1 , T1 end1 , T2 p2 , T2 end2 , P p ) ;
		bool ilessc( char c1 , char c2 ) ;
		bool iless( const std::string & a , const std::string & b ) ;
		bool imatchc( char c1 , char c2 ) ;
		bool imatch( const std::string & a , const std::string & b ) ;
		bool match( const std::string & a , const std::string & b , bool ignore_case ) ;
		template <typename T, typename V> T unique( T in , T end , V repeat , V replacement ) ;
		bool inList( StringArray::const_iterator begin , StringArray::const_iterator end , const std::string & s , bool i ) ;
		bool notInList( StringArray::const_iterator begin , StringArray::const_iterator end , const std::string & s , bool i ) ;
		unsigned short toUShort( const std::string & s , bool & overflow , bool & invalid ) ;
		unsigned long toULong( const std::string & s , bool & overflow , bool & invalid ) ;
		unsigned long toULongHex( const std::string & s , bool limited ) ;
		unsigned int toUInt( const std::string & s , bool & overflow , bool & invalid ) ;
		short toShort( const std::string & s , bool & overflow , bool & invalid ) ;
		long toLong( const std::string & s , bool & overflow , bool & invalid ) ;
		int toInt( const std::string & s , bool & overflow , bool & invalid ) ;
		void join( const std::string & , std::string & , const std::string & ) ;
		void strncpy( char * , const char * , size_t ) ;
	}
}

std::string G::Str::escaped( const std::string & s_in )
{
	std::string s( s_in ) ;
	escape( s ) ;
	return s ;
}

std::string G::Str::escaped( const std::string & s_in , char c_escape , const std::string & specials_in , const std::string & specials_out )
{
	std::string s( s_in ) ;
	escape( s , c_escape , specials_in , specials_out ) ;
	return s ;
}

std::string G::Str::escaped( const std::string & s_in , char c_escape , const char * specials_in , const char * specials_out )
{
	std::string s( s_in ) ;
	escape( s , c_escape , specials_in , specials_out ) ;
	return s ;
}

void G::StrImp::escape( std::string & s , char c_escape , const char * specials_in , const char * specials_out , bool with_nul )
{
	G_ASSERT( specials_in != nullptr && specials_out != nullptr && (std::strlen(specials_out)-std::strlen(specials_in)) <= 1U ) ;
	std::size_t pos = 0U ;
	for(;;)
	{
		char c_in = '\0' ;
		pos = s.find_first_of( specials_in , pos ) ;
		if( pos != std::string::npos )
			c_in = s.at( pos ) ;
		else if( with_nul )
			pos = s.find( '\0' , pos ) ;
		if( pos == std::string::npos )
			break ;

		G_ASSERT( std::strchr(specials_in,c_in) != nullptr ) ;
		const std::size_t special_index = std::strchr(specials_in,c_in) - specials_in ;
		G_ASSERT( special_index < std::strlen(specials_out) ) ;

		s.insert( pos , 1U , c_escape ) ;
		pos++ ;
		s.at(pos) = specials_out[special_index] ;
		pos++ ;
	}
}

void G::Str::escape( std::string & s )
{
	namespace imp = G::StrImp ;
	imp::escape( s , '\\' , "\\\r\n\t" , "\\rnt0" , true ) ;
}

void G::Str::escape( std::string & s , char c_escape , const char * specials_in , const char * specials_out )
{
	namespace imp = G::StrImp ;
	bool with_nul = std::strlen(specials_in) != std::strlen(specials_out) ;
	imp::escape( s , c_escape , specials_in , specials_out , with_nul ) ;
}

void G::Str::escape( std::string & s , char c_escape , const std::string & specials_in , const std::string & specials_out )
{
	namespace imp = G::StrImp ;
	G_ASSERT( specials_in.length() == specials_out.length() ) ;
	bool with_nul = !specials_in.empty() && specials_in.at(specials_in.length()-1U) == '\0' ;
	imp::escape( s , c_escape , specials_in.c_str() , specials_out.c_str() , with_nul ) ;
}

std::string G::Str::dequote( const std::string & s , char qq , char esc , const std::string & ws )
{
	std::string result ;
	result.reserve( s.size() ) ;
	bool in_quote = false ;
	bool escaped = false ;
	for( char c : s )
	{
		if( c == esc && !escaped )
		{
			escaped = true ;
		}
		else
		{
			if( c == qq && !escaped && !in_quote )
			{
				in_quote = true ;
			}
			else if( c == qq && !escaped )
			{
				in_quote = false ;
			}
			else if( ws.find(c) != std::string::npos && in_quote )
			{
				result.append( 1U , esc ) ;
				result.append( 1U , c ) ;
			}
			else
			{
				result.append( 1U , c ) ;
			}
			escaped = false ;
		}
	}
	return result ;
}

void G::Str::unescape( std::string & s )
{
	unescape( s , '\\' , "rnt0" , "\r\n\t" ) ;
}

void G::Str::unescape( std::string & s , char c_escape , const char * specials_in , const char * specials_out )
{
	G_ASSERT( specials_in != nullptr && specials_out != nullptr && (std::strlen(specials_in)-std::strlen(specials_out)) <= 1U ) ;
	bool escaped = false ;
	auto out = s.begin() ; // output in-place
	for( char & c_in : s )
	{
		const char * specials_in_p = std::strchr( specials_in , c_in ) ;
		const char * specials_out_p = specials_in_p ? (specials_out+(specials_in_p-specials_in)) : nullptr ;
		if( escaped && specials_out_p )
			*out++ = *specials_out_p , escaped = false ;
		else if( escaped && c_in == c_escape )
			*out++ = c_escape , escaped = false ;
		else if( escaped )
			*out++ = c_in , escaped = false ;
		else if( c_in == c_escape )
			escaped = true ;
		else
			*out++ = c_in , escaped = false ;
	}
	if( out != s.end() ) s.erase( out , s.end() ) ;
}

std::string G::Str::unescaped( const std::string & s_in )
{
	std::string s( s_in ) ;
	unescape( s ) ;
	return s ;
}

bool G::Str::replace( std::string & s , const std::string & from , const std::string & to , std::size_t * pos_p )
{
	if( from.length() == 0 )
		return false ;

	std::size_t pos = pos_p == nullptr ? 0 : *pos_p ;
	if( pos >= s.length() )
		return false ;

	pos = s.find( from , pos ) ;
	if( pos == std::string::npos )
	{
		return false ;
	}
	else
	{
		s.replace( pos , from.length() , to ) ;
		if( pos_p != nullptr )
			*pos_p = pos + to.length() ;
		return true ;
	}
}

unsigned int G::Str::replaceAll( std::string & s , const std::string & from , const std::string & to )
{
	unsigned int count = 0U ;
	for( std::size_t pos = 0U ; replace(s,from,to,&pos) ; count++ )
		{;} // no-op
	return count ;
}

unsigned int G::Str::replaceAll( std::string & s , const char * from , const char * to )
{
	if( s.find(from) != std::string::npos )
	{
		unsigned int count = 0U ;
		for( std::size_t pos = 0U ; replace(s,from,to,&pos) ; count++ )
			{;} // no-op
		return count ;
	}
	else
	{
		return 0U ;
	}
}

std::string G::Str::replaced( const std::string & s , char from , char to )
{
	std::string result( s ) ;
	replaceAll( result , std::string(1U,from) , std::string(1U,to) ) ;
	return result ;
}

void G::Str::removeAll( std::string & s , char c_in )
{
	const auto end = s.end() ;
	s.erase( std::remove_if( s.begin() , end , [c_in](char c){return c==c_in;} ) , end ) ;
}

std::string G::Str::only( const std::string & chars , const std::string & s )
{
	std::string result ;
	result.reserve( s.size() ) ;
	for( char c : s )
	{
		if( chars.find(c) != std::string::npos )
			result.append( 1U , c ) ;
	}
	return result ;
}

void G::Str::trimLeft( std::string & s , const std::string & ws , std::size_t limit )
{
	std::size_t n = s.find_first_not_of( ws ) ;
	if( limit != 0U && ( n == std::string::npos || n > limit ) )
		n = limit >= s.length() ? std::string::npos : limit ;
	if( n == std::string::npos )
		s = std::string() ;
	else if( n != 0U )
		s.erase( 0U , n ) ;
}

void G::Str::trimRight( std::string & s , const std::string & ws , std::size_t limit )
{
	std::size_t n = s.find_last_not_of( ws ) ;
	if( limit != 0U && ( n == std::string::npos || s.length() > (limit+n+1U) ) )
		n = limit >= s.length() ? std::string::npos : (s.length()-limit-1U) ;
	if( n == std::string::npos )
		s = std::string() ;
	else if( (n+1U) != s.length() )
		s.resize( n + 1U ) ;
}

void G::Str::trim( std::string & s , const std::string & ws )
{
	trimLeft(s,ws) ; trimRight(s,ws) ;
}

std::string G::Str::trimmed( const std::string & s_in , const std::string & ws )
{
	std::string s( s_in ) ;
	trim( s , ws ) ;
	return s ;
}

bool G::StrImp::isDigit( char c )
{
	auto uc = static_cast<unsigned char>(c) ;
	return uc >= 48U && uc <= 57U ;
}

bool G::StrImp::isHex( char c )
{
	auto uc = static_cast<unsigned char>(c) ;
	return ( uc >= 48U && uc <= 57U ) || ( uc >= 65U && uc <= 70U ) || ( uc >= 97U && uc <= 102U ) ;
}

bool G::StrImp::isPrintableAscii( char c )
{
	auto uc = static_cast<unsigned char>(c) ;
	return uc >= 32U && uc < 127U ;
}

char G::StrImp::toLower( char c )
{
	const auto uc = static_cast<unsigned char>(c) ;
	return ( uc >= 65U && uc <= 90U ) ? static_cast<char>(c+'\x20') : c ;
}

char G::StrImp::toUpper( char c )
{
	const auto uc = static_cast<unsigned char>(c) ;
	return ( uc >= 97U && uc <= 122U ) ? static_cast<char>(c-'\x20') : c ;
}

bool G::Str::isNumeric( const std::string & s , bool allow_minus_sign )
{
	namespace imp = G::StrImp ;
	const auto end = s.end() ;
	auto p = s.begin() ;
	if( allow_minus_sign && p != end && *p == '-' ) ++p ;
	return std::all_of( p , end , imp::isDigit ) ;
}

bool G::Str::isHex( const std::string & s )
{
	namespace imp = G::StrImp ;
	return std::all_of( s.begin() , s.end() , imp::isHex ) ;
}

bool G::Str::isPrintableAscii( const std::string & s )
{
	namespace imp = G::StrImp ;
	return std::all_of( s.begin() , s.end() , imp::isPrintableAscii ) ;
}

bool G::Str::isInt( const std::string & s )
{
	namespace imp = G::StrImp ;
	bool overflow = false ;
	bool invalid = false ;
	imp::toInt( s , overflow , invalid ) ;
	return !overflow && !invalid ;
}

bool G::Str::isUShort( const std::string & s )
{
	namespace imp = G::StrImp ;
	bool overflow = false ;
	bool invalid = false ;
	imp::toUShort( s , overflow , invalid ) ;
	return !overflow && !invalid ;
}

bool G::Str::isUInt( const std::string & s )
{
	namespace imp = G::StrImp ;
	bool overflow = false ;
	bool invalid = false ;
	imp::toUInt( s , overflow , invalid ) ;
	return !overflow && !invalid ;
}

bool G::Str::isULong( const std::string & s )
{
	namespace imp = G::StrImp ;
	bool overflow = false ;
	bool invalid = false ;
	imp::toULong( s , overflow , invalid ) ;
	return !overflow && !invalid ;
}

std::string G::Str::fromBool( bool b )
{
	return b ? "true" : "false" ;
}

std::string G::Str::fromDouble( double d )
{
	std::ostringstream ss ;
	ss << std::setprecision(16) << d ; // was "setprecision(DBL_DIG+1)"
	return ss.str() ;
}

std::string G::Str::fromInt( int i )
{
	return std::to_string( i ) ;
}

std::string G::Str::fromLong( long l )
{
	return std::to_string( l ) ;
}

std::string G::Str::fromShort( short s )
{
	return std::to_string( s ) ;
}

std::string G::Str::fromUInt( unsigned int ui )
{
	return std::to_string( ui ) ;
}

std::string G::Str::fromULong( unsigned long ul )
{
	return std::to_string( ul ) ;
}

std::string G::Str::fromUShort( unsigned short us )
{
	return std::to_string( us ) ;
}

bool G::Str::toBool( const std::string & s )
{
	std::string str = lower( s ) ;
	bool result = true ;
	if( str == "true" )
		{;}
	else if( str == "false" )
		result = false ;
	else
		throw InvalidFormat( "expected true/false" , s ) ;
	return result ;
}

double G::Str::toDouble( const std::string & s )
{
	try
	{
		std::size_t end = 0U ;
		double result = std::stod( s , &end ) ;
		if( end == 0U || end == s.size() )
			throw InvalidFormat( "expected floating point number" , s ) ;
		return result ;
	}
	catch( std::invalid_argument & )
	{
		throw InvalidFormat( "expected floating point number" , s ) ;
	}
	catch( std::out_of_range & )
	{
	 	throw Overflow( s ) ;
	}
}

int G::Str::toInt( const std::string & s )
{
	namespace imp = G::StrImp ;
	bool overflow = false ;
	bool invalid = false ;
	int result = imp::toInt( s , overflow , invalid ) ;
	if( invalid )
		throw InvalidFormat( "expected integer" , s ) ;
	if( overflow )
		throw Overflow( s ) ;
	return result ;
}

int G::StrImp::toInt( const std::string & s , bool & overflow , bool & invalid )
{
	long long_val = toLong( s , overflow , invalid ) ;
	int result = static_cast<int>( long_val ) ;
	if( result != long_val )
		overflow = true ;
	return result ;
}

long G::Str::toLong( const std::string & s )
{
	namespace imp = G::StrImp ;
	bool overflow = false ;
	bool invalid = false ;
	long result = imp::toLong( s , overflow , invalid ) ;
	if( invalid )
		throw InvalidFormat( "expected long integer" , s ) ;
	if( overflow )
		throw Overflow( s ) ;
	return result ;
}

long G::StrImp::toLong( const std::string & s , bool & overflow , bool & invalid )
{
	try
	{
		std::size_t end = 0U ;
		long result = std::stol( s , &end , 10 ) ;
		if( end == 0U || end != s.size() )
			invalid = true ;
		return result ;
	}
	catch( std::invalid_argument & )
	{
		invalid = true ;
		return 0L ;
	}
	catch( std::out_of_range & )
	{
		overflow = true ;
		return 0L ;
	}
}

short G::Str::toShort( const std::string & s )
{
	namespace imp = G::StrImp ;
	bool overflow = false ;
	bool invalid = false ;
	short result = imp::toShort( s , overflow , invalid ) ;
	if( invalid )
		throw InvalidFormat( "expected short integer" , s ) ;
	if( overflow )
		throw Overflow( s ) ;
	return result ;
}

short G::StrImp::toShort( const std::string & s , bool & overflow , bool & invalid )
{
	long long_val = toLong( s , overflow , invalid ) ;
	auto result = static_cast<short>( long_val ) ;
	if( result != long_val )
		overflow = true ;
	return result ;
}

unsigned int G::Str::toUInt( const std::string & s1 , const std::string & s2 )
{
	return !s1.empty() && isUInt(s1) ? toUInt(s1) : toUInt(s2) ;
}

unsigned int G::Str::toUInt( const std::string & s , Limited )
{
	namespace imp = G::StrImp ;
	bool overflow = false ;
	bool invalid = false ;
	unsigned int result = imp::toUInt( s , overflow , invalid ) ;
	if( invalid )
		throw InvalidFormat( "expected unsigned integer" , s ) ;
	if( overflow )
		result = std::numeric_limits<unsigned int>::max() ;
	return result ;
}

unsigned int G::Str::toUInt( const std::string & s )
{
	namespace imp = G::StrImp ;
	bool overflow = false ;
	bool invalid = false ;
	unsigned int result = imp::toUInt( s , overflow , invalid ) ;
	if( invalid )
		throw InvalidFormat( "expected unsigned integer" , s ) ;
	if( overflow )
		throw Overflow( s ) ;
	return result ;
}

unsigned int G::StrImp::toUInt( const std::string & s , bool & overflow , bool & invalid )
{
	unsigned long ulong_val = toULong( s , overflow , invalid ) ;
	auto result = static_cast<unsigned int>( ulong_val ) ;
	if( result != ulong_val )
		overflow = true ;
	return result ;
}

unsigned long G::Str::toULong( const std::string & s , Limited )
{
	namespace imp = G::StrImp ;
	bool overflow = false ;
	bool invalid = false ;
	unsigned long result = imp::toULong( s , overflow , invalid ) ;
	if( invalid )
		throw InvalidFormat( "expected unsigned long integer" , s ) ;
	if( overflow )
		result = std::numeric_limits<unsigned long>::max() ;
	return result ;
}

unsigned long G::Str::toULong( const std::string & s , Hex , Limited )
{
	return StrImp::toULongHex( s , true ) ;
}

unsigned long G::Str::toULong( const std::string & s , Hex )
{
	return StrImp::toULongHex( s , false ) ;
}

unsigned long G::StrImp::toULongHex( const std::string & s , bool limited )
{
	unsigned long n = 0U ;
	if( s.empty() ) return 0U ;
	size_t i0 = s.find_first_not_of('0') ;
	if( i0 == std::string::npos ) i0 = 0U ;
	if( (s.size()-i0) > (sizeof(unsigned long)*2U) )
	{
		if( limited ) return ~0UL ;
		throw Str::Overflow( s ) ;
	}
	for( std::size_t i = i0 ; i < s.size() ; i++ )
	{
		unsigned int c = static_cast<unsigned char>(s.at(i)) ;
		if( c >= 97U && c <= 102U ) c -= 87U ;
		else if( c >= 65U && c <= 70U ) c -= 55U ;
		else if( c >= 48U && c <= 57U ) c -= 48U ;
		else throw Str::InvalidFormat( "invalid hexadecimal" , s ) ;
		n <<= 4 ;
		n += c ;
	}
	return n ;
}

unsigned long G::Str::toULong( const std::string & s )
{
	namespace imp = G::StrImp ;
	bool overflow = false ;
	bool invalid = false ;
	unsigned long result = imp::toULong( s , overflow , invalid ) ;
	if( invalid )
		throw InvalidFormat( "expected unsigned long integer" , s ) ;
	else if( overflow )
		throw Overflow( s ) ;
	return result ;
}

unsigned long G::Str::toULong( const std::string & s1 , const std::string & s2 )
{
	return !s1.empty() && isULong(s1) ? toULong(s1) : toULong(s2) ;
}

unsigned long G::StrImp::toULong( const std::string & s , bool & overflow , bool & invalid )
{
	try
	{
		if( s.empty() )
		{
			invalid = true ;
			overflow = false ;
			return 0UL ;
		}
		else
		{
			std::size_t end = 0U ;
			unsigned long result = std::stoul( s , &end , 10 ) ;
			if( end == 0U || end != s.size() )
				invalid = true ;
			return result ;
		}
	}
	catch( std::invalid_argument & )
	{
		invalid = true ;
		return 0UL ;
	}
	catch( std::out_of_range & )
	{
		overflow = true ;
		return 0UL ;
	}
}

unsigned short G::Str::toUShort( const std::string & s , Limited )
{
	namespace imp = G::StrImp ;
	bool overflow = false ;
	bool invalid = false ;
	unsigned short result = imp::toUShort( s , overflow , invalid ) ;
	if( invalid )
		throw InvalidFormat( "expected unsigned short integer" , s ) ;
	if( overflow )
		result = std::numeric_limits<unsigned short>::max() ;
	return result ;
}

unsigned short G::Str::toUShort( const std::string & s )
{
	namespace imp = G::StrImp ;
	bool overflow = false ;
	bool invalid = false ;
	unsigned short result = imp::toUShort( s , overflow , invalid ) ;
	if( invalid )
		throw InvalidFormat( "expected unsigned short integer" , s ) ;
	else if( overflow )
		throw Overflow( s ) ;
	return result ;
}

unsigned short G::StrImp::toUShort( const std::string & s , bool & overflow , bool & invalid )
{
	unsigned long ulong_val = toULong( s , overflow , invalid ) ;
	auto result = static_cast<unsigned short>( ulong_val ) ;
	if( result != ulong_val )
		overflow = true ;
	return result ;
}

void G::Str::toLower( std::string & s )
{
	namespace imp = G::StrImp ;
	std::transform( s.begin() , s.end() , s.begin() , imp::toLower ) ;
}

std::string G::Str::lower( const std::string & in )
{
	std::string out = in ;
	toLower( out ) ;
	return out ;
}

void G::Str::toUpper( std::string & s )
{
	namespace imp = G::StrImp ;
	std::transform( s.begin() , s.end() , s.begin() , imp::toUpper ) ;
}

std::string G::Str::upper( const std::string & in )
{
	std::string out = in ;
	toUpper( out ) ;
	return out ;
}

template <typename Tchar, typename Tuchar>
struct G::StrImp::PrintableAppender /// A character appender used by G::Str.
{
	std::string & s ;
	Tchar escape_in ;
	char escape_out ;
	bool eight_bit ;
	PrintableAppender( std::string & s_ , Tchar escape_ , bool eight_bit_ ) :
		s(s_) ,
		escape_in(escape_) ,
		escape_out(static_cast<char>(escape_)) ,
		eight_bit(eight_bit_)
	{
	}
	void operator()( Tchar c )
	{
		static_assert( sizeof(Tchar) == sizeof(Tuchar) , "char size mismatch" ) ;
		const auto uc = static_cast<Tuchar>(c) ; // NOLINT bogus warning
		if( c == escape_in )
		{
			s.append( 2U , escape_out ) ;
		}
		else if( !eight_bit && uc >= 0x20U && uc < 0x7FU && uc != 0xFFU )
		{
			s.append( 1U , static_cast<char>(c) ) ;
		}
		else if( eight_bit && ( ( uc >= 0x20U && uc < 0x7FU ) || uc >= 0xA0 ) && uc != 0xFFU )
		{
			s.append( 1U , static_cast<char>(c) ) ;
		}
		else
		{
			s.append( 1U , escape_out ) ;
			if( static_cast<char>(c) == '\n' )
			{
				s.append( 1U , 'n' ) ;
			}
			else if( static_cast<char>(c) == '\r' )
			{
				s.append( 1U , 'r' ) ;
			}
			else if( static_cast<char>(c) == '\t' )
			{
				s.append( 1U , 't' ) ;
			}
			else if( c == 0 )
			{
				s.append( 1U , '0' ) ;
			}
			else
			{
				s.append( 1U , 'x' ) ;
				const char * const map = "0123456789abcdef" ;
				unsigned long n = uc ;
				if( sizeof(Tchar) == 1 )
				{
					n &= 0xFFUL ;
					s.append( 1U , map[(n/16UL)%16UL] ) ;
					s.append( 1U , map[n%16UL] ) ;
				}
				else
				{
					n &= 0xFFFFUL ;
					s.append( 1U , map[(n/4096UL)%16UL] ) ;
					s.append( 1U , map[(n/256UL)%16UL] ) ;
					s.append( 1U , map[(n/16UL)%16UL] ) ;
					s.append( 1U , map[n%16UL] ) ;
				}
			}
		}
	}
} ;

std::string G::Str::printable( const std::string & in , char escape )
{
	namespace imp = G::StrImp ;
	std::string result ;
	result.reserve( in.length() + 1U ) ;
	std::for_each( in.begin() , in.end() , imp::PrintableAppender<>(result,escape,true) ) ;
	return result ;
}

std::string G::Str::toPrintableAscii( const std::string & in , char escape )
{
	namespace imp = G::StrImp ;
	std::string result ;
	result.reserve( in.length() + 1U ) ;
	std::for_each( in.begin() , in.end() , imp::PrintableAppender<>(result,escape,false) ) ;
	return result ;
}

std::string G::Str::toPrintableAscii( char c , char escape )
{
	namespace imp = G::StrImp ;
	std::string result ;
	result.reserve( 2U ) ;
	imp::PrintableAppender<> append_printable( result , escape , false ) ;
	append_printable( c ) ;
	return result ;
}

std::string G::Str::toPrintableAscii( const std::wstring & in , wchar_t escape )
{
	namespace imp = G::StrImp ;
	std::string result ;
	result.reserve( in.length() * 3U ) ;
	std::for_each( in.begin() , in.end() , imp::PrintableAppender<wchar_t>(result,escape,false) ) ;
	return result ;
}

std::string G::Str::readLineFrom( std::istream & stream , const std::string & eol )
{
	std::string result ;
	readLineFrom( stream , eol.empty() ? std::string(1U,'\n') : eol , result , true ) ;
	return result ;
}

void G::StrImp::readLineFrom( std::istream & stream , const std::string & eol , std::string & line )
{
	const std::size_t limit = line.max_size() ;
	const std::size_t eol_length = eol.length() ;
	const char eol_final = eol.at( eol_length - 1U ) ;
	std::size_t line_length = line.length() ;

	bool changed = false ;
	char c = '\0' ;
	for(;;)
	{
		// (maybe optimise by hoisting the sentry and calling rdbuf() methods)
		stream.get( c ) ;

		if( stream.fail() ) // get(char) always sets the failbit at eof, not necessarily eofbit
		{
			// set eofbit, reset failbit -- cf. std::getline() in <string>
			stream.clear( ( stream.rdstate() & ~std::ios_base::failbit ) | std::ios_base::eofbit ) ;
			break ;
		}

		if( line_length == limit ) // pathological case -- see also std::getline()
		{
			stream.setstate( std::ios_base::failbit ) ;
			break ;
		}

		line.append( 1U , c ) ;
		changed = true ;
		++line_length ;

		if( line_length >= eol_length && c == eol_final )
		{
			const std::size_t offset = line_length - eol_length ;
			if( line.find(eol,offset) == offset )
			{
				line.erase(offset) ;
				break ;
			}
		}
	}
	if( !changed )
		stream.setstate( std::ios_base::failbit ) ;
}

void G::Str::readLineFrom( std::istream & stream , const std::string & eol , std::string & line , bool pre_erase )
{
	namespace imp = G::StrImp ;
	G_ASSERT( eol.length() != 0U ) ;

	if( pre_erase )
		line.erase() ;

	// this is a special speed optimisation for a two-character terminator with a one-character initial string ;-)
	if( eol.length() == 2U && eol[0] != eol[1] && line.length() == 1U )
	{
		// save the initial character, use std::getline() for speed (terminating
		// on the second character of the two-character terminator), check that the
		// one-character terminator was actually part of the required two-character
		// terminator, remove the first character of the two-character terminator,
		// and finally re-insert the initial character
		//
		const char c = line[0] ;
		line.erase() ; // since getline() doesnt erase it if already at eof
		std::getline( stream , line , eol[1] ) ; // fast
		const std::size_t line_length = line.length() ;
		bool complete = line_length > 0U && line[line_length-1U] == eol[0] ;
		if( complete )
		{
			line.resize( line_length - 1U ) ;
			line.insert( 0U , &c , 1U ) ;
		}
		else
		{
			line.insert( 0U , &c , 1U ) ;
			if( stream.good() )
			{
				line.append( 1U , eol[1] ) ;
				imp::readLineFrom( stream , eol , line ) ;
			}
		}
	}
	else
	{
		imp::readLineFrom( stream , eol , line ) ;
	}
}

std::string G::Str::wrap( std::string text , const std::string & prefix_1 ,
	const std::string & prefix_2 , std::size_t width )
{
	std::string ws( " \t\n" ) ;
	std::ostringstream ss ;
	for( bool first_line = true ; text.length() ; first_line = false )
	{
		const std::size_t prefix_length =
			first_line ? prefix_1.length() : prefix_2.length() ;
		std::size_t w = (width > prefix_length) ? (width-prefix_length) : width ;

		const std::size_t pos_nl = text.find_first_of('\n') ;
		if( pos_nl != std::string::npos && pos_nl != 0U && pos_nl < w )
		{
			w = pos_nl ;
		}

		std::string line = text ;
		if( text.length() > w ) // (should use wcwidth() for utf-8 compatibility)
		{
			line = text.substr( 0U , w ) ;
			if( text.find_first_of(ws,w) != w )
			{
				const std::size_t white_space = line.find_last_of( ws ) ;
				const std::size_t black_space = line.find_first_not_of( ws ) ;
				if( white_space != std::string::npos &&
					black_space != std::string::npos &&
					(white_space+1U) != black_space )
				{
					line = line.substr( 0U , white_space ) ;
				}
			}
		}

		if( line.length() != 0U )
		{
			ss << ( first_line ? prefix_1 : prefix_2 ) << line << std::endl ;
		}

		text = text.length() == line.length() ?
			std::string() : text.substr(line.length()) ;

		const std::size_t black_space = text.find_first_not_of( ws ) ;
		if( black_space != 0U && black_space != std::string::npos )
		{
			unsigned int newlines = 0U ;
			for( std::size_t pos = 0U ; pos < black_space ; ++pos )
			{
				if( text.at(pos) == '\n' )
				{
					newlines++ ;
					if( newlines > 1U )
						ss << prefix_2 << std::endl ;
				}
			}

			text = text.substr( black_space ) ;
		}
	}
	return ss.str() ;
}

template <typename T>
void G::StrImp::splitIntoTokens( const std::string & in , T & out , const std::string & ws )
{
	for( std::size_t p = 0U ; p != std::string::npos ; )
	{
		p = in.find_first_not_of( ws , p ) ;
		if( p != std::string::npos )
		{
			std::size_t end = in.find_first_of( ws , p ) ;
			std::size_t len = end == std::string::npos ? end : (end-p) ;
			out.push_back( in.substr(p,len) ) ;
			p = end ;
		}
	}
}

void G::Str::splitIntoTokens( const std::string & in , StringArray & out , const std::string & ws )
{
	namespace imp = G::StrImp ;
	imp::splitIntoTokens( in , out , ws ) ;
}

G::StringArray G::Str::splitIntoTokens( const std::string & in , const std::string & ws )
{
	namespace imp = G::StrImp ;
	StringArray out ;
	imp::splitIntoTokens( in , out , ws ) ;
	return out ;
}

template <typename T>
void G::StrImp::splitIntoFields( const std::string & in , T & out , const std::string & ws )
{
	if( in.length() )
	{
		std::size_t start = 0U ;
		std::size_t pos = 0U ;
		for(;;)
		{
			if( pos >= in.length() ) break ;
			pos = in.find_first_of( ws , pos ) ;
			if( pos == std::string::npos ) break ;
			out.push_back( in.substr(start,pos-start) ) ;
			pos++ ;
			start = pos ;
		}
		out.push_back( in.substr(start,pos-start) ) ;
	}
}

template <typename T>
void G::StrImp::splitIntoFields( const std::string & in_in , T & out , const std::string & ws ,
	char escape , bool remove_escapes )
{
	std::string ews( ws ) ; // escape+whitespace
	if( escape != '\0' ) ews.append( 1U , escape ) ;
	if( in_in.length() )
	{
		std::string in = in_in ;
		std::size_t start = 0U ;
		std::size_t pos = 0U ;
		for(;;)
		{
			if( pos >= in.length() ) break ;
			pos = in.find_first_of( ews , pos ) ;
			if( pos == std::string::npos ) break ;
			if( in.at(pos) == escape )
			{
				if( remove_escapes )
					in.erase( pos , 1U ) ;
				else
					pos++ ;
				pos++ ;
			}
			else
			{
				out.push_back( in.substr(start,pos-start) ) ;
				pos++ ;
				start = pos ;
			}
		}
		out.push_back( in.substr(start,pos-start) ) ;
	}
}

void G::Str::splitIntoFields( const std::string & in , StringArray & out , const std::string & ws ,
	char escape , bool remove_escapes )
{
	namespace imp = G::StrImp ;
	imp::splitIntoFields( in , out , ws , escape , remove_escapes ) ;
}

G::StringArray G::Str::splitIntoFields( const std::string & in , const std::string & ws )
{
	namespace imp = G::StrImp ;
	G::StringArray out ;
	imp::splitIntoFields( in , out , ws ) ;
	return out ;
}

template <typename T>
struct G::StrImp::Joiner /// A sub-string joiner used by G::Str.
{
	T & result ;
	const T & sep ;
	bool & first ;
	Joiner( T & result_ , const T & sep_ , bool & first_ ) :
		result(result_) ,
		sep(sep_) ,
		first(first_)
	{
		first = true ;
	}
	void operator()( const T & s )
	{
		if( !first ) result.append( sep ) ;
		result.append( s ) ;
		first = false ;
	}
} ;

std::string G::Str::join( const std::string & sep , const StringMap & map ,
	const std::string & pre_in , const std::string & post )
{
	namespace imp = G::StrImp ;
	std::string pre = pre_in.empty() ? std::string("=") : pre_in ; // c++14: using namespace std::string_literals
	std::string result ;
	bool first = true ;
	imp::Joiner<std::string> joiner( result , sep , first ) ;
	for( const auto & map_item : map )
		joiner( std::string(map_item.first).append(pre).append(map_item.second).append(post) ) ;
	return result ;
}

std::string G::Str::join( const std::string & sep , const StringArray & strings )
{
	namespace imp = G::StrImp ;
	std::string result ;
	bool first = true ;
	std::for_each( strings.begin() , strings.end() , imp::Joiner<std::string>(result,sep,first) ) ;
	return result ;
}

std::string G::Str::join( const std::string & sep , const std::set<std::string> & strings )
{
	namespace imp = G::StrImp ;
	std::string result ;
	bool first = true ;
	std::for_each( strings.begin() , strings.end() , imp::Joiner<std::string>(result,sep,first) ) ;
	return result ;
}

std::string G::Str::join( const std::string & sep , const std::string & s1 , const std::string & s2 ,
	const std::string & s3 , const std::string & s4 , const std::string & s5 , const std::string & s6 ,
	const std::string & s7 , const std::string & s8 , const std::string & s9 )
{
	namespace imp = G::StrImp ;
	std::string result ;
	imp::join( sep , result , s1 ) ;
	imp::join( sep , result , s2 ) ;
	imp::join( sep , result , s3 ) ;
	imp::join( sep , result , s4 ) ;
	imp::join( sep , result , s5 ) ;
	imp::join( sep , result , s6 ) ;
	imp::join( sep , result , s7 ) ;
	imp::join( sep , result , s8 ) ;
	imp::join( sep , result , s9 ) ;
	return result ;
}

void G::StrImp::join( const std::string & sep , std::string & result , const std::string & s )
{
	if( !result.empty() && !s.empty() )
		result.append( sep ) ;
	result.append( s ) ;
}

std::set<std::string> G::Str::keySet( const StringMap & map )
{
	std::set<std::string> result ;
	std::transform( map.begin() , map.end() , std::inserter(result,result.end()) ,
		[](const StringMap::value_type & pair){return pair.first;} ) ;
	return result ;
}

G::StringArray G::Str::keys( const StringMap & map )
{
	StringArray result ;
	result.reserve( map.size() ) ;
	std::transform( map.begin() , map.end() , std::back_inserter(result) ,
		[](const StringMap::value_type & pair){return pair.first;} ) ;
	return result ;
}

std::string G::Str::ws()
{
	return std::string(" \t\n\r") ;
}

std::string G::Str::alnum()
{
	return std::string("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789") ;
}

std::string G::Str::meta()
{
	return std::string("~<>[]*$|?\\(){}\"`'&;=") ; // bash meta-characters plus "~"
}

std::string G::Str::head( const std::string & in , std::size_t pos , const std::string & default_ )
{
	return
		pos == std::string::npos ?
			default_ :
			( pos == 0U ? std::string() : ( pos >= in.length() ? in : in.substr(0U,pos) ) ) ;
}

std::string G::Str::head( const std::string & in , const std::string & sep , bool default_empty )
{
	std::size_t pos = sep.empty() ? std::string::npos : in.find( sep ) ;
	return head( in , pos , default_empty ? std::string() : in ) ;
}

std::string G::Str::tail( const std::string & in , std::size_t pos , const std::string & default_ )
{
	return
		pos == std::string::npos ?
			default_ :
			( (pos+1U) >= in.length() ? std::string() : in.substr(pos+1U) ) ;
}

std::string G::Str::tail( const std::string & in , const std::string & sep , bool default_empty )
{
	std::size_t pos = sep.empty() ? std::string::npos : in.find(sep) ;
	if( pos != std::string::npos ) pos += (sep.length()-1U) ;
	return tail( in , pos , default_empty ? std::string() : in ) ;
}

bool G::Str::tailMatch( const std::string & in , const std::string & tail )
{
	return
		tail.empty() ||
		( in.length() >= tail.length() && 0 == in.compare(in.length()-tail.length(),tail.length(),tail) ) ;
}

bool G::Str::tailMatch( const StringArray & in , const std::string & tail )
{
	using namespace std::placeholders ;
	bool (*tail_match_fn)(const std::string&,const std::string&) = &tailMatch ;
	return std::any_of( in.begin() , in.end() , std::bind(tail_match_fn,_1,std::cref(tail)) ) ;
}

bool G::Str::headMatch( const std::string & in , const char * head )
{
	if( head == nullptr || *head == '\0' ) return true ;
	std::size_t head_length = std::strlen( head ) ;
	return ( in.length() >= head_length && 0 == in.compare(0U,head_length,head) ) ;
}

bool G::Str::headMatch( const std::string & in , const std::string & head )
{
	return
		head.empty() ||
		( in.length() >= head.length() && 0 == in.compare(0U,head.length(),head) ) ;
}

bool G::Str::headMatch( const StringArray & in , const std::string & head )
{
	using namespace std::placeholders ;
	bool (*head_match_fn)(const std::string&,const std::string&) = &headMatch ;
	return std::any_of( in.begin() , in.end() , std::bind(head_match_fn,_1,std::cref(head)) ) ;
}

std::string G::Str::headMatchResidue( const StringArray & in , const std::string & head )
{
	const auto end = in.end() ;
	for( auto p = in.begin() ; p != end ; ++p )
	{
		if( headMatch( *p , head ) )
			return (*p).substr( head.length() ) ;
	}
	return std::string() ;
}

std::string G::Str::positive()
{
	return "yes" ;
}

std::string G::Str::negative()
{
	return "no" ;
}

bool G::Str::isPositive( const std::string & s_in )
{
	std::string s = trimmed( lower(s_in) , ws() ) ;
	return !s.empty() && ( s == "y" || s == "yes" || s == "t" || s == "true" || s == "1" || s == "on" ) ;
}

bool G::Str::isNegative( const std::string & s_in )
{
	std::string s = trimmed( lower(s_in) , ws() ) ;
	return !s.empty() && ( s == "n" || s == "no" || s == "f" || s == "false" || s == "0" || s == "off" ) ;
}

bool G::Str::match( const std::string & a , const std::string & b )
{
	return a == b ;
}

bool G::Str::match( const StringArray & a , const std::string & b )
{
	return std::find( a.begin() , a.end() , b ) != a.end() ;
}

template <typename T1, typename T2, typename P>
bool G::StrImp::equal4( T1 p1 , T1 end1 , T2 p2 , T2 end2 , P p )
{
	// (std::equal with four iterators is c++14 or later)
	for( ; p1 != end1 && p2 != end2 ; ++p1 , ++p2 )
	{
		if( !p(*p1,*p2) )
			return false ;
	}
	return p1 == end1 && p2 == end2 ;
}

bool G::StrImp::ilessc( char c1 , char c2 )
{
	if( c1 >= 'a' && c1 <= 'z' ) c1 -= '\x20' ;
	if( c2 >= 'a' && c2 <= 'z' ) c2 -= '\x20' ;
	return c1 < c2 ;
}

bool G::StrImp::iless( const std::string & a , const std::string & b )
{
	return std::lexicographical_compare( a.begin() , a.end() , b.begin() , b.end() , StrImp::ilessc ) ;
}

bool G::Str::iless( const std::string & a , const std::string & b )
{
	return StrImp::iless( a , b ) ;
}

bool G::StrImp::imatchc( char c1 , char c2 )
{
	if( c1 >= 'A' && c1 <= 'Z' ) c1 += '\x20' ;
	if( c2 >= 'A' && c2 <= 'Z' ) c2 += '\x20' ;
	return c1 == c2 ;
}

bool G::Str::imatch( char c1 , char c2 )
{
	return StrImp::imatchc( c1 , c2 ) ;
}

bool G::StrImp::imatch( const std::string & a , const std::string & b )
{
	return a.size() == b.size() && equal4( a.begin() , a.end() , b.begin() , b.end() , imatchc ) ;
}

bool G::StrImp::match( const std::string & a , const std::string & b , bool ignore_case )
{
	return ignore_case ? imatch(a,b) : (a==b) ;
}

bool G::Str::imatch( const std::string & a , const std::string & b )
{
	namespace imp = G::StrImp ;
	return imp::imatch( a , b ) ;
}

bool G::Str::imatch( const StringArray & a , const std::string & b )
{
	namespace imp = G::StrImp ;
	using namespace std::placeholders ;
	return std::any_of( a.begin() , a.end() , std::bind(imp::imatch,_1,std::cref(b)) ) ;
}

std::size_t G::Str::ifind( const std::string & s , const std::string & key , std::size_t pos )
{
	namespace imp = G::StrImp ;
	if( s.empty() || key.empty() || pos > s.length() ) return std::string::npos ;
	auto p = std::search( s.begin()+pos , s.end() , key.begin() , key.end() , imp::imatchc ) ;
	return p == s.end() ? std::string::npos : std::distance(s.begin(),p) ;
}

template <typename T, typename V>
T G::StrImp::unique( T in , T end , V repeat , V replacement )
{
	// (maybe use std::adjacent_find())
	T out = in ;
	while( in != end )
	{
		T in_next = in ; ++in_next ;
		if( *in == repeat && *in_next == repeat )
		{
			while( *in == repeat )
				++in ;
			*out++ = replacement ;
		}
		else
		{
			*out++ = *in++ ;
		}
	}
	return out ; // new end
}

std::string G::Str::unique( const std::string & s_in , char c , char r )
{
	namespace imp = G::StrImp ;
	std::string s( s_in ) ;
	s.erase( imp::unique( s.begin() , s.end() , c , r ) , s.end() ) ;
	return s ;
}

std::string G::Str::unique( const std::string & s , char c )
{
	return s.find(c) == std::string::npos ? s : unique( s , c , c ) ;
}

bool G::StrImp::inList( StringArray::const_iterator begin , StringArray::const_iterator end , const std::string & s , bool i )
{
	using namespace std::placeholders ;
	return std::any_of( begin , end , std::bind(match,_1,std::cref(s),i) ) ;
}

bool G::StrImp::notInList( StringArray::const_iterator begin , StringArray::const_iterator end , const std::string & s , bool i )
{
	return !inList( begin , end , s , i ) ;
}

G::StringArray::iterator G::Str::keepMatch( StringArray::iterator begin , StringArray::iterator end ,
	const StringArray & match_list , bool ignore_case )
{
	namespace imp = G::StrImp ;
	using namespace std::placeholders ;
	if( match_list.empty() ) return end ;
	return std::remove_if( begin , end , std::bind(imp::notInList,match_list.begin(),match_list.end(),_1,ignore_case) ) ;
}

G::StringArray::iterator G::Str::removeMatch( StringArray::iterator begin , StringArray::iterator end ,
	const StringArray & match_list , bool ignore_case )
{
	namespace imp = G::StrImp ;
	using namespace std::placeholders ;
	return std::remove_if( begin , end , std::bind(imp::inList,match_list.begin(),match_list.end(),_1,ignore_case) ) ;
}

void G::StrImp::strncpy( char * dst , const char * src , size_t n )
{
	for( ; n ; n-- , dst++ , src++ )
	{
		*dst = *src ;
		if( *src == '\0' )
			break ;
	}
}

errno_t G::Str::strncpy_s( char * dst , size_t n_dst , const char * src , size_t count )
{
	if( dst == nullptr || n_dst == 0U )
		return EINVAL ;

	if( src == nullptr )
	{
		*dst = '\0' ;
		return EINVAL ;
	}

	std::size_t n = std::strlen( src ) ;
	if( count != truncate && count < n )
		n = count ;

	if( count == truncate && n >= n_dst )
	{
		StrImp::strncpy( dst , src , n_dst ) ;
		dst[n_dst-1U] = '\0' ;
	}
	else if( n >= n_dst )
	{
		*dst = '\0' ;
		return ERANGE ; // dst too small
	}
	else
	{
		StrImp::strncpy( dst , src , n ) ;
		dst[n] = '\0' ;
	}
	return 0 ;
}

/// \file gstr.cpp
