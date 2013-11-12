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
/// \file gstr.h
///

#ifndef G_STR_H
#define G_STR_H

#include "gdef.h"
#include "gexception.h"
#include "gstrings.h"
#include <string>
#include <sstream>
#include <iostream>
#include <list>
#include <vector>

/// \namespace G
namespace G
{
	class Str ;
}

/// \class G::Str
/// A static class which provides string helper functions.
///
class G::Str 
{
public:
	G_EXCEPTION_CLASS( Overflow , "conversion error: over/underflow" ) ;
	G_EXCEPTION_CLASS( InvalidFormat, "conversion error: invalid format" ) ;
	typedef std::string::size_type size_type ;

	static bool replace( std::string & s , 
		const std::string & from , const std::string & to , 
		size_type * pos_p = NULL ) ;
			///< Replaces 'from' with 'to', starting at offset '*pos_p'. 
			///< Returns true if a substitution was made, and adjusts 
			///< '*pos_p' by to.length().

	static unsigned int replaceAll( std::string & s , const std::string & from , const std::string & to ) ;
		///< Does a global replace on string 's', replacing all 
		///< occurences of sub-string 'from' with 'to'. Returns 
		///< the number of substitutions made. Consider using 
		///< in a while loop if 'from' is more than one character.

	static unsigned int replaceAll( std::string & s , const char * from , const char * to ) ;
		///< A c-string overload, provided for performance reasons.

	static void removeAll( std::string & , char ) ;
		///< Removes all occurrences of the character from the string.

	static void trimLeft( std::string & s , const std::string & ws , size_type limit = 0U ) ;
		///< Trims the lhs of s, taking off up to 'limit' of the 'ws' characters.

	static void trimRight( std::string & s , const std::string & ws , size_type limit = 0U ) ;
		///< Trims the rhs of s, taking off up to 'limit' of the 'ws' characters.

	static void trim( std::string & s , const std::string & ws ) ;
		///< Trims both ends of s, taking off any of the 'ws' characters.

	static std::string trimmed( const std::string & s , const std::string & ws ) ;
		///< Returns a trim()med version of s.

	static bool isNumeric( const std::string & s , bool allow_minus_sign = false ) ;
		///< Returns true if every character is a decimal digit.
		///< Empty strings return true.

	static bool isPrintableAscii( const std::string & s ) ;
		///< Returns true if every character is a 7-bit, non-control
		///< character (ie. 0x20<=c<0x7f). Empty strings return true.

	static bool isUShort( const std::string & s ) ;
		///< Returns true if the string can be converted into
		///< an unsigned short without throwing an exception.

	static bool isUInt( const std::string & s ) ;
		///< Returns true if the string can be converted into
		///< an unsigned integer without throwing an exception.

	static bool isULong( const std::string & s ) ;
		///< Returns true if the string can be converted into
		///< an unsigned long without throwing an exception.

	static std::string fromBool( bool b ) ;
		///< Converts boolean 'b' to a string.

	static std::string fromDouble( double d ) ;
		///< Converts double 'd' to a string.

	static std::string fromInt( int i ) ;
		///< Converts int 'i' to a string.

	static std::string fromLong( long l ) ;
		///< Converts long 'l' to a string.

	static std::string fromShort( short s ) ;
		///< Converts short 's' to a string.

	static std::string fromUInt( unsigned int ui ) ;
		///< Converts unsigned int 'ui' to a string.

	static std::string fromULong( unsigned long ul ) ;
		///< Converts unsigned long 'ul' to a string.

	static std::string fromUShort( unsigned short us ) ;
		///< Converts unsigned short 'us' to a string.

	static bool toBool( const std::string & s ) ;
		///< Converts string 's' to a bool.
		///<
		///< Exception: InvalidFormat

	static double toDouble( const std::string & s ) ;
		///< Converts string 's' to a double.
		///<
		///< Exception: Overflow
		///< Exception: InvalidFormat

	static int toInt( const std::string & s ) ;
		///< Converts string 's' to an int.
		///<
		///< Exception: Overflow
		///< Exception: InvalidFormat

	static long toLong( const std::string & s ) ;
		///< Converts string 's' to a long.
		///<
		///< Exception: Overflow
		///< Exception: InvalidFormat

	static short toShort( const std::string & s ) ;
		///< Converts string 's' to a short.
		///<
		///< Exception: Overflow
		///< Exception: InvalidFormat

	static unsigned int toUInt( const std::string & s , bool limited = false ) ;
		///< Converts string 's' to an unsigned int.
		///<
		///< If 'limited' is true then very large numeric strings
		///< are limited to the maximum value of the numeric type,
		///< without an Overflow exception.
		///<
		///< Exception: Overflow
		///< Exception: InvalidFormat

	static unsigned long toULong( const std::string & s , bool limited = false ) ;
		///< Converts string 's' to an unsigned long.
		///<
		///< If 'limited' is true then very large numeric strings
		///< are limited to the maximum value of the numeric type,
		///< without an Overflow exception.
		///<
		///< Exception: Overflow
		///< Exception: InvalidFormat

	static unsigned short toUShort( const std::string & s , bool limited = false ) ;
		///< Converts string 's' to an unsigned short.
		///<
		///< If 'limited' is true then very large numeric strings
		///< are limited to the maximum value of the numeric type,
		///< without an Overflow exception.
		///<
		///< Exception: Overflow
		///< Exception: InvalidFormat

