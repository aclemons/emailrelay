//
// Copyright (C) 2001-2015 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// ggetopt.cpp
//	

#include "gdef.h"
#include "ggetopt.h"
#include "gmapfile.h"
#include "goptions.h"
#include "goptionvalue.h"
#include "goptionparser.h"
#include "gstrings.h"
#include "gstr.h"
#include "gassert.h"
#include "gdebug.h"
#include <fstream>

G::GetOpt::GetOpt( const Arg & args_in , const std::string & spec ) :
	m_spec(spec) ,
	m_args(args_in)
{
	parseArgs( m_args ) ;
}

G::GetOpt::GetOpt( const G::StringArray & args_in , const std::string & spec ) :
	m_spec(spec) ,
	m_args(args_in)
{
	parseArgs( m_args ) ;
}

void G::GetOpt::parseArgs( Arg & args )
{
	OptionParser parser( m_spec , m_map , m_errors ) ;
	size_t consumed = parser.parse( args.array() ) ;
	if( consumed != 0U )
		args.removeAt( 1U , consumed-1U ) ;
}

const G::Options & G::GetOpt::options() const
{
	return m_spec ;
}

G::StringArray G::GetOpt::errorList() const
{
	return m_errors ;
}

void G::GetOpt::reload( const G::StringArray & args_in )
{
	m_map.clear() ;
	m_errors.clear() ;
	m_args = Arg( args_in ) ;
	parseArgs( m_args ) ;
}

void G::GetOpt::addOptionsFromFile( size_t n )
{
	if( n < m_args.c() )
	{
		std::string filename = m_args.v( n ) ;
		m_args.removeAt( n ) ;

		if( !filename.empty() )
		{
			MapFile mapfile( filename ) ;
			OptionParser parser( m_spec , m_map , m_errors ) ;
			parser.parse( mapfile.map() ) ;
		}
	}
}

bool G::GetOpt::contains( char c ) const
{
	OptionValueMap::const_iterator p = m_map.find( m_spec.lookup(c) ) ;
	return p != m_map.end() && !(*p).second.is_off() ;
}

bool G::GetOpt::contains( const std::string & name ) const
{
	OptionValueMap::const_iterator p = m_map.find( name ) ;
	return p != m_map.end() && !(*p).second.is_off() ;
}

std::string G::GetOpt::value( char c ) const
{
	G_ASSERT( contains(c) ) ;
	return value( m_spec.lookup(c) ) ;
}

std::string G::GetOpt::value( const std::string & name ) const
{
	G_ASSERT( contains(name) ) ;
	OptionValueMap::const_iterator p = m_map.find( name ) ;
	OptionValue value ;
	if( p != m_map.end() )
		value = (*p).second ;
	return value.value() ;
}

G::Arg G::GetOpt::args() const
{
	return m_args ;
}

bool G::GetOpt::hasErrors() const
{
	return m_errors.size() != 0U ;
}

void G::GetOpt::showErrors( std::ostream & stream ) const
{
	showErrors( stream , m_args.prefix() + ": error" ) ;
}

void G::GetOpt::showErrors( std::ostream & stream , std::string prefix_1 , std::string prefix_2 ) const
{
	for( StringArray::const_iterator p = m_errors.begin() ; p != m_errors.end() ; ++p )
	{
		stream << prefix_1 << prefix_2 << *p << std::endl ;
	}
}

/// \file ggetopt.cpp
