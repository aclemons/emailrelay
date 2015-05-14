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
/// \file ggetopt.h
///

#ifndef G_GETOPT_H
#define G_GETOPT_H

#include "gdef.h"
#include "goptions.h"
#include "goptionvalue.h"
#include "garg.h"
#include "gstrings.h"
#include "gexception.h"
#include <string>
#include <list>
#include <map>

/// \namespace G
namespace G
{
	class GetOpt ;
}

/// \class G::GetOpt
/// A command line option parser.
///
/// Usage:
/// \code
///		G::Arg arg( argc , argv ) ;
///		G::GetOpt opt( arg , "e!extra!does something! extra!1!something!1" "|" "h!help!shows help!!0!!1" ) ;
///		if( opt.hasErrors() ) { opt.showErrors( std::cerr ) ; exit( 2 ) ; }
///		if( opt.contains("help") ) { opt.options().showUsage( std::cout , "" , " [<more>]" ) ; exit( 0 ) ; }
///		run( opt.args() , opt.contains("extra") ? opt.value("extra") : std::string() ) ;
/// \endcode
///
/// \see G::Arg
///
class G::GetOpt
{
public:
	typedef std::string::size_type size_type ;

	GetOpt( const Arg & arg , const std::string & spec ) ;
		///< Constructor taking a Arg reference and a G::Options specification string.
		///< Parsing errors are reported via errorList().

	GetOpt( const StringArray & arg , const std::string & spec ) ;
		///< An overload taking a vector of command-line arguments.
		///< The program name in the first argument is expected but
		///< ignored.

	void reload( const StringArray & arg ) ;
		///< Reinitialises the object with the given command-line arguments.
		///< The program name in the first position is expected but ignored.

	void addOptionsFromFile( size_type n = 1U ) ;
		///< Adds options from the config file named by the n'th non-option
		///< command-line argument (zero-based but allowing for the program
		///< name in argv0). The n'th argument is then removed. Does nothing
		///< if the n'th argument does not exists or if it is empty. Throws
		///< if the file is specified but cannot be opened. Parsing errors 
		///< are added to errorList().

	const Options & options() const ;
		///< Returns a reference to the internal option specification object.

	Arg args() const ;
		///< Returns all the non-option command-line arguments.

	StringArray errorList() const ;
		///< Returns the list of errors.

	bool hasErrors() const ;
		///< Returns true if there are errors.

	void showErrors( std::ostream & stream , std::string prefix_1 ,
		std::string prefix_2 = std::string(": ") ) const ;
			///< A convenience function which streams out each errorList()
			///< item to the given stream, prefixed with the given
			///< prefix(es). The two prefixes are simply concatenated.

	void showErrors( std::ostream & stream ) const ;
		///< An overload which has a sensible prefix.

	bool contains( char option_letter ) const ;
		///< Returns true if the command line contains the given short-form option.

	bool contains( const std::string & option_name ) const ;
		///< Returns true if the command line contains the given long-form option.

	std::string value( const std::string & option_name ) const ;
		///< Returns the value related to the given valued option.
		///< Precondition: contains(option_name)

	std::string value( char option_letter ) const ;
		///< Returns the value related to the given valued option.
		///< Precondition: contains(option_letter)

private:
	void operator=( const GetOpt & ) ;
	GetOpt( const GetOpt & ) ;
	void parseArgs( Arg & ) ;
	StringArray readFile( std::istream & f , const StringArray & stop ) ;

private:
	typedef std::map<std::string,OptionValue> OptionValueMap ;
	Options m_spec ;
	Arg m_args ;
	OptionValueMap m_map ;
	StringArray m_errors ;
} ;

#endif
