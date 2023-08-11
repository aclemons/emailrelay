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
/// \file ggetopt.h
///

#ifndef G_GETOPT_H
#define G_GETOPT_H

#include "gdef.h"
#include "goption.h"
#include "goptions.h"
#include "goptionvalue.h"
#include "goptionparser.h"
#include "goptional.h"
#include "garg.h"
#include "gpath.h"
#include "gstringview.h"
#include "gstringarray.h"
#include "gexception.h"
#include <string>
#include <list>

namespace G
{
	class GetOpt ;
}

//| \class G::GetOpt
/// A command-line option parser.
///
/// Usage:
/// \code
///  using Layout = G::OptionsOutputLayout ;
///  using Output = G::OptionsOutput ;
///  G::Arg arg( argc , argv ) ;
///  G::GetOpt opt( arg , "e!extra!does something! extra!1!something!1" "|" "h!help!shows help!!0!!1" ) ;
///  if( opt.hasErrors() ) { opt.showErrors( std::cerr ) ; exit( 2 ) ; }
///  if( opt.contains("help"sv) ) { Output(opt.options()).showUsage( Layout().set_whatever() ,
///    std::cout , arg.prefix() , " [<more>]" ) ; exit( 0 ) ; }
///  run( opt.args() , opt.contains("extra"sv) ? opt.value("extra"sv) : std::string() ) ;
/// \endcode
///
/// This class is a thin layer over G::Options, G::OptionMap, G::OptionParser etc,
/// but also adding file loading. The G::OptionsOutput class is kept separate in
/// order to minimise dependencies.
///
class G::GetOpt
{
public:
	GetOpt( const Arg & arg , const std::string & spec , std::size_t ignore_non_options = 0U ) ;
		///< Constructor taking a Arg object and a G::Options specification string.
		///< Parsing errors are reported via errorList().

	GetOpt( const StringArray & arg , const std::string & spec , std::size_t ignore_non_options = 0U ) ;
		///< An overload taking a vector of command-line arguments.
		///< The program name in the first argument is expected but
		///< ignored.

	GetOpt( const Arg & arg , const Options & spec , std::size_t ignore_non_options = 0U ) ;
		///< A constructor overload taking an Options object.

	GetOpt( const StringArray & arg , const Options & spec , std::size_t ignore_non_options = 0U ) ;
		///< A constructor overload taking an Options object.

	void reload( const StringArray & arg , std::size_t ignore_non_options = 0U ) ;
		///< Reinitialises the object with the given command-line arguments.
		///< The program name in the first position is expected but ignored.

	void addOptionsFromFile( std::size_t n = 1U ,
		const std::string & varkey = {} , const std::string & varvalue = {} ) ;
			///< Adds options from the config file named by the n'th non-option
			///< command-line argument (zero-based and allowing for the program
			///< name in argv0). The n'th argument is then removed. Does nothing
			///< if the n'th argument does not exist or if it is empty. Throws
			///< if the file is specified but cannot be opened. Parsing errors
			///< are added to errorList(). The optional trailing string parameters
			///< are used to perform leading sub-string substitution on the
			///< filename.

	bool addOptionsFromFile( size_t n , const StringArray & extension_blocklist ) ;
		///< Adds options from the config file named by the n'th non-option
		///< argument, but not if the file extension matches any in the block
		///< list. Returns false if blocked.

	void addOptionsFromFile( const Path & file ) ;
		///< Adds options from the given config file. Throws if the file
		///< cannot be opened. Parsing errors are added to errorList().

	const std::vector<Option> & options() const ;
		///< Returns the list of option specification objects.

	const OptionMap & map() const ;
		///< Returns the map of option-values.

	Arg args() const ;
		///< Returns the G::Arg command-line, excluding options.

	StringArray errorList() const ;
		///< Returns the list of errors.

	bool hasErrors() const ;
		///< Returns true if there are errors.

	void showErrors( std::ostream & stream , const std::string & prefix_1 ,
		const std::string & prefix_2 = std::string(": ") ) const ;
			///< A convenience function which streams out each errorList()
			///< item to the given stream, prefixed with the given
			///< prefix(es). The two prefixes are simply concatenated.

	void showErrors( std::ostream & stream ) const ;
		///< An overload which has a sensible prefix.

	bool contains( char option_letter ) const ;
		///< Returns true if the command-line contains the option identified by its
		///< short-form letter.

	bool contains( string_view option_name ) const ;
		///< Returns true if the command-line contains the option identified by its
		///< long-form name.

	std::size_t count( string_view option_name ) const ;
		///< Returns the option's repeat count.

	std::string value( string_view option_name , string_view default_ = {} ) const ;
		///< Returns the value for the option identified by its long-form name.
		///< If the option is multi-valued then the returned value is a
		///< comma-separated list. If the option-value is 'on' then
		///< Str::positive() is returned; if the option-value is 'off'
		///< then the given default is returned.

	std::string value( char option_letter , string_view default_ = {} ) const ;
		///< An overload that returns the value of the option identified
		///< by its short-form letter.
		///< Precondition: contains(option_letter)

	G::optional<std::string> optional( string_view option_name ) const ;
		///< Returns an optional value identified by its long-form name.

	static StringArray readOptionsFromFile( const Path & ) ;
		///< Reads options from file as a list of strings like "--foo=bar".
		///< Throws on error.

private:
	void parseArgs( std::size_t ) ;

private:
	Options m_spec ;
	Arg m_args ;
	OptionMap m_map ;
	StringArray m_errors ;
} ;

#endif
