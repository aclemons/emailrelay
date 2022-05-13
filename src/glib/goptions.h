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
/// \file goptions.h
///

#ifndef G_OPTIONS_H
#define G_OPTIONS_H

#include "gdef.h"
#include "gstringarray.h"
#include "gexception.h"
#include "goption.h"
#include <string>

namespace G
{
	class Options ;
}

//| \class G::Options
/// A class to assemble a list of command-line options and provide access
/// by name.
///
class G::Options
{
public:
	G_EXCEPTION( InvalidSpecification , tx("invalid options specification string") ) ;

	explicit Options( const std::string & spec , char sep_major = '|' , char sep_minor = '!' , char escape = '^' ) ;
		///< Constructor taking a specification string.
		///<
		///< Uses specifications like "p!port!the port! for listening!1!port!1|v!verbose!more logging! and help!0!!1"
		///< made up of (1) an optional single-character-option-letter, (2) a multi-character-option-name
		///< (3) an option-description, (4) optional option-description-extra text, (5) a value-type
		///< (with '0' for unvalued, '1' for a single value, '2' for a comma-separated list (possibly
		///< multiple times), or '01' for a defaultable single value) or (6) a value-description
		///< (unless unvalued), (7) a level enumeration, and optional trailing tags.
		///<
		///< By convention mainstream options should have a level of 1, and obscure ones level 2 and above.
		///< If the option-description field is empty or if the level is zero then the option is hidden.

	Options() ;
		///< Default constructor for no options.

	void add( const StringArray & ) ;
		///< Adds one component of the specification, broken down into its seven
		///< or more separate parts.

	void add( const StringArray & , char sep , char escape = '\\' ) ;
		///< Adds one component of the specification, broken down into its
		///< separate parts.
		///<
		///< In this overload if the third field contains the unescaped 'sep'
		///< character then it is split into two parts and the second part
		///< replaces the fourth field, which must be empty. This can make
		///< localisation easier.

	void add( const Option & , char sep , char escape = '\\' ) ;
		///< Adds one component of the specification. If the 'description'
		///< contains the unescaped 'sep' character then it is split into
		///< two parts and the second part replaces the 'description_extra',
		///< which must be empty.

	const std::vector<Option> & list() const ;
		///< Returns the sorted list of option structures.

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

private:
	using List = std::vector<Option> ;
	void parseSpec( const std::string & spec , char , char , char ) ;
	void addImp( Option , char , char ) ;
	List::const_iterator find( const std::string & ) const ;

private:
	List m_list ;
} ;

#endif
