//
// Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// garg.h
//	

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

// Class: G::Arg
// Description: A class which holds a represention of the
// argc/argv command line array. Also does simple command line 
// parsing, and, under Windows, command line splitting (the 
// single command line string is split into an argv[] array, 
// including argv[0]).
//
// See also: G::GetOpt
//
class G::Arg 
{
public:
	Arg( int argc , char *argv[] ) ;
		// Constructor taking argc/argv.

	Arg() ;
		// Default constructor for Windows. 
		// Initialise (once) with parse().
		
	void parse( HINSTANCE hinstance , const std::string & command_line ) ;
		// Windows only.
		//
		// Parses the given command line, splitting
		// it up into an array of tokens.
		// The full exe name is automatically
		// added as the first token (cf. argv[0]).

	void reparse( const std::string & command_line ) ;
		// Reinitialises the object with the given
		// command-line. The command-line should not
		// contain the program name: the v(0) value 
		// and prefix() are unchanged.
		
	~Arg() ;
		// Destructor.
		
	size_t c() const ;
		// Returns the number of tokens in the
		// command line, including the program
		// name.

	std::string v( size_t i ) const ;
		// Returns the i'th argument.
		// Precondition: i < c()

	std::string prefix() const ;
		// Returns the basename of v(0) without
		// any extension.

	static const char * prefix( char * argv[] ) ; // throw()
		// An exception-free version of prefix() which can
		// be used in main() outside of the outermost try
		// block.

	bool contains( const std::string & sw , 
		size_t sw_args = 0U , bool case_sensitive = true ) const ;
			// Returns true if the command line
			// contains the given switch with enough
			// command line arguments left to satisfy 
			// the given number of switch arguments.

	size_t index( const std::string & sw , size_t sw_args = 0U ) const ;
		// Returns the index of the given switch.
		// Returns zero if not present.

	void remove( const std::string & sw , size_t sw_args = 0U ) ;
		// Removes the given switch and its
		// arguments.
		//
		// Precondition: contains()

	void removeAt( size_t sw_index , size_t sw_args = 0U ) ;
		// Removes the given argument and the
		// following <sw_args> ones.

	Arg & operator=( const Arg & ) ;
		// Assignment operator.

	Arg( const Arg & ) ;
		// Copy constructor.

private:
	static std::string moduleName( HINSTANCE h ) ;
	bool find( bool , const std::string & , size_t , size_t * ) const ;
	void setPrefix() ;
	static bool match( bool , const std::string & , const std::string & ) ;
	void parseCore( const std::string & ) ;
	void protect( std::string & ) ;
	void unprotect( StringArray & ) ;

private:
	StringArray m_array ;
	std::string m_prefix ;
} ;

#endif
