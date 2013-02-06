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
/// \file gconvert.h
///

#ifndef G_CONVERT_H
#define G_CONVERT_H

#include "gdef.h"
#include "gexception.h"
#include <string>

/// \namespace G
namespace G
{
	class Convert ;
}

/// \class G::Convert
/// A static class which provides string encoding conversion functions.
///
class G::Convert 
{
public:
	G_EXCEPTION_CLASS( Error , "string conversion error" ) ;
	typedef std::basic_string<TCHAR> tstring ;

	/// A string wrapper that indicates UTF-8 encoding.
	struct utf8 
	{
		utf8() {}
		explicit utf8( const std::string & s_ ) : s(s_) {}
		std::string s ;
	} ;

	/// Holds error context information for when convert() may fail.
	struct ThrowOnError 
	{
		ThrowOnError() {}
		explicit ThrowOnError( const std::string & context_ ) : context(context_) {}
		std::string context ;
	} ;

	static void convert( utf8 & utf_out , const std::string & in_ ) ;
		///< Converts between string types/encodings.

	static void convert( utf8 & utf_out , const utf8 & in_ ) ;
		///< Converts between string types/encodings.

	static void convert( utf8 & utf_out , const std::wstring & in_ ) ;
		///< Converts between string types/encodings.

	static void convert( std::string & ansi_out , const std::string & in_ ) ;
		///< Converts between string types/encodings.

	static void convert( std::string & ansi_out , const std::string & in_ , 
		const ThrowOnError & just_for_tchar_overloading ) ;
			///< Converts between string types/encodings.

	static void convert( std::string & ansi_out , const utf8 & in_ , const ThrowOnError & ) ;
		///< Converts between string types/encodings.

	static void convert( std::string & ansi_out , const std::wstring & in_ , const ThrowOnError & ) ;
		///< Converts between string types/encodings.

	static void convert( std::wstring & out_ , const std::string & in_ ) ;
		///< Converts between string types/encodings.

	static void convert( std::wstring & out_ , const utf8 & in_ ) ;
		///< Converts between string types/encodings.

	static void convert( std::wstring & out_ , const std::wstring & in_ ) ;
		///< Converts between string types/encodings.

private:
	static std::string narrow( const std::wstring & s , bool is_utf8 , const std::string & = std::string() ) ;
	static std::wstring widen( const std::string & s , bool is_utf8 , const std::string & = std::string() ) ;
	Convert() ; // not implemented
} ;

#endif
