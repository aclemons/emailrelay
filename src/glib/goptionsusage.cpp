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
/// \file goptionsusage.cpp
///

#include "gdef.h"
#include "goptionsusage.h"
#include "genvironment.h"
#include "ggettext.h"
#include "gstr.h"
#include "gstringwrap.h"
#include "gassert.h"
#include <algorithm>

G::OptionsUsage::OptionsUsage( const std::vector<Option> & options , std::function<bool(const Option&,const Option&)> sort_fn ) :
	m_options(options)
{
	if( sort_fn )
		std::sort( m_options.begin() , m_options.end() , sort_fn ) ;
}

std::string G::OptionsUsage::summary( const Config & config_in ,
	const std::string & exe , const std::string & args ) const
{
	Config config = applyDefault( config_in ) ;
	const char * usage = txt( "usage: " ) ;
	const char * alt_usage = txt( "abbreviated usage: " ) ;
	std::string s = std::string()
		.append(config.alt_usage?alt_usage:usage)
		.append(exe)
		.append(" ")
		.append(summaryPartOne(config))
		.append(summaryPartTwo(config))
		.append(args.empty()||args.at(0U)==' '?"":" ")
		.append(args) ;
	std::string indent( 2U , ' ' ) ;
	return config.width == 0U ? s : StringWrap::wrap( s , "" , indent , config.width , 0U , true ) ;
}

std::string G::OptionsUsage::help( const Config & config_in , bool * state_p ) const
{
	Config config = applyDefault( config_in ) ;
	std::string result = helpImp( config ) ;

	// check for width overflow even after wrapping, which can
	// happen as 'config.width' shrinks towards 'config.column'
	//
	bool overflow = ( state_p && *state_p ) || (
		config.width && config.column && config.separator.empty() &&
		config.width <= (config.column+20) &&
		longestSubLine(result) > config.width ) ;

	if( overflow && state_p )
		*state_p = true ;

	// on width-overflow give up on columns and make the description
	// part wrap onto a completely new line by setting a huge separator
	if( overflow )
	{
		result = helpImp( Config(config).set_separator( std::string(config.column+config.column,' ') ) ) ;
	}

	return result ;
}

void G::OptionsUsage::output( const Config & config , std::ostream & stream ,
	const std::string & exe , const std::string & args ) const
{
	stream
		<< summary(config,exe,args) << std::endl
		<< std::endl
		<< help(config) ;
}

std::string G::OptionsUsage::summaryPartOne( const Config & config ) const
{
	// summarise the single-character switches, excluding those which take a value
	std::ostringstream ss ;
	bool first = true ;
	for( const auto & option : m_options )
	{
		if( option.c != '\0' && !option.valued() && option.visible({config.level_min,config.level_max},config.main_tag,config.tag_bits) )
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

std::string G::OptionsUsage::summaryPartTwo( const Config & config ) const
{
	std::ostringstream ss ;
	const char * sep = "" ;
	for( const auto & option : m_options )
	{
		if( option.visible({config.level_min,config.level_max},config.main_tag,config.tag_bits) )
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

std::string G::OptionsUsage::helpSyntax( const Option & option , char placeholder ) const
{
	std::string syntax ;
	if( option.c != '\0' )
	{
		syntax.append( 1U , '-' ) ;
		syntax.append( 1U , option.c ) ;
		if( option.name.length() )
			syntax.append( ", " ) ;
	}
	else
	{
		syntax.append( 4U , placeholder ) ;
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

std::string G::OptionsUsage::helpDescription( const Option & option , const Config & config ) const
{
	std::string description = option.description ;
	if( config.extra )
		description.append( option.description_extra ) ;
	return description ;
}

std::string G::OptionsUsage::helpSeparator( const Config & config , std::size_t syntax_length ) const
{
	std::string separator ;
	if( !config.separator.empty() )
		separator = config.separator ;
	else if( (config.margin+syntax_length) > config.column )
		separator = std::string( 1U , ' ' ) ;
	else
		separator = std::string( config.column-syntax_length-config.margin , ' ' ) ;
	return separator ;
}

std::string G::OptionsUsage::helpWrap( const Config & config , const std::string & line_in ,
	const std::string & margin ) const
{
	std::string line( line_in ) ;
	std::size_t wrap_width = config.width>config.margin ? (config.width-config.margin) : 1U ;
	bool separator_is_tab = config.separator.length() == 1U && config.separator.at(0U) == '\t' ;
	if( separator_is_tab )
	{
		std::string prefix_other = std::string(config.margin,' ').append(1U,'\t') ;
		line = margin + StringWrap::wrap( line , std::string() , prefix_other ,
			wrap_width , config.width2 , true ) ;
	}
	else if( !config.separator.empty() )
	{
		// wrap with a one-space indent for wrapped descriptions wrt. the syntax
		if( line.length() > config.width )
		{
			std::string prefix_other( config.margin+1U , ' ' ) ;
			line = margin + StringWrap::wrap( line , std::string() , prefix_other ,
				wrap_width , config.width2 , true ) ;
		}
	}
	else
	{
		std::string prefix_other( config.column , ' ' ) ;
		line = margin + StringWrap::wrap( line , std::string() , prefix_other ,
			wrap_width , config.width2 , true ) ;
	}
	return line ;
}

std::size_t G::OptionsUsage::longestSubLine( const std::string & s )
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

std::string G::OptionsUsage::helpImp( const Config & config ) const
{
	std::string result ;
	for( const auto & option : m_options )
	{
		if( option.visible({config.level_min,config.level_max},config.main_tag,config.tag_bits) )
		{
			char placeholder = '\x01' ;
			std::string margin( config.margin , ' ' ) ;
			std::string syntax = helpSyntax( option , placeholder ) ;
			std::string description = helpDescription( option , config ) ;
			std::string separator = helpSeparator( config , syntax.length() ) ;

			std::string line ;
			line.append(margin).append(syntax).append(separator).append(description) ;

			if( config.width )
				line = helpWrap( config , line , margin ) ;

			G::Str::replace( line , placeholder , ' ' ) ;
			line.append( 1U , '\n' ) ;
			result.append( line ) ;
		}
	}
	return result ;
}

G::OptionsUsage::Config G::OptionsUsage::applyDefault( const Config & config_in )
{
	bool set1 = config_in.width == Config::default_ ;
	bool set2 = config_in.width2 == Config::default_ ;
	std::size_t w = set1||set2 ? Str::toUInt( Environment::get("COLUMNS",{}) , "79" ) : 0U ;
	return Config(config_in).set_width(set1?w:config_in.width).set_width2(set2?w:config_in.width2) ;
}

G::OptionsUsage::SortFn G::OptionsUsage::sort()
{
	return []( const Option & a , const Option & b ){
		// sort by level, then by name
		return a.level == b.level ? G::Str::iless(a.name,b.name) : ( a.level < b.level ) ;
	} ;
}

