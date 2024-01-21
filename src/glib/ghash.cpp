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
/// \file ghash.cpp
///

#include "gdef.h"
#include "ghash.h"
#include "gassert.h"
#include <sstream>

std::string G::Hash::xor_( const std::string & s1 , const std::string & s2 )
{
	G_ASSERT( s1.length() == s2.length() ) ;
	std::string::const_iterator p1 = s1.begin() ;
	std::string::const_iterator p2 = s2.begin() ;
	std::string result ;
	result.reserve( s1.length() ) ;
	for( ; p1 != s1.end() ; ++p1 , ++p2 )
	{
		auto c1 = static_cast<unsigned char>(*p1) ;
		auto c2 = static_cast<unsigned char>(*p2) ;
		auto c = static_cast<unsigned char>( c1 ^ c2 ) ;
		result.append( 1U , static_cast<char>(c) ) ;
	}
	return result ;
}

std::string G::Hash::ipad( std::size_t blocksize )
{
	return std::string( blocksize , '\066' ) ; // NOLINT not return {...}
}

std::string G::Hash::opad( std::size_t blocksize )
{
	return std::string( blocksize , '\134' ) ; // NOLINT not return {...}
}

std::string G::Hash::printable( const std::string & input )
{
	std::string result ;
	result.reserve( input.length() * 2U ) ;
	const char * hex = "0123456789abcdef" ;
	const std::size_t n = input.length() ;
	for( std::size_t i = 0U ; i < n ; i++ )
	{
		auto c = static_cast<unsigned char>(input.at(i)) ;
		result.append( 1U , hex[(c>>4U)&0x0F] ) ;
		result.append( 1U , hex[(c>>0U)&0x0F] ) ;
	}
	G_ASSERT( result.size() == (input.size()*2U) ) ;
	return result ;
}

