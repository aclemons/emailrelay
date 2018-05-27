//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include <set>

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
	G_EXCEPTION_CLASS( Overflow , "string conversion error: over/underflow" ) ;
	G_EXCEPTION_CLASS( InvalidFormat, "string conversion error: invalid format" ) ;
	G_EXCEPTION_CLASS( NotEmpty, "internal error: string container not empty" ) ;
	typedef std::string::size_type size_type ;

	struct Limited /// Overload discrimiator for G::Str::toUWhatever()
		{} ;

	static bool replace( std::string & s , const std::string & from , const std::string & to ,
		size_type * pos_p = nullptr ) ;
			///< Replaces 'from' with 'to', starting at offset '*pos_p'.
			///< Returns true if a substitution was made, and adjusts
			///< '*pos_p' by to.length().

	static unsigned int replaceAll( std::string & s , const std::string & from , const std::string & to ) ;
		///< Does a global replace on string 's', replacing all occurances
		///< of sub-string 'from' with 'to'. Returns the number of substitutions
		///< made. Consider using in a while loop if 'from' is more than one
		///< character.

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

	static bool isInt( const std::string & s ) ;
		///< Returns true if the string can be converted into
		///< an integer without throwing an exception.

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

	static unsigned int toUInt( const std::string & s ) ;
		///< Converts string 's' to an unsigned int.
		///<
		///< Exception: Overflow
		///< Exception: InvalidFormat

	static unsigned int toUInt( const std::string & s , Limited ) ;
		///< Converts string 's' to an unsigned int.
		///<
		///< Very large numeric strings are limited to the maximum
		///< value of the numeric type, without an Overflow exception.
		///<
		///< Exception: InvalidFormat

	static unsigned int toUInt( const std::string & s1 , const std::string & s2 ) ;
		///< Overload that converts the first string if it can be converted
		///< without throwing, or otherwise the second string.

	static unsigned long toULong( const std::string & s , Limited ) ;
		///< Converts string 's' to an unsigned long.
		///<
		///< Very large numeric strings are limited to the maximum value
		///< of the numeric type, without an Overflow exception.
		///<
		///< Exception: InvalidFormat

	static unsigned long toULong( const std::string & s ) ;
		///< Converts string 's' to an unsigned long.
		///<
		///< Exception: Overflow
		///< Exception: InvalidFormat

	static unsigned short toUShort( const std::string & s , Limited ) ;
		///< Converts string 's' to an unsigned short.
		///<
		///< Very large numeric strings are limited to the maximum value
		///< of the numeric type, without an Overflow exception.
		///<
		///< Exception: InvalidFormat

	static unsigned short toUShort( const std::string & s ) ;
		///< Converts string 's' to an unsigned short.
		///<
		///< Exception: Overflow
		///< Exception: InvalidFormat

	static void toUpper( std::string & s ) ;
		///< Replaces all Latin-1 lowercase characters in string 's' by
		///< uppercase characters.

	static void toLower( std::string & s ) ;
		///< Replaces all Latin-1 uppercase characters in string 's' by
		///< lowercase characters.

	static std::string upper( const std::string & s ) ;
		///< Returns a copy of 's' in which all Latin-1 lowercase characters
		///< have been replaced by uppercase characters.

	static std::string lower( const std::string & s ) ;
		///< Returns a copy of 's' in which all Latin-1 uppercase characters
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

	static void escape( std::string & s , char c_escape , const std::string & specials_in ,
		const std::string & specials_out ) ;
			///< Prefixes each occurrence of one of the special-in characters with
			///< the escape character and its corresponding special-out character.
			///<
			///< If the specials-in string contains the nul character it must be
			///< at the end, otherwise the two specials strings must be the same
			///< length. The specials-out string cannot contain the nul character.
			///<
			///< The specials-in string should normally include the escape character
			///< itself, otherwise unescaping will not recover the original.

	static void escape( std::string & s , char c_escape , const char * specials_in ,
		const char * specials_out ) ;
			///< Overload for c-style 'special' strings.

	static void escape( std::string & s ) ;
		///< Overload for 'normal' backslash escaping of whitespace.

	static std::string escaped( const std::string & , char c_escape , const std::string & specials_in ,
		const std::string & specials_out ) ;
			///< Returns the escape()d string.

	static std::string escaped( const std::string & , char c_escape , const char * specials_in ,
		const char * specials_out ) ;
			///< Returns the escape()d string.

	static std::string escaped( const std::string & ) ;
		///< Returns the escape()d string.

	static void unescape( std::string & s , char c_escape , const char * specials_in , const char * specials_out ) ;
		///< Unescapes the string by replacing e-e with e, e-special-in with
		///< special-out, and e-other with other.
		///<
		///< If the specials-out string includes the nul character then it must be at
		///< the end, otherwise the two specials strings must be the same length.

	static void unescape( std::string & s ) ;
		///< Overload for "normal" unescaping where the string has backslash escaping
		///< of whitespace.

	static std::string unescaped( const std::string & s ) ;
		///< Returns the unescape()d version of s.

	static std::string meta() ;
		///< Returns a list of shell meta-characters with a tilde as the
		///< first character. Does not contain the nul character. This is
		///< typically used with escape().

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
		///< Boolean tests on a stream are equivalent to using fail(),
		///< and fail() tests for failbit or badbit, so to process
		///< even incomplete lines at the end we can use a read
		///< loop like "while(s.good()){read(s);if(s)...}".

	static void readLineFrom( std::istream & stream , const std::string & eol , std::string & result ,
		bool pre_erase_result = true ) ;
			///< An overload which avoids string copying.

	static std::string wrap( std::string text ,
		const std::string & prefix_first_line , const std::string & prefix_subsequent_lines ,
		size_type width = 70U ) ;
			///< Does word-wrapping. The return value is a string with
			///< embedded newlines.

	static void splitIntoTokens( const std::string & in , StringArray & out , const std::string & ws ) ;
		///< Splits the string into 'ws'-delimited tokens. The behaviour is like
		///< strtok() in that adjacent delimiters count as one and leading and
		///< trailing delimiters are ignored. The output array is cleared first.

	static StringArray splitIntoTokens( const std::string & in , const std::string & ws = Str::ws() ) ;
		///< Overload that returns by value.

	static void splitIntoFields( const std::string & in , StringArray & out ,
		const std::string & separators , char escape = '\0' ,
		bool remove_escapes = true ) ;
			///< Splits the string into fields. Duplicated, leading and trailing
			///< separator characters are all significant. Ths output array is
			///< cleared first.
			///<
			///< If a non-null escape character is given then escaped separators
			///< do not result in a split. If the 'remove-escapes' parameter is
			///< true then all unescaped escapes are removed from the output; this
			///< is generally the preferred behaviour for nested escaping. If the
			///< 'remove-escapes' parameter is false then escapes are not removed;
			///< unescaped escapes are used to prevent splitting but they remain
			///< in the output.

	static StringArray splitIntoFields( const std::string & in , const std::string & ws = Str::ws() ) ;
		///< Overload that returns by value.

	static std::string dequote( const std::string & , char qq = '\"' , char esc = '\\' , const std::string & ws = Str::ws() ) ;
		///< Dequotes a string by removing unescaped quotes and escaping
		///< quoted whitespace, so "qq-aaa-esc-qq-bbb-ws-ccc-qq" becomes
		///< "aaa-qq-bbb-esc-ws-ccc".

	static std::string join( const std::string & sep , const StringArray & strings ) ;
		///< Concatenates an array of strings with separators.

	static std::string join( const std::string & sep , const std::set<std::string> & strings ) ;
		///< Concatenates a set of strings with separators.

	static std::string join( const std::string & sep , const std::string & s1 , const std::string & s2 ,
		const std::string & s3 = std::string() , const std::string & s4 = std::string() , const std::string & s5 = std::string() ,
		const std::string & s6 = std::string() , const std::string & s7 = std::string() , const std::string & s8 = std::string() ,
		const std::string & s9 = std::string() ) ;
			///< Concatenates a small number of strings with separators.
			///< In this overload empty strings are ignored.

	static std::string join( const std::string & sep , const StringMap & ,
		const std::string & eq = std::string(1U,'=') , const std::string & tail = std::string() ) ;
			///< Contatenates entries in a map, where an entry is "<key><eq><value><tail>".

	static std::set<std::string> keySet( const StringMap & string_map ) ;
		///< Extracts the keys from a map of strings.

	static std::string head( const std::string & in , size_type pos ,
		const std::string & default_ = std::string() ) ;
			///< Returns the first part of the string up to just before the given position.
			///< The character at pos is not returned. Returns the supplied default
			///< if pos is npos. Returns the whole string if pos is one-or-more
			///< off the end.

	static std::string head( const std::string & in , const std::string & sep , bool default_empty = true ) ;
		///< Overload taking a separator string, and with the default
		///< as either the input string or the empty string. If the
		///< separator occurs more than once in the input then only the
		///< first occurence is relevant.

	static std::string tail( const std::string & in , size_type pos ,
		const std::string & default_ = std::string() ) ;
			///< Returns the last part of the string after the given position.
			///< The character at pos is not returned. Returns the supplied default
			///< if pos is npos. Returns the empty string if pos is one-or-more
			///< off the end.

	static std::string tail( const std::string & in , const std::string & sep , bool default_empty = true ) ;
		///< Overload taking a separator string, and with the default
		///< as either the input string or the empty string. If the
		///< separator occurs more than once in the input then only the
		///< first occurence is relevant.

	static bool match( const std::string & , const std::string & ) ;
		///< Returns true if the two strings are the same.

	static bool match( const StringArray & , const std::string & ) ;
		///< Returns true if any string in the array matches the given string.

	static bool imatch( const std::string & , const std::string & ) ;
		///< Returns true if the two strings are the same, ignoring Latin-1 case.
		///< The locale is ignored.

	static bool imatch( const StringArray & , const std::string & ) ;
		///< Returns true if any string in the array matches the given string, ignoring
		///< Latin-1 case. The locale is ignored.

	static bool tailMatch( const std::string & in , const std::string & ending ) ;
		///< Returns true if the string has the given ending (or ending is empty).

	static bool tailMatch( const StringArray & in , const std::string & ending ) ;
		///< Returns true if any string in the array has the given ending
		///< (or ending is empty).

	static bool headMatch( const std::string & in , const std::string & head ) ;
		///< Returns true if the string has the given start (or head is empty).

	static bool headMatch( const StringArray & in , const std::string & head ) ;
		///< Returns true if any string in the array has the given start
		///< (or head is empty).

	static std::string headMatchResidue( const StringArray & in , const std::string & head ) ;
		///< Returns the unmatched part of the first string in the array that has
		///< the given start. Returns the empty string if nothing matches or if
		///< the first match is an exact match.

	static std::string ws() ;
		///< Returns a string of standard whitespace characters.

	static std::string positive() ;
		///< Returns a default positive string. See isPositive().

	static std::string negative() ;
		///< Returns a default negative string. See isNegative().

	static bool isPositive( const std::string & ) ;
		///< Returns true if the string has a positive meaning, such as "1", "true", "yes".

	static bool isNegative( const std::string & ) ;
		///< Returns true if the string has a negative meaning, such as "0", "false", "no".

	static size_type ifind( const std::string & s , const std::string & key ,
		size_type pos = 0U ) ;
			///< Returns the position of the key in 's' using a Latin-1 case-insensitive
			///< search. The locale is ignored.

	static std::string unique( std::string s , char c = ' ' , char r = ' ' ) ;
		///< Returns a string with repeated 'c' characters replaced by
		///< one 'r' character. Single 'c' characters are not replaced.

private:
	Str() ; // not implemented
	static void readLineFromImp( std::istream & , const std::string & , std::string & ) ;
	static unsigned short toUShortImp( const std::string & s , bool & overflow , bool & invalid ) ;
	static unsigned long toULongImp( const std::string & s , bool & overflow , bool & invalid ) ;
	static unsigned int toUIntImp( const std::string & s , bool & overflow , bool & invalid ) ;
	static short toShortImp( const std::string & s , bool & overflow , bool & invalid ) ;
	static long toLongImp( const std::string & s , bool & overflow , bool & invalid ) ;
	static int toIntImp( const std::string & s , bool & overflow , bool & invalid ) ;
	static void escapeImp( std::string & , char , const char * , const char * , bool ) ;
	static void joinImp( const std::string & , std::string & , const std::string & ) ;
} ;

#endif
