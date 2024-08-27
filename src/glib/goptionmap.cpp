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
/// \file goptionmap.cpp
///

#include "gdef.h"
#include "goptionmap.h"
#include "gstringfield.h"
#include "gassert.h"
#include <algorithm>

void G::OptionMap::insert( const Map::value_type & value )
{
	m_map.insert( value ) ;
}

G::OptionMap::Range G::OptionMap::findRange( std::string_view key ) const noexcept
{
	// 'generic associative lookup' is c++14 and c++11 equal_range()
	// is not noexcept, so do it ourselves
	auto first = std::find_if( m_map.cbegin() , m_map.cend() ,
		[key](const Map::value_type & v_){return key == v_.first ;} ) ;
	auto second = std::find_if( first , m_map.cend() ,
		[key](const Map::value_type & v_){return key != v_.first ;} ) ;
	return {first,second} ;
}

G::OptionMap::Map::iterator G::OptionMap::findFirst( std::string_view key ) noexcept
{
	// as above
	auto result = std::find_if( m_map.begin() , m_map.end() ,
		[key](const Map::value_type & v_){return key == v_.first ;} ) ;
	G_ASSERT( result == m_map.find(sv_to_string(key)) ) ;
	return result ;
}

G::OptionMap::const_iterator G::OptionMap::find( std::string_view key ) const noexcept
{
	auto pair = findRange( key ) ;
	return pair.first == pair.second ? m_map.end() : pair.first ;
}

void G::OptionMap::replace( std::string_view key , const std::string & value )
{
	auto pair = findRange( key ) ;
	if( pair.first != pair.second )
		m_map.erase( pair.first , pair.second ) ;
	m_map.insert( Map::value_type(sv_to_string(key),OptionValue(value)) ) ;
}

void G::OptionMap::increment( std::string_view key ) noexcept
{
	auto p = findFirst( key ) ;
	if( p != m_map.end() )
	{
		Map::value_type & value = *p ;
		value.second.increment() ;
	}
}

G::OptionMap::const_iterator G::OptionMap::begin() const noexcept
{
	return m_map.cbegin() ;
}

#ifndef G_LIB_SMALL
G::OptionMap::const_iterator G::OptionMap::cbegin() const noexcept
{
	return begin() ;
}
#endif

G::OptionMap::const_iterator G::OptionMap::end() const noexcept
{
	return m_map.cend() ;
}

#ifndef G_LIB_SMALL
G::OptionMap::const_iterator G::OptionMap::cend() const noexcept
{
	return end() ;
}
#endif

void G::OptionMap::clear()
{
	m_map.clear() ;
}

bool G::OptionMap::contains( std::string_view key ) const noexcept
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

bool G::OptionMap::contains( const char * key ) const noexcept
{
	return contains( std::string_view(key) ) ;
}

bool G::OptionMap::contains( const std::string & key ) const noexcept
{
	return contains( std::string_view(key) ) ;
}

std::size_t G::OptionMap::count( std::string_view key ) const noexcept
{
	std::size_t n = 0U ;
	auto pair = findRange( key ) ;
	for( auto p = pair.first ; p != pair.second ; ++p )
		n += (*p).second.count() ;
	return n ;
}

std::string G::OptionMap::value( std::string_view key , std::string_view default_ ) const
{
	auto range = findRange( key ) ;
	if( range.first == range.second )
		return sv_to_string(default_) ;
	else
		return join( range.first , range.second , default_ ) ;
}

std::string G::OptionMap::join( Map::const_iterator p , Map::const_iterator end , std::string_view off_value )
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

unsigned int G::OptionMap::number( std::string_view key , unsigned int default_ ) const noexcept
{
	G_ASSERT( !G::Str::isUInt("") ) ;
	auto p = find( key ) ;
	if( p == m_map.end() )
	{
		return default_ ;
	}
	else
	{
		static_assert( std::is_same<decltype(*p),const Map::value_type&>::value , "" ) ;
		//static_assert( noexcept(*p) , "" ) ; // not declared noexcept by msvc
		const Map::value_type & value = *p ; // noexcept in practice
		return G::Str::toUInt( value.second.valueref() , default_ ) ;
	}
}

