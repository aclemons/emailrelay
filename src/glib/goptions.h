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
/// \file goptions.h
///

#ifndef G_OPTIONS_H
#define G_OPTIONS_H

#include "gdef.h"
#include "gstrings.h"
#include "gexception.h"
#include <string>
#include <map>

namespace G
{
	class Options ;
	struct OptionsLayout ;
}

//| \class G::OptionsLayout
/// Describes the layout for G::Options output.
///
struct G::OptionsLayout
{
	std::string separator ; ///< separator between syntax and description
	std::size_t column ; ///< left hand column width if no separator (includes margin)
	std::size_t width ; ///< overall width for wrapping, or zero for none
	std::size_t width2 ; ///< width after the first line, or zero for 'width'
	std::size_t margin ; ///< spaces to the left of the syntax part
	unsigned int level ; ///< show options at-or-below this level
	bool level_exact ; ///< .. or exactly at that level
	bool extra ; ///< include descriptions' extra text
	bool alt_usage ; ///< use alternate "usage:" string
	OptionsLayout() ;
	explicit OptionsLayout( std::size_t column ) ;
	OptionsLayout( std::size_t column , std::size_t width ) ;
	OptionsLayout & set_column( std::size_t ) ;
	OptionsLayout & set_extra( bool = true ) ;
	OptionsLayout & set_level( unsigned int ) ;
	OptionsLayout & set_level_if( bool , unsigned int ) ;
	OptionsLayout & set_level_exact( bool = true ) ;
	OptionsLayout & set_alt_usage( bool = true ) ;
} ;

//| \class G::Options
/// A class to represent allowed command-line options and to provide command-line
/// usage text.
///
class G::Options
{
public:
	using Layout = OptionsLayout ;
	G_EXCEPTION( InvalidSpecification , "invalid options specification string" ) ;

	explicit Options( const std::string & spec , char sep_major = '|' , char sep_minor = '!' , char escape = '^' ) ;
		///< Constructor taking a specification string.
		///<
		///< Uses specifications like "p!port!defines the port number!!1!port!1|v!verbose!shows more logging!!0!!1"
		///< made up of (1) an optional single-character-option-letter, (2) a multi-character-option-name
		///< (3) an option-description, (4) optional option-description-extra text, (5) a value-type
		///< (with '0' for unvalued, '1' for a single value, '2' for a comma-separated list (possibly
		///< multiple times), or '01' for a defaultable single value) or (6) a value-description
		///< (unless unvalued), and (7) a level enumeration.
		///<
		///< By convention mainstream options should have a level of 1, and obscure ones level 2 and above.
		///< If the option-description field is empty or if the level is zero then the option is hidden.

	Options() ;
		///< Default constructor for no options.

	void add( StringArray ) ;
		///< Adds one component of the specification, broken down into its seven
		///< separate parts.

	void add( StringArray , char sep , char escape = '\\' ) ;
		///< Adds one component of the specification broken down into six separate
		///< parts with fields 3 and 4 joined by the given separator character.

	const StringArray & names() const ;
		///< Returns the sorted list of long-form option names.

	std::string lookup( char c ) const ;
		///< Converts from short-form option character to the corresponding
		///< long-form name. Returns the empty string if none.

	bool valid( const std::string & ) const ;
		///< Returns true if the long-form option name is valid.

	bool visible( const std::string & name , unsigned int level , bool level_exact = false ) const ;
		///< Returns true if the option is visible at the given level.
		///< Deliberately hidden options at level zero are never
		///< visible(). Returns false if not a valid() name .

	bool visible( const std::string & name ) const ;
		///< Returns true if the option is visible. Returns false if
		///< the option is hidden or the name is not valid().

	bool valued( char ) const ;
		///< Returns true if the given short-form option takes a value,
		///< Returns true if the short-form option character is valued,
		///< including multivalued() and defaulting().
		///< Returns false if not valid().

	bool valued( const std::string & ) const ;
		///< Returns true if the given long-form option takes a value,
		///< including multivalued() and defaulting(). Returns false
		///< if not valid().

