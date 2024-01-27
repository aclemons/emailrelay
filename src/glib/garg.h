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
/// \file garg.h
///

#ifndef G_ARG_H
#define G_ARG_H

#include "gdef.h"
#include "gstringarray.h"
#include <vector>
#include <string>

namespace G
{
	class Arg ;
}

//| \class G::Arg
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
	Arg( int argc , char ** argv ) ;
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

	static std::string v0() ;
		///< Returns a copy of argv[0] from the first call to the constructor
		///< that takes argc/argv. Returns the empty string if that constructor
		///< overload has never been used. See also exe().

	static std::string exe( bool do_throw = true ) ;
		///< Returns Process::exe() or an absolute path constructed from v0()
		///< and possibly using the cwd. Throws on error by default, or
		///< optionally returns the empty string.
		///< See also v0().

	std::size_t c() const ;
		///< Returns the number of tokens in the command line, including the
		///< program name.

	std::string v( std::size_t i ) const ;
		///< Returns the i'th argument.
		///< Precondition: i < c()

	std::string v( std::size_t i , const std::string & default_ ) const ;
		///< Returns the i'th argument or the default if out of range.

	std::string prefix() const ;
		///< Returns the basename of v(0) without any extension. Typically used
		///< as a prefix in error messages.

	static const char * prefix( char ** argv ) noexcept ;
		///< An exception-free version of prefix() which can be used in
		///< main() outside of the outermost try block.

	bool contains( const std::string & option , std::size_t option_args = 0U , bool case_sensitive = true ) const ;
		///< Returns true if the command line contains the given option
		///< with enough command line arguments left to satisfy the required
		///< number of option arguments. (By convention an option starts
		///< with a dash, but that is not required here; it's just a string
		///< that is matched against command-line arguments.)

	std::size_t count( const std::string & option ) const ;
		///< Returns the number of times the given string appears in the
		///< list of arguments.

	std::size_t index( const std::string & option , std::size_t option_args = 0U ,
		std::size_t default_ = 0U ) const ;
			///< Returns the index of the given option. Returns zero
			///< (or the given default) if not present.

	std::size_t match( const std::string & prefix ) const ;
		///< Returns the index of the first argument that matches the
		///< given prefix. Returns zero if none.

	bool remove( const std::string & option , std::size_t option_args = 0U ) ;
		///< Removes the given option and its arguments. Returns false if
		///< the option does not exist.

	std::string removeValue( const std::string & option , const std::string & default_ = {} ) ;
		///< Removes the given single-valued option and its value. Returns
		///< the option value or the default if the option does not exist.

	std::string removeAt( std::size_t option_index , std::size_t option_args = 0U ) ;
		///< Removes the given argument and the following 'option_args' ones.
		///< Returns v(option_index+(option_args?1:0),""). Does nothing and
		///< returns the empty string if the index is zero or out of range.

	StringArray array( unsigned int shift = 0U ) const ;
		///< Returns the arguments as a string array, with an optional shift.
		///< A shift of one will remove the program name.

	StringArray::const_iterator cbegin() const ;
		///< Returns a begin operator, advanced to exclude argv0.

	StringArray::const_iterator cend() const ;
		///< Returns the end operator.

private:
	std::size_t find( bool , const std::string & , std::size_t , std::size_t * ) const ;
	static bool strmatch( bool , const std::string & , const std::string & ) ;
	void parseImp( const std::string & ) ;

private:
	StringArray m_array ;
	static std::string m_v0 ;
	static std::string m_cwd ;
} ;

namespace G
{
	inline G::StringArray::const_iterator begin( const G::Arg & arg ) { return arg.cbegin() ; }
	inline G::StringArray::const_iterator end( const G::Arg & arg ) { return arg.cend() ; }
}

#endif
