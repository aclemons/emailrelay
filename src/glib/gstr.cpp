//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gdebug.h"
#include <cmath>
#include <algorithm>
#include <functional>
#include <ctype.h>
#include <iomanip>
#include <climits>
#include <string>
#include <sstream>

std::string G::Str::escaped( const std::string & s_in , const std::string & specials , char c_escape )
{
	std::string s( s_in ) ;
	escape( s , specials , c_escape ) ;
	return s ;
}

void G::Str::escape( std::string & s , const std::string & specials , char c_escape )
{
	size_type pos = 0U ;
	for(;;)
	{
		pos = s.find_first_of( specials , pos ) ;
		if( pos == std::string::npos )
			break ;

		s.insert( pos , 1U , c_escape ) ;
		pos++ ;
		pos++ ;
	}
}

bool G::Str::replace( std::string & s , const std::string & from , const std::string & to , size_type * pos_p )
{
	if( from.length() == 0 )
		return false ;

	size_type pos = pos_p == NULL ? 0 : *pos_p ;
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
		if( pos_p != NULL ) 
			*pos_p = pos + to.length() ;
		return true ;
	}
}

unsigned int G::Str::replaceAll( std::string & s , const std::string & from , const std::string & to )
{
	unsigned int count = 0U ;
	for( size_type pos = 0U ; replace(s,from,to,&pos) ; count++ )
		; // no-op
	return count ;
}

unsigned int G::Str::replaceAll( std::string & s , const char * from , const char * to )
{
	// this char* overload is an optimisation to avoid constructing
	// temporaries when starting with a c-string -- we only construct
	// std-strings if there is work to do

	if( s.find(from) != std::string::npos )
	{
		unsigned int count = 0U ;
		for( size_type pos = 0U ; replace(s,from,to,&pos) ; count++ )
			; // no-op
		return count ;
	}
	else
	{
		return 0U ;
	}
}

void G::Str::removeAll( std::string & s , char c )
{
	const std::string::iterator end = s.end() ;
	s.erase( std::remove_if( s.begin() , end , std::bind1st(std::equal_to<char>(),c) ) , end ) ;
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
	else if( n != 0U )
		s.resize( n+1U ) ;
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
	struct IsDigit : std::unary_function<char,bool>
	{
		bool operator()( char c ) const { return !! isdigit( c ) ; }
	} ;
}
bool G::Str::isNumeric( const std::string & s , bool allow_minus_sign )
{
	const std::string::const_iterator end = s.end() ;
	std::string::const_iterator p = s.begin() ;
	if( allow_minus_sign && p != end && *p == '-' ) ++p ;
	return std::find_if( p , end , std::not1(IsDigit()) ) == end ;
}

namespace
{
	struct IsPrintableAscii : std::unary_function<char,bool>
	{
		bool operator()( char c ) const { return c >= 0x20 && c < 0x7f ; }
	} ;
}
bool G::Str::isPrintableAscii( const std::string & s )
{
	const std::string::const_iterator end = s.end() ;
	return std::find_if( s.begin() , end , std::not1(IsPrintableAscii()) ) == end ;
}

bool G::Str::isUShort( const std::string & s )
{
	try
	{
		G_IGNORE_RETURN(unsigned short) toUShort(s) ;
	}
	catch( Overflow & )
	{
		return false ;
	}
	catch( InvalidFormat & )
	{
		return false ;
	}
	return true ;
}

bool G::Str::isUInt( const std::string & s )
{
	try
	{
		G_IGNORE_RETURN(unsigned int) toUInt(s) ;
	}
	catch( Overflow & )
	{
		return false ;
	}
	catch( InvalidFormat & )
	{
		return false ;
	}
	return true ;
}

bool G::Str::isULong( const std::string & s )
{
	try
	{
		G_IGNORE_RETURN(unsigned long) toULong(s) ;
	}
	catch( Overflow & )
	{
		return false ;
	}
	catch( InvalidFormat & )
	{
		return false ;
	}
	return true ;
}

std::string G::Str::fromBool( bool b )
{
	return b ? "true" : "false" ;
}

