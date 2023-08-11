//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#ifndef G_OPTION_PARSER_H
#define G_OPTION_PARSER_H

#include "gdef.h"
#include "gstringarray.h"
#include "goptions.h"
#include "goptionvalue.h"
#include "goptionmap.h"
#include "gpath.h"
#include <string>
#include <functional>

namespace G
{
	class OptionParser ;
}

//| \class G::OptionParser
/// A parser for command-line arguments that operates according to an Options
/// specification and returns an OptionValue multimap.
/// \see G::Options, G::OptionValue
///
class G::OptionParser
{
public:
	OptionParser( const Options & spec , OptionMap & values_out , StringArray & errors_out ) ;
		///< Constructor. All references are kept (including the const
		///< reference). The output map is a multimap, but with methods
		///< that also allow it to be used as a simple map with multi-valued
		///< options concatenated into a comma-separated list.

	OptionParser( const Options & spec , OptionMap & values_out , StringArray * errors_out = nullptr ) ;
		///< Constructor overload taking an optional errors-out parameter.

	StringArray parse( const StringArray & args_in , std::size_t start_position = 1U ,
		std::size_t ignore_non_options = 0U ,
		std::function<std::string(const std::string&,bool)> callback_fn = {} ) ;
			///< Parses the given command-line arguments into the value map and/or
			///< error list defined by the constructor. This can be called
			///< more than once, with options accumulating in the internal
			///< OptionMap.
			///<
			///< By default the program name is expected to be the first item in
			///< the array and it is ignored, although the 'start-position' parameter
			///< can be used to change this. See also G::Arg::array().
			///<
			///< Parsing stops at the first non-option or at "--" and the
			///< remaining non-options are returned. Optionally some number of
			///< non-options are tolerated without stopping the parsing if
			///< 'ignore_non_options' is non-zero. This is to allow for
			///< sub-commands like "exe subcmd --opt arg".
			///<
			///< Individual arguments can be in short-form like "-c", or long-form
			///< like "--foo" or "--foo=bar". Long-form arguments can be passed in
			///< two separate arguments, eg. "--foo" followed by "bar". Short-form
			///< options can be grouped (eg. "-abc"). Boolean options can be enabled
			///< by (eg.) "--verbose" or "--verbose=yes", and disabled by "--verbose=no".
			///< Boolean options cannot use two separate arguments (eg. "--verbose"
			///< followed by "yes").
			///<
			///< Entries in the output map are keyed by the option's long name,
			///< even if supplied in short-form.
			///<
			///< Errors are appended to the caller's error list.
			///<
			///< The optional callback function can be used to modify the parsing
			///< of each option. For a double-dash option the callback is passed
			///< the string after the double-dash (with false) and it should return
			///< the option name to be used for parsing, with an optional leading
			///< '-' character to indicate that the parsing results should be
			///< discarded. For single-dash options the option letter is looked up
			///< in the Options spec first and the long name is passed to the
			///< callback (with true).
			///<
			///< Returns the non-option arguments.

	void errorDuplicate( const std::string & ) ;
		///< Adds a 'duplicate' error in the constructor's error list
		///< for the given option.

	static StringArray parse( const StringArray & args_in , const Options & spec ,
		OptionMap & values_out , StringArray * errors_out = nullptr ,
		std::size_t start_position = 1U , std::size_t ignore_non_options = 0U ,
		std::function<std::string(const std::string&,bool)> callback_fn = {} ) ;
			///< A static function to contruct an OptionParser object
			///< and call its parse() method. Returns the residual
			///< non-option arguments. Throws on error.

public:
	~OptionParser() = default ;
	OptionParser( const OptionParser & ) = delete ;
	OptionParser( OptionParser && ) = delete ;
	OptionParser & operator=( const OptionParser & ) = delete ;
	OptionParser & operator=( OptionParser && ) = delete ;

private:
	bool haveSeen( const std::string & ) const ;
	bool haveSeenSame( const std::string & , const std::string & ) const ;
	static std::string::size_type eqPos( const std::string & ) ;
	static std::string eqValue( const std::string & , std::string::size_type ) ;
	void processOptionOn( char c ) ;
	void processOption( char c , const std::string & value ) ;
	void processOptionOn( const std::string & s ) ;
	void processOptionOff( const std::string & s ) ;
	void processOption( const std::string & s , const std::string & value , bool ) ;
	void errorNoValue( char ) ;
	void errorNoValue( const std::string & ) ;
	void errorUnknownOption( char ) ;
	void errorUnknownOption( const std::string & ) ;
	void errorDubiousValue( const std::string & , const std::string & ) ;
	void errorDuplicate( char ) ;
	void errorExtraValue( char , const std::string & ) ;
	void errorExtraValue( const std::string & , const std::string & ) ;
	void errorConflict( const std::string & ) ;
	void error( const std::string & ) ;
	bool haveSeenOn( const std::string & name ) const ;
	bool haveSeenOff( const std::string & name ) const ;
	static bool isOldOption( const std::string & ) ;
	static bool isNewOption( const std::string & ) ;
	static bool isAnOptionSet( const std::string & ) ;
	static std::size_t valueCount( const std::string & ) ;

private:
	const Options & m_spec ;
	OptionMap & m_map ;
	StringArray * m_errors ;
} ;

#endif
