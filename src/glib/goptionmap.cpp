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
#include <algorithm>

void G::OptionMap::insert( const Map::value_type & value )
{
	m_map.insert( value ) ;
}

G::OptionMap::Range G::OptionMap::findRange( string_view key ) const
{
	return m_map.equal_range( sv_to_string(key) ) ; // or c++14 'generic associative lookup' of string_view
}

G::OptionMap::Map::iterator G::OptionMap::findFirst( string_view key )
{
	return m_map.find( sv_to_string(key) ) ; // or c++14 'generic associative lookup' of string_view
}

G::OptionMap::const_iterator G::OptionMap::find( string_view key ) const
{
	auto pair = findRange( key ) ;
	return pair.first == pair.second ? m_map.end() : pair.first ;
}

void G::OptionMap::replace( string_view key , const std::string & value )
{
	auto pair = findRange( key ) ;
	if( pair.first != pair.second )
		m_map.erase( pair.first , pair.second ) ;
	m_map.insert( Map::value_type(sv_to_string(key),OptionValue(value)) ) ;
}

void G::OptionMap::increment( string_view key )
{
	auto p = findFirst( key ) ;
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

void G::OptionMap::clear()
{
	m_map.clear() ;
}

bool G::OptionMap::contains( string_view key ) const
{
	auto range = findRange( key ) ;
	for( auto p = range.first ; p != range.second ; ++p )
	{
		if( (*p).second.isOff() )
			continue ;
		return true ;
	}
	return false ;
}

std::size_t G::OptionMap::count( string_view key ) const
{
	std::size_t n = 0U ;
	auto pair = findRange( key ) ;
	for( auto p = pair.first ; p != pair.second ; ++p )
		n += (*p).second.count() ;
	return n ;
}

std::string G::OptionMap::value( string_view key , string_view default_ ) const
{
	auto range = findRange( key ) ;
	if( range.first == range.second )
		return sv_to_string(default_) ;
	else
		return join( range.first , range.second , default_ ) ;
}

std::string G::OptionMap::join( Map::const_iterator p , Map::const_iterator end , string_view off_value ) const
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
			return sv_to_string(off_value) ;
	}
	return result ;
}