std::string G::Str::fromDouble( double d )
{
	std::ostringstream ss ;
	ss << std::setprecision(16) << d ; // was "setprecision(DBL_DIG+1)
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
	if( str == "true" )
	{
		return true ;
	}
	else if( str == "false" )
	{
		return false ;
	}
	else
	{
		throw InvalidFormat( s ) ;
		return false ; // never gets here -- the return pacifies some compilers
	}
}

double G::Str::toDouble( const std::string &s )
{
	char * end = NULL ;
	double result = ::strtod( s.c_str(), &end ) ; 

	if( end == 0 || end[0] != '\0' )
		throw InvalidFormat( s ) ;

	if( result == HUGE_VAL || result == -(HUGE_VAL) )
	 	throw Overflow( s ) ;

	return result ;
}

int G::Str::toInt( const std::string &s )
{
	long long_val = toLong( s ) ;
	int int_val = static_cast<int>( long_val ) ;

	if( int_val != long_val )
		throw Overflow( s ) ;

	return int_val ;
}

long G::Str::toLong( const std::string &s )
{
	char * end = NULL ;
	long result = ::strtol( s.c_str(), &end, 0 ) ; 

	if( end == 0 || end[0] != '\0' )
		throw InvalidFormat( s ) ;

	if( result == LONG_MAX || result == LONG_MIN )
		throw Overflow( s ) ;

	return result ;
}

short G::Str::toShort( const std::string &s )
{
	long long_val = toLong( s ) ;
	short short_val = static_cast<short>( long_val ) ;

	if( short_val != long_val )
		throw Overflow( s ) ;

	return short_val ;
}

unsigned int G::Str::toUInt( const std::string &s , bool limited )
{
	unsigned long ulong_val = toULong( s ) ;
	unsigned int uint_val = static_cast<unsigned int>( ulong_val ) ;

	if( uint_val != ulong_val )
	{
		if( limited )
			uint_val = UINT_MAX ;
		else
			throw Overflow( s ) ;
	}

	return uint_val ;
}

unsigned long G::Str::toULong( const std::string &s , bool limited )
{
	char * end = NULL ;
	unsigned long result = ::strtoul( s.c_str() , &end , 10 ) ; 

	if( end == 0 || end[0] != '\0' )
		throw InvalidFormat( s ) ;

	if( result == ULONG_MAX )
	{
		if( limited )
			result = ULONG_MAX ;
		else
			throw Overflow( s ) ;
	}

	return result ;
}

unsigned short G::Str::toUShort( const std::string &s , bool limited )
{
	unsigned long ulong_val = toULong( s ) ;
	unsigned short ushort_val = static_cast<unsigned short>( ulong_val ) ;

	if( ushort_val != ulong_val )
	{
		if( limited )
			ushort_val = USHRT_MAX ;
		else
			throw Overflow( s ) ;
	}

	return ushort_val ;
}

namespace
{
	struct ToLower : std::unary_function<char,char>
	{
		char operator()( char c ) { return static_cast<char>( tolower(c) ) ; }
	} ;
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

namespace
{
	struct ToUpper : std::unary_function<char,char>
	{
		char operator()( char c ) { return static_cast<char>( toupper(c) ) ; }
	} ;
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
	struct PrintableAppender : std::unary_function<Tchar,void>
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
		// Save the initial character, use std::getline() for speed (terminating
		// on the second character of the two-character terminator), check that the 
		// one-character terminator was actually part of the required two-character 
		// terminator, remove the first character of the two-character terminator, 
		// and finally re-insert the initial character.
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
		stream.get( c ) ; // sets the fail bit at eof
		if( stream.fail() )
		{
			// work more like std::getline() in <string> -- reset the failbit and set eof
			// (remember that clear() sets all the flags explicitly to the given values)
			stream.clear( ( stream.rdstate() & ~std::ios_base::failbit ) | std::ios_base::eofbit ) ;
			break ;
		}

		if( line_length == limit ) // pathological case -- see also std::getline()
		{
			stream.setstate( std::ios_base::failbit ) ;
			break ;
		}

