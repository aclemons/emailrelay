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
	class OptionsLevel ;
	class OptionsLayout ;
}

/// \class G::OptionsLevel
/// Used by G::Options for extra type safety.
///
class G::OptionsLevel
{
public:
	unsigned int level ;
	explicit OptionsLevel( unsigned int ) ;
} ;

/// \class G::OptionsLayout
/// Describes the layout for G::Options output.
///
class G::OptionsLayout
{
public:
	std::string separator ; ///< separator between columns for two-column output
	std::string indent ; ///< indent for wrapped lines in two-column output
	std::size_t column ; ///< left hand column width if no separator defined
	std::size_t width ; ///< overall width for wrapping, or zero for no newlines
	explicit OptionsLayout( std::size_t column ) ;
	OptionsLayout( std::size_t column , std::size_t width ) ;
} ;

/// \class G::Options
/// A class to represent allowed command-line options and to provide command-line
/// usage text.
///
class G::Options
{
public:
	using Level = OptionsLevel ;
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

	const StringArray & names() const ;
		///< Returns the sorted list of long-form option names.

	std::string lookup( char c ) const ;
		///< Converts from short-form option character to the corresponding
		///< long-form name. Returns the empty string if none.

	bool valid( const std::string & ) const ;
		///< Returns true if the long-form option name is valid.

	bool visible( const std::string & name , Level , bool exact ) const ;
		///< Returns true if the option is visible at the given level.
		///< Returns false if not valid().

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

	static std::size_t widthDefault() ;
		///< Returns a default, non-zero word-wrapping width, reflecting
		///< the size of the standard output where possible.

	static Layout layoutDefault() ;
		///< Returns a default column layout.

	static Level levelDefault() ;
		///< Returns the default level.

	static std::string introducerDefault() ;
		///< Returns the string "usage: ".

	std::string usageSummary( const std::string & exe , const std::string & args ,
		const std::string & introducer = introducerDefault() ,
		Level level = levelDefault() , std::size_t wrap_width = widthDefault() ) const ;
			///< Returns a one-line (or line-wrapped) usage summary, as
			///< "usage: <exe> <options> <args>"

	std::string usageHelp( Level level = levelDefault() ,
		const Layout & layout = layoutDefault() , bool level_exact = false , bool extra = true ) const ;
			///< Returns a multi-line string giving help on each option.

	void showUsage( std::ostream & stream , const std::string & exe ,
		const std::string & args = std::string() , const std::string & introducer = introducerDefault() ,
		Level level = levelDefault() , const Layout & layout = layoutDefault() ,
		bool extra = true ) const ;
			///< Streams out multi-line usage text using usageSummary() and
			///< usageHelp(). The 'args' parameter should represent the non-option
			///< arguments (with a leading space), like " <foo> [<bar>]".

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

private:
	void parseSpec( const std::string & spec , char , char , char ) ;
	void addSpec( const std::string & , char c , const std::string & , const std::string & ,
		const std::string & , const std::string & , unsigned int ) ;
	static std::size_t widthFloor( std::size_t w ) ;
	std::string usageSummaryPartOne( Level ) const ;
	std::string usageSummaryPartTwo( Level ) const ;
	std::string usageHelpCore( const std::string & , Level , const Layout & , bool , bool ) const ;

private:
	using Map = std::map<std::string,Option> ;
	StringArray m_names ;
	Map m_map ;
} ;

inline
G::OptionsLevel::OptionsLevel( unsigned int l ) :
	level(l)
{
}

inline
G::OptionsLayout::OptionsLayout( std::size_t column_ ) :
	column(column_) ,
	width(G::Options::widthDefault())
{
}

inline
G::OptionsLayout::OptionsLayout( std::size_t column_ , std::size_t width_ ) :
	column(column_) ,
	width(width_)
{
}

#endif
