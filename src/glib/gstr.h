//
// Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// gstr.h
//

#ifndef G_STR_H
#define G_STR_H

#include "gdef.h"
#include "gexception.h"
#include "gstrings.h"
#include <string>
#include <iostream>
#include <list>
#include <vector>

namespace G
{
	class Str ;
} ;

// Class: G::Str
// Description: A static class which provides string helper functions.
//
class G::Str 
{
public:
	G_EXCEPTION( Overflow , "conversion error: over/underflow" ) ;
	G_EXCEPTION( InvalidFormat, "conversion error: invalid format" ) ;

	static bool replace( std::string &s , 
		const std::string &from , const std::string &to , 
		size_t *pos_p = NULL ) ;
			// Replaces 'from' with 'to', starting at offset '*pos_p'. 
			// Returns true if a substitution was made, and adjusts 
			// '*pos_p' by to.length().

	static size_t replaceAll( std::string &s , const std::string &from , 
		const std::string &to ) ;
			// Does a global replace on string 's', replacing all 
			// occurences of sub-string 'from' with 'to'. Returns 
			// the number of substitutions made.

	static void trimLeft( std::string & s , const std::string & ws ) ;
		// Trims the lhs of s, taking off any of the 'ws' characters.

	static void trimRight( std::string & s , const std::string & ws ) ;
		// Trims the rhs of s, taking off any of the 'ws' characters.

	static void trim( std::string & s , const std::string & ws ) ;
		// Trims both ends of s, taking off any of the 'ws' characters.

	static bool isNumeric( const std::string & s , bool allow_minus_sign = false ) ;
		// Returns true if every character is a decimal digit.
		// Empty strings return true.

	static bool isPrintableAscii( const std::string & s ) ;
		// Returns true if every character is a 7-bit, non-control
		// character (ie. 0x20<=c<0x7f). Empty strings return true.

	static bool isUShort( const std::string & s ) ;
		// Returns true if the string can be converted into
		// an unsigned short without throwing an exception.

	static bool isUInt( const std::string & s ) ;
		// Returns true if the string can be converted into
		// an unsigned integer without throwing an exception.

	static bool isULong( const std::string & s ) ;
		// Returns true if the string can be converted into
		// an unsigned long without throwing an exception.

	static unsigned int toUInt( const std::string & s , bool limited = false ) ;
		// Converts string 's' to an unsigned int.
		//
		// If 'limited' is true then very large numeric strings
		// are limited to the maximum value of the numeric type,
		// without an Overflow exception.
		//
		// Exception: Overflow
		// Exception: InvalidFormat

	static unsigned long toULong( const std::string &s , bool limited = false ) ;
		// Converts string 's' to an unsigned long.
		//
		// If 'limited' is true then very large numeric strings
		// are limited to the maximum value of the numeric type,
		// without an Overflow exception.
		//
		// Exception: Overflow
		// Exception: InvalidFormat

	static unsigned short toUShort( const std::string &s , bool limited = false ) ;
		// Converts string 's' to an unsigned short.
		//
		// If 'limited' is true then very large numeric strings
		// are limited to the maximum value of the numeric type,
		// without an Overflow exception.
		//
		// Exception: Overflow
		// Exception: InvalidFormat

	static std::string fromInt( int i ) ;
		// Converts int 'i' to a string.

	static std::string fromUInt( unsigned int ) ;
		// Converts from unsigned int to a decimal string.

	static std::string fromULong( unsigned long ul ) ;
		// Converts unsigned long to a decimal string.

	static void toUpper( std::string &s ) ;
		// Replaces all lowercase characters in string 's' by 
		// uppercase characters.
		
	static void toLower( std::string &s ) ;
		// Replaces all uppercase characters in string 's' by 
		// lowercase characters.
		
	static std::string upper( const std::string &s ) ;
		// Returns a copy of 's' in which all lowercase characters 
		// have been replaced by uppercase characters.
		
	static std::string lower( const std::string &s ) ;
		// Returns a copy of 's' in which all uppercase characters 
		// have been replaced by lowercase characters.

	static std::string toPrintableAscii( char c , char escape = '\\' ) ;
		// Returns a printable, 7-bit-ascii string representing the given 
		// character. Typical return values include "\\", "\n", 
		// "\t", "\007", etc.

	static std::string toPrintableAscii( const std::string & in , char escape = '\\' ) ;
		// Returns a printable, 7-bit-ascii string representing the given input.

	static std::string readLineFrom( std::istream &stream , char ignore = '\015' ) ;
		// Reads a string from the given stream using newline as the
		// terminator. The terminator is read from the stream
		// but not put into the returned string. 
		//
		// May return a partial line at the end of the stream.
		//
		// Any 'ignore' characters (control-m by default) read
		// from the stream are discarded. An 'ignore' 
		// character of NUL ('\0') disables this feature.

	static std::string readLineFrom( std::istream &stream , const std::string & eol ) ;
		// An overload which uses 'eol' as the terminator, and
		// without the 'ignore' feature.

	static void readLineFrom( std::istream & stream , const std::string & eol , std::string & result ) ;
		// An overload which avoids string copying.

	static std::string wrap( std::string text , 
		const std::string & prefix_first_line , const std::string & prefix_subsequent_lines , 
		size_t width = 70U ) ;
			// Does word-wrapping. The return value is a string with
			// embedded newlines.

	static void splitIntoTokens( const std::string & in , Strings &out , 
		const std::string & ws ) ;
			// Splits the string into 'ws'-delimited tokens. The 
			// behaviour is like ::strtok() in that adjacent delimiters
			// count as one and leading and trailing delimiters are ignored.
			// Ths output array is cleared first.

	static void splitIntoTokens( const std::string & in , StringArray & out ,
		const std::string & ws ) ;
			// Overload for vector<string>.

	static void splitIntoFields( const std::string & in , Strings &out , 
		const std::string & seperators , char escape = '\0' , 
		bool discard_bogus_escapes = true ) ;
			// Splits the string into fields. Duplicated, leading
			// and trailing separator characters are all significant.
			// Ths output array is cleared first.
			//
			// If a non-null escape character is given then any escaped 
			// separator is not used for splitting. If the 'discard...' 
			// parameter is true then escapes will never appear in the 
			// output, except for where there were originally double escapes. 
			// This is the preferred behaviour but it can create problems 
			// if doing nested splitting -- the escapes are lost by the 
			// time the sub-strings are split.

	static void splitIntoFields( const std::string & in , StringArray & out ,
		const std::string & seperators , char escape = '\0' , 
		bool discard_bogus_escapes = true ) ;
			// Overload for vector<string>.

	static std::string join( const Strings & strings , const std::string & sep ) ;
		// Concatenates a set of strings.

	static std::string join( const StringArray & strings , const std::string & sep ) ;
		// Concatenates a set of strings.

	static Strings keys( const StringMap & string_map ) ;
		// Extracts the keys from a map of strings.

private:
	static void listPushBack( void * , const std::string & ) ;
	static void arrayPushBack( void * , const std::string & ) ;
	static void splitIntoFields( const std::string & , void * ,
		void (*fn)(void*,const std::string&) ,
		const std::string & , char , bool ) ;
	static void splitIntoTokens( const std::string & , void * ,
		void (*fn)(void*,const std::string&) , const std::string & ) ;
	Str() ; // not implemented
} ;

#endif

