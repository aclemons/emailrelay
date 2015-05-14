//
// Copyright (C) 2001-2015 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#ifndef G_OPTIONS_H__
#define G_OPTIONS_H__

#include "gdef.h"
#include "gstrings.h"
#include "gexception.h"
#include <string>

/// \namespace G
namespace G
{
	class Options ;
	class OptionsImp ;
}

/// \class G::Options
/// A class to represent allowed command-line options and to
/// provide command-line usage text.
///
class G::Options
{
public:
	/// Used by G::Options for extra type safety.
	struct Level 
		{ unsigned int level ; explicit Level(unsigned int l) : level(l) {} } ;
	G_EXCEPTION( InvalidSpecification , "invalid options specification string" ) ;

	explicit Options( const std::string & spec , char sep_major = '|' , char sep_minor = '!' , char escape = '^' ) ;
		///< Constructor taking a specification string.
		///<
		///< Uses specifications like "p!port!defines the port number!!1!port!1|v!verbose!shows more logging!!0!!1"
		///< made up of (1) a single-character-option-letter, (2) a multi-character-option-name
		///< (3) an option-description, (4) option-description-extra text, (5) a 'value-type' 
		///< (0 is for unvalued, 1 for a string value, and 2 for a comma-separated list),
		///< (6) a value-description, and (7) a level enumeration.
		///<
		///< By convention mainstream options should have a level of 1, and obscure 
		///< ones level 2 and above. If the option-description field is empty or if
		///< the level is zero then the option is hidden.

	~Options() ;
		///< Destructor.

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
		///< Returns true if the short-form option character is valid.
		///< Returns false if not valid().

	bool valued( const std::string & ) const ;
		///< Returns true if the long-form option name is valid.
		///< Returns false if not valid().

	bool multivalued( char ) const ;
		///< Returns true if the short-form option can have multiple values.
		///< Returns false if not valid().

	bool multivalued( const std::string & ) const ;
		///< Returns true if the long-form option can have multiple values.
		///< Returns false if not valid().

	bool unvalued( const std::string & option_name ) const ;
		///< Returns true if the given option name is valid and
		///< takes no value. Returns false if not valid().

	static size_t wrapDefault() ;
		///< Returns a default word-wrapping width.

	static size_t tabDefault() ;
		///< Returns a default tab-stop.

	static Level levelDefault() ;
		///< Returns the default level.

	static std::string introducerDefault() ;
		///< Returns the string "usage: ".

	std::string usageSummary( const std::string & exe , const std::string & args , 
		const std::string & introducer = introducerDefault() ,
		Level level = levelDefault() , size_t wrap_width = wrapDefault() ) const ;
			/// \code
			///<< Returns a one-line (or line-wrapped) usage summary, as
			///<< "usage: <exe> <options> <args>"
			/// \endcode

	std::string usageHelp( Level level = levelDefault() ,
		size_t tab_stop = tabDefault() , size_t wrap_width = wrapDefault() ,
		bool level_exact = false , bool extra = true ) const ;
			/// \code
			///<< Returns a multi-line string giving help on each option.
			/// \endcode

	void showUsage( std::ostream & stream , const std::string & exe = std::string() , 
		const std::string & args = std::string() , const std::string & introducer = introducerDefault() ,
		Level level = levelDefault() , size_t tab_stop = tabDefault() , 
		size_t wrap_width = wrapDefault() , bool extra = true ) const ;
			/// \code
			///<< Streams out multi-line usage text using usageSummary() 
			///<< and usageHelp(). The 'args' parameter should represent
			///<< the non-option arguments (with a leading space), like 
			///<< " <foo> [<bar>]".
			/// \endcode

private:
	void operator=( const Options & ) ;
	Options( const Options & ) ;
	void parseSpec( const std::string & spec , char , char , char ) ;
	void addSpec( const std::string & , char c , const std::string & , const std::string & , 
		unsigned int , const std::string & , unsigned int ) ;
	static size_t widthLimit( size_t w ) ;
	std::string usageSummaryPartOne( Level ) const ;
	std::string usageSummaryPartTwo( Level ) const ;
	std::string usageHelpCore( const std::string & , Level , size_t , size_t , bool , bool ) const ;

private:
	std::string m_spec ;
	StringArray m_names ;
	OptionsImp * m_imp ;
} ;

#endif