	static void toUpper( std::string & s ) ;
		///< Replaces all lowercase characters in string 's' by 
		///< uppercase characters.
		
	static void toLower( std::string & s ) ;
		///< Replaces all uppercase characters in string 's' by 
		///< lowercase characters.
		
	static std::string upper( const std::string & s ) ;
		///< Returns a copy of 's' in which all lowercase characters 
		///< have been replaced by uppercase characters.
		
	static std::string lower( const std::string & s ) ;
		///< Returns a copy of 's' in which all uppercase characters 
		///< have been replaced by lowercase characters.

	static std::string toPrintableAscii( char c , char escape = '\\' ) ;
		///< Returns a 7-bit printable representation of the given input character.

	static std::string toPrintableAscii( const std::string & in , char escape = '\\' ) ;
		///< Returns a 7-bit printable representation of the given input string.

	static std::string toPrintableAscii( const std::wstring & in , wchar_t escape = L'\\' ) ;
		///< Returns a 7-bit printable representation of the given wide input string.

	static std::string printable( const std::string & in , char escape = '\\' ) ;
		///< Returns a printable represention of the given input string.
		///< Typically used to prevent escape sequences getting into log files.

	static void escape( std::string & , const std::string & specials , char escape = '\\' ) ;
		///< Prefixes each occurrence of one of the special characters
		///< with the escape character.

	static std::string escaped( const std::string & , const std::string & specials , char escape = '\\' ) ;
		///< Prefixes each occurrence of one of the special characters
		///< with the escape character.

	static std::string readLineFrom( std::istream & stream , const std::string & eol = std::string() ) ;
		///< Reads a line from the stream using the given line terminator.
		///< The line terminator is not part of the returned string.
		///< The terminator defaults to the newline.
		///<
		///< Note that alternatives in the standard library such as 
		///< std::istream::getline() or std::getline(stream,string) 
		///< in the standard "string" header are limited to a single 
		///< character as the terminator.
		///<
		///< The stream's fail bit is set if (1) an empty string was 
		///< returned because the stream was already at eof or (2)
		///< the string overflowed. Therefore, ignoring overflow, if 
		///< the stream ends in an incomplete line that line fragment 
		///< is returned with the stream's eof flag set but the fail bit 
		///< reset and the next attempted read will return an empty string
		///< with the fail bit set. If the stream ends with a complete
		///< line then the last line is returned with eof and fail
		///< bits reset and the next attempted read will return
		///< an empty string with eof and fail bits set. 
		///<
		///< If we don't worry too much about the 'bad' state and note that
		///< the boolean tests on a stream test its 'fail' flag (not eof) 
		///< we can use a read loop like "while(s.good()){read(s);if(s)...}".

	static void readLineFrom( std::istream & stream , const std::string & eol , std::string & result ,
		bool pre_erase_result = true ) ;
			///< An overload which avoids string copying.

	static std::string wrap( std::string text , 
		const std::string & prefix_first_line , const std::string & prefix_subsequent_lines , 
		size_type width = 70U ) ;
			///< Does word-wrapping. The return value is a string with
			///< embedded newlines.

	static void splitIntoTokens( const std::string & in , Strings & out , const std::string & ws ) ;
		///< Splits the string into 'ws'-delimited tokens. The 
		///< behaviour is like strtok() in that adjacent delimiters
		///< count as one and leading and trailing delimiters are ignored.
		///< Ths output array is cleared first.

	static void splitIntoTokens( const std::string & in , StringArray & out , const std::string & ws ) ;
		///< Overload for vector<string>.

	static void splitIntoFields( const std::string & in , Strings & out , 
		const std::string & seperators , char escape = '\0' , 
		bool discard_bogus_escapes = true ) ;
			///< Splits the string into fields. Duplicated, leading
			///< and trailing separator characters are all significant.
			///< Ths output array is cleared first.
			///<
			///< If a non-null escape character is given then any escaped 
			///< separator is not used for splitting. If the 'discard...' 
			///< parameter is true then escapes will never appear in the 
			///< output except for where there were originally double escapes. 
			///< This is the preferred behaviour but it can create problems 
			///< if doing nested splitting -- the escapes are lost by the 
			///< time the sub-strings are split.

	static void splitIntoFields( const std::string & in , StringArray & out ,
		const std::string & seperators , char escape = '\0' , bool discard_bogus_escapes = true ) ;
			///< Overload for vector<string>.

	static std::string join( const Strings & strings , const std::string & sep ) ;
		///< Concatenates a set of strings.

	static std::string join( const StringArray & strings , const std::string & sep ) ;
		///< Concatenates a set of strings.

	static Strings keys( const StringMap & string_map ) ;
		///< Extracts the keys from a map of strings.

	static std::string head( const std::string & in , std::string::size_type pos , 
		const std::string & default_ = std::string() ) ;
			///< Returns the first part of the string up to just before the given position.
			///< The character at pos is not returned. Returns the supplied default 
			///< if pos is npos.

	static std::string tail( const std::string & in , std::string::size_type pos , 
		const std::string & default_ = std::string() ) ;
			///< Returns the last part of the string after the given position.
			///< The character at pos is not returned. Returns the supplied default 
			///< if pos is npos.

	static bool tailMatch( const std::string & in , const std::string & ending ) ;
		///< Returns true if the given string has the given ending.

	static std::string ws() ;
		///< A convenience function returning standard whitespace characters.

private:
	static void readLineFromImp( std::istream & , const std::string & , std::string & ) ;
	Str() ; // not implemented
} ;

#endif
