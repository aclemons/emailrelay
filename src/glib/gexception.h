//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gexception.h
///

#ifndef G_EXCEPTION_H
#define G_EXCEPTION_H

#include "gdef.h"
#include <string>
#include <iostream>

/// \namespace G
namespace G
{
	class Exception ;
}

/// \class G::Exception
/// A general-purpose exception class derived from std::exception
/// and containing a std::string. Provides constructors that simplify the
/// assembly of multi-part error messages.
///
/// Usage:
/// \code
///	throw G::Exception( "initialisation error" , "no such file" , path ) ;
/// \endcode
///
class G::Exception : public std::exception 
{
protected:
	std::string m_what ;

public:
	Exception() ;
		///< Default constructor.

	explicit Exception( const char * what ) ;
		///< Constructor.

	explicit Exception( const std::string & what ) ;
		///< Constructor.

	Exception( const char * what , const std::string & more ) ;
		///< Constructor.

	Exception( const std::string & what , const std::string & more ) ;
		///< Constructor.

	Exception( const std::string & what , const std::string & more1 , const std::string & more2 ) ;
		///< Constructor.

	virtual ~Exception() throw() ;
		///< Destructor.

	virtual const char * what() const throw() ;
		///< Override from std::exception.

	void prepend( const char * context ) ;
		///< Prepends context to the what string.
		///< Inserts a separator as needed.

	void append( const char * more ) ;
		///< Appends 'more' to the what string.
		///< Inserts a separator as needed.

	void append( const std::string & more ) ;
		///< Appends 'more' to the what string.
		///< Inserts a separator as needed.
} ;

#define G_EXCEPTION_CLASS( class_name , description ) class class_name : public G::Exception { public: class_name() : G::Exception(description) {} explicit class_name(const char *more) : G::Exception(description,more) {} explicit class_name(const std::string &more) : G::Exception(description,more) {} class_name(const std::string &more1,const std::string &more2) : G::Exception(description,more1,more2) {} }

/// define as a function rather than a type if optimising for size
#ifdef USE_SMALL_EXCEPTIONS
 #define G_EXCEPTION( fn_name , description ) static inline int fn_name() { throw G::Exception(description) ; return 0 ; } static inline int fn_name( const std::string & more ) { throw G::Exception(description,more) ; return 0 ; }
#else
 #define G_EXCEPTION( class_name , description ) G_EXCEPTION_CLASS( class_name , description )
#endif

#endif
