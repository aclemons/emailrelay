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
/// \file gexception.h
///

#ifndef G_EXCEPTION_H
#define G_EXCEPTION_H

#include "gdef.h"
#include "gstringview.h"
#include "ggettext.h"
#include <initializer_list>
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
/// The G_EXCEPTION macro is normally used in class declarations, as follows:
/// \code
/// class Foo
/// {
/// public:
///    G_EXCEPTION( Error , tx("foo error") ) ;
/// ...
/// };
/// \endcode
///
/// The tx() identifies the string at build-time as requiring translation.
/// The G_EXCEPTION macro adds a "G::" scope so that at compile-time the
/// do-nothing function G::tx() from "ggettext.h" is used. The xgettext
/// utility should be run as "xgettext --c++ --keyword=tx". Note that
/// xgettext keywords cannot use scoped names.
///
class G::Exception : public std::runtime_error
{
public:
	Exception( std::initializer_list<string_view> ) ;
		///< Constructor.

	explicit Exception( string_view what ) ;
		///< Constructor.

	Exception( string_view what , string_view more ) ;
		///< Constructor.

	Exception( string_view what , string_view more1 , string_view more2 ) ;
		///< Constructor.

	Exception( string_view what , string_view more1 , string_view more2 , string_view more3 ) ;
		///< Constructor.

	Exception( string_view what , string_view more1 , string_view more2 , string_view more3 , string_view more4 ) ;
		///< Constructor.

private:
	static std::string join( std::initializer_list<string_view> ) ;
} ;

#define G_EXCEPTION_CLASS_( class_name , description ) \
	class class_name : public G::Exception { public: /* NOLINT bugprone-macro-parentheses */ \
	class_name() : G::Exception(description) {} \
	explicit class_name( G::string_view more ) : G::Exception(description,more) {} \
	class_name( G::string_view more1 , G::string_view more2 ) : G::Exception(description,more1,more2) {} \
	class_name( G::string_view more1 , G::string_view more2 , G::string_view more3 ) : G::Exception(description,more1,more2,more3) {} }

#define G_EXCEPTION_FUNCTION_( name , description ) \
	inline static G::Exception name() { return G::Exception((description)) ; } \
	inline static G::Exception name( G::string_view s ) { return G::Exception(description,s) ; } \
	inline static G::Exception name( G::string_view s1 , G::string_view s2 ) { return G::Exception(description,s1,s2) ; } \
	inline static G::Exception name( G::string_view s1 , G::string_view s2 , G::string_view s3 ) { return G::Exception(description,s1,s2,s3) ; } \
	inline static G::Exception name( G::string_view s1 , G::string_view s2 , G::string_view s3 , G::string_view s4 ) { return G::Exception(description,s1,s2,s3,s4) ; }

#define G_EXCEPTION_CLASS( class_name , tx_description ) G_EXCEPTION_CLASS_( class_name , G::tx_description )
#define G_EXCEPTION_FUNCTION( name , tx_description ) G_EXCEPTION_FUNCTION_( name , G::tx_description )
#define G_EXCEPTION( class_name , tx_description ) G_EXCEPTION_FUNCTION_( class_name , G::tx_description )

#endif
