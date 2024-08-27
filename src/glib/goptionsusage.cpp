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
/// \file goptionsusage.cpp
///

#include "gdef.h"
#include "goptionsusage.h"
#include "genvironment.h"
#include "ggettext.h"
#include "gstr.h"
#include "gstringfield.h"
#include "gtest.h"
#include "gstringwrap.h"
#include "gassert.h"
#include <algorithm>

G::OptionsUsage::OptionsUsage( const std::vector<Option> & options , std::function<bool(const Option&,const Option&)> sort_fn ) :
	m_options(options)
{
	if( sort_fn )
		std::sort( m_options.begin() , m_options.end() , sort_fn ) ;

	if( G::Test::enabled("options-usage-debug") )
	{
		m_space_margin = 'M' ;
		m_space_separator = 'S' ;
		m_space_indent = 'I' ;
		m_space_padding = 'P' ;
		m_space_overflow = '_' ;
		m_space_syntax = '.' ;
	}
}

std::string G::OptionsUsage::summary( const Config & config_in ,
	const std::string & exe , const std::string & args ) const
{
	Config config = config_in ;
	config.setDefaults() ;

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

std::string G::OptionsUsage::help( const Config & config_in , bool * overflow_p ) const
{
	Config config = config_in ;
	config.setDefaults() ;
	config.setWidthsWrtMargin() ;

	if( overflow_p == nullptr )
	{
		bool overflow = false ;
		std::string s = helpImp( config , false , overflow ) ;
		if( overflow )
			s = helpImp( config , true , overflow ) ;
		return s ;
	}
	else
	{
		bool overflow = *overflow_p ;
		return helpImp( config , overflow , *overflow_p ) ;
	}
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
	if( !s.empty() ) s.append( "] " ) ;
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
			if( !option.name.empty() )
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

std::string G::OptionsUsage::helpImp( const Config & config , bool overflow , bool & overflow_out ) const
{
	std::string result ;
	for( const auto & option : m_options )
	{
		if( option.visible({config.level_min,config.level_max},config.main_tag,config.tag_bits) )
		{
			result.append( optionHelp(config,option,overflow,overflow_out) ).append( 1U , '\n' ) ;
		}
	}
	return result ;
}

std::string G::OptionsUsage::optionHelp( const Config & config , const Option & option ,
	bool overflow , bool & overflow_out ) const
{
	char syntax_non_space = '\x01' ;
	std::string syntax_simple = helpSyntax( option ) ;
	std::string syntax_aligned = helpSyntax( option , true , syntax_non_space ) ;
	std::string description = helpDescription( option , config.extra ) ;
	std::string separator = helpSeparator( config , syntax_aligned.length() ) ;

	// concatentate and wrap
	std::string line = helpWrap( config , syntax_simple , syntax_aligned , separator , description , overflow , overflow_out ) ;

	// add a margin
	if( config.margin )
	{
		std::string margin = std::string( config.margin , m_space_margin ) ;
		G::Str::replaceAll( line , "\n" , std::string(1U,'\n').append(margin) ) ;
		line = margin + line ;
	}

	// fix up the placeholders in the syntax part
	G::Str::replace( line , syntax_non_space , m_space_syntax ) ;

	return line ;
}

std::string G::OptionsUsage::helpWrap( const Config & config , const std::string & syntax_simple ,
	const std::string & syntax_aligned , const std::string & separator , const std::string & description ,
	bool overflow_in , bool & overflow_out ) const
{
	std::string text = std::string(syntax_aligned).append(separator).append(description) ;
	if( config.width == 0 )
	{
		// no wrapping
		return text ;
	}
	else if( config.separator == "\t" )
	{
		// wrapped lines are indented with a tab
		return StringWrap::wrap( text , "" , "\t" , config.width , config.width2 , true ) ;
	}
	else if( overflow_in )
	{
		// overflow mode -- first line is syntax, word-wrapped description from line two
		std::string indent( config.overflow_spaces , m_space_overflow ) ;
		return syntax_simple + "\n" + StringWrap::wrap( description , indent , indent , config.width2 , config.width2 , true ) ;
	}
	else if( config.separator.empty() )
	{
		// no separator so wrapped lines are indented to the required column
		std::string s = StringWrap::wrap( text , "" , helpPadding(config) ,
			config.width , config.width2 , true ) ;

		// suggest overflow mode if width has been squished down too close to column
		if( !overflow_out && std::min(config.width,config.width2) <= config.overflow )
			overflow_out = true ;

		return s ;
	}
	else
	{
		// separator defined -- no column for the wrapped lines to indent to --
		// just add (typically) one leading space to wrapped lines
		return StringWrap::wrap( text , "" , std::string(config.separator_spaces,m_space_separator) ,
			config.width , config.width2 , true ) ;
	}
}

std::string G::OptionsUsage::helpSyntax( const Option & option , bool with_non_space , char non_space ) const
{
	std::string syntax ;
	if( option.c != '\0' )
	{
		syntax.append( 1U , '-' ) ;
		syntax.append( 1U , option.c ) ;
		if( !option.name.empty() )
			syntax.append( ", " ) ;
	}
	else if( with_non_space )
	{
		syntax.append( 4U , non_space ) ; // (new)
	}
	if( !option.name.empty() )
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
	return syntax ;
}

std::string G::OptionsUsage::helpDescription( const Option & option , bool config_extra ) const
{
	std::string description = option.description ;
	if( config_extra )
		description.append( option.description_extra ) ;
	return description ;
}

std::string G::OptionsUsage::helpSeparator( const Config & config , std::size_t syntax_length ) const
{
	if( !config.separator.empty() )
		return config.separator ;
	else if( (config.margin+syntax_length) >= config.column )
		return std::string( 1U , m_space_separator ) ; // NOLINT not return {...}
	else
		return std::string( config.column-syntax_length-config.margin , m_space_separator ) ; // NOLINT not return {...}
}

std::string G::OptionsUsage::helpPadding( const Config & config ) const
{
	std::size_t n = std::max( std::size_t(1U) , config.column - std::min(config.column,config.margin) ) ;
	return std::string( n , m_space_padding ) ; // NOLINT not return {...}
}

G::OptionsUsage::SortFn G::OptionsUsage::sort()
{
	return []( const Option & a , const Option & b ){
		// sort by level, then by name
		return a.level == b.level ? G::Str::iless(a.name,b.name) : ( a.level < b.level ) ;
	} ;
}

// ==

G::OptionsUsage::Config & G::OptionsUsage::Config::setDefaults()
{
	if( width == Config::default_ )
		width = Str::toUInt( Environment::get("COLUMNS",{}) , "79" ) ;
	if( width2 == 0U )
		width2 = width ;
	return *this ;
}

G::OptionsUsage::Config & G::OptionsUsage::Config::setWidthsWrtMargin()
{
	// adjust the widths wrt. the margin to pass to StringWrap::wrap()
	width -= std::min( width , margin ) ;
	width = std::max( width , std::size_t(1U) ) ;
	width2 -= std::min( width2 , margin ) ;
	width2 = std::max( width2 , std::size_t(1U) ) ;
	return *this ;
}

