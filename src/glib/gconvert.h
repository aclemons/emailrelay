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
/// \file gconvert.h
///

#ifndef G_CONVERT_H
#define G_CONVERT_H

#include "gdef.h"
#include "gstringview.h"
#include "gexception.h"
#include <string>
#include <vector>
#include <type_traits>
#include <cstdint> // std::uint_least32_t
#include <functional>

namespace G
{
	class Convert ;
}

//| \class G::Convert
/// A static class which provides string encoding conversion functions between
/// UTF-8 and wchar_t. On Unix wchar_t strings are unencoded UCS-4; on Windows
/// wchar_t strings are UTF-16.
///
class G::Convert
{
public:
	G_EXCEPTION_CLASS( NarrowError , tx("string character-set narrowing error") )
	G_EXCEPTION_CLASS( WidenError , tx("string character-set widening error") )
	using unicode_type = std::uint_least32_t ;
	static_assert( sizeof(unicode_type) >= sizeof(wchar_t) , "" ) ;
	static constexpr unicode_type unicode_error = ~(unicode_type)0 ;
	using ParseFn = std::function<bool(unicode_type,std::size_t,std::size_t)> ;

	static std::wstring widen( std::string_view ) ;
		///< Widens from UTF-8 to UTF-16/UCS-4 wstring. Invalid input characters
		///< are substituted with L'\xFFFD'.

	static bool valid( std::string_view ) noexcept ;
		///< Returns true if the string is valid UTF-8.

	static std::string narrow( const std::wstring & ) ;
		///< Narrows from UTF-16/UCS-4 wstring to UTF-8. Invalid input characters
		///< are substituted with u8"\uFFFD", ie. "\xEF\xBF\xBD".

	static std::string narrow( const wchar_t * ) ;
		///< Pointer overload.

	static std::string narrow( const wchar_t * , std::size_t n ) ;
		///< String-view overload.

	static bool invalid( const std::wstring & ) ;
		///< Returns true if the string contains L'\xFFFD'.

	static bool invalid( const std::string & ) ;
		///< Returns true if the string contains u8"\uFFFD".

	static std::size_t u8out( unicode_type , char * & ) noexcept ;
		///< Puts a Unicode character value into a character buffer with
		///< UTF-8 encoding. Advances the pointer by reference and returns
		///< the number of bytes (1..4). Returns zero on error, without
		///< advancing the pointer.

	static std::pair<unicode_type,std::size_t> u8in( const unsigned char * , std::size_t n ) noexcept ;
		///< Reads a Unicode character from a UTF-8 buffer together with
		///< the number of bytes consumed. Returns [unicode_error,1] on
		///< error.

	static void u8parse( std::string_view , ParseFn ) ;
		///< Calls a function for each Unicode value in the given
		///< UTF-8 string. Stops if the callback returns false. The
		///< callback parameters are: Unicode value (0xFFFD on
		///< error), UTF-8 bytes consumed, and UTF-8 byte offset.

	static bool utf16( bool ) ;
		///< Forces UTF-16 even if wchar_t is 4 bytes. Used in testing.

public:
	Convert() = delete ;

private:
	static bool m_utf16 ;
	static std::wstring widenImp( const char * , std::size_t ) ;
	static std::size_t widenImp( const char * , std::size_t , wchar_t * , bool * = nullptr ) noexcept ;
	static std::string narrowImp( const wchar_t * , std::size_t ) ;
	static std::size_t narrowImp( const wchar_t * , std::size_t , char * ) noexcept ;
	static unicode_type unicode_cast( wchar_t c ) noexcept ;
	static char char_cast( unsigned int ) noexcept ;
} ;

#endif
