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
// gstr.cpp
//

#include "gdef.h"
#include "gstr.h"
#include "glog.h"
#include "gassert.h"
#include <cmath>
#include <algorithm>
#include <iterator>
#include <functional>
#include <ctype.h>
#include <iomanip>
#include <climits>
#include <string>
#include <sstream>

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

void G::Str::escape( std::string & s )
{
	escapeImp( s , '\\' , "\\\r\n\t" , "\\rnt0" , true ) ;
}

void G::Str::escape( std::string & s , char c_escape , const char * specials_in , const char * specials_out )
{
	bool with_nul = std::strlen(specials_in) != std::strlen(specials_out) ;
	escapeImp( s , c_escape , specials_in , specials_out , with_nul ) ;
}

void G::Str::escape( std::string & s , char c_escape , const std::string & specials_in , const std::string & specials_out )
{
	G_ASSERT( specials_in.length() == specials_out.length() ) ;
	bool with_nul = !specials_in.empty() && specials_in.at(specials_in.length()-1U) == '\0' ;
	escapeImp( s , c_escape , specials_in.c_str() , specials_out.c_str() , with_nul ) ;
}

void G::Str::escapeImp( std::string & s , char c_escape , const char * specials_in , const char * specials_out , bool with_nul )
{
	G_ASSERT( specials_in != nullptr && specials_out != nullptr && (std::strlen(specials_out)-std::strlen(specials_in)) <= 1U ) ;
	size_type pos = 0U ;
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
		const size_t special_index = std::strchr(specials_in,c_in) - specials_in ;
		G_ASSERT( special_index < std::strlen(specials_out) ) ;

		s.insert( pos , 1U , c_escape ) ;
		pos++ ;
		s.at(pos) = specials_out[special_index] ;
		pos++ ;
	}
}

