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
/// \file gconvert.h
///

#ifndef G_CONVERT_H
#define G_CONVERT_H

#include "gdef.h"
#include "gexception.h"
#include <string>

namespace G
{
	class Convert ;
}

//| \class G::Convert
/// A static class which provides string encoding conversion functions. Supported
/// encodings are a 'native' encoding (which might be a SBCS code page, DBCS, or
/// UTF-8, and possibly related to locale environment variables), UTF-16 wchar,
/// and UTF-8 multi-byte char.
///
/// Conversions that can fail take a ThrowOnError parameter which is used to add
/// context information to the G::Convert::Error exception that is thrown.
///
/// Eg:
/// \code
/// std::string to_utf8( const std::wstring & wide_input )
/// {
///     G::Convert::utf8 utf8_result ;
///     G::Convert::convert( utf8_result , wide_input ) ;
///     return utf8_result.s ;
/// }
/// std::string to_native( const std::wstring & wide_input )
/// {
///     std::string native_result ;
///     G::Convert::convert( native_result , wide_input , G::Convert::ThrowOnError("to_native") ) ;
///     return native_result ;
/// }
/// \endcode
///
class G::Convert
{
public:
	G_EXCEPTION_CLASS( Error , tx("string character-set conversion error") ) ;
	using tstring = std::basic_string<TCHAR> ;

	struct utf8 /// A string wrapper that indicates UTF-8 encoding.
	{
		utf8() = default;
		explicit utf8( const std::string & s_ ) : s(s_) {}
		std::string s ;
	} ;

	struct ThrowOnError /// Holds context information which convert() adds to the exception when it fails.
	{
		ThrowOnError() = default;
		explicit ThrowOnError( const std::string & context_ ) : context(context_) {}
		std::string context ;
	} ;

	static void convert( utf8 & utf_out , const std::string & in_ ) ;
		///< Converts between string types/encodings:
		///< native to utf8.

	static void convert( utf8 & utf_out , const utf8 & in_ ) ;
		///< Converts between string types/encodings:
		///< utf8 to utf8.

	static void convert( utf8 & utf_out , const std::wstring & in_ ) ;
		///< Converts between string types/encodings:
		///< utf16 to utf8.

	static void convert( std::string & native_out , const std::string & in_ ) ;
		///< Converts between string types/encodings:
		///< native to native.

	static void convert( std::string & native_out , const utf8 & in_ , const ThrowOnError & ) ;
		///< Converts between string types/encodings:
		///< utf8 to native.

	static void convert( std::string & native_out , const std::wstring & in_ , const ThrowOnError & ) ;
		///< Converts between string types/encodings:
		///< utf16 to native.

	static void convert( std::wstring & wide_out , const std::string & native_in ) ;
		///< Converts between string types/encodings:
		///< native to utf16.

	static void convert( std::wstring & wide_out , const utf8 & utf_in ) ;
		///< Converts between string types/encodings:
		///< utf8 to utf16.

	static void convert( std::wstring & wide_out , const std::wstring & wide_in ) ;
		///< Converts between string types/encodings:
		///< utf16 to utf16.

	static void convert( std::string & native_out , const std::string & in_ , const ThrowOnError & ) ;
		///< An overload for TCHAR shenanigans on windows. Note that
		///< a TCHAR can sometimes be a char, depending on the build
		///< options, so this three-parameter overload allows for the
		///< input to be a basic_string<TCHAR>, whatever the build.
		///<
		///< Converts between string types/encodings:
		///< native to native.

public:
	Convert() = delete ;

private:
	static std::string narrow( const std::wstring & s , bool is_utf8 , const std::string & = {} ) ;
	static std::wstring widen( const std::string & s , bool is_utf8 , const std::string & = {} ) ;
} ;

#endif
