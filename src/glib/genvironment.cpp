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
/// \file genvironment.cpp
///

#include "gdef.h"
#include "genvironment.h"
#include "gassert.h"
#include "gstr.h"
#include <string>
#include <algorithm>
#include <numeric>
#include <stdexcept>

G::Environment::Environment( const Map & map ) :
	m_map(map)
{
	sanitise( m_map ) ;
}

void G::Environment::sanitise( Map & map )
{
	for( auto p = map.begin() ; p != map.end() ; )
	{
		if( (*p).first.empty() || (*p).first.find('\0') != std::string::npos || (*p).second.find('\0') != std::string::npos )
			p = map.erase( p ) ;
		else
			++p ;
	}
}

#ifndef G_LIB_SMALL
bool G::Environment::add( std::string_view key , std::string_view value )
{
	if( !key.empty() && key.find('\0') == std::string::npos && value.find('\0') == std::string::npos )
	{
		m_map[sv_to_string(key)] = sv_to_string(value) ;
		return true ;
	}
	else
	{
		return false ;
	}
}
#endif

bool G::Environment::contains( const std::string & name ) const
{
	return m_map.find(name) != m_map.end() ;
}

#ifndef G_LIB_SMALL
std::string G::Environment::value( const std::string & name , const std::string & default_ ) const
{
	return contains(name) ? (*m_map.find(name)).second : default_ ;
}
#endif

std::string G::Environment::block() const
{
	std::size_t n = std::accumulate( m_map.begin() , m_map.end() , std::size_t(0U) ,
		[](std::size_t n_,const Map::value_type &p_){return n_+p_.first.size()+p_.second.size()+2U;} ) ;
	std::string result ;
	result.reserve( n+1U ) ;
	for( const auto & p : m_map )
		result.append(p.first).append(1U,'=').append(p.second).append(1U,'\0') ;
	result.append( 1U , '\0' ) ;
	return result ;
}

#ifndef G_LIB_SMALL
std::wstring G::Environment::block( std::wstring (*fn)(std::string_view) ) const
{
	std::size_t n = std::accumulate( m_map.begin() , m_map.end() , std::size_t(0U) ,
		[](std::size_t n_,const Map::value_type &p_){return n_+p_.first.size()+p_.second.size()+2U;} ) ;
	std::wstring result ;
	result.reserve( n+1U ) ;
	for( const auto & p : m_map )
		result.append(fn(std::string(p.first).append(1U,L'=').append(p.second))).append(1U,L'\0') ;
	result.append( 1U , L'\0' ) ;
	return result ;
}
#endif

std::vector<char*> G::Environment::array( const std::string & block )
{
	G_ASSERT( block.size() >= 2U && block[block.size()-1U] == 0 && block[block.size()-2U] == 0 ) ;
	std::vector<char*> result ;
	if( block.size() >= 2U && block[block.size()-1U] == 0 && block[block.size()-2U] == 0 )
	{
		result.reserve( std::count( block.begin() , block.end() , 0 ) ) ;
		const char * const end = block.data() + block.size() ;
		for( const char * p = block.data() ; p < end && *p ; p += (std::strlen(p)+1U) )
			result.push_back( const_cast<char*>(p) ) ;
	}
	result.push_back( nullptr ) ;
	return result ;
}