		line.append( 1U , c ) ; // fast enough if 'line' has sufficient capacity
		changed = true ;
		++line_length ;

		if( line_length >= eol_length && c == eol_final ) // optimisation
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
	{
		// set the failbit
		// (remember that setstate() sets the flag(s) identified by the given bitmask to 1)
		stream.setstate( std::ios_base::failbit ) ; 
	}
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
	void splitIntoTokens_( const std::string & in , T & out , const std::string & ws )
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
void G::Str::splitIntoTokens( const std::string & in , Strings & out , const std::string & ws )
{
	splitIntoTokens_( in , out , ws ) ;
}
void G::Str::splitIntoTokens( const std::string & in , StringArray & out , const std::string & ws )
{
	splitIntoTokens_( in , out , ws ) ;
}

namespace
{
	template <typename T>
	void splitIntoFields_( const std::string & in_in , T & out , const std::string & ws , 
		char escape , bool discard_bogus )
	{
		typedef G::Str::size_type size_type ;
		std::string all( ws ) ;
		if( escape != '\0' )
			all.append( 1U , escape ) ;

		if( in_in.length() ) 
		{
			std::string in = in_in ;
			size_type start = 0U ;
			size_type last_pos = in.length() - 1U ;
			size_type pos = 0U ;
			for(;;)
			{
				if( pos >= in.length() ) break ;
				pos = in.find_first_of( all , pos ) ;
				if( pos == std::string::npos ) break ;
				if( in.at(pos) == escape )
				{
					const bool valid = pos != last_pos && in.find(all,pos+1U) == (pos+1U) ;
					if( valid || discard_bogus )
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
void G::Str::splitIntoFields( const std::string & in , Strings & out , const std::string & ws , 
	char escape , bool discard_bogus )
{
	splitIntoFields_( in , out , ws , escape , discard_bogus ) ;
}
void G::Str::splitIntoFields( const std::string & in , StringArray & out , const std::string & ws , 
	char escape , bool discard_bogus )
{
	splitIntoFields_( in , out , ws , escape , discard_bogus ) ;
}

namespace
{
	template <typename T>
	struct Joiner : std::unary_function<const T&,void>
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
std::string G::Str::join( const Strings & strings , const std::string & sep )
{
	std::string result ;
	bool first = true ;
	std::for_each( strings.begin() , strings.end() , Joiner<std::string>(result,sep,first) ) ;
	return result ;
}
std::string G::Str::join( const StringArray & strings , const std::string & sep )
{
	std::string result ;
	bool first = true ;
	std::for_each( strings.begin() , strings.end() , Joiner<std::string>(result,sep,first) ) ; 
	return result ;
}

namespace
{
	template <typename T>
	struct Firster : std::unary_function<const T&,const typename T::first_type&>
	{
		const typename T::first_type & operator()( const T & pair ) { return pair.first ; }
	} ;
}
G::Strings G::Str::keys( const StringMap & map )
{
	Strings result ;
	std::transform( map.begin() , map.end() , std::back_inserter(result) , Firster<StringMap::value_type>() ) ;
	return result ;
}

std::string G::Str::ws()
{
	return std::string(" \t\n\r") ;
}

std::string G::Str::head( const std::string & in , std::string::size_type pos , const std::string & default_ )
{
	return
		pos == std::string::npos ?
			default_ :
			( pos == 0U ? std::string() : ( (pos+1U) >= in.length() ? in : in.substr(0U,pos) ) ) ;
}

std::string G::Str::tail( const std::string & in , std::string::size_type pos , const std::string & default_ )
{
	return
		pos == std::string::npos ?
			default_ :
			( (pos+1U) >= in.length() ? std::string() : in.substr(pos+1U) ) ;
}

bool G::Str::tailMatch( const std::string & in , const std::string & ending )
{
	return
		ending.empty() ||
		( in.length() >= ending.length() && 
			in.substr( in.length() - ending.length() ) == ending ) ;
			// 0 == in.compare( in.length() - ending.length() , ending.length() , ending ) // faster, but not gcc2.95
}

/// \file gstr.cpp