	bool multivalued( char ) const ;
		///< Returns true if the short-form option can have multiple values.
		///< Returns false if not valid().

	bool multivalued( const std::string & ) const ;
		///< Returns true if the long-form option can have multiple values.
		///< Returns false if not valid().

	bool unvalued( const std::string & ) const ;
		///< Returns true if the given option name is valid and
		///< takes no value. Returns false if not valid().

	bool defaulting( const std::string & ) const ;
		///< Returns true if the given long-form single-valued() option
		///< can optionally have no explicit value, so "--foo=" and "--foo"
		///< are equivalent, having an empty value, and "--foo=bar" has
		///< a value of 'bar' but "--foo bar" is interpreted as 'foo'
		///< taking its default (empty) value followed by a separate
		///< argument 'bar'.

	std::string usageSummary( const Layout & ,
		const std::string & exe , const std::string & args = std::string() ) const ;
			///< Returns a one-line (or line-wrapped) usage summary, as
			///< "usage: <exe> <options> <args>". The 'args' parameter
			///< should represent the non-option arguments (with a
			///< leading space), like " <foo> [<bar>]".
			///<
			///< Eg:
			///< \code
			///< std::cout << options.usageSummary(
			///<   OptionsLayout().set_level_if(!verbose,1U).set_extra(verbose) ,
			///<   getopt.args().prefix() , " <arg> [<arg> ...]" ) << std::endl ;
			///< \endcode

	std::string usageHelp( const Layout & ) const ;
		///< Returns a multi-line string giving help on each option.

	void showUsage( const Layout & , std::ostream & stream ,
		const std::string & exe , const std::string & args = std::string() ) const ;
			///< Streams out multi-line usage text using usageSummary() and
			///< usageHelp().

private:
	struct Option
	{
		enum class Multiplicity { zero , zero_or_one , one , many } ;
		char c ;
		std::string name ;
		std::string description ;
		std::string description_extra ;
		Multiplicity value_multiplicity ;
		bool hidden ;
		std::string value_description ;
		unsigned int level ;

		Option( char c_ , const std::string & name_ , const std::string & description_ ,
			const std::string & description_extra_ , const std::string & multiplicity_ ,
			const std::string & vd_ , unsigned int level_ ) ;
		static Multiplicity decode( const std::string & ) ;
	} ;
	using Map = std::map<std::string,Option> ;

private:
	void parseSpec( const std::string & spec , char , char , char ) ;
	void addImp( const std::string & , char c , const std::string & , const std::string & ,
		const std::string & , const std::string & , unsigned int ) ;
	std::string usageSummaryPartOne( const Layout & ) const ;
	std::string usageSummaryPartTwo( const Layout & ) const ;
	std::string usageHelpSyntax( Map::const_iterator ) const ;
	std::string usageHelpDescription( Map::const_iterator , const Layout & ) const ;
	std::string usageHelpSeparator( const Layout & , std::size_t syntax_length ) const ;
	std::string usageHelpWrap( const Layout & , const std::string & line_in ,
		const std::string & ) const ;
	std::string usageHelpImp( const Layout & ) const ;
	static std::size_t longestSubLine( const std::string & ) ;

private:
	StringArray m_names ; // sorted
	Map m_map ;
} ;

inline G::OptionsLayout & G::OptionsLayout::set_column( std::size_t c ) { column = c ; return *this ; }
inline G::OptionsLayout & G::OptionsLayout::set_extra( bool e ) { extra = e ; return *this ; }
inline G::OptionsLayout & G::OptionsLayout::set_level( unsigned int l ) { level = l ; return *this ; }
inline G::OptionsLayout & G::OptionsLayout::set_level_if( bool b , unsigned int l ) { if(b) level = l ; return *this ; }
inline G::OptionsLayout & G::OptionsLayout::set_level_exact( bool le ) { level_exact = le ; return *this ; }
inline G::OptionsLayout & G::OptionsLayout::set_alt_usage( bool au ) { alt_usage = au ; return *this ; }

#endif
