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
/// \file goptionparser.h
///

#ifndef G_OPTION_PARSER_H__
#define G_OPTION_PARSER_H__

#include "gdef.h"
#include "gstrings.h"
#include "goptions.h"
#include "gmapfile.h"
#include "goptionvalue.h"
#include <string>
#include <map>

/// \namespace G
namespace G
{
	class OptionParser ;
}

/// \class G::OptionParser
/// A parser for command-line arguments according to a command-line option specification.
///
class G::OptionParser
{
public:
	typedef std::map<std::string,OptionValue> OptionValueMap ;

	OptionParser( const Options & spec , OptionValueMap & values_out , StringArray & errors_out ) ;
		///< Constructor. References are kept.

	size_t parse( const StringArray & args , size_t start_position = 1U ) ;
		///< Parses the given command-line arguments into the associated
		///< value map and/or error list. By default the program name is 
		///< expected to be the first item in the array and it is ignored,
		///< although the second parameter can be used to change this.
		///<
		///< Individual arguments can be in short-form like "-c", or long-form 
		///< like "--foo" or "--foo=bar". Long-form arguments can be passed in 
		///< two separate arguments, eg. "--foo" followed by "bar". Short-form 
		///< options can be grouped (eg. "-abc"). Boolean options can be enabled
		///< by (eg.) "--verbose" or "--verbose=yes", and disabled by "--verbose=no". 
		///< Boolean options cannot use two separate arguments (eg. "--verbose" 
		///< followed by "yes").
		///<
		///< Entries in the output map are keyed by the option's
		///< long name, even if supplied in short-form; multi-valued
		///< values are concatenated into comma-separated lists.
		///<
		///< Errors are reported into the error list.
		///<
		///< Returns the position in the array where the non-option command-line
		///< arguments begin.

	size_t parse( const StringMap & map ) ;
		///< An overload where each item in the map is a long-form, two-part 
		///< option without the double-dash prefix.

	static MapFile parse( const std::string & spec , const StringArray & args , 
		size_t pos = 1U , bool no_throw = false ) ;
			///< A convenience function that parses the broken-up command-line
			///< according to the option specification and returns a MapFile
			///< object containing the long-form options. Throws on error,
			///< unless the no-throw parameter is set.

private:
	OptionParser( const OptionParser & ) ;
	void operator=( const OptionParser & ) ;
	bool haveSeen( const std::string & ) const ;
	static std::string::size_type eqPos( const std::string & ) ;
	static std::string eqValue( const std::string & , std::string::size_type ) ;
	void processOptionOn( char c ) ;
	void processOption( char c , const std::string & value ) ;
	void processOptionOn( const std::string & s ) ;
	void processOptionOff( const std::string & s ) ;
	void processOption( const std::string & s , const std::string & value ) ;
	void errorNoValue( char ) ;
	void errorNoValue( const std::string & ) ;
	void errorUnknownOption( char ) ;
	void errorUnknownOption( const std::string & ) ;
	void errorOptionLikeValue( const std::string & , const std::string & ) ;
	void errorDuplicate( char ) ;
	void errorDuplicate( const std::string & ) ;
	void errorExtraValue( char ) ;
	void errorExtraValue( const std::string & ) ;
	void errorConflict( const std::string & ) ;
	bool contains( const std::string & name ) const ;
	static bool isOldOption( const std::string & ) ;
	static bool isNewOption( const std::string & ) ;
	static bool isAOptionSet( const std::string & ) ;
	static std::string join( const std::string & , const std::string & ) ;
	static std::string join( const OptionValueMap & map ,
		const std::string & key , const std::string & new_value ) ;

private:
	const Options & m_spec ;
	OptionValueMap & m_map ;
	StringArray & m_errors ;
} ;

#endif
