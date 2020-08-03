//
// Copyright (C) 2001-2020 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// goptions.cpp
//

#include "gdef.h"
#include "gstr.h"
#include "gassert.h"
#include "goptions.h"
#include "genvironment.h"
#include <algorithm>

G::Options::Options()
= default;

G::Options::Options( const std::string & spec , char sep_major , char sep_minor , char escape )
{
	parseSpec( spec , sep_major , sep_minor , escape ) ;
	std::sort( m_names.begin() , m_names.end() ) ;
}

void G::Options::parseSpec( const std::string & spec , char sep_major , char sep_minor , char escape )
{
	// split into separate options
	StringArray spec_items ;
	spec_items.reserve( 40U ) ;
	std::string ws_major( 1U , sep_major ) ;
	Str::splitIntoFields( spec , spec_items , ws_major , escape , false ) ;

	// for each option
	for( auto & spec_item : spec_items )
	{
		// split into separate fields
		if( spec_item.empty() ) continue ;
		StringArray inner_part ;
		inner_part.reserve( 7U ) ;
		std::string ws_minor( 1U , sep_minor ) ;
		Str::splitIntoFields( spec_item , inner_part , ws_minor , escape ) ;
		if( inner_part.size() != 7U )
		{
			std::ostringstream ss ;
			ss << "\"" << spec_item << "\" (" << ws_minor << ")" ;
			throw InvalidSpecification( ss.str() ) ;
		}

		unsigned int level = Str::toUInt( inner_part[6U] ) ;
		std::string short_form = inner_part[0] ;
		char c = short_form.empty() ? '\0' : short_form.at(0U) ;

		addSpec( inner_part[1U] , c , inner_part[2U] ,
			inner_part[3U] , inner_part[4U] , inner_part[5U] , level ) ;
	}
}

void G::Options::addSpec( const std::string & name , char c ,
	const std::string & description , const std::string & description_extra ,
	const std::string & value_multiplicity , const std::string & value_description , unsigned int level )
{
	std::pair<Map::iterator,bool> rc = m_map.insert( std::make_pair( name ,
		Option(c,name,description,description_extra,value_multiplicity,value_description,level) ) ) ;
	if( ! rc.second )
		throw InvalidSpecification("duplication") ;
	m_names.push_back( name ) ; // defines the display order
}

bool G::Options::defaulting( const std::string & name ) const
{
	auto p = m_map.find( name ) ;
	return p == m_map.end() ? false : ( (*p).second.value_multiplicity == Option::Multiplicity::zero_or_one ) ;
}

bool G::Options::valued( char c ) const
{
	return valued( lookup(c) ) ;
}

bool G::Options::valued( const std::string & name ) const
{
	auto p = m_map.find( name ) ;
	return p == m_map.end() ? false : ( (*p).second.value_multiplicity != Option::Multiplicity::zero ) ;
}

bool G::Options::unvalued( const std::string & name ) const
{
	return valid(name) && !valued(name) ;
}

bool G::Options::multivalued( char c ) const
{
	return multivalued( lookup(c) ) ;
}

bool G::Options::multivalued( const std::string & name ) const
{
	auto p = m_map.find( name ) ;
	return p == m_map.end() ? false : ( (*p).second.value_multiplicity == Option::Multiplicity::many ) ;
}

bool G::Options::visible( const std::string & name , Level level , bool exact ) const
{
	auto p = m_map.find( name ) ;
	if( p == m_map.end() ) return false ;
	return
		exact ?
			( !(*p).second.hidden && (*p).second.level == level.level ) :
			( !(*p).second.hidden && (*p).second.level <= level.level ) ;
}

bool G::Options::valid( const std::string & name ) const
{
	return m_map.find(name) != m_map.end() ;
}

std::string G::Options::lookup( char c ) const
{
	for( auto p = m_map.begin() ; c != '\0' && p != m_map.end() ; ++p )
	{
		if( (*p).second.c == c )
			return (*p).second.name ;
	}
	return std::string() ;
}

const G::StringArray & G::Options::names() const
{
	return m_names ;
}

// --

std::size_t G::Options::widthDefault()
{
	return Str::toUInt( Environment::get("COLUMNS",std::string()) , "79" ) ;
}

std::size_t G::Options::widthFloor( std::size_t w )
{
	return (w != 0U && w < 50U) ? 50U : w ;
}

G::Options::Layout G::Options::layoutDefault()
{
	return Layout( 30U ) ;
}

std::string G::Options::introducerDefault()
{
	return "usage: " ;
}

G::Options::Level G::Options::levelDefault()
{
	return Level(99U) ;
}

std::string G::Options::usageSummaryPartOne( Level level ) const
{
	// summarise the single-character switches, excluding those which take a value
	std::ostringstream ss ;
	bool first = true ;
	for( const auto & name : m_names )
	{
		auto spec_p = m_map.find( name ) ;
		if( (*spec_p).second.c != '\0' && !valued(name) && visible(name,level,false) )
		{
			if( first )
				ss << "[-" ;
			first = false ;
			ss << (*spec_p).second.c ;
		}
	}

	std::string s = ss.str() ;
	if( s.length() ) s.append( "] " ) ;
	return s ;
}

