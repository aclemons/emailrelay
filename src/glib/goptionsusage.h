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
/// \file goptionsusage.h
///

#ifndef G_OPTIONS_USAGE_H
#define G_OPTIONS_USAGE_H

#include "gdef.h"
#include "goptions.h"
#include <functional>
#include <string>
#include <vector>

namespace G
{
	class OptionsUsage ;
}

//| \class G::OptionsUsage
/// Provides help text for a set of options.
///
/// If configured with a fixed separator the help() text is
/// formatted as:
/// \code
///  <margin><syntax><separator><detail-to-width1>
///  <margin><separator-spaces><more-detail-to-width2>
/// \endcode
///
/// Or with a tab separator:
/// \code
///  <margin><syntax><TAB><detail-to-width1>
///  <margin><TAB><more-detail-to-width2>
/// \endcode
///
/// Or with a two-column layout:
/// \code
///  <margin><syntax><spaces-to-column><detail-to-width1>
///  <margin><spaces-to-column><more-detail-to-width2>
/// \endcode
///
/// Or two-column layout in 'overflow' mode:
/// \code
///  <margin><syntax>
///  <margin><overflow-spaces><detail-to-width2>
///  <margin><overflow-spaces><more-detail-to-width2>
/// \endcode
///
class G::OptionsUsage
{
public:
	struct Config /// A configuration structure for G::OptionsUsage.
	{
		static constexpr std::size_t default_ = static_cast<std::size_t>(-1) ;

		std::string separator ; ///< separator between syntax and description
		std::size_t separator_spaces {1U} ; ///< extra spaces on wrapped lines if using a separator
		std::size_t column {30U} ; ///< left hand column width if no separator (includes margin)
		std::size_t width {default_} ; ///< overall width for wrapping, or zero for none, defaults to $COLUMNS
		std::size_t width2 {0U} ; ///< width after the first line, or zero for 'width'
		std::size_t margin {2U} ; ///< spaces added to the left
		std::size_t overflow {20U} ; ///< use 'overflow' format if rhs is squashed down to this
		std::size_t overflow_spaces {1U} ; ///< 'overflow' format extra spaces on wrapped lines

		bool extra {false} ; ///< include descriptions' extra text
		bool alt_usage {false} ; ///< use alternate "usage:" string

		unsigned int level_max {99U} ; ///< show options at-or-below this level
		unsigned int level_min {1U} ; ///< .. and at-or-above this level
		unsigned int main_tag {0U} ; ///< show options with this main tag, or zero for all
		unsigned int tag_bits {0U} ; ///< show options with matching tag bits, or zero for all

		Config & set_separator( const std::string & ) ;
		Config & set_column( std::size_t ) noexcept ;
		Config & set_width( std::size_t ) noexcept ;
		Config & set_width2( std::size_t ) noexcept ;
		Config & set_margin( std::size_t ) noexcept ;
		Config & set_extra( bool = true ) noexcept ;
		Config & set_alt_usage( bool = true ) noexcept ;

		Config & set_level_max( unsigned int ) noexcept ;
		Config & set_level_min( unsigned int ) noexcept ;
		Config & set_main_tag( unsigned int ) noexcept ;
		Config & set_tag_bits( unsigned int ) noexcept ;
		Config & setDefaults() ;
		Config & setWidthsWrtMargin() ;
		Config & setOverflowFormat( char = ' ' ) ;
	} ;

	using SortFn = std::function<bool(const Option&,const Option&)> ;

	explicit OptionsUsage( const std::vector<Option> & , SortFn = sort() ) ;
		///< Constructor.

	std::string summary( const Config & , const std::string & exe ,
		const std::string & args = {} ) const ;
			///< Returns a one-line (or line-wrapped) usage summary, as
			///< "usage: <exe> <options> <args>". The 'args' parameter
			///< should represent the non-option arguments (with a
			///< leading space), like " <foo> [<bar>]".
			///<
			///< Eg:
			///< \code
			///< std::cout << usage.summary(
			///<   OptionsUsage::Config().set_level_max(verbose?1U:99U).set_extra(verbose) ,
			///<   getopt.args().prefix() , " <arg> [<arg> ...]" ) << std::endl ;
			///< \endcode

	std::string help( const Config & , bool * overflow_p = nullptr ) const ;
		///< Returns a multi-line string giving help on each option.
		///< Use the optional overflow flag if using help() for separate
		///< sections that should have the same layout: initialise to
		///< false, call help() for each section and discard the result,
		///< then call help() again for each section.

	void output( const Config & , std::ostream & stream ,
		const std::string & exe , const std::string & args = {} ) const ;
			///< Streams out multi-line usage text using summary() and
			///< help().

	static SortFn sort() ;
		///< Returns the default sort function that sorts by level first
		///< and then by name.

private:
	std::string summaryPartOne( const Config & ) const ;
	std::string summaryPartTwo( const Config & ) const ;
	std::string helpSyntax( const Option & , bool = false , char = '\0' ) const ;
	std::string helpDescription( const Option & , bool ) const ;
	std::string helpSeparator( const Config & , std::size_t syntax_length ) const ;
	std::string helpPadding( const Config & ) const ;
	std::string helpImp( const Config & , bool , bool & ) const ;
	std::string optionHelp( const Config & , const Option & option , bool , bool & ) const ;
	std::string helpWrap( const Config & , const std::string & syntax_simple ,
		const std::string & syntax_aligned , const std::string & separator ,
		const std::string & description , bool , bool & ) const ;
	static Config setDefaults( const Config & ) ;
	static Config setWidthsWrtMargin( const Config & ) ;

private:
	std::vector<Option> m_options ;
	char m_space_margin {' '} ;
	char m_space_separator {' '} ;
	char m_space_indent {' '} ;
	char m_space_padding {' '} ;
	char m_space_overflow {' '} ;
	char m_space_syntax {' '} ;
} ;

inline G::OptionsUsage::Config & G::OptionsUsage::Config::set_separator( const std::string & s ) { separator = s ; return *this ; }
inline G::OptionsUsage::Config & G::OptionsUsage::Config::set_column( std::size_t n ) noexcept { column  = n ; return *this ; }
inline G::OptionsUsage::Config & G::OptionsUsage::Config::set_width( std::size_t n ) noexcept { width = n ; return *this ; }
inline G::OptionsUsage::Config & G::OptionsUsage::Config::set_width2( std::size_t n ) noexcept { width2 = n ; return *this ; }
inline G::OptionsUsage::Config & G::OptionsUsage::Config::set_margin( std::size_t n ) noexcept { margin = n ; return *this ; }
inline G::OptionsUsage::Config & G::OptionsUsage::Config::set_extra( bool b ) noexcept { extra = b ; return *this ; }
inline G::OptionsUsage::Config & G::OptionsUsage::Config::set_alt_usage( bool b ) noexcept { alt_usage = b ; return *this ; }
inline G::OptionsUsage::Config & G::OptionsUsage::Config::set_level_max( unsigned int n ) noexcept { level_max = n ; return *this ; }
inline G::OptionsUsage::Config & G::OptionsUsage::Config::set_level_min( unsigned int n ) noexcept { level_min = n ; return *this ; }
inline G::OptionsUsage::Config & G::OptionsUsage::Config::set_main_tag( unsigned int n ) noexcept { main_tag = n ; return *this ; }
inline G::OptionsUsage::Config & G::OptionsUsage::Config::set_tag_bits( unsigned int n ) noexcept { tag_bits = n ; return *this ; }

#endif
