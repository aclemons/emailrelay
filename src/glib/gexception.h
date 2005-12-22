//
// Copyright (C) 2001-2005 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gexception.h
//

#ifndef G_EXCEPTION_H
#define G_EXCEPTION_H

#include "gdef.h"
#include <string>
#include <iostream>

namespace G
{
	class Exception ;
}

// Class: G::Exception
// Description: A general-purpose exception class derived from std::exception
// and containing a std::string.
//
class G::Exception : public std::exception 
{
protected:
	std::string m_what ;

public:
	Exception() ;
		// Default constructor.

	explicit Exception( const char * what ) ;
		// Constructor.

	explicit Exception( const std::string & what ) ;
		// Constructor.

	virtual ~Exception() throw() ;
		// Destructor.

	virtual const char * what() const throw() ;
		// Override from std::exception.

	void prepend( const char * context ) ;
		// Prepends context to the what string.
		// Inserts a separator as needed.

	void append( const char * more ) ;
		// Appends 'more' to the what string.
		// Inserts a separator as needed.

	void append( const std::string & more ) ;
		// Appends 'more' to the what string.
		// Inserts a separator as needed.

	void append( std::ostream & s ) ;
		// Appends the contents of the given std::ostringstream 
		// (sic) to the what string. Does nothing if the
		// dynamic type of 's' is not a std::ostringstream. 
		// Inserts a separator as needed.
		//
		// This method allows a derived-class exception 
		// to be constructed and thrown on one line using
		// iostream formatting.
		// Eg. throw Error( std::ostringstream() << a << b ) ;
} ;

#define G_EXCEPTION( class_name , description ) class class_name : public G::Exception { public: class_name() { m_what = description ; } public: explicit class_name ( std::ostream & stream ) { m_what = description ; append(stream) ; } public: explicit class_name( const char * more ) { m_what = description ; append(more) ; } public: explicit class_name( const std::string & more ) { m_what = description ; append(more) ; } }

#endif