std::string G::Options::usageSummaryPartTwo( Level level ) const
{
	std::ostringstream ss ;
	const char * sep = "" ;
	for( const auto & name : m_names )
	{
		if( visible(name,level,false) )
		{
			auto spec_p = m_map.find( name ) ;
			ss << sep << "[" ;
			if( (*spec_p).second.name.length() )
			{
				ss << "--" << (*spec_p).second.name ;
			}
			else
			{
				G_ASSERT( (*spec_p).second.c != '\0' ) ;
				ss << "-" << (*spec_p).second.c ;
			}
			if( (*spec_p).second.value_multiplicity != Option::Multiplicity::zero )
			{
				std::string vd = (*spec_p).second.value_description ;
				if( vd.empty() ) vd = "value" ;
				ss << "=<" << vd << ">" ;
			}
			ss << "]" ;
			sep = " " ;
		}
	}
	return ss.str() ;
}

std::string G::Options::usageHelp( Level level , const Layout & layout_in , bool exact , bool extra ) const
{
	Layout layout( layout_in ) ;
	layout.width = widthFloor( layout_in.width ) ;
	return usageHelpCore( "  " , level , layout , exact , extra ) ;
}

std::string G::Options::usageHelpCore( const std::string & prefix , Level level ,
	const Layout & layout , bool exact , bool extra ) const
{
	std::string result ;
	for( const auto & name : m_names )
	{
		if( visible(name,level,exact) )
		{
			auto spec_p = m_map.find( name ) ;
			std::string line( prefix ) ;
			if( (*spec_p).second.c != '\0' )
			{
				line.append( "-" ) ;
				line.append( 1U , (*spec_p).second.c ) ;
				if( (*spec_p).second.name.length() )
					line.append( ", " ) ;
			}
			if( (*spec_p).second.name.length() )
			{
				line.append( "--" ) ;
				line.append( (*spec_p).second.name ) ;
			}
			if( (*spec_p).second.value_multiplicity != Option::Multiplicity::zero )
			{
				bool defaulting = (*spec_p).second.value_multiplicity == Option::Multiplicity::zero_or_one ;
				std::string vd = (*spec_p).second.value_description ;
				if( vd.empty() ) vd = "value" ;
				if( defaulting ) line.append( "[" ) ;
				line.append( "=<" ) ;
				line.append( vd ) ;
				line.append( ">" ) ;
				if( defaulting ) line.append( "]" ) ;
			}
			line.append( 1U , ' ' ) ;

			if( !layout.separator.empty() )
				line.append( layout.separator ) ;
			else if( line.length() < layout.column )
				line.append( layout.column-line.length() , ' ' ) ;

			line.append( (*spec_p).second.description ) ;
			if( extra )
				line.append( (*spec_p).second.description_extra ) ;

			if( layout.width )
			{
				std::string indent = layout.indent.empty() ? std::string(layout.column,' ') : layout.indent ;
				line = Str::wrap( line , "" , indent , layout.width ) ;
			}
			else
			{
				line.append( 1U , '\n' ) ;
			}

			result.append( line ) ;
		}
	}
	return result ;
}

void G::Options::showUsage( std::ostream & stream , const std::string & exe , const std::string & args ,
	const std::string & introducer , Level level , const Layout & layout , bool extra ) const
{
	stream
		<< usageSummary(exe,args,introducer,level,layout.width) << std::endl
		<< usageHelp(level,layout,false,extra) ;
}

std::string G::Options::usageSummary( const std::string & exe , const std::string & args ,
	const std::string & introducer , Level level , std::size_t width ) const
{
	std::string s = introducer + exe + " " + usageSummaryPartOne(level) + usageSummaryPartTwo(level) + (args.empty()||args.at(0U)==' '?"":" ") + args ;
	std::string indent( 2U , ' ' ) ; // or from OptionsLayout ?
	return width == 0U ? s : Str::wrap( s , "" , indent , widthFloor(width) ) ;
}

// ==

G::Options::Option::Option( char c_ , const std::string & name_ , const std::string & description_ ,
	const std::string & description_extra_ , const std::string & value_multiplicity_ ,
	const std::string & vd_ , unsigned int level_ ) :
		c(c_) ,
		name(name_) ,
		description(description_) ,
		description_extra(description_extra_) ,
		value_multiplicity(decode(value_multiplicity_)) ,
		hidden(description_.empty()||level_==0U) ,
		value_description(vd_) ,
		level(level_)
{
}

G::Options::Option::Multiplicity G::Options::Option::decode( const std::string & s )
{
	if( s == "0" )
		return Multiplicity::zero ;
	else if( s == "01" )
		return Multiplicity::zero_or_one ;
	else if( s == "1" )
		return Multiplicity::one ;
	else if( s == "2" )
		return Multiplicity::many ;
	else
		throw InvalidSpecification( "multiplicity" ) ;
}

/// \file goptions.cpp
