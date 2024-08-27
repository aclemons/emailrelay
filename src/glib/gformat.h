//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gformat.h
///

#ifndef G_FORMAT_H
#define G_FORMAT_H

#include "gdef.h"
#include "gstringarray.h"
#include <string>
#include <ostream>
#include <sstream>

namespace G
{
	class format ;
}

//| \class G::format
/// A simple version of boost::format for formatting strings in
/// an i18n-friendly way.
///
/// Eg:
/// \code
/// using G::format ; // or boost::format
/// std::cout << format("a %2% %1% d") % "c" % "b" << "\n" ;
/// \endcode
///
class G::format
{
public:
	explicit format( const std::string & fmt ) ;
		///< Constructor.

	explicit format( const char * fmt ) ;
		///< Constructor.

	format & parse( const std::string & fmt ) ;
		///< Resets the object with the given format string.

	format & parse( const char * fmt ) ;
		///< Resets the object with the given format string.

	std::string str() const ;
		///< Returns the string.

	std::size_t size() const ;
		///< Returns the string size.

	template <typename T> format & operator%( const T & ) ;
		///< Applies a substitution value.

private:
	void apply( const std::string & ) ;
	static bool isdigit( char ) noexcept ;

private:
	std::string m_fmt ;
	std::size_t m_i {0U} ;
	StringArray m_values ;
} ;

namespace G
{
	std::ostream & operator<<( std::ostream & stream , const format & f ) ;
	inline std::string str( const format & f )
	{
		return f.str() ;
	}
	template <typename T> format & format::operator%( const T & item )
	{
		std::ostringstream ss ;
		ss << item ;
		apply( ss.str() ) ;
		return *this ;
	}
}

#endif
