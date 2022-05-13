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
/// \file goptionsoutput.cpp
///

#include "gdef.h"
#include "goptionsoutput.h"
#include "genvironment.h"
#include "ggettext.h"
#include "gstr.h"
#include "gstringwrap.h"
#include "gassert.h"
#include <algorithm>

namespace G
{
	namespace OptionsOutputLayoutImp
	{
		unsigned int widthDefault() ;
	}
}

G::OptionsOutput::OptionsOutput( const std::vector<Option> & options ) :
	m_options(options)
{
}

std::string G::OptionsOutput::usageSummary( const Layout & layout ,
	const std::string & exe , const std::string & args ) const
{
	const char * usage = txt( "usage: " ) ;
	const char * alt_usage = txt( "abbreviated usage: " ) ;
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

std::string G::OptionsOutput::usageHelp( const Layout & layout ) const
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

void G::OptionsOutput::showUsage( const Layout & layout , std::ostream & stream ,
	const std::string & exe , const std::string & args ) const
{
	stream
		<< usageSummary(layout,exe,args) << std::endl
		<< std::endl
		<< usageHelp(layout) ;
}

std::string G::OptionsOutput::usageSummaryPartOne( const Layout & layout ) const
{
	// summarise the single-character switches, excluding those which take a value
	std::ostringstream ss ;
	bool first = true ;
	for( const auto & option : m_options )
	{
		if( option.c != '\0' && !option.valued() && option.visible(layout.level,layout.level_exact) )
		{
			if( first )
				ss << "[-" ;
			first = false ;
			ss << option.c ;
		}
	}

	std::string s = ss.str() ;
	if( s.length() ) s.append( "] " ) ;
	return s ;
}

std::string G::OptionsOutput::usageSummaryPartTwo( const Layout & layout ) const
{
	std::ostringstream ss ;
	const char * sep = "" ;
	for( const auto & option : m_options )
	{
		if( option.visible(layout.level,layout.level_exact) )
		{
			ss << sep << "[" ;
			if( option.name.length() )
			{
				ss << "--" << option.name ;
			}
			else
			{
				G_ASSERT( option.c != '\0' ) ;
				ss << "-" << option.c ;
			}
			if( option.valued() )
			{
				std::string vd = option.value_description ;
				if( vd.empty() ) vd = "value" ;
				ss << "=<" << vd << ">" ;
			}
			ss << "]" ;
			sep = " " ;
		}
	}
	return ss.str() ;
}

std::string G::OptionsOutput::usageHelpSyntax( const Option & option ) const
{
	std::string syntax ;
	if( option.c != '\0' )
	{
		syntax.append( "-" ) ;
		syntax.append( 1U , option.c ) ;
		if( option.name.length() )
			syntax.append( ", " ) ;
	}
	if( option.name.length() )
	{
		syntax.append( "--" ) ;
		syntax.append( option.name ) ;
	}
	if( option.valued() )
	{
		std::string vd = option.value_description ;
		if( vd.empty() ) vd = "value" ;
		if( option.defaulting() ) syntax.append( "[" ) ;
		syntax.append( "=<" ) ;
		syntax.append( vd ) ;
		syntax.append( ">" ) ;
		if( option.defaulting() ) syntax.append( "]" ) ;
	}
	syntax.append( 1U , ' ' ) ;
	return syntax ;
}

std::string G::OptionsOutput::usageHelpDescription( const Option & option , const Layout & layout ) const
{
	std::string description = option.description ;
	if( layout.extra )
		description.append( option.description_extra ) ;
	return description ;
}

std::string G::OptionsOutput::usageHelpSeparator( const Layout & layout , std::size_t syntax_length ) const
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

std::string G::OptionsOutput::usageHelpWrap( const Layout & layout , const std::string & line_in ,
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

std::size_t G::OptionsOutput::longestSubLine( const std::string & s )
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

std::string G::OptionsOutput::usageHelpImp( const Layout & layout ) const
{
	std::string result ;
	for( const auto & option : m_options )
	{
		if( option.visible(layout.level,layout.level_exact) )
		{
			std::string margin( layout.margin , ' ' ) ;
			std::string syntax = usageHelpSyntax( option ) ;
			std::string description = usageHelpDescription( option , layout ) ;
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

// ==

G::OptionsOutputLayout::OptionsOutputLayout() :
	column(30U) ,
	width(0U) ,
	width2(0U) ,
	margin(2U) ,
	level(99U) ,
	level_exact(false) ,
	extra(false) ,
	alt_usage(false)
{
	width = width2 = OptionsOutputLayoutImp::widthDefault() ;
}

G::OptionsOutputLayout::OptionsOutputLayout( std::size_t column_ ) :
	column(column_) ,
	width(0U) ,
	width2(0U) ,
	margin(2U) ,
	level(99U) ,
	level_exact(false) ,
	extra(false) ,
	alt_usage(false)
{
	width = width2 = OptionsOutputLayoutImp::widthDefault() ;
}

G::OptionsOutputLayout::OptionsOutputLayout( std::size_t column_ , std::size_t width_ ) :
	column(column_) ,
	width(width_) ,
	width2(width_) ,
	margin(2U) ,
	level(99U) ,
	level_exact(false) ,
	extra(false) ,
	alt_usage(false)
{
	width = width2 = OptionsOutputLayoutImp::widthDefault() ;
}

unsigned int G::OptionsOutputLayoutImp::widthDefault()
{
	return Str::toUInt( Environment::get("COLUMNS",std::string()) , "79" ) ;
}

