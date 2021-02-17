//
// Copyright (C) 2001-2021 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file goptions.cpp
///

#include "gdef.h"
#include "gstr.h"
#include "gstringwrap.h"
#include "ggettext.h"
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
	Str::splitIntoFields( spec , spec_items , {&sep_major,1U} , escape , false ) ;

	// for each option
	for( auto & spec_item : spec_items )
	{
		// split into separate fields
		if( spec_item.empty() ) continue ;
		StringArray inner_parts ;
		inner_parts.reserve( 7U ) ;
		Str::splitIntoFields( spec_item , inner_parts , {&sep_minor,1U} , escape ) ;
		if( inner_parts.size() != 7U )
		{
			std::ostringstream ss ;
			ss << "[" << spec_item << "] (" << sep_minor << ")" ;
			throw InvalidSpecification( ss.str() ) ;
		}

		add( inner_parts ) ;
	}
}

void G::Options::add( StringArray spec_parts )
{
	if( spec_parts.size() != 7U )
		throw InvalidSpecification( "[" + G::Str::join(",",spec_parts) + "]" ) ;

	unsigned int level = Str::toUInt( spec_parts[6U] ) ;
	std::string short_form = spec_parts[0] ;
	char c = short_form.empty() ? '\0' : short_form.at(0U) ;

	addImp( spec_parts[1U] , c , spec_parts[2U] ,
		spec_parts[3U] , spec_parts[4U] , spec_parts[5U] , level ) ;
}

void G::Options::add( StringArray spec_parts , char sep , char escape )
{
	if( spec_parts.size() != 6U )
		throw InvalidSpecification( "[" + G::Str::join(",",spec_parts) + "]" ) ;

	StringArray sub_parts ;
	sub_parts.reserve( 2U ) ;
	Str::splitIntoFields( spec_parts[2U] , sub_parts , {&sep,1U} , escape ) ;
	if( sub_parts.empty() || sub_parts.size() > 2U )
		throw InvalidSpecification( G::Str::join(",",spec_parts) ) ;

	spec_parts[2U] = sub_parts[0U] ;
	spec_parts.insert( spec_parts.begin()+3U , sub_parts.size() == 2U ? sub_parts[1U] : std::string() ) ;

	add( spec_parts ) ;
}

