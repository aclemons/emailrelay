//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
	class ConvertImp ;
}

/// \class G::Convert
/// A static class which provides string encoding conversion functions. Supported
/// encodings are "ANSI" (CP-1252/ISO-8859-1) char, UTF-16 wchar, and UTF-8
/// multi-byte char. Conversions that can fail take a ThrowOnError parameter
/// which is used to add context information to the G::Convert::Error exception
/// that is thrown.
///
/// Eg:
/// \code
/// std::string to_utf8( const std::wstring & wide_input )
/// {
///     G::Convert::utf8 utf8_result ;
///     G::Convert::convert( utf8_result , wide_input ) ;
///     return utf8_result.s ;
/// }
/// std::string to_ansi( const std::wstring & wide_input )
/// {
///     std::string ansi_result ;
///     G::Convert::convert( ansi_result , wide_input , G::Convert::ThrowOnError("to_ansi") ) ;
///     return ansi_result ;
/// }
/// \endcode
///
class G::Convert
{
public:
	G_EXCEPTION_CLASS( Error , "string conversion error" ) ;
	typedef std::basic_string<TCHAR> tstring ;

	struct utf8 /// A string wrapper that indicates UTF-8 encoding.
	{
		utf8() {}
		explicit utf8( const std::string & s_ ) : s(s_) {}
		std::string s ;
	} ;

	struct ThrowOnError /// Holds context information which convert() adds to the exception when it fails.
	{
		ThrowOnError() {}
		explicit ThrowOnError( const std::string & context_ ) : context(context_) {}
		std::string context ;
	} ;

	static void convert( utf8 & utf_out , const std::string & in_ ) ;
		///< Converts between string types/encodings:
		///< ansi to utf8.

	static void convert( utf8 & utf_out , const utf8 & in_ ) ;
		///< Converts between string types/encodings:
		///< utf8 to utf8.

	static void convert( utf8 & utf_out , const std::wstring & in_ ) ;
		///< Converts between string types/encodings:
		///< utf16 to utf8.

	static void convert( std::string & ansi_out , const std::string & in_ ) ;
		///< Converts between string types/encodings:
		///< ansi to ansi.

	static void convert( std::string & ansi_out , const utf8 & in_ , const ThrowOnError & ) ;
		///< Converts between string types/encodings:
		///< utf8 to ansi.

	static void convert( std::string & ansi_out , const std::wstring & in_ , const ThrowOnError & ) ;
		///< Converts between string types/encodings:
		///< utf16 to ansi.

	static void convert( std::wstring & wide_out , const std::string & ansi_in ) ;
		///< Converts between string types/encodings:
		///< ansi to utf16.

	static void convert( std::wstring & wide_out , const utf8 & utf_in ) ;
		///< Converts between string types/encodings:
		///< utf8 to utf16.

	static void convert( std::wstring & wide_out , const std::wstring & wide_in ) ;
		///< Converts between string types/encodings:
		///< utf8 to utf16.

	static void convert( std::string & ansi_out , const std::string & in_ , const ThrowOnError & ) ;
		///< An overload for TCHAR shenanigans on windows.
		///< Converts between string types/encodings:
		///< ansi to ansi.

private:
	Convert() g__eq_delete ;
	static std::string narrow( const std::wstring & s , bool is_utf8 , const std::string & = std::string() ) ;
	static std::wstring widen( const std::string & s , bool is_utf8 , const std::string & = std::string() ) ;
} ;

#endif
