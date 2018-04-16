//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file garg.h
///

#ifndef G_ARG_H
#define G_ARG_H

#include "gdef.h"
#include "gstrings.h"
#include <vector>
#include <string>

namespace G
{
	class Arg ;
}

/// \class G::Arg
/// A class which holds a represention of the argc/argv command line array,
/// and supports simple command-line parsing.
///
/// A copy of argv[0] is squirrelled away and made accessible via a static
/// method.
///
/// \see G::GetOpt
///
class G::Arg
{
public:
	typedef size_t size_type ;

	Arg( int argc , char * argv [] ) ;
		///< Constructor taking argc/argv. Should not be used in a shared
		///< object or dll.

	explicit Arg( const G::StringArray & ) ;
		///< Constructor taking an array of command-line arguments. The
		///< program name in the first position is expected but may be
		///< ignored.

	Arg() ;
		///< Default constructor. Initialise with parse().

	void parse( HINSTANCE hinstance , const std::string & command_line_tail ) ;
		///< Parses the given command-line tail, splitting it up into
		///< an array of tokens. The v(0) value comes from the supplied
		///< instance handle.

	void parse( const std::string & command_line ) ;
		///< Parses the given command line, splitting it up into an array
		///< of tokens. The command-line should start with the v(0) value.

	void reparse( const std::string & command_line_tail ) ;
		///< Reinitialises the object with the given command-line tail. The
		///< command-line should not contain the program name: the v(0)
		///< value and prefix() are unchanged.

	~Arg() ;
		///< Destructor.

	static std::string v0() ;
		///< Returns a copy of argv[0] from the first call to the constructor
		///< that takes argc/argv. Returns the empty string if that constructor
		///< overload has never been used. See also exe().

	static std::string exe( bool do_throw = true ) ;
		///< Returns Process::exe() or an absolute path constructed from v0().
		///< Throws on error by default, or optionally returns the empty string.
		///< See also v0().

	size_type c() const ;
		///< Returns the number of tokens in the command line, including the
		///< program name.

	std::string v( size_type i ) const ;
		///< Returns the i'th argument.
		///< Precondition: i < c()

	std::string prefix() const ;
		///< Returns the basename of v(0) without any extension. Typically used
		///< as a prefix in error messages.

	static const char * prefix( char * argv[] ) ;
		///< An exception-free version of prefix() which can be used in
		///< main() outside of the outermost try block.

	bool contains( const std::string & option , size_type option_args = 0U , bool case_sensitive = true ) const ;
		///< Returns true if the command line contains the given option
		///< with enough command line arguments left to satisfy the required
		///< number of option arguments. (By convention an option starts
		///< with a dash, but that is not required here; it's just a string
		///< that is matched against command-line arguments.)

	size_type index( const std::string & option , size_type option_args = 0U ) const ;
		///< Returns the index of the given option. Returns zero if not present.

	bool remove( const std::string & option , size_type option_args = 0U ) ;
		///< Removes the given option and its arguments. Returns false if
		///< the option does not exist.

	void removeAt( size_type option_index , size_type option_args = 0U ) ;
		///< Removes the given argument and the following 'option_args' ones.

	StringArray array( unsigned int shift = 0U ) const ;
		///< Returns the arguments a string array, including the program name
		///< in the first position.

	Arg & operator=( const Arg & ) ;
		///< Assignment operator.

	Arg( const Arg & ) ;
		///< Copy constructor.

private:
	bool find( bool , const std::string & , size_type , size_type * ) const ;
	void setPrefix() ;
	void setExe() ;
	static bool match( bool , const std::string & , const std::string & ) ;
	void parseCore( const std::string & ) ;
	static void protect( std::string & ) ;
	static void unprotect( StringArray & ) ;
	static void dequote( StringArray & ) ;

private:
	StringArray m_array ;
	static bool m_first ;
	static std::string m_v0 ;
	static std::string m_cwd ;
} ;

#endif