std::string G::Str::dequote( const std::string & s , char qq , char esc , const std::string & ws )
{
	std::string result ;
	result.reserve( s.size() ) ;
	bool in_quote = false ;
	bool escaped = false ;
	for( std::string::const_iterator p = s.begin() ; p != s.end() ; ++p )
	{
		if( *p == esc && !escaped )
		{
			escaped = true ;
		}
		else
		{
			if( *p == qq && !escaped && !in_quote )
			{
				in_quote = true ;
			}
			else if( *p == qq && !escaped )
			{
				in_quote = false ;
			}
			else if( ws.find(*p) != std::string::npos && in_quote )
			{
				result.append( 1U , esc ) ;
				result.append( 1U , *p ) ;
			}
			else
			{
				result.append( 1U , *p ) ;
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
	std::string::iterator out = s.begin() ; // output in-place
	for( std::string::iterator in = s.begin() ; in != s.end() ; ++in )
	{
		const char * specials_in_p = std::strchr( specials_in , *in ) ;
		const char * specials_out_p = specials_in_p ? (specials_out+(specials_in_p-specials_in)) : nullptr ;
		if( escaped && specials_out_p )
			*out++ = *specials_out_p , escaped = false ;
		else if( escaped && *in == c_escape )
			*out++ = c_escape , escaped = false ;
		else if( escaped )
			*out++ = *in , escaped = false ;
		else if( *in == c_escape )
			escaped = true ;
		else
			*out++ = *in , escaped = false ;
	}
	if( out != s.end() ) s.erase( out , s.end() ) ;
}

std::string G::Str::unescaped( const std::string & s_in )
{
	std::string s( s_in ) ;
	unescape( s ) ;
	return s ;
}

bool G::Str::replace( std::string & s , const std::string & from , const std::string & to , size_type * pos_p )
{
	if( from.length() == 0 )
		return false ;

	size_type pos = pos_p == nullptr ? 0 : *pos_p ;
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
	for( size_type pos = 0U ; replace(s,from,to,&pos) ; count++ )
		{;} // no-op
	return count ;
}

unsigned int G::Str::replaceAll( std::string & s , const char * from , const char * to )
{
	if( s.find(from) != std::string::npos )
	{
		unsigned int count = 0U ;
		for( size_type pos = 0U ; replace(s,from,to,&pos) ; count++ )
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

void G::Str::removeAll( std::string & s , char c )
{
	const std::string::iterator end = s.end() ;
	s.erase( std::remove_if( s.begin() , end , std::bind1st(std::equal_to<char>(),c) ) , end ) ;
}

std::string G::Str::only( const std::string & chars , const std::string & s )
{
	std::string result ;
	result.reserve( s.size() ) ;
	for( std::string::const_iterator p = s.begin() ; p != s.end() ; ++p )
	{
		if( chars.find(*p) != std::string::npos )
			result.append( 1U , *p ) ;
	}
	return result ;
}

void G::Str::trimLeft( std::string & s , const std::string & ws , size_type limit )
{
	size_type n = s.find_first_not_of( ws ) ;
	if( limit != 0U && ( n == std::string::npos || n > limit ) )
		n = limit >= s.length() ? std::string::npos : limit ;
	if( n == std::string::npos )
		s = std::string() ;
	else if( n != 0U )
		s.erase( 0U , n ) ;
}

void G::Str::trimRight( std::string & s , const std::string & ws , size_type limit )
{
	size_type n = s.find_last_not_of( ws ) ;
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

namespace
{
	struct IsDigit // : std::unary_function<char,bool>
	{
		bool operator()( char c ) const
		{
			unsigned char uc = static_cast<unsigned char>(c) ;
			return uc >= 48U && uc <= 57U ;
		}
	} ;
	struct NotIsDigit // std::not1(IsDigit())
	{
		bool operator()( char c ) const
		{
			return !IsDigit()(c) ;
		}
	} ;
	struct IsPrintableAscii // : std::unary_function<char,bool>
	{
		bool operator()( char c ) const
		{
			unsigned char uc = static_cast<unsigned char>(c) ;
			return uc >= 32U && uc < 127U ;
		}
	} ;
	struct NotIsPrintableAscii // std::not1(IsPrintableAscii())
	{
		bool operator()( char c ) const
		{
			return !IsPrintableAscii()(c) ;
		}
	} ;
	struct ToLower // : std::unary_function<char,char>
	{
		char operator()( char c )
		{
			const unsigned char uc = static_cast<unsigned char>(c) ;
			return ( uc >= 65U && uc <= 90U ) ? ( c + '\x20' ) : c ;
		}
	} ;
	struct ToUpper // : std::unary_function<char,char>
	{
		char operator()( char c )
		{
			const unsigned char uc = static_cast<unsigned char>(c) ;
			return ( uc >= 97U && uc <= 122U ) ? ( c - '\x20' ) : c ;
		}
	} ;
}

bool G::Str::isNumeric( const std::string & s , bool allow_minus_sign )
{
	const std::string::const_iterator end = s.end() ;
	std::string::const_iterator p = s.begin() ;
	if( allow_minus_sign && p != end && *p == '-' ) ++p ;
	return std::find_if( p , end , NotIsDigit() ) == end ;
}

bool G::Str::isPrintableAscii( const std::string & s )
{
	const std::string::const_iterator end = s.end() ;
	return std::find_if( s.begin() , end , NotIsPrintableAscii() ) == end ;
}

bool G::Str::isInt( const std::string & s )
{
	bool overflow = false ;
	bool invalid = false ;
	toIntImp( s , overflow , invalid ) ;
	return !overflow && !invalid ;
}

bool G::Str::isUShort( const std::string & s )
{
	bool overflow = false ;
	bool invalid = false ;
	toUShortImp( s , overflow , invalid ) ;
	return !overflow && !invalid ;
}

bool G::Str::isUInt( const std::string & s )
{
	bool overflow = false ;
	bool invalid = false ;
	toUIntImp( s , overflow , invalid ) ;
	return !overflow && !invalid ;
}

bool G::Str::isULong( const std::string & s )
{
	bool overflow = false ;
	bool invalid = false ;
	toULongImp( s , overflow , invalid ) ;
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
	std::ostringstream ss ;
	ss << i ;
	return ss.str() ;
}

std::string G::Str::fromLong( long l )
{
	std::ostringstream ss ;
	ss << l ;
	return ss.str() ;
}

std::string G::Str::fromShort( short s )
{
	std::ostringstream ss ;
	ss << s ;
	return ss.str() ;
}

std::string G::Str::fromUInt( unsigned int ui )
{
	std::ostringstream ss ;
	ss << ui ;
	return ss.str() ;
}

std::string G::Str::fromULong( unsigned long ul )
{
	std::ostringstream ss ;
	ss << ul ;
	return ss.str() ;
}

std::string G::Str::fromUShort( unsigned short us )
{
	std::ostringstream ss ;
	ss << us ;
	return ss.str() ;
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
	char * end = nullptr ;
	double result = ::strtod( s.c_str(), &end ) ;

	if( end == 0 || end[0] != '\0' )
		throw InvalidFormat( "expected floating point number" , s ) ;

	if( result == HUGE_VAL )
	 	throw Overflow( s ) ;

	if( result == -(HUGE_VAL) )
	 	throw Overflow( s ) ;

	return result ;
}

int G::Str::toInt( const std::string & s )
{
	bool overflow = false ;
	bool invalid = false ;
	int result = toIntImp( s , overflow , invalid ) ;
	if( invalid )
		throw InvalidFormat( "expected integer" , s ) ;
	if( overflow )
		throw Overflow( s ) ;
	return result ;
}

int G::Str::toIntImp( const std::string & s , bool & overflow , bool & invalid )
{
	long long_val = toLongImp( s , overflow , invalid ) ;
	int result = static_cast<int>( long_val ) ;
	if( result != long_val )
		overflow = true ;
	return result ;
}

long G::Str::toLong( const std::string & s )
{
	bool overflow = false ;
	bool invalid = false ;
	long result = toLongImp( s , overflow , invalid ) ;
	if( invalid )
		throw InvalidFormat( "expected long integer" , s ) ;
	if( overflow )
		throw Overflow( s ) ;
	return result ;
}

long G::Str::toLongImp( const std::string & s , bool & overflow , bool & invalid )
{
	char * end = nullptr ;
	long result = ::strtol( s.c_str(), &end, 10 ) ; // was radix 0
	if( end == 0 || end[0] != '\0' )
		invalid = true ;
	if( result == LONG_MAX || result == LONG_MIN )
		overflow = true ;
	return result ;
}

short G::Str::toShort( const std::string & s )
{
	bool overflow = false ;
	bool invalid = false ;
	short result = toShortImp( s , overflow , invalid ) ;
	if( invalid )
		throw InvalidFormat( "expected short integer" , s ) ;
	if( overflow )
		throw Overflow( s ) ;
	return result ;
}

short G::Str::toShortImp( const std::string & s , bool & overflow , bool & invalid )
{
	long long_val = toLongImp( s , overflow , invalid ) ;
	short result = static_cast<short>( long_val ) ;
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
	bool overflow = false ;
	bool invalid = false ;
	unsigned int result = toUIntImp( s , overflow , invalid ) ;
	if( invalid )
		throw InvalidFormat( "expected unsigned integer" , s ) ;
	if( overflow )
		result = UINT_MAX ;
	return result ;
}

unsigned int G::Str::toUInt( const std::string & s )
{
	bool overflow = false ;
	bool invalid = false ;
	unsigned int result = toUIntImp( s , overflow , invalid ) ;
	if( invalid )
		throw InvalidFormat( "expected unsigned integer" , s ) ;
	if( overflow )
		throw Overflow( s ) ;
	return result ;
}

unsigned int G::Str::toUIntImp( const std::string & s , bool & overflow , bool & invalid )
{
	unsigned long ulong_val = toULongImp( s , overflow , invalid ) ;
	unsigned int result = static_cast<unsigned int>( ulong_val ) ;
	if( result != ulong_val )
		overflow = true ;
	return result ;
}

unsigned long G::Str::toULong( const std::string & s , Limited )
{
	bool overflow = false ;
	bool invalid = false ;
	unsigned long result = toULongImp( s , overflow , invalid ) ;
	if( invalid )
		throw InvalidFormat( "expected unsigned long integer" , s ) ;
	if( overflow )
		result = ULONG_MAX ;
	return result ;
}

unsigned long G::Str::toULong( const std::string & s )
{
	bool overflow = false ;
	bool invalid = false ;
	unsigned long result = toULongImp( s , overflow , invalid ) ;
	if( invalid )
		throw InvalidFormat( "expected unsigned long integer" , s ) ;
	else if( overflow )
		throw Overflow( s ) ;
	return result ;
}

unsigned long G::Str::toULongImp( const std::string & s , bool & overflow , bool & invalid )
{
	if( s.empty() ) // new
	{
		invalid = true ;
		overflow = false ;
		return 0UL ;
	}
	else
	{
		char * end = nullptr ;
		unsigned long result = ::strtoul( s.c_str() , &end , 10 ) ;
		if( end == 0 || end[0] != '\0' )
			invalid = true ;
		if( result == ULONG_MAX )
			overflow = true ;
		return result ;
	}
}

unsigned short G::Str::toUShort( const std::string & s , Limited )
{
	bool overflow = false ;
	bool invalid = false ;
	unsigned short result = toUShortImp( s , overflow , invalid ) ;
	if( invalid )
		throw InvalidFormat( "expected unsigned short integer" , s ) ;
	if( overflow )
		result = USHRT_MAX ;
	return result ;
}

unsigned short G::Str::toUShort( const std::string & s )
{
	bool overflow = false ;
	bool invalid = false ;
	unsigned short result = toUShortImp( s , overflow , invalid ) ;
	if( invalid )
		throw InvalidFormat( "expected unsigned short integer" , s ) ;
	else if( overflow )
		throw Overflow( s ) ;
	return result ;
}

unsigned short G::Str::toUShortImp( const std::string & s , bool & overflow , bool & invalid )
{
	unsigned long ulong_val = toULongImp( s , overflow , invalid ) ;
	unsigned short result = static_cast<unsigned short>( ulong_val ) ;
	if( result != ulong_val )
		overflow = true ;
	return result ;
}

void G::Str::toLower( std::string & s )
{
	std::transform( s.begin() , s.end() , s.begin() , ToLower() ) ;
}

std::string G::Str::lower( const std::string & in )
{
	std::string out = in ;
	toLower( out ) ;
	return out ;
}

void G::Str::toUpper( std::string & s )
{
	std::transform( s.begin() , s.end() , s.begin() , ToUpper() ) ;
}

std::string G::Str::upper( const std::string & in )
{
	std::string out = in ;
	toUpper( out ) ;
	return out ;
}

namespace
{
	template <typename Tchar = char , typename Tuchar = unsigned char>
	struct PrintableAppender // : std::unary_function<Tchar,void>
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
			const Tuchar uc = static_cast<Tuchar>(c) ;
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
}

std::string G::Str::printable( const std::string & in , char escape )
{
	std::string result ;
	result.reserve( in.length() + 1U ) ;
	std::for_each( in.begin() , in.end() , PrintableAppender<char,unsigned char>(result,escape,true) ) ;
	return result ;
}

std::string G::Str::toPrintableAscii( const std::string & in , char escape )
{
	std::string result ;
	result.reserve( in.length() + 1U ) ;
	std::for_each( in.begin() , in.end() , PrintableAppender<char,unsigned char>(result,escape,false) ) ;
	return result ;
}

std::string G::Str::toPrintableAscii( char c , char escape )
{
	std::string result ;
	result.reserve( 2U ) ;
	PrintableAppender<char,unsigned char> append_printable( result , escape , false ) ;
	append_printable( c ) ;
	return result ;
}

std::string G::Str::toPrintableAscii( const std::wstring & in , wchar_t escape )
{
	std::string result ;
	result.reserve( in.length() * 3U ) ;
	std::for_each( in.begin() , in.end() , PrintableAppender<wchar_t,unsigned long>(result,escape,false) ) ;
	return result ;
}

std::string G::Str::readLineFrom( std::istream & stream , const std::string & eol )
{
	std::string result ;
	readLineFrom( stream , eol.empty() ? std::string(1U,'\n') : eol , result , true ) ;
	return result ;
}

void G::Str::readLineFrom( std::istream & stream , const std::string & eol , std::string & line , bool pre_erase )
{
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
		const std::string::size_type line_length = line.length() ;
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
				readLineFromImp( stream , eol , line ) ;
			}
		}
	}
	else
	{
		readLineFromImp( stream , eol , line ) ;
	}
}

void G::Str::readLineFromImp( std::istream & stream , const std::string & eol , std::string & line )
{
	const size_type limit = line.max_size() ;
	const size_type eol_length = eol.length() ;
	const char eol_final = eol.at( eol_length - 1U ) ;
	size_type line_length = line.length() ;

	bool changed = false ;
	char c ;
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
			const size_type offset = line_length - eol_length ;
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

std::string G::Str::wrap( std::string text , const std::string & prefix_1 ,
	const std::string & prefix_2 , size_type width )
{
	std::string ws( " \t\n" ) ;
	std::ostringstream ss ;
	for( bool first_line = true ; text.length() ; first_line = false )
	{
		const size_type prefix_length =
			first_line ? prefix_1.length() : prefix_2.length() ;
		size_type w = (width > prefix_length) ? (width-prefix_length) : width ;

		const size_type pos_nl = text.find_first_of("\n") ;
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
				const size_type white_space = line.find_last_of( ws ) ;
				const size_type black_space = line.find_first_not_of( ws ) ;
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

		const size_type black_space = text.find_first_not_of( ws ) ;
		if( black_space != 0U && black_space != std::string::npos )
		{
			unsigned int newlines = 0U ;
			for( size_type pos = 0U ; pos < black_space ; ++pos )
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

namespace
{
	template <typename T>
	void splitIntoTokensImp( const std::string & in , T & out , const std::string & ws )
	{
		typedef G::Str::size_type size_type ;
		for( size_type p = 0U ; p != std::string::npos ; )
		{
			p = in.find_first_not_of( ws , p ) ;
			if( p != std::string::npos )
			{
				size_type end = in.find_first_of( ws , p ) ;
				size_type len = end == std::string::npos ? end : (end-p) ;
				out.push_back( in.substr(p,len) ) ;
				p = end ;
			}
		}
	}
}
void G::Str::splitIntoTokens( const std::string & in , StringArray & out , const std::string & ws )
{
	splitIntoTokensImp( in , out , ws ) ;
}
G::StringArray G::Str::splitIntoTokens( const std::string & in , const std::string & ws )
{
	StringArray out ;
	splitIntoTokensImp( in , out , ws ) ;
	return out ;
}

namespace
{
	template <typename T>
	void splitIntoFieldsImp( const std::string & in , T & out , const std::string & ws )
	{
		if( in.length() )
		{
			size_t start = 0U ;
			size_t pos = 0U ;
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
	void splitIntoFieldsImp( const std::string & in_in , T & out , const std::string & ws ,
		char escape , bool remove_escapes )
	{
		std::string ews( ws ) ; // escape+whitespace
		if( escape != '\0' ) ews.append( 1U , escape ) ;
		if( in_in.length() )
		{
			std::string in = in_in ;
			size_t start = 0U ;
			size_t pos = 0U ;
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
}
void G::Str::splitIntoFields( const std::string & in , StringArray & out , const std::string & ws ,
	char escape , bool remove_escapes )
{
	splitIntoFieldsImp( in , out , ws , escape , remove_escapes ) ;
}
G::StringArray G::Str::splitIntoFields( const std::string & in , const std::string & ws )
{
	G::StringArray out ;
	splitIntoFieldsImp( in , out , ws ) ;
	return out ;
}

namespace
{
	template <typename T>
	struct Joiner
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
}

std::string G::Str::join( const std::string & sep , const StringMap & map ,
	const std::string & pre_in , const std::string & post )
{
	std::string pre = pre_in.empty() ? std::string("=") : pre_in ;
	std::string result ;
	bool first = true ;
	Joiner<std::string> joiner( result , sep , first ) ;
	for( StringMap::const_iterator p = map.begin() ; p != map.end() ; ++p )
		joiner( (*p).first + pre + (*p).second + post ) ;
	return result ;
}

std::string G::Str::join( const std::string & sep , const StringArray & strings )
{
	std::string result ;
	bool first = true ;
	std::for_each( strings.begin() , strings.end() , Joiner<std::string>(result,sep,first) ) ;
	return result ;
}

std::string G::Str::join( const std::string & sep , const std::set<std::string> & strings )
{
	std::string result ;
	bool first = true ;
	std::for_each( strings.begin() , strings.end() , Joiner<std::string>(result,sep,first) ) ;
	return result ;
}

std::string G::Str::join( const std::string & sep , const std::string & s1 , const std::string & s2 ,
	const std::string & s3 , const std::string & s4 , const std::string & s5 , const std::string & s6 ,
	const std::string & s7 , const std::string & s8 , const std::string & s9 )
{
	std::string result ;
	joinImp( sep , result , s1 ) ;
	joinImp( sep , result , s2 ) ;
	joinImp( sep , result , s3 ) ;
	joinImp( sep , result , s4 ) ;
	joinImp( sep , result , s5 ) ;
	joinImp( sep , result , s6 ) ;
	joinImp( sep , result , s7 ) ;
	joinImp( sep , result , s8 ) ;
	joinImp( sep , result , s9 ) ;
	return result ;
}

void G::Str::joinImp( const std::string & sep , std::string & result , const std::string & s )
{
	if( !result.empty() && !s.empty() )
		result.append( sep ) ;
	result.append( s ) ;
}

namespace
{
	template <typename T>
	struct Firster
	{
		const typename T::first_type & operator()( const T & pair ) { return pair.first ; }
	} ;
}
std::set<std::string> G::Str::keySet( const StringMap & map )
{
	std::set<std::string> result ;
	std::transform( map.begin() , map.end() , std::inserter(result,result.end()) , Firster<StringMap::value_type>() ) ;
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

std::string G::Str::head( const std::string & in , std::string::size_type pos , const std::string & default_ )
{
	return
		pos == std::string::npos ?
			default_ :
			( pos == 0U ? std::string() : ( pos >= in.length() ? in : in.substr(0U,pos) ) ) ;
}

std::string G::Str::head( const std::string & in , const std::string & sep , bool default_empty )
{
	size_t pos = sep.empty() ? std::string::npos : in.find( sep ) ;
	return head( in , pos , default_empty ? std::string() : in ) ;
}

std::string G::Str::tail( const std::string & in , std::string::size_type pos , const std::string & default_ )
{
	return
		pos == std::string::npos ?
			default_ :
			( (pos+1U) >= in.length() ? std::string() : in.substr(pos+1U) ) ;
}

std::string G::Str::tail( const std::string & in , const std::string & sep , bool default_empty )
{
	size_t pos = sep.empty() ? std::string::npos : in.find(sep) ;
	if( pos != std::string::npos ) pos += (sep.length()-1U) ;
	return tail( in , pos , default_empty ? std::string() : in ) ;
}

bool G::Str::tailMatch( const std::string & in , const std::string & tail )
{
	return
		tail.empty() ||
		( in.length() >= tail.length() && 0 == in.compare(in.length()-tail.length(),tail.length(),tail) ) ;
}

namespace
{
	struct TailMatch
	{
		const std::string & m_tail ;
		explicit TailMatch( const std::string & s ) : m_tail(s) {}
		bool operator()( const std::string & s ) const { return G::Str::tailMatch(s,m_tail) ; }
	} ;
}
bool G::Str::tailMatch( const StringArray & in , const std::string & tail )
{
	TailMatch matcher( tail ) ;
	return std::find_if( in.begin() , in.end() , matcher ) != in.end() ;
}

bool G::Str::headMatch( const std::string & in , const char * head )
{
	if( head == nullptr || *head == '\0' ) return true ;
	size_t head_length = std::strlen( head ) ;
	return ( in.length() >= head_length && 0 == in.compare(0U,head_length,head) ) ;
}

bool G::Str::headMatch( const std::string & in , const std::string & head )
{
	return
		head.empty() ||
		( in.length() >= head.length() && 0 == in.compare(0U,head.length(),head) ) ;
}

namespace
{
	struct HeadMatch
	{
		const std::string & m_head ;
		explicit HeadMatch( const std::string & s ) : m_head(s) {}
		bool operator()( const std::string & s ) const { return G::Str::headMatch(s,m_head) ; }
	} ;
}
bool G::Str::headMatch( const StringArray & in , const std::string & head )
{
	HeadMatch matcher( head ) ;
	return std::find_if( in.begin() , in.end() , matcher ) != in.end() ;
}

std::string G::Str::headMatchResidue( const StringArray & in , const std::string & head )
{
	StringArray::const_iterator const end = in.end() ;
	for( StringArray::const_iterator p = in.begin() ; p != end ; ++p )
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

namespace
{
	template <typename T1, typename T2, typename P> bool std_equal( T1 p1 , T1 end1 , T2 p2 , T2 end2 , P p )
	{
		// (std::equal with four iterators is c++14 or later)
		for( ; p1 != end1 && p2 != end2 ; ++p1 , ++p2 )
		{
			if( !p(*p1,*p2) )
				return false ;
		}
		return p1 == end1 && p2 == end2 ;
	}
	bool icompare( char c1 , char c2 )
	{
		if( c1 >= 'A' && c1 <= 'Z' ) c1 += '\x20' ;
		if( c2 >= 'A' && c2 <= 'Z' ) c2 += '\x20' ;
		return c1 == c2 ;
	}
	bool imatchImp( const std::string & a , const std::string & b )
	{
		return a.size() == b.size() && ::std_equal( a.begin() , a.end() , b.begin() , b.end() , icompare ) ;
	}
	struct StringMatcher // : std::unary_function<std::string,bool>
	{
		StringMatcher( const std::string & s , bool i ) : m_s(s) , m_i(i) {}
		bool operator()( const std::string & s ) const { return m_i ? imatchImp(m_s,s) : (m_s==s) ; }
		std::string m_s ;
		bool m_i ;
	} ;
	struct InListMatcher // : std::unary_function<std::string,bool>
	{
		typedef G::StringArray::const_iterator T ;
		InListMatcher( T begin , T end , bool i ) : m_begin(begin) , m_end(end) , m_i(i) {}
		bool operator()( const std::string & s ) const { return std::find_if(m_begin,m_end,StringMatcher(s,m_i)) != m_end ; }
		T m_begin ;
		T m_end ;
		bool m_i ;
	} ;
	struct NotInListMatcher // std::not1(InListMatcher())
	{
		typedef G::StringArray::const_iterator T ;
		NotInListMatcher( T begin , T end , bool i ) : m_begin(begin) , m_end(end) , m_i(i) {}
		bool operator()( const std::string & s ) const { return std::find_if(m_begin,m_end,StringMatcher(s,m_i)) == m_end ; }
		T m_begin ;
		T m_end ;
		bool m_i ;
	} ;
}

bool G::Str::imatch( const std::string & a , const std::string & b )
{
	return imatchImp( a , b ) ;
}

bool G::Str::imatch( const StringArray & a , const std::string & b )
{
	return InListMatcher(a.begin(),a.end(),true)( b ) ;
}

std::string::size_type G::Str::ifind( const std::string & s , const std::string & key , std::string::size_type pos )
{
	if( s.empty() || key.empty() || pos > s.length() ) return std::string::npos ;
	std::string::const_iterator p = std::search( s.begin()+pos , s.end() , key.begin() , key.end() , icompare ) ;
	return p == s.end() ? std::string::npos : std::distance(s.begin(),p) ;
}

namespace
{
	template <typename T, typename V>
	T uniqueImp( T in , T end , V repeat , V replacement )
	{
		// replace repeated repeat-s with a single replacement
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
}

std::string G::Str::unique( const std::string & s_in , char c , char r )
{
	std::string s( s_in ) ;
	s.erase( uniqueImp( s.begin() , s.end() , c , r ) , s.end() ) ;
	return s ;
}

std::string G::Str::unique( const std::string & s , char c )
{
	return s.find(c) == std::string::npos ? s : unique( s , c , c ) ;
}

G::StringArray::iterator G::Str::keepMatch( StringArray::iterator begin , StringArray::iterator end ,
	const StringArray & match_list , bool ignore_case )
{
	if( match_list.empty() ) return end ;
	return std::remove_if( begin , end , NotInListMatcher(match_list.begin(),match_list.end(),ignore_case) ) ;
}

G::StringArray::iterator G::Str::removeMatch( StringArray::iterator begin , StringArray::iterator end ,
	const StringArray & match_list , bool ignore_case )
{
	return std::remove_if( begin , end , InListMatcher(match_list.begin(),match_list.end(),ignore_case) ) ;
}

/// \file gstr.cpp
