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
/// \file ggetopt.cpp
///

#include "gdef.h"
#include "ggetopt.h"
#include "gmapfile.h"
#include "goptions.h"
#include "goptionvalue.h"
#include "goptionparser.h"
#include "gstr.h"
#include "gstringmap.h"
#include "gassert.h"
#include "glog.h"
#include <fstream>
#include <algorithm>

G::GetOpt::GetOpt( const Arg & args_in , const std::string & spec , std::size_t ignore_non_options ) :
	m_spec(spec) ,
	m_args(args_in)
{
	parseArgs( ignore_non_options ) ;
}

G::GetOpt::GetOpt( const Arg & args_in , const Options & spec , std::size_t ignore_non_options ) :
	m_spec(spec) ,
	m_args(args_in)
{
	parseArgs( ignore_non_options ) ;
}

G::GetOpt::GetOpt( const G::StringArray & args_in , const std::string & spec , std::size_t ignore_non_options ) :
	m_spec(spec) ,
	m_args(args_in)
{
	parseArgs( ignore_non_options ) ;
}

G::GetOpt::GetOpt( const G::StringArray & args_in , const Options & spec , std::size_t ignore_non_options ) :
	m_spec(spec) ,
	m_args(args_in)
{
	parseArgs( ignore_non_options ) ;
}

void G::GetOpt::reload( const G::StringArray & args_in , std::size_t ignore_non_options )
{
	m_map.clear() ;
	m_errors.clear() ;
	m_args = Arg( args_in ) ;
	parseArgs( ignore_non_options ) ;
}

void G::GetOpt::parseArgs( std::size_t ignore_non_options )
{
	StringArray new_args = OptionParser::parse( m_args.array() , m_spec , m_map , &m_errors , 1U , ignore_non_options ) ;
	new_args.insert( new_args.begin() , m_args.v(0U) ) ;
	m_args = Arg( new_args ) ;
}

bool G::GetOpt::addOptionsFromFile( std::size_t n , const StringArray & block )
{
	if( n < m_args.c() )
	{
		G::Path path = m_args.v( n ) ;
		if( std::find( block.begin() , block.end() , path.extension() ) != block.end() )
			return false ;
		m_args.removeAt( n ) ;
		addOptionsFromFile( path ) ;
	}
	return true ;
}

void G::GetOpt::addOptionsFromFile( std::size_t n , const std::string & varkey , const std::string & varvalue )
{
	if( n < m_args.c() )
	{
		std::string filename = m_args.v( n ) ;
		m_args.removeAt( n ) ;

		if( !filename.empty() )
		{
			if( !varkey.empty() && !varvalue.empty() && filename.find(varkey) == 0 )
				G::Str::replace( filename , varkey , varvalue ) ;
			addOptionsFromFile( filename ) ;
		}
	}
}

G::StringArray G::GetOpt::optionsFromFile( const Options & spec , const Path & filename )
{
	StringArray result ;
	StringMap map = MapFile(filename,"config").map() ;
	for( const auto & map_item : map )
	{
		const std::string & key = map_item.first ;
		const std::string & value = map_item.second ;
		if( spec.valued(key) )
			result.push_back( std::string("--").append(key).append(1U,'=').append(value) ) ;
		else
			result.push_back( std::string("--").append(key) ) ;
	}
	return result ;
}

void G::GetOpt::addOptionsFromFile( const G::Path & filename )
{
	OptionParser::parse( optionsFromFile(m_spec,filename) , m_spec , m_map , &m_errors , 0U ) ;
}

const std::vector<G::Option> & G::GetOpt::options() const
{
	return m_spec.list() ;
}

const G::OptionMap & G::GetOpt::map() const
{
	return m_map ;
}

G::StringArray G::GetOpt::errorList() const
{
	return m_errors ;
}

bool G::GetOpt::contains( char c ) const
{
	return m_map.contains( m_spec.lookup(c) ) ;
}

bool G::GetOpt::contains( const std::string & name ) const
{
	return m_map.contains( name ) ;
}

std::size_t G::GetOpt::count( const std::string & name ) const
{
	return m_map.count( name ) ;
}

std::string G::GetOpt::value( char c , const std::string & default_ ) const
{
	G_ASSERT( contains(c) ) ;
	return value( m_spec.lookup(c) , default_ ) ;
}

std::string G::GetOpt::value( const std::string & name , const std::string & default_ ) const
{
	return m_map.value( name , default_ ) ;
}

G::Arg G::GetOpt::args() const
{
	return m_args ;
}

bool G::GetOpt::hasErrors() const
{
	return !m_errors.empty() ;
}

void G::GetOpt::showErrors( std::ostream & stream ) const
{
	showErrors( stream , m_args.prefix() + ": error" ) ;
}

void G::GetOpt::showErrors( std::ostream & stream , const std::string & prefix_1 , const std::string & prefix_2 ) const
{
	for( const auto & error : m_errors )
	{
		stream << prefix_1 << prefix_2 << error << std::endl ;
	}
}

