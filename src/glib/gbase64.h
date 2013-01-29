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
/// \file gbase64.h
///

#ifndef G_BASE64_H
#define G_BASE64_H

#include "gdef.h"
#include "gexception.h"
#include <string>

/// \namespace G
namespace G
{
	class Base64 ;
}

/// \class G::Base64
/// A base64 codec class.
/// \see RFC 1341 section 5.2
///
class G::Base64 
{
public:
	G_EXCEPTION( Error , "base64 encoding error" ) ;

	static std::string encode( const std::string & s , const std::string & line_break ) ;
		///< Encodes the given string.

	static std::string encode( const std::string & s ) ;
		///< Encodes the given string. Uses carriage-return-line-feed
		///< as the line-break string.

	static std::string decode( const std::string & ) ;
		///< Decodes the given string. Throws an exception
		///< if not a valid encoding.

	static bool valid( const std::string & ) ;
		///< Returns true if the string can be decoded.

private:
	Base64() ;
	static inline g_uint32_t numeric( char c ) ;
	static inline void accumulate_8( g_uint32_t & n , std::string::const_iterator & , 
		std::string::const_iterator , int & ) ;
	static inline size_t hi_6( g_uint32_t n ) ;
	static inline void generate_6( g_uint32_t & n , int & i , std::string & result ) ;
	static inline char to_char( g_uint32_t n ) ;
	static inline size_t index( char c , bool & error ) ;
	static inline size_t accumulate_6( g_uint32_t & n , char c_in , int & , bool & error ) ;
	static inline g_uint32_t hi_8( g_uint32_t n ) ;
	static inline void generate_8( g_uint32_t & n , int & i , std::string & result ) ;
	static std::string decode( const std::string & s , bool & error ) ;
} ;

#endif

