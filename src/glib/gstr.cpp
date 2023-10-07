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
/// \file gstr.cpp
///

#include "gdef.h"
#include "gstr.h"
#include "gassert.h"
#include <algorithm>
#include <type_traits> // std::make_unsigned
#include <stdexcept>
#include <iterator>
#include <limits>
#include <functional>
#include <iomanip>
#include <string>
#include <sstream>
#include <cstring>
#include <cerrno>
#include <cmath>

namespace G
{
	namespace StrImp
	{
		string_view hexmap() noexcept ;
		bool isDigit( char c ) noexcept ;
		bool isHex( char c ) noexcept ;
		bool isPrintableAscii( char c ) noexcept ;
		bool isPrintable( char c ) noexcept ;
		bool isSimple( char c ) noexcept ;
		char toLower( char c ) noexcept ;
		char toUpper( char c ) noexcept ;
		unsigned short toUShort( string_view , bool & overflow , bool & invalid ) noexcept ;
		unsigned long toULong( string_view , bool & overflow , bool & invalid ) noexcept ;
		unsigned long toULongHex( string_view , bool limited ) ;
		unsigned int toUInt( string_view , bool & overflow , bool & invalid ) noexcept ;
		short toShort( string_view , bool & overflow , bool & invalid ) noexcept ;
		long toLong( string_view , bool & overflow , bool & invalid ) noexcept ;
		int toInt( string_view , bool & overflow , bool & invalid ) noexcept ;
		template <typename U> string_view fromUnsignedToHex( U u , char * out_p ) noexcept ;
		void strncpy( char * , const char * , std::size_t ) noexcept ;
		template <typename Tstr, typename Fn> bool readLine( std::istream & , Tstr & , char * , std::size_t , Fn ) ;
		template <typename S, typename T, typename SV> void splitIntoTokens( const S & in , T & out , const SV & ws ) ;
		template <typename S, typename T> void splitIntoTokens( const S & in , T & out , const S & ws , typename S::value_type esc ) ;
		template <typename T> void splitIntoFields( string_view in , T & out , string_view ws ) ;
		template <typename T> void splitIntoFields( string_view in_in , T & out , string_view ws , char escape , bool remove_escapes ) ;
		bool ilessc( char c1 , char c2 ) noexcept ;
		bool imatchc( char c1 , char c2 ) noexcept ;
		bool imatch( const std::string & a , const std::string & b ) ;
		template <typename T, typename V> T unique( T in , T end , V repeat , V replacement ) ;
		bool inList( StringArray::const_iterator begin , StringArray::const_iterator end , const std::string & s , bool i ) ;
		bool notInList( StringArray::const_iterator begin , StringArray::const_iterator end , const std::string & s , bool i ) ;
		void join( string_view , std::string & , string_view ) ;
		template <typename Tout> std::size_t outputHex( Tout out , char c ) ;
		template <typename Tout> std::size_t outputHex( Tout out , wchar_t c ) ;
		template <typename Tout, typename Tchar> std::size_t outputPrintable( Tout , Tchar , Tchar , char , bool ) ;
		bool allOf( string_view s , bool (*fn)(char) ) noexcept ;
	}
}

bool G::StrImp::allOf( string_view s , bool (*fn)(char) ) noexcept
{
	return std::all_of( s.begin() , s.end() , fn ) ; // (true if empty)
}

#ifndef G_LIB_SMALL
std::string G::Str::escaped( string_view s_in )
{
	if( s_in.empty() ) return {} ;
	std::string s( s_in.data() , s_in.size() ) ;
	escape( s ) ;
	return s ;
}
#endif

std::string G::Str::escaped( string_view s_in , char c_escape , string_view specials_in , string_view specials_out )
{
	if( s_in.empty() ) return {} ;
	std::string s( s_in.data() , s_in.size() ) ;
	escape( s , c_escape , specials_in , specials_out ) ;
	return s ;
}

void G::Str::escape( std::string & s , char c_escape , string_view specials_in , string_view specials_out )
{
	G_ASSERT( specials_in.size() == specials_out.size() ) ;
	std::size_t pos = 0U ;
	for(;;)
	{
		char c_in = '\0' ;
		pos = s.find_first_of( specials_in.data() , pos , specials_in.size() ) ;
		if( pos == std::string::npos )
			break ;
		else
			c_in = s.at( pos ) ;

		const std::size_t special_index = specials_in.find( c_in ) ;
		char c_out = specials_out.at( special_index ) ;

		s.insert( pos , 1U , c_escape ) ;
		pos++ ;
		s.at(pos) = c_out ;
		pos++ ;
	}
}

void G::Str::escape( std::string & s )
{
	Str::escape( s , '\\' , "\0\\\r\n\t"_sv , "0\\rnt"_sv ) ;
}