void G::Options::addImp( const std::string & name , char c ,
	const std::string & description , const std::string & description_extra ,
	const std::string & value_multiplicity , const std::string & value_description , unsigned int level )
{
	std::pair<Map::iterator,bool> rc = m_map.insert( std::make_pair( name ,
		Option(c,name,description,description_extra,value_multiplicity,value_description,level) ) ) ;
	if( ! rc.second )
		throw InvalidSpecification( "duplication" ) ;
	m_names.insert( std::lower_bound(m_names.begin(),m_names.end(),name) , name ) ;
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

bool G::Options::visible( const std::string & name , unsigned int level , bool level_exact ) const
{
	auto p = m_map.find( name ) ;
	if( p == m_map.end() ) return false ;
	return
		level_exact ?
			( !(*p).second.hidden && (*p).second.level == level ) :
			( !(*p).second.hidden && (*p).second.level <= level ) ;
}

bool G::Options::visible( const std::string & name ) const
{
	return visible( name , 99U , false ) ;
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

namespace G
{
	namespace OptionsLayoutImp
	{
		unsigned int widthDefault() ;
	}
}

G::OptionsLayout::OptionsLayout() :
	column(30U) ,
	width(0U) ,
	width2(0U) ,
	margin(2U) ,
	level(99U) ,
	level_exact(false) ,
	extra(false) ,
	alt_usage(false)
{
	width = width2 = OptionsLayoutImp::widthDefault() ;
}

G::OptionsLayout::OptionsLayout( std::size_t column_ ) :
	column(column_) ,
	width(0U) ,
	width2(0U) ,
	margin(2U) ,
	level(99U) ,
	level_exact(false) ,
	extra(false) ,
	alt_usage(false)
{
	width = width2 = OptionsLayoutImp::widthDefault() ;
}

G::OptionsLayout::OptionsLayout( std::size_t column_ , std::size_t width_ ) :
	column(column_) ,
	width(width_) ,
	width2(width_) ,
	margin(2U) ,
	level(99U) ,
	level_exact(false) ,
	extra(false) ,
	alt_usage(false)
{
	width = width2 = OptionsLayoutImp::widthDefault() ;
}

unsigned int G::OptionsLayoutImp::widthDefault()
{
	return Str::toUInt( Environment::get("COLUMNS",std::string()) , "79" ) ;
}

// --

std::string G::Options::usageSummaryPartOne( const Layout & layout ) const
{
	// summarise the single-character switches, excluding those which take a value
	std::ostringstream ss ;
	bool first = true ;
	for( const auto & name : m_names )
	{
		auto spec_p = m_map.find( name ) ;
		if( (*spec_p).second.c != '\0' && !valued(name) && visible(name,layout.level,layout.level_exact) )
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

std::string G::Options::usageSummaryPartTwo( const Layout & layout ) const
{
	std::ostringstream ss ;
	const char * sep = "" ;
	for( const auto & name : m_names )
	{
		if( visible(name,layout.level,layout.level_exact) )
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

std::string G::Options::usageHelpSyntax( Map::const_iterator spec_p ) const
{
	std::string syntax ;
	if( (*spec_p).second.c != '\0' )
	{
		syntax.append( "-" ) ;
		syntax.append( 1U , (*spec_p).second.c ) ;
		if( (*spec_p).second.name.length() )
			syntax.append( ", " ) ;
	}
	if( (*spec_p).second.name.length() )
	{
		syntax.append( "--" ) ;
		syntax.append( (*spec_p).second.name ) ;
	}
	if( (*spec_p).second.value_multiplicity != Option::Multiplicity::zero )
	{
		bool defaulting = (*spec_p).second.value_multiplicity == Option::Multiplicity::zero_or_one ;
		std::string vd = (*spec_p).second.value_description ;
		if( vd.empty() ) vd = "value" ;
		if( defaulting ) syntax.append( "[" ) ;
		syntax.append( "=<" ) ;
		syntax.append( vd ) ;
		syntax.append( ">" ) ;
		if( defaulting ) syntax.append( "]" ) ;
	}
	syntax.append( 1U , ' ' ) ;
	return syntax ;
}

std::string G::Options::usageHelpDescription( Map::const_iterator spec_p , const Layout & layout ) const
{
	std::string description = (*spec_p).second.description ;
	if( layout.extra )
		description.append( (*spec_p).second.description_extra ) ;
	return description ;
}

std::string G::Options::usageHelpSeparator( const Layout & layout , std::size_t syntax_length ) const
{
	std::string separator ;
	if( !layout.separator.empty() )
		separator = layout.separator ;
	else if( (layout.margin+syntax_length) > layout.column )
		separator = std::string( 1U , ' ' ) ;
	else
		separator = std::string( layout.column-syntax_length-layout.margin , ' ' ) ;
	return separator ;
}

std::string G::Options::usageHelpWrap( const Layout & layout , const std::string & line_in ,
	const std::string & margin ) const
{
	std::string line( line_in ) ;
	std::size_t wrap_width = layout.width>layout.margin ? (layout.width-layout.margin) : 1U ;
	bool separator_is_tab = layout.separator.length() == 1U && layout.separator.at(0U) == '\t' ;
	if( separator_is_tab )
	{
		std::string prefix_other = std::string(layout.margin,' ').append(1U,'\t') ;
		line = margin + StringWrap::wrap( line , std::string() , prefix_other ,
			wrap_width , layout.width2 , true ) ;
	}
	else if( !layout.separator.empty() )
	{
		// wrap with a one-space indent for wrapped descriptions wrt. the syntax
		if( line.length() > layout.width )
		{
			std::string prefix_other( layout.margin+1U , ' ' ) ;
			line = margin + StringWrap::wrap( line , std::string() , prefix_other ,
				wrap_width , layout.width2 , true ) ;
		}
	}
	else
	{
		std::string prefix_other( layout.column , ' ' ) ;
		line = margin + StringWrap::wrap( line , std::string() , prefix_other ,
			wrap_width , layout.width2 , true ) ;
	}
	return line ;
}

std::size_t G::Options::longestSubLine( const std::string & s )
{
	std::size_t result = 0U ;
	std::size_t n = 0U ;
	for( auto c : s )
	{
		if( c == '\n' )
		{
			if( n > result )
				result = n ;
			n = 0 ;
		}
		else
		{
			n++ ;
		}
	}
	return result ;
}

std::string G::Options::usageHelp( const Layout & layout ) const
{
	std::string result = usageHelpImp( layout ) ;

	// check for width overflow even after wrapping, which can
	// happen as 'layout.width' shrinks towards 'layout.column'
	if( layout.width && layout.column && layout.separator.empty() &&
		layout.width <= (layout.column+20) )
	{
		std::size_t longest = longestSubLine( result ) ;
		if( longest > layout.width )
		{
			// give up on columns and make the description part wrap
			// onto a completely new line by setting a huge separator
			Layout new_layout( layout ) ;
			new_layout.separator = std::string( layout.column+layout.column , ' ' ) ;
			result = usageHelpImp( new_layout ) ;
		}
	}

	return result ;
}

std::string G::Options::usageHelpImp( const Layout & layout ) const
{
	std::string result ;
	for( const auto & name : m_names )
	{
		if( visible(name,layout.level,layout.level_exact) )
		{
			auto spec_p = m_map.find( name ) ;

			std::string margin( layout.margin , ' ' ) ;
			std::string syntax = usageHelpSyntax( spec_p ) ;
			std::string description = usageHelpDescription( spec_p , layout ) ;
			std::string separator = usageHelpSeparator( layout , syntax.length() ) ;

			std::string line ;
			line.append(margin).append(syntax).append(separator).append(description) ;

			if( layout.width )
				line = usageHelpWrap( layout , line , margin ) ;

			line.append( 1U , '\n' ) ;
			result.append( line ) ;
		}
	}
	return result ;
}

void G::Options::showUsage( const Layout & layout , std::ostream & stream ,
	const std::string & exe , const std::string & args ) const
{
	stream
		<< usageSummary(layout,exe,args) << std::endl
		<< std::endl
		<< usageHelp(layout) ;
}

std::string G::Options::usageSummary( const Layout & layout ,
	const std::string & exe , const std::string & args ) const
{
	const char * usage = gettext( "usage: " ) ;
	const char * alt_usage = gettext( "abbreviated usage: " ) ;
	std::string s = std::string()
		.append(layout.alt_usage?alt_usage:usage)
		.append(exe)
		.append(" ")
		.append(usageSummaryPartOne(layout))
		.append(usageSummaryPartTwo(layout))
		.append(args.empty()||args.at(0U)==' '?"":" ")
		.append(args) ;
	std::string indent( 2U , ' ' ) ;
	return layout.width == 0U ? s : StringWrap::wrap( s , "" , indent , layout.width , 0U , true ) ;
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

