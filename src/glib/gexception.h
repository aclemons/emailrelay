//
// Copyright (C) 2001-2021 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include <stdexcept>

namespace G
{
	class Exception ;
}

//| \class G::Exception
/// A general-purpose exception class derived from std::exception and containing
/// an error message. Provides constructors that simplify the assembly of multi-part
/// error messages.
///
/// Usage:
/// \code
///	throw G::Exception( "initialisation error" , "no such file" , path ) ;
/// \endcode
///
class G::Exception : public std::runtime_error
{
public:
	explicit Exception( const char * what ) ;
		///< Constructor.

	explicit Exception( const std::string & what ) ;
		///< Constructor.

	Exception( const char * what , const std::string & more ) ;
		///< Constructor.

	Exception( const std::string & what , const std::string & more ) ;
		///< Constructor.

	Exception( const char * what , const std::string & more1 , const std::string & more2 ) ;
		///< Constructor.

	Exception( const std::string & what , const std::string & more1 , const std::string & more2 ) ;
		///< Constructor.

	Exception( const char * what , const std::string & more1 , const std::string & more2 , const std::string & more3 ) ;
		///< Constructor.

	Exception( const std::string & what , const std::string & more1 , const std::string & more2 , const std::string & more3 ) ;
		///< Constructor.

	Exception translated() const ;
		///< Returns a new exception that is possibly translated using
		///< gettext(). (Use gettext_noop() in G_EXCEPTION declarations
		///< to mark the string for translation.)
} ;

#define G_EXCEPTION_CLASS( class_name , description ) class class_name : public G::Exception { public: class_name() : G::Exception((description)) {} explicit class_name(const char *more) : G::Exception((description),more) {} explicit class_name(const std::string &more) : G::Exception((description),more) {} class_name(const std::string &more1,const std::string &more2) : G::Exception((description),more1,more2) {} class_name(const std::string &more1,const std::string &more2,const std::string &more3) : G::Exception((description),more1,more2,more3) {} }

#define G_EXCEPTION_FUNCTION( name , description ) \
	inline static G::Exception name() { return G::Exception((description)) ; } \
	inline static G::Exception name( const std::string & s ) { return G::Exception((description),s) ; } \
	inline static G::Exception name( const std::string & s1 , const std::string & s2 ) { return G::Exception((description),s1,s2) ; } \
	inline static G::Exception name( const std::string & s1 , const std::string & s2 , const std::string & s3 ) { return G::Exception((description),s1,s2,s3) ; }

// as a code-size optimisation G_EXCEPTION can be implemented as inline
// functions using G_EXCEPTION_FUNCION rather than separate classes
// with G_EXCEPTION_CLASS -- G_EXCEPTION_CLASS is required when catching
// specific errors
//
#ifndef G_EXCEPTION
#define G_EXCEPTION( class_name , description ) G_EXCEPTION_FUNCTION( class_name , description )
#endif

#endif
