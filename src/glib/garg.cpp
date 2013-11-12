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
// garg.cpp
//

#include "gdef.h"
#include "garg.h"
#include "gpath.h"
#include "gstr.h"
#include "gdebug.h"
#include "gassert.h"
#include <cstring>

G::Arg::Arg( int argc , char *argv[] )
{
	G_ASSERT( argc > 0 ) ;
	G_ASSERT( argv != NULL ) ;
	for( int i = 0 ; i < argc ; i++ )
		m_array.push_back( argv[i] ) ;

	setExe() ;
	setPrefix() ;
}

G::Arg::~Arg()
{ 
}

G::Arg::Arg()
{
	setExe() ;
	setPrefix() ;
	// now use parse()
}

G::Arg::Arg( const Arg & other ) :
	m_array(other.m_array) ,
	m_prefix(other.m_prefix)
{
}

G::Arg & G::Arg::operator=( const Arg & rhs )
{
	if( this != &rhs )
	{
		m_array = rhs.m_array ;
		m_prefix = rhs.m_prefix ;
	}
	return *this ;
}

void G::Arg::setPrefix()
{
	G_ASSERT( m_array.size() > 0U ) ;
	Path path( m_array[0U] ) ;
	path.removeExtension() ;
	m_prefix = path.basename() ;
}

bool G::Arg::contains( const std::string & sw , size_type sw_args , bool cs ) const
{
	return find( cs , sw , sw_args , NULL ) ;
}

bool G::Arg::find( bool cs , const std::string & sw , size_type sw_args , size_type * index_p ) const
{
	for( size_type i = 1 ; i < m_array.size() ; i++ ) // start from v[1]
	{
		if( match(cs,sw,m_array[i]) && (i+sw_args) < m_array.size() )
		{
			if( index_p != NULL )
				*index_p = i ;
			return true ;
		}
	}
	return false ;
}

bool G::Arg::match( bool cs , const std::string & s1 , const std::string & s2 )
{
	return
		cs ?
			s1 == s2 :
			Str::upper(s1) == Str::upper(s2) ;
}

bool G::Arg::remove( const std::string & sw , size_type sw_args )
{
	size_type i = 0U ;
	const bool found = find( true , sw , sw_args , &i ) ;
	if( found )
		removeAt( i , sw_args ) ;
	return found ;
}

void G::Arg::removeAt( size_type sw_index , size_type sw_args )
{
	G_ASSERT( sw_index > 0U && sw_index < m_array.size() ) ;
	if( sw_index > 0U && sw_index < m_array.size() )
	{
		StringArray::iterator p = m_array.begin() ; 
		for( size_type i = 0U ; i < sw_index ; i++ ) ++p ; // (rather than cast)
		p = m_array.erase( p ) ;
		for( size_type i = 0U ; i < sw_args && p != m_array.end() ; i++ )
			p = m_array.erase( p ) ;
	}
}

G::Arg::size_type G::Arg::index( const std::string & sw , size_type sw_args ) const
{
	size_type i = 0U ;
	const bool found = find( true , sw , sw_args , &i ) ;
	return found ? i : 0U ;
}

G::Arg::size_type G::Arg::c() const
{
	return m_array.size() ;
}

std::string G::Arg::v( size_type i ) const
{
	G_ASSERT( i < m_array.size() ) ;
	return m_array[i] ;
}

std::string G::Arg::prefix() const
{
	return m_prefix ;
}

const char * G::Arg::prefix( char * argv [] ) // throw()
{
	const char * exe = argv[0] ;
	const char * p1 = std::strrchr( exe , '/' ) ;
	const char * p2 = std::strrchr( exe , '\\' ) ;
	p1 = p1 ? (p1+1U) : exe ;
	p2 = p2 ? (p2+1U) : exe ;
	return p1 > p2 ? p1 : p2 ;
}

/// \file garg.cpp
