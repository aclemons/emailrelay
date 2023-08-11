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
/// \file genvironment.cpp
///

#include "gdef.h"
#include "genvironment.h"
#include "gstr.h"
#include <algorithm>
#include <stdexcept>

G::Environment::Environment()
{
	setup() ;
}

#ifndef G_LIB_SMALL
G::Environment::Environment( const std::map<std::string,std::string> & map ) :
	m_map(map)
{
	setup() ;
}
#endif

G::Environment::Environment( const Environment & other ) :
	m_map(other.m_map)
{
	setup() ;
}

G::Environment::Environment( Environment && other ) noexcept :
	m_map(std::move(other.m_map)) ,
	m_list(std::move(other.m_list)) ,
	m_pointers(std::move(other.m_pointers)) ,
	m_block(std::move(other.m_block))
{
}

void G::Environment::swap( Environment & other ) noexcept
{
	m_map.swap( other.m_map ) ;
	m_list.swap( other.m_list ) ;
	m_pointers.swap( other.m_pointers ) ;
	std::swap( m_block , other.m_block ) ;
}

#ifndef G_LIB_SMALL
bool G::Environment::valid() const
{
	return
		m_map.size() == m_list.size() &&
		(m_list.size()+1U) == m_pointers.size() &&
		( m_list.empty() || m_pointers.at(0U) == m_list.at(0U).c_str() ) ;
}
#endif

G::Environment & G::Environment::operator=( const Environment & other )
{
	Environment(other).swap( *this ) ;
	return *this ;
}

#ifndef G_LIB_SMALL
G::Environment & G::Environment::operator=( Environment && other ) noexcept
{
	Environment(std::move(other)).swap( *this ) ;
	return *this ;
}
#endif

void G::Environment::setup()
{
	setList() ;
	setPointers() ;
	setBlock() ;
}

void G::Environment::setList()
{
	m_list.clear() ;
	m_list.reserve( m_map.size() ) ;
	StringArray keys = Str::keys( m_map ) ;
	std::sort( keys.begin() , keys.end() ) ;
	for( const auto & key : keys )
	{
		m_list.push_back( key + "=" + (*m_map.find(key)).second ) ;
	}
}

void G::Environment::setPointers()
{
	m_pointers.clear() ;
	m_pointers.reserve( m_list.size() + 1U ) ;
	for( const auto & s : m_list )
		m_pointers.push_back( const_cast<char*>(s.c_str()) ) ;
	m_pointers.push_back( nullptr ) ;
}

void G::Environment::setBlock()
{
	std::size_t n = 0U ;
	for( auto & s : m_list )
		n += (s.size()+1U) ;
	m_block.reserve( n + 1U ) ;
	for( auto & s : m_list )
	{
		m_block.append( s ) ;
		m_block.append( 1U , '\0' ) ;
	}
	m_block.append( 1U , '\0' ) ;
}

void G::Environment::add( const std::string & name , const std::string & value )
{
	if( name.find('=') != std::string::npos )
		throw Error( name ) ;
	m_map.insert( std::make_pair(name,value) ) ;
	setup() ;
}

#ifndef G_LIB_SMALL
void G::Environment::set( const std::string & name , const std::string & value )
{
	m_map[name] = value ;
	setup() ;
}
#endif

char ** G::Environment::v() const noexcept
{
	return const_cast<char**>(&m_pointers[0]) ;
}

#ifndef G_LIB_SMALL
const char * G::Environment::ptr() const noexcept
{
	return m_block.data() ;
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