std::string G::Str::dequote( const std::string & s , char qq , char esc , string_view ws , string_view nbws )
{
	G_ASSERT( ws.size() == nbws.size() ) ;
	std::string result ;
	result.reserve( s.size() ) ;
	bool in_quote = false ;
	bool escaped = false ;
	for( char c : s )
	{
		if( c == esc && !escaped )
		{
			escaped = true ;
			result.append( 1U , esc ) ;
		}
		else
		{
			std::size_t wspos = 0 ;
			if( c == qq && !escaped && !in_quote )
			{
				in_quote = true ;
			}
			else if( c == qq && !escaped )
			{
				in_quote = false ;
			}
			else if( in_quote && (wspos=ws.find(c)) != std::string::npos )
			{
				if( escaped )
				{
					result.append( 1U , nbws.at(wspos) ) ;
				}
				else
				{
					result.append( 1U , esc ) ;
					result.append( 1U , c ) ;
				}
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
	unescape( s , '\\' , "0rnt"_sv , "\0\r\n\t"_sv ) ;
}

void G::Str::unescape( std::string & s , char c_escape , string_view specials_in , string_view specials_out )
{
	G_ASSERT( specials_in.size() == specials_out.size() ) ;
	bool escaped = false ;
	std::size_t cpos = 0U ;
	auto out = s.begin() ; // output in-place
	for( char & c_in : s )
	{
		if( escaped && (cpos=specials_in.find(c_in)) != std::string::npos ) // was BS now 'n'
			*out++ = specials_out.at(cpos) , escaped = false ; // emit NL
		else if( escaped && c_in == c_escape ) // was BS now BS
			*out++ = c_escape , escaped = false ; // emit BS
		else if( escaped ) // was BS
			*out++ = c_in , escaped = false ; // emit c
		else if( c_in == c_escape ) // is BS
			escaped = true ;
		else
			*out++ = c_in , escaped = false ;
	}
	if( out != s.end() ) s.erase( out , s.end() ) ;
}

#ifndef G_LIB_SMALL
std::string G::Str::unescaped( const std::string & s_in )
{
	std::string s( s_in ) ;
	unescape( s ) ;
	return s ;
}
#endif

void G::Str::replace( std::string & s , char from , char to )
{
	for( char & c : s )
	{
		if( c == from )
			c = to ;
	}
}

void G::Str::replace( StringArray & a , char from , char to )
{
	for( std::string & s : a )
		replace( s , from , to ) ;
}

bool G::Str::replace( std::string & s , string_view from , string_view to , std::size_t * pos_p )
{
	if( from.empty() )
		return false ;

	std::size_t pos = pos_p == nullptr ? 0 : *pos_p ;
	if( pos >= s.size() )
		return false ;

	pos = s.find( from.data() , pos , from.size() ) ;
	if( pos == std::string::npos )
		return false ;
	else if( to.empty() )
		s.erase( pos , from.size() ) ;
	else
		s.replace( pos , from.size() , to.data() , to.size() ) ;
	if( pos_p != nullptr )
		*pos_p = pos + to.size() ;
	return true ;
}

unsigned int G::Str::replaceAll( std::string & s , string_view from , string_view to )
{
	unsigned int count = 0U ;
	for( std::size_t pos = 0U ; replace(s,from,to,&pos) ; count++ )
		{;} // no-op
	return count ;
}

std::string G::Str::replaced( const std::string & s , char from , char to )
{
	std::string result( s ) ;
	replaceAll( result , {&from,1U} , {&to,1U} ) ;
	return result ;
}

void G::Str::removeAll( std::string & s , char c )
{
	s.erase( std::remove_if( s.begin() , s.end() , [c](char x){return x==c;} ) , s.end() ) ;
}

#ifndef G_LIB_SMALL
std::string G::Str::removedAll( const std::string & s_in , char c )
{
	std::string s( s_in ) ;
	removeAll( s , c ) ;
	return s ;
}
#endif

std::string G::Str::only( string_view chars , string_view s )
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

std::string & G::Str::trimLeft( std::string & s , string_view ws , std::size_t limit )
{
	std::size_t n = s.find_first_not_of( ws.data() , 0U , ws.size() ) ;
	if( limit != 0U && ( n == std::string::npos || n > limit ) )
		n = limit >= s.size() ? std::string::npos : limit ;
	if( n == std::string::npos )
		s.erase() ;
	else if( n != 0U )
		s.erase( 0U , n ) ;
	return s ;
}

G::string_view G::Str::trimLeftView( string_view sv , string_view ws , std::size_t limit ) noexcept
{
	std::size_t n = sv.find_first_not_of( ws ) ;
	if( limit != 0U && ( n == std::string::npos || n > limit ) )
		n = limit >= sv.size() ? std::string::npos : limit ;
	if( n == std::string::npos )
		return sv_substr( sv , 0U , 0U ) ;
	else if( n != 0U )
		return sv_substr( sv , n ) ;
	else
		return sv ;
}

std::string & G::Str::trimRight( std::string & s , string_view ws , std::size_t limit )
{
	std::size_t n = s.find_last_not_of( ws.data() , std::string::npos , ws.size() ) ;
	if( limit != 0U && ( n == std::string::npos || s.size() > (limit+n+1U) ) )
		n = limit >= s.size() ? std::string::npos : (s.size()-limit-1U) ;
	if( n == std::string::npos )
		s.erase() ;
	else if( (n+1U) != s.size() )
		s.resize( n + 1U ) ;
	return s ;
}

G::string_view G::Str::trimRightView( string_view sv , string_view ws , std::size_t limit ) noexcept
{
	std::size_t n = sv.find_last_not_of( ws ) ;
	if( limit != 0U && ( n == std::string::npos || sv.size() > (limit+n+1U) ) )
		n = limit >= sv.size() ? std::string::npos : (sv.size()-limit-1U) ;
	if( n == std::string::npos )
		return sv_substr( sv , 0U , 0U ) ;
	else if( (n+1U) != sv.size() )
		return sv_substr( sv , 0U , n+1U ) ;
	else
		return sv ;
}

std::string & G::Str::trim( std::string & s , string_view ws )
{
	return trimLeft( trimRight(s,ws) , ws ) ;
}

std::string G::Str::trimmed( const std::string & s_in , string_view ws )
{
	std::string s( s_in ) ;
	return trim( s , ws ) ;
}

std::string G::Str::trimmed( std::string && s , string_view ws )
{
	return std::move( trimLeft(trimRight(s,ws),ws) ) ;
}

G::string_view G::Str::trimmedView( string_view s , string_view ws ) noexcept
{
	return trimLeftView( trimRightView(s,ws) , ws ) ;
}

bool G::StrImp::isDigit( char c ) noexcept
{
	auto uc = static_cast<unsigned char>(c) ;
	return uc >= 48U && uc <= 57U ;
}

bool G::StrImp::isHex( char c ) noexcept
{
	auto uc = static_cast<unsigned char>(c) ;
	return ( uc >= 48U && uc <= 57U ) || ( uc >= 65U && uc <= 70U ) || ( uc >= 97U && uc <= 102U ) ;
}

bool G::StrImp::isPrintableAscii( char c ) noexcept
{
	auto uc = static_cast<unsigned char>(c) ;
	return uc >= 32U && uc < 127U ;
}

bool G::StrImp::isPrintable( char c ) noexcept
{
	auto uc = static_cast<unsigned char>(c) ;
	return ( uc >= 32U && uc < 127U ) || ( uc >= 0xa0 && uc < 0xff ) ;
}

bool G::StrImp::isSimple( char c ) noexcept
{
	auto uc = static_cast<unsigned char>(c) ;
	return isDigit(c) || c == '-' || c == '_' ||
		( uc >= 65U && uc <= 90U ) ||
		( uc >= 97U && uc <= 122U ) ;
}

char G::StrImp::toLower( char c ) noexcept
{
	const auto uc = static_cast<unsigned char>(c) ;
	return ( uc >= 65U && uc <= 90U ) ? static_cast<char>(c+'\x20') : c ;
}

char G::StrImp::toUpper( char c ) noexcept
{
	const auto uc = static_cast<unsigned char>(c) ;
	return ( uc >= 97U && uc <= 122U ) ? static_cast<char>(c-'\x20') : c ;
}

bool G::Str::isNumeric( string_view s , bool allow_minus_sign ) noexcept
{
	bool bump = allow_minus_sign && s.size() > 1U && s[0] == '-' ;
	return StrImp::allOf( bump?sv_substr(s,1U):s , StrImp::isDigit ) ; // (true if empty)
}

#ifndef G_LIB_SMALL
bool G::Str::isHex( string_view s ) noexcept
{
	return StrImp::allOf( s , StrImp::isHex ) ;
}
#endif

bool G::Str::isPrintableAscii( string_view s ) noexcept
{
	return StrImp::allOf( s , StrImp::isPrintableAscii ) ;
}

bool G::Str::isPrintable( string_view s ) noexcept
{
	return StrImp::allOf( s , StrImp::isPrintable ) ;
}

bool G::Str::isSimple( string_view s ) noexcept
{
	return StrImp::allOf( s , StrImp::isSimple ) ;
}

bool G::Str::isInt( string_view s ) noexcept
{
	bool overflow = false ;
	bool invalid = false ;
	StrImp::toInt( s , overflow , invalid ) ;
	return !overflow && !invalid ;
}

#ifndef G_LIB_SMALL
bool G::Str::isUShort( string_view s ) noexcept
{
	bool overflow = false ;
	bool invalid = false ;
	StrImp::toUShort( s , overflow , invalid ) ;
	return !overflow && !invalid ;
}
#endif

bool G::Str::isUInt( string_view s ) noexcept
{
	bool overflow = false ;
	bool invalid = false ;
	StrImp::toUInt( s , overflow , invalid ) ;
	return !overflow && !invalid ;
}

bool G::Str::isULong( string_view s ) noexcept
{
	bool overflow = false ;
	bool invalid = false ;
	StrImp::toULong( s , overflow , invalid ) ;
	return !overflow && !invalid ;
}

#ifndef G_LIB_SMALL
std::string G::Str::fromBool( bool b )
{
	return b ? "true" : "false" ;
}
#endif

#ifndef G_LIB_SMALL
std::string G::Str::fromDouble( double d )
{
	if( std::isnan(d) ) return "nan" ;
	if( std::isinf(d) ) return "inf" ;
	std::ostringstream ss ;
	ss << std::setprecision(16) << d ;
	return ss.str() ;
}
#endif

#ifndef G_LIB_SMALL
bool G::Str::toBool( string_view s )
{
	bool result = true ;
	if( imatch( s , "true"_sv ) )
		{;}
	else if( imatch( s , "false"_sv ) )
		result = false ;
	else
		throw InvalidFormat( "expected true/false" , s ) ;
	return result ;
}
#endif

#ifndef G_LIB_SMALL
double G::Str::toDouble( const std::string & s )
{
	try
	{
		std::size_t end = 0U ;
		double result = std::stod( s , &end ) ;
		if( end != s.size() )
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
#endif

#ifndef G_LIB_SMALL
float G::Str::toFloat( const std::string & s )
{
	try
	{
		std::size_t end = 0U ;
		float result = std::stof( s , &end ) ;
		if( end != s.size() )
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
#endif

int G::Str::toInt( string_view s )
{
	bool overflow = false ;
	bool invalid = false ;
	int result = StrImp::toInt( s , overflow , invalid ) ;
	if( invalid )
		throw InvalidFormat( "expected integer" , s ) ;
	if( overflow )
		throw Overflow( s ) ;
	return result ;
}

#ifndef G_LIB_SMALL
int G::Str::toInt( string_view s1 , string_view s2 )
{
	return !s1.empty() && isInt(s1) ? toInt(s1) : toInt(s2) ;
}
#endif

int G::StrImp::toInt( string_view s , bool & overflow , bool & invalid ) noexcept
{
	long long_val = toLong( s , overflow , invalid ) ;
	int result = static_cast<int>( long_val ) ;
	if( result != long_val )
		overflow = true ;
	return result ;
}

#ifndef G_LIB_SMALL
long G::Str::toLong( string_view s )
{
	bool overflow = false ;
	bool invalid = false ;
	long result = StrImp::toLong( s , overflow , invalid ) ;
	if( invalid )
		throw InvalidFormat( "expected long integer" , s ) ;
	if( overflow )
		throw Overflow( s ) ;
	return result ;
}
#endif

long G::StrImp::toLong( string_view s , bool & overflow , bool & invalid ) noexcept
{
	bool negative = !s.empty() && s[0] == '-' ;
	bool positive = !s.empty() && s[0] == '+' ;
	if( (negative || positive) && s.size() == 1U )
	{
		invalid = true ;
		return 0L ;
	}
	unsigned long ul = toULong( sv_substr(s,(negative||positive)?1U:0U) , overflow , invalid ) ;
	static constexpr long long_max = std::numeric_limits<long>::max() ;
	if( ul > long_max || (negative && (ul==long_max)) )
	{
		overflow = true ;
		return 0L ;
	}
	return negative ? -static_cast<long>(ul) : static_cast<long>(ul) ;
}

#ifndef G_LIB_SMALL
short G::Str::toShort( string_view s )
{
	bool overflow = false ;
	bool invalid = false ;
	short result = StrImp::toShort( s , overflow , invalid ) ;
	if( invalid )
		throw InvalidFormat( "expected short integer" , s ) ;
	if( overflow )
		throw Overflow( s ) ;
	return result ;
}
#endif

short G::StrImp::toShort( string_view s , bool & overflow , bool & invalid ) noexcept
{
	long long_val = toLong( s , overflow , invalid ) ;
	auto result = static_cast<short>( long_val ) ;
	if( result != long_val )
		overflow = true ;
	return result ;
}

unsigned int G::Str::toUInt( string_view s1 , string_view s2 )
{
	return !s1.empty() && isUInt(s1) ? toUInt(s1) : toUInt(s2) ;
}

#ifndef G_LIB_SMALL
unsigned int G::Str::toUInt( string_view s , unsigned int default_ )
{
	return !s.empty() && isUInt(s) ? toUInt(s) : default_ ;
}
#endif

#ifndef G_LIB_SMALL
unsigned int G::Str::toUInt( string_view s , Limited )
{
	bool overflow = false ;
	bool invalid = false ;
	unsigned int result = StrImp::toUInt( s , overflow , invalid ) ;
	if( invalid )
		throw InvalidFormat( "expected unsigned integer" , s ) ;
	if( overflow )
		result = std::numeric_limits<unsigned int>::max() ;
	return result ;
}
#endif

unsigned int G::Str::toUInt( string_view s )
{
	bool overflow = false ;
	bool invalid = false ;
	unsigned int result = StrImp::toUInt( s , overflow , invalid ) ;
	if( invalid )
		throw InvalidFormat( "expected unsigned integer" , s ) ;
	if( overflow )
		throw Overflow( s ) ;
	return result ;
}

unsigned int G::StrImp::toUInt( string_view s , bool & overflow , bool & invalid ) noexcept
{
	unsigned long ulong_val = toULong( s , overflow , invalid ) ;
	auto result = static_cast<unsigned int>( ulong_val ) ;
	if( result != ulong_val )
		overflow = true ;
	return result ;
}

unsigned long G::Str::toULong( string_view s , Limited )
{
	bool overflow = false ;
	bool invalid = false ;
	unsigned long result = StrImp::toULong( s , overflow , invalid ) ;
	if( invalid )
		throw InvalidFormat( "expected unsigned long integer" , s ) ;
	if( overflow )
		result = std::numeric_limits<unsigned long>::max() ;
	return result ;
}

#ifndef G_LIB_SMALL
unsigned long G::Str::toULong( string_view s , Hex , Limited )
{
	return StrImp::toULongHex( s , true ) ;
}
#endif

#ifndef G_LIB_SMALL
unsigned long G::Str::toULong( string_view s , Hex )
{
	return StrImp::toULongHex( s , false ) ;
}
#endif

unsigned long G::StrImp::toULongHex( string_view s , bool limited )
{
	unsigned long n = 0U ;
	if( s.empty() ) return 0U ;
	std::size_t i0 = s.find_first_not_of( '0' ) ;
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
		n <<= 4U ;
		n += c ;
	}
	return n ;
}

unsigned long G::Str::toULong( string_view s )
{
	bool overflow = false ;
	bool invalid = false ;
	unsigned long result = StrImp::toULong( s , overflow , invalid ) ;
	if( invalid )
		throw InvalidFormat( "expected unsigned long integer" , s ) ;
	else if( overflow )
		throw Overflow( s ) ;
	return result ;
}

#ifndef G_LIB_SMALL
unsigned long G::Str::toULong( string_view s1 , string_view s2 )
{
	return !s1.empty() && isULong(s1) ? toULong(s1) : toULong(s2) ;
}
#endif

unsigned long G::StrImp::toULong( string_view s , bool & overflow , bool & invalid ) noexcept
{
	// note that stoul()/strtoul() do too much in that they skip leading
	// spaces, allow initial +/- even for unsigned, and are C-locale
	// dependent -- also there is no string_view version of std::stol()
	// and std::strtol() requires a null-terminated string and so cannot
	// take string_view::data()

	return Str::toUnsigned<unsigned long>( s.data() , s.data()+s.size() , overflow , invalid ) ;
}

#ifndef G_LIB_SMALL
unsigned short G::Str::toUShort( string_view s , Limited )
{
	bool overflow = false ;
	bool invalid = false ;
	unsigned short result = StrImp::toUShort( s , overflow , invalid ) ;
	if( invalid )
		throw InvalidFormat( "expected unsigned short integer" , s ) ;
	if( overflow )
		result = std::numeric_limits<unsigned short>::max() ;
	return result ;
}
#endif

#ifndef G_LIB_SMALL
unsigned short G::Str::toUShort( string_view s )
{
	bool overflow = false ;
	bool invalid = false ;
	unsigned short result = StrImp::toUShort( s , overflow , invalid ) ;
	if( invalid )
		throw InvalidFormat( "expected unsigned short integer" , s ) ;
	else if( overflow )
		throw Overflow( s ) ;
	return result ;
}
#endif

unsigned short G::StrImp::toUShort( string_view s , bool & overflow , bool & invalid ) noexcept
{
	unsigned long ulong_val = toULong( s , overflow , invalid ) ;
	auto result = static_cast<unsigned short>( ulong_val ) ;
	if( result != ulong_val )
		overflow = true ;
	return result ;
}

#ifndef G_LIB_SMALL
G::string_view G::Str::fromULongToHex( unsigned long u , char * out_p ) noexcept
{
	return StrImp::fromUnsignedToHex( u , out_p ) ;
}
#endif

#ifndef G_LIB_SMALL
G::string_view G::Str::fromULongLongToHex( unsigned long long u , char * out_p ) noexcept
{
	return StrImp::fromUnsignedToHex( u , out_p ) ;
}
#endif

template <typename U>
G::string_view G::StrImp::fromUnsignedToHex( U u , char * out_p ) noexcept
{
	// (std::to_chars() is c++17 so do it long-hand)
	static constexpr unsigned bytes = sizeof(U) ;
	static constexpr unsigned bits = 8U * bytes ;
	static constexpr unsigned buffer_size = bytes * 2U ;
	static const char * map = "0123456789abcdef" ;
	unsigned int shift = bits - 4U ;
	char * out = out_p ;
	for( unsigned i = 0 ; i < (bytes*2U) ; i++ ) // unrollable, no break
	{
		*out++ = map[(u>>shift) & 0xfUL] ;
		shift -= 4U ;
	}
	string_view sv( out_p , buffer_size ) ;
	return sv_substr( sv , std::min( sv.find_first_not_of('0') , static_cast<std::size_t>(buffer_size-1U) ) ) ;
}

void G::Str::toLower( std::string & s )
{
	std::transform( s.begin() , s.end() , s.begin() , StrImp::toLower ) ;
}

std::string G::Str::lower( G::string_view in )
{
	std::string out = sv_to_string( in ) ;
	toLower( out ) ;
	return out ;
}

void G::Str::toUpper( std::string & s )
{
	std::transform( s.begin() , s.end() , s.begin() , StrImp::toUpper ) ;
}

std::string G::Str::upper( G::string_view in )
{
	std::string out = sv_to_string( in ) ;
	toUpper( out ) ;
	return out ;
}

G::string_view G::StrImp::hexmap() noexcept
{
	static constexpr string_view chars_hexmap = "0123456789abcdef"_sv ;
	static_assert( chars_hexmap.size() == 16U , "" ) ;
	return chars_hexmap ;
}

template <typename Tout>
std::size_t G::StrImp::outputHex( Tout out , char c )
{
	std::size_t n = static_cast<unsigned char>( c ) ;
	n &= 0xffU ;
	out( hexmap()[(n>>4U)%16U] ) ;
	out( hexmap()[(n>>0U)%16U] ) ;
	return 2U ;
}

template <typename Tout>
std::size_t G::StrImp::outputHex( Tout out , wchar_t c )
{
	using uwchar_t = typename std::make_unsigned<wchar_t>::type ;
	std::size_t n = static_cast<uwchar_t>( c ) ;
	n &= 0xffffU ;
	out( hexmap()[(n>>12U)%16U] ) ;
	out( hexmap()[(n>>8U)%16U] ) ;
	out( hexmap()[(n>>4U)%16U] ) ;
	out( hexmap()[(n>>0U)%16U] ) ;
	return 4U ;
}

template <typename Tout, typename Tchar>
std::size_t G::StrImp::outputPrintable( Tout out , Tchar c , Tchar escape_in , char escape_out , bool eight_bit )
{
	using Tuchar = typename std::make_unsigned<Tchar>::type ;
	const auto uc = static_cast<Tuchar>( c ) ; // NOLINT not bugprone-signed-char-misuse
	std::size_t n = 1U ;
	if( c == escape_in )
	{
		out( escape_out ) ;
		n++ ;
		out( escape_out ) ;
	}
	else if( !eight_bit && uc >= 0x20U && uc < 0x7fU )
	{
		out( static_cast<char>(c) ) ;
	}
	else if( eight_bit && ( uc >= 0x20U && uc != 0x7fU ) )
	{
		out( static_cast<char>(c) ) ;
	}
	else
	{
		out( escape_out ) ;
		n++ ;
		char c_out = 'x' ;
		if( uc == 10U )
			c_out = 'n' ;
		else if( uc == 13U )
			c_out = 'r' ;
		else if( uc == 9U )
			c_out = 't' ;
		else if( uc == 0U )
			c_out = '0' ;
		out( c_out ) ;
		if( c_out == 'x' )
			n += outputHex( out , c ) ;
	}
	return n ;
}

std::string G::Str::printable( const std::string & in , char escape )
{
	std::string result ;
	result.reserve( in.length()*2U + 1U ) ;
	for( auto c : in )
		StrImp::outputPrintable( [&result](char cc){result.append(1U,cc);} , c , escape , escape , true ) ;
	return result ;
}

std::string G::Str::printable( G::string_view in , char escape )
{
	std::string result ;
	result.reserve( in.length()*2U + 1U ) ;
	for( auto c : in )
		StrImp::outputPrintable( [&result](char cc){result.append(1U,cc);} , c , escape , escape , true ) ;
	return result ;
}

std::string G::Str::toPrintableAscii( const std::string & in , char escape )
{
	std::string result ;
	result.reserve( in.length()*2U + 1U ) ;
	for( auto c : in )
		StrImp::outputPrintable( [&result](char cc){result.append(1U,cc);} , c , escape , escape , false ) ;
	return result ;
}

#ifndef G_LIB_SMALL
std::string G::Str::toPrintableAscii( const std::wstring & in , wchar_t escape )
{
	std::string result ;
	result.reserve( in.length()*2U + 1U ) ;
	for( auto c : in )
		StrImp::outputPrintable( [&result](wchar_t cc){result.append(1U,static_cast<char>(cc));} , c , escape , static_cast<char>(escape) , false ) ;
	return result ;
}
#endif

std::string G::Str::readLineFrom( std::istream & stream , string_view eol )
{
	std::string result ;
	readLine( stream , result , eol , false , 0U ) ;
	return result ;
}

std::istream & G::Str::readLine( std::istream & stream , std::string & line , string_view eol ,
	bool pre_erase , std::size_t limit )
{
	if( !stream.good() ) // (new)
	{
		// don't pre-erase -- surprising but cf. std::getline()
		stream.setstate( std::ios_base::failbit ) ;
		return stream ;
	}

	if( pre_erase )
		line.clear() ;

	if( line.empty() && ( eol.empty() || eol.size() == 1U ) && limit == 0U )
	{
		std::getline( stream , line , eol.empty() ? '\n' : eol[0] ) ;
	}
	else
	{
		if( eol.empty() ) eol = string_view( "\n" , 1U ) ; // new
		const char eol_last = eol.at( eol.size()-1U ) ;
		const std::size_t eol_size = eol.size() ;
		bool got_eol = StrImp::readLine( stream , line , nullptr , limit ? limit : line.max_size() ,
			[eol,eol_size,eol_last](std::string &s,char c)
			{
				return
					( c == eol_last && s.size() >= eol_size ) ?
						s.find(eol.data(),s.size()-eol_size,eol_size) == (s.size()-eol_size) :
						false ;
			} ) ;
		if( got_eol )
			line.erase( line.size() - eol.size() ) ;
	}
	return stream ;
}

std::istream & G::Str::readLine( std::istream & stream , std::string & line , Eol eol ,
	bool pre_erase , std::size_t limit )
{
	if( eol == Eol::CrLf )
	{
		return readLine( stream , line , "\r\n"_sv , pre_erase , limit ) ;
	}
	else // Cr_Lf_CrLf
	{
		if( pre_erase && stream.good() ) // cf. std::getline()
			line.clear() ;

		char next = '\0' ;
		bool got_eol = StrImp::readLine( stream , line , &next , limit ? limit : line.max_size() ,
			[](std::string &,char c)
			{
				return c == '\n' || c == '\r' ;
			} ) ;
		if( got_eol )
		{
			if( line.at(line.size()-1U) == '\r' && next == '\n' )
				stream.get() ; // advance over the peeked '\n'
			line.erase( line.size()-1U ) ;
		}
		return stream ;
	}
}

template <typename Tstring, typename Fn>
bool G::StrImp::readLine( std::istream & stream , Tstring & line , char * next_p , std::size_t limit , Fn eol_fn )
{
	bool got_eol = false ;
	bool got_some = false ;
	std::istream::sentry sentry( stream , true ) ;
	if( sentry )
	{
		using traits = std::istream::traits_type ;
		std::size_t count = 0U ;
		int c = stream.rdbuf()->sgetc() ; // c = *p
		while( count < limit && c != traits::eof() )
		{
			line.append( 1U , traits::to_char_type(c) ) ;
			got_eol = eol_fn( line , traits::to_char_type(c) ) ;
			if( got_eol )
				break ;

			count++ ;
			c = stream.rdbuf()->snextc() ; // c = *(++p)
		}
		got_some = count > 0U ;
		if( count == limit )
		{
			stream.setstate( std::ios_base::failbit ) ;
		}
		else if( c == traits::eof() )
		{
			stream.setstate( std::ios_base::eofbit ) ;
		}
		else
		{
			got_some = true ;
			stream.rdbuf()->sbumpc() ; // ++p
			if( next_p )
				*next_p = traits::to_char_type( stream.rdbuf()->sgetc() ) ; // next = *p
		}
	}
	if( !got_some )
		stream.setstate( std::ios_base::failbit ) ;
	return got_eol ;
}

template <typename S, typename T, typename SV>
void G::StrImp::splitIntoTokens( const S & in , T & out , const SV & ws )
{
	for( std::size_t p = 0U ; p != S::npos ; )
	{
		p = in.find_first_not_of( ws.data() , p , ws.size() ) ;
		if( p != S::npos )
		{
			std::size_t end = in.find_first_of( ws.data() , p , ws.size() ) ;
			std::size_t len = end == S::npos ? end : (end-p) ;
			out.push_back( in.substr(p,len) ) ;
			p = end ;
		}
	}
}

template <typename S, typename T>
void G::StrImp::splitIntoTokens( const S & in , T & out , const S & ws , typename S::value_type esc )
{
	using string_type = S ;
	string_type ews = ws + string_type(1U,esc) ;
	for( std::size_t p = 0U ; p != S::npos ; )
	{
		// find the token start
		p = in.find_first_not_of( ws.data() , p , ws.size() ) ;
		if( p == S::npos || ( in.at(p) == esc && (p+1) == in.size() ) )
			break ;

		// find the token end
		std::size_t end = in.find_first_of( ews.data() , p , ews.size() ) ;
		while( end != S::npos && end < in.size() && in.at(end) == esc )
			end = (end+2) < in.size() ? in.find_first_of( ews.data() , end+2 , ews.size() ) : S::npos ;

		// extract the token
		std::size_t len = end == std::string::npos ? end : (end-p) ;
		string_type w( in.substr(p,len) ) ;

		// remove whitespace escapes
		for( std::size_t i = 0 ; esc && i < w.size() ; i++ )
		{
			if( w[i] == esc )
			{
				if( (i+1) < w.size() && ws.find(w[i+1]) != S::npos )
					w.erase( i , 1U ) ;
				else
					i++ ;
			}
		}

		out.push_back( w ) ;
		p = end ;
	}
}

void G::Str::splitIntoTokens( const std::string & in , StringArray & out , string_view ws , char esc )
{
	if( esc && in.find(esc) != std::string::npos )
		StrImp::splitIntoTokens( in , out , sv_to_string(ws) , esc ) ;
	else
		StrImp::splitIntoTokens( in , out , ws ) ;
}

G::StringArray G::Str::splitIntoTokens( const std::string & in , string_view ws , char esc )
{
	StringArray out ;
	splitIntoTokens( in , out , ws , esc ) ;
	return out ;
}

template <typename T>
void G::StrImp::splitIntoFields( string_view in , T & out , string_view ws )
{
	if( !in.empty() )
	{
		std::size_t start = 0U ;
		std::size_t pos = 0U ;
		for(;;)
		{
			if( pos >= in.size() ) break ;
			pos = in.find_first_of( ws.data() , pos , ws.size() ) ;
			if( pos == std::string::npos ) break ;
			out.push_back( sv_to_string(in.substr(start,pos-start)) ) ;
			pos++ ;
			start = pos ;
		}
		out.push_back( sv_to_string(in.substr(start,pos-start)) ) ;
	}
}

template <typename T>
void G::StrImp::splitIntoFields( string_view in_in , T & out , string_view ws ,
	char escape , bool remove_escapes )
{
	std::string ews ; // escape+whitespace
	ews.reserve( ws.size() + 1U ) ;
	ews.assign( ws.data() , ws.size() ) ;
	if( escape != '\0' ) ews.append( 1U , escape ) ;
	if( !in_in.empty() )
	{
		std::string in = sv_to_string( in_in ) ;
		std::size_t start = 0U ;
		std::size_t pos = 0U ;
		for(;;)
		{
			if( pos >= in.size() ) break ;
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

void G::Str::splitIntoFields( string_view in , StringArray & out , char sep ,
	char escape , bool remove_escapes )
{
	StrImp::splitIntoFields( in , out , string_view(&sep,1U) , escape , remove_escapes ) ;
}

G::StringArray G::Str::splitIntoFields( string_view in , char sep )
{
	G::StringArray out ;
	StrImp::splitIntoFields( in , out , string_view(&sep,1U) ) ;
	return out ;
}

#ifndef G_LIB_SMALL
std::string G::Str::join( string_view sep , const StringMap & map , string_view pre ,
	string_view post )
{
	std::string result ;
	int n = 0 ;
	for( const auto & map_item : map )
		result
			.append(sep.data(),(n++)?sep.size():0U)
			.append(map_item.first)
			.append(pre.data(),pre.size())
			.append(map_item.second)
			.append(post.data(),post.size()) ;
	return result ;
}
#endif

std::string G::Str::join( string_view sep , const StringArray & strings )
{
	std::string result ;
	int n = 0 ;
	for( const auto & item : strings )
		result
			.append(sep.data(),(n++)?sep.size():0U)
			.append(item) ;
	return result ;
}

std::string G::Str::join( string_view sep , string_view s1 , string_view s2 ,
	string_view s3 , string_view s4 , string_view s5 , string_view s6 ,
	string_view s7 , string_view s8 , string_view s9 )
{
	std::string result ;
	StrImp::join( sep , result , s1 ) ;
	StrImp::join( sep , result , s2 ) ;
	StrImp::join( sep , result , s3 ) ;
	StrImp::join( sep , result , s4 ) ;
	StrImp::join( sep , result , s5 ) ;
	StrImp::join( sep , result , s6 ) ;
	StrImp::join( sep , result , s7 ) ;
	StrImp::join( sep , result , s8 ) ;
	StrImp::join( sep , result , s9 ) ;
	return result ;
}

void G::StrImp::join( string_view sep , std::string & result , string_view s )
{
	if( !result.empty() && !s.empty() )
		result.append( sep.data() , sep.size() ) ;
	result.append( s.data() , s.size() ) ;
}

G::StringArray G::Str::keys( const StringMap & map )
{
	StringArray result ;
	result.reserve( map.size() ) ;
	std::transform( map.begin() , map.end() , std::back_inserter(result) ,
		[](const StringMap::value_type & pair){return pair.first;} ) ;
	return result ;
}

G::string_view G::Str::ws() noexcept
{
	static constexpr string_view chars_ws = " \t\n\r"_sv ;
	static_assert( chars_ws.size() == 4U , "" ) ;
	return chars_ws ;
}

#ifndef G_LIB_SMALL
G::string_view G::Str::alnum() noexcept
{
	return sv_substr( alnum_() , 0U , alnum_().size()-1U ) ;
}
#endif

G::string_view G::Str::alnum_() noexcept
{
	static constexpr string_view chars_alnum_ = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz_"_sv ;
	static_assert( chars_alnum_.size() == 26U+10U+26U+1U , "" ) ;
	return chars_alnum_ ;
}

#ifndef G_LIB_SMALL
G::string_view G::Str::meta() noexcept
{
	static constexpr string_view chars_meta = "~<>[]*$|?\\(){}\"`'&;="_sv ; // bash meta-chars plus "~"
	return chars_meta ;
}
#endif

std::string G::Str::head( string_view in , std::size_t pos , string_view default_ )
{
	return
		pos == std::string::npos ?
			sv_to_string(default_) :
			( pos == 0U ? std::string() : sv_to_string( pos >= in.size() ? in : in.substr(0U,pos) ) ) ;
}

std::string G::Str::head( string_view in , string_view sep , bool default_empty )
{
	std::size_t pos = sep.empty() ? std::string::npos : in.find( sep ) ;
	return head( in , pos , default_empty ? string_view() : in ) ;
}

G::string_view G::Str::headView( string_view in , std::size_t pos , string_view default_ ) noexcept
{
	return
		pos == std::string::npos ?
			default_ :
			( pos == 0U ? string_view(in.data(),std::size_t(0U)) : ( pos >= in.size() ? in : sv_substr(in,0U,pos) ) ) ;
}

G::string_view G::Str::headView( string_view in , string_view sep , bool default_empty ) noexcept
{
	std::size_t pos = sep.empty() ? std::string::npos : in.find( sep ) ;
	return headView( in , pos , default_empty ? string_view(in.data(),std::size_t(0U)) : in ) ;
}

std::string G::Str::tail( string_view in , std::size_t pos , string_view default_ )
{
	return
		pos == std::string::npos ?
			sv_to_string(default_) :
			( (pos+1U) >= in.size() ? std::string() : sv_to_string(in.substr(pos+1U)) ) ;
}

std::string G::Str::tail( string_view in , string_view sep , bool default_empty )
{
	std::size_t pos = sep.empty() ? std::string::npos : in.find(sep) ;
	if( pos != std::string::npos ) pos += (sep.size()-1U) ;
	return tail( in , pos , default_empty ? string_view() : in ) ;
}

G::string_view G::Str::tailView( string_view in , std::size_t pos , string_view default_ ) noexcept
{
	return
		pos == std::string::npos ?
			default_ :
			( (pos+1U) >= in.size() ? string_view() : sv_substr(in,pos+1U) ) ;
}

G::string_view G::Str::tailView( string_view in , string_view sep , bool default_empty ) noexcept
{
	std::size_t pos = sep.empty() ? std::string::npos : in.find(sep) ;
	if( pos != std::string::npos ) pos += (sep.size()-1U) ;
	return tailView( in , pos , default_empty ? string_view() : in ) ;
}

bool G::Str::tailMatch( const std::string & in , string_view tail ) noexcept
{
	return
		tail.empty() ||
		( in.size() >= tail.size() && 0 == in.compare(in.size()-tail.size(),tail.size(),tail.data()) ) ;
}

bool G::Str::headMatch( const std::string & in , string_view head ) noexcept
{
	return
		head.empty() ||
		( in.size() >= head.size() && 0 == in.compare(0U,head.size(),head.data()) ) ;
}

std::string G::Str::positive()
{
	return "yes" ;
}

std::string G::Str::negative()
{
	return "no" ;
}

bool G::Str::isPositive( string_view s_in ) noexcept
{
	string_view s = trimmedView( s_in , ws() ) ;
	return !s.empty() && (
		sv_imatch(s,"y"_sv) || sv_imatch(s,"yes"_sv) || sv_imatch(s,"t"_sv) ||
		sv_imatch(s,"true"_sv) || sv_imatch(s,"1"_sv) || sv_imatch(s,"on"_sv) ) ;
}

bool G::Str::isNegative( string_view s_in ) noexcept
{
	string_view s = trimmedView( s_in , ws() ) ;
	return !s.empty() && (
		sv_imatch(s,"n"_sv) || sv_imatch(s,"no"_sv) || sv_imatch(s,"f"_sv) ||
		sv_imatch(s,"false"_sv) || sv_imatch(s,"0"_sv) || sv_imatch(s,"off"_sv) ) ;
}

bool G::Str::match( string_view a , string_view b ) noexcept
{
	return a == b ;
}

bool G::StrImp::ilessc( char c1 , char c2 ) noexcept
{
	if( c1 >= 'a' && c1 <= 'z' ) c1 -= '\x20' ;
	if( c2 >= 'a' && c2 <= 'z' ) c2 -= '\x20' ;
	return c1 < c2 ;
}

bool G::Str::iless( string_view a , string_view b ) noexcept
{
	return std::lexicographical_compare( a.begin() , a.end() , b.begin() , b.end() , StrImp::ilessc ) ; // noexcept?
}

bool G::StrImp::imatchc( char c1 , char c2 ) noexcept
{
	return sv_imatch( string_view(&c1,1U) , string_view(&c2,1U) ) ;
}

#ifndef G_LIB_SMALL
bool G::Str::imatch( char c1 , char c2 ) noexcept
{
	return StrImp::imatchc( c1 , c2 ) ;
}
#endif

bool G::Str::imatch( string_view a , string_view b ) noexcept
{
	return sv_imatch( a , b ) ;
}

#ifndef G_LIB_SMALL
bool G::StrImp::imatch( const std::string & a , const std::string & b )
{
	return sv_imatch( string_view(a) , string_view(b) ) ;
}
#endif

std::size_t G::Str::ifind( string_view s , string_view key )
{
	return ifindat( s , key , 0U ) ;
}

std::size_t G::Str::ifindat( string_view s , string_view key , std::size_t pos )
{
	if( s.empty() || key.empty() || pos >= s.size() ) return std::string::npos ;
	auto p = std::search( s.data()+pos , s.data()+s.size() , key.data() , key.data()+key.size() , StrImp::imatchc ) ; // NOLINT narrowing
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
		if( *in == repeat && in_next != end && *in_next == repeat )
		{
			while( in != end && *in == repeat )
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
	std::string s( s_in ) ;
	s.erase( StrImp::unique( s.begin() , s.end() , c , r ) , s.end() ) ;
	return s ;
}

std::string G::Str::unique( const std::string & s , char c )
{
	return s.find(c) == std::string::npos ? s : unique( s , c , c ) ;
}

void G::StrImp::strncpy( char * dst , const char * src , std::size_t n ) noexcept
{
	// (because 'strncpy considered dangerous' analytics)
	for( ; n ; n-- , dst++ , src++ )
	{
		*dst = *src ;
		if( *src == '\0' )
			break ;
	}
}

errno_t G::Str::strncpy_s( char * dst , std::size_t n_dst , const char * src , std::size_t count ) noexcept
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
		return 0 ; // STRUNCATE
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
		return 0 ;
	}
}

