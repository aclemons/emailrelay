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
// goptionmap.cpp
//

#include "gdef.h"
#include "goptionmap.h"

G::OptionMap::OptionMap()
{
}

void G::OptionMap::insert( const Map::value_type & value )
{
	m_map.insert( value ) ;
}

G::OptionMap::const_iterator G::OptionMap::begin() const
{
	return m_map.begin() ;
}

G::OptionMap::const_iterator G::OptionMap::end() const
{
	return m_map.end() ;
}

G::OptionMap::const_iterator G::OptionMap::find( const std::string & key ) const
{
	return m_map.find( key ) ;
}

void G::OptionMap::clear()
{
	m_map.clear() ;
}

bool G::OptionMap::contains( const char * key ) const
{
	return contains( std::string(key) ) ;
}

bool G::OptionMap::contains( const std::string & key ) const
{
	const Map::const_iterator end = m_map.end() ;
	for( Map::const_iterator p = m_map.find(key) ; p != end && (*p).first == key ; ++p )
	{
		if( (*p).second.valued() || !(*p).second.is_off() )
			return true ;
	}
	return false ;
}

size_t G::OptionMap::count( const std::string & key ) const
{
	size_t n = 0U ;
	const Map::const_iterator end = m_map.end() ;
	for( Map::const_iterator p = m_map.find(key) ; p != end && (*p).first == key ; ++p )
		n++ ;
	return n ;
}

std::string G::OptionMap::value( const char * key , const char * default_ ) const
{
	return value( std::string(key) , default_ ? std::string(default_) : std::string() ) ;
}

std::string G::OptionMap::value( const std::string & key , const std::string & default_ ) const
{
	Map::const_iterator p = m_map.find( key ) ;
	if( p == m_map.end() || !(*p).second.valued() )
		return default_ ;
	else if( count(key) == 1U )
		return (*p).second.value() ;
	else
		return join( p , std::upper_bound(p,m_map.end(),*p,m_map.value_comp()) ) ;
}

std::string G::OptionMap::join( Map::const_iterator p , Map::const_iterator end ) const
{
	std::string result ;
	for( const char * sep = "" ; p != end ; ++p )
	{
		if( (*p).second.valued() )
		{
			result.append( sep ) ; sep = "," ;
			result.append( (*p).second.value() ) ;
		}
	}
	return result ;
}
/// \file goptionmap.cpp
