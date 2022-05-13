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
/// \file goptionsoutput.h
///

#ifndef G_OPTIONS_OUTPUT_H
#define G_OPTIONS_OUTPUT_H

#include "gdef.h"
#include "goptions.h"
#include <string>
#include <vector>

namespace G
{
	class OptionsOutput ;
	struct OptionsOutputLayout ;
}

//| \class G::OptionsOutputLayout
/// Describes the layout for G::OptionsOutput.
///
struct G::OptionsOutputLayout
{
	std::string separator ; ///< separator between syntax and description
	std::size_t column ; ///< left hand column width if no separator (includes margin)
	std::size_t width ; ///< overall width for wrapping, or zero for none
	std::size_t width2 ; ///< width after the first line, or zero for 'width'
	std::size_t margin ; ///< spaces to the left of the syntax part
	unsigned int level ; ///< show options at-or-below this level
	bool level_exact ; ///< .. or exactly at some level
	bool extra ; ///< include descriptions' extra text
	bool alt_usage ; ///< use alternate "usage:" string

	OptionsOutputLayout() ;
	explicit OptionsOutputLayout( std::size_t column ) ;
	OptionsOutputLayout( std::size_t column , std::size_t width ) ;
	OptionsOutputLayout & set_column( std::size_t ) ;
	OptionsOutputLayout & set_extra( bool = true ) ;
	OptionsOutputLayout & set_level( unsigned int ) ;
	OptionsOutputLayout & set_level_if( bool , unsigned int ) ;
	OptionsOutputLayout & set_level_exact( bool = true ) ;
	OptionsOutputLayout & set_alt_usage( bool = true ) ;
} ;

//| \class G::OptionsOutput
/// Provides help text for a set of options.
///
class G::OptionsOutput
{
public:
	using Layout = OptionsOutputLayout ;

	explicit OptionsOutput( const std::vector<Option> & ) ;
		///< Constructor.

	std::string usageSummary( const Layout & , const std::string & exe ,
		const std::string & args = {} ) const ;
			///< Returns a one-line (or line-wrapped) usage summary, as
			///< "usage: <exe> <options> <args>". The 'args' parameter
			///< should represent the non-option arguments (with a
			///< leading space), like " <foo> [<bar>]".
			///<
			///< Eg:
			///< \code
			///< std::cout << output.usageSummary(
			///<   OptionsOutputLayout().set_level_if(!verbose,1U).set_extra(verbose) ,
			///<   getopt.args().prefix() , " <arg> [<arg> ...]" ) << std::endl ;
			///< \endcode

	std::string usageHelp( const Layout & ) const ;
		///< Returns a multi-line string giving help on each option.

	void showUsage( const Layout & , std::ostream & stream ,
		const std::string & exe , const std::string & args = {} ) const ;
			///< Streams out multi-line usage text using usageSummary() and
			///< usageHelp().

private:
	std::string usageSummaryPartOne( const Layout & ) const ;
	std::string usageSummaryPartTwo( const Layout & ) const ;
	std::string usageHelpSyntax( const Option & ) const ;
	std::string usageHelpDescription( const Option & , const Layout & ) const ;
	std::string usageHelpSeparator( const Layout & , std::size_t syntax_length ) const ;
	std::string usageHelpWrap( const Layout & , const std::string & line_in ,
		const std::string & ) const ;
	std::string usageHelpImp( const Layout & ) const ;
	static std::size_t longestSubLine( const std::string & ) ;

private:
	std::vector<Option> m_options ;
} ;

inline G::OptionsOutputLayout & G::OptionsOutputLayout::set_column( std::size_t c ) { column = c ; return *this ; }
inline G::OptionsOutputLayout & G::OptionsOutputLayout::set_extra( bool e ) { extra = e ; return *this ; }
inline G::OptionsOutputLayout & G::OptionsOutputLayout::set_level( unsigned int l ) { level = l ; return *this ; }
inline G::OptionsOutputLayout & G::OptionsOutputLayout::set_level_if( bool b , unsigned int l ) { if(b) level = l ; return *this ; }
inline G::OptionsOutputLayout & G::OptionsOutputLayout::set_level_exact( bool le ) { level_exact = le ; return *this ; }
inline G::OptionsOutputLayout & G::OptionsOutputLayout::set_alt_usage( bool au ) { alt_usage = au ; return *this ; }

#endif
