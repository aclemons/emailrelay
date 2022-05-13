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
/// \file goptionmap.cpp
///

#include "gdef.h"
#include "goptionmap.h"

void G::OptionMap::insert( const Map::value_type & value )
{
	m_map.insert( value ) ;
}

void G::OptionMap::replace( const std::string & key , const std::string & value )
{
	auto pair = m_map.equal_range( key ) ;
	if( pair.first != pair.second )
		m_map.erase( pair.first , pair.second ) ;
	m_map.insert( Map::value_type(key,OptionValue(value)) ) ;
}

void G::OptionMap::increment( const std::string & key )
{
	auto p = m_map.find( key ) ;
	if( p != m_map.end() )
		(*p).second.increment() ;
}

G::OptionMap::const_iterator G::OptionMap::begin() const
{
	return m_map.begin() ;
}

G::OptionMap::const_iterator G::OptionMap::cbegin() const
{
	return begin() ;
}

G::OptionMap::const_iterator G::OptionMap::end() const
{
	return m_map.end() ;
}

G::OptionMap::const_iterator G::OptionMap::cend() const
{
	return end() ;
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
	for( auto p = m_map.find(key) ; p != end && (*p).first == key ; ++p )
	{
		if( (*p).second.isOff() )
			continue ;
		return true ;
	}
	return false ;
}

std::size_t G::OptionMap::count( const std::string & key ) const
{
	std::size_t n = 0U ;
	auto pair = m_map.equal_range( key ) ;
	for( auto p = pair.first ; p != pair.second ; ++p )
		n += (*p).second.count() ;
	return n ;
}

std::string G::OptionMap::value( const char * key , const char * default_ ) const
{
	return value( std::string(key) , default_ ? std::string(default_) : std::string() ) ;
}

std::string G::OptionMap::value( const std::string & key , const std::string & default_ ) const
{
	auto range = m_map.equal_range( key ) ;
	if( range.first == range.second )
		return default_ ;
	else
		return join( range.first , range.second , default_ ) ;
}

std::string G::OptionMap::join( Map::const_iterator p , Map::const_iterator end , const std::string & off_value ) const
{
	std::string result ;
	const char * sep = "" ;
	for( ; p != end ; ++p )
	{
		result.append( sep ) ; sep = "," ;
		result.append( (*p).second.value() ) ;
		if( (*p).second.isOn() )
			return (*p).second.value() ;
		if( (*p).second.isOff() )
			return off_value ;
	}
	return result ;
}

bool G::OptionMap::compare( const Map::value_type & pair1 , const Map::value_type & pair2 )
{
	return pair1.first < pair2.first ;
}

