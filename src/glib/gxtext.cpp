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
//
// gxtext.cpp
//

#include "gdef.h"
#include "gxtext.h"
#include "gstr.h"
#include "gassert.h"

namespace
{
	inline char hex( unsigned int n )
	{
		static const char * map = "0123456789ABCDEF" ;
		return map[n] ;
	}
	inline bool ishex( char c )
	{
		return
			( c >= '0' && c <= '9' ) ||
			( c >= 'a' && c <= 'f' ) || // moot
			( c >= 'A' && c <= 'F' ) ;
	}
	inline unsigned int unhex( char c )
	{
		unsigned int rc = 0U ;
		switch( c )
		{
			case '0': rc = 0U ; break ;
			case '1': rc = 1U ; break ;
			case '2': rc = 2U ; break ;
			case '3': rc = 3U ; break ;
			case '4': rc = 4U ; break ;
			case '5': rc = 5U ; break ;
			case '6': rc = 6U ; break ;
			case '7': rc = 7U ; break ;
			case '8': rc = 8U ; break ;
			case '9': rc = 9U ; break ;
			case 'A': rc = 10U ; break ;
			case 'B': rc = 11U ; break ;
			case 'C': rc = 12U ; break ;
			case 'D': rc = 13U ; break ;
			case 'E': rc = 14U ; break ;
			case 'F': rc = 15U ; break ;
			case 'a': rc = 10U ; break ; // moot
			case 'b': rc = 11U ; break ;
			case 'c': rc = 12U ; break ;
			case 'd': rc = 13U ; break ;
			case 'e': rc = 14U ; break ;
			case 'f': rc = 15U ; break ;
		}
		return rc ;
	}
}

bool G::Xtext::valid( const std::string & s )
{
	if( s.find('\x7f') != std::string::npos ) return false ;
	if( s.find(' ') != std::string::npos ) return false ;
	if( s.find('+') == std::string::npos ) return G::Str::isPrintableAscii(s) ;
	for( size_t pos = s.find('+') ; pos != std::string::npos ; pos = ((pos+1U)==s.size()?std::string::npos:s.find('+',pos+1U)) )
	{
		if( (pos+2U) >= s.size() ) return false ;
		if( !ishex(s.at(pos+1U)) ) return false ;
		if( !ishex(s.at(pos+2U)) ) return false ;
	}
	return true ;
}

std::string G::Xtext::encode( const std::string & s )
{
	std::string result ;
	for( std::string::const_iterator p = s.begin() ; p != s.end() ; ++p )
	{
		if( *p >= '!' && *p <= '~' && *p != '=' && *p != '+' )
		{
			result.append( 1U , *p ) ;
		}
		else
		{
			unsigned int n = static_cast<unsigned char>(*p) ;
			result.append( 1U , '+' ) ;
			result.append( 1U , hex( n >> 4U ) ) ;
			result.append( 1U , hex( n & 0x0f ) ) ;
		}
	}
	G_ASSERT( decode(result) == s ) ;
	return result ;
}

std::string G::Xtext::decode( const std::string & s )
{
	std::string result ;
	for( std::string::const_iterator p = s.begin() ; p != s.end() ; ++p )
	{
		if( *p == '+' )
		{
			++p ; if( p == s.end() ) break ;
			char h1 = *p++ ; if( p == s.end() ) break ;
			char h2 = *p ;
			unsigned int c = ( unhex(h1) << 4U ) | unhex(h2) ;
			result.append( 1U , static_cast<char>(static_cast<unsigned char>(c)) ) ;
		}
		else
		{
			result.append( 1U , *p ) ;
		}
	}
	return result ;
}

/// \file gxtext.cpp
