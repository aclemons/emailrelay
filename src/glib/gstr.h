//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gstringarray.h"
#include "gstringmap.h"
#include "gstringview.h"
#include <string>
#include <sstream>
#include <iostream>
#include <list>
#include <vector>
#include <set>
#include <limits>
#include <type_traits>

namespace G
{
	class Str ;
}

//| \class G::Str
/// A static class which provides string helper functions.
///
class G::Str
{
public:
	G_EXCEPTION_CLASS( Overflow , tx("string conversion error: over/underflow") ) ;
	G_EXCEPTION_CLASS( InvalidFormat, tx("string conversion error: invalid format") ) ;
	G_EXCEPTION_CLASS( NotEmpty, tx("internal error: string container not empty") ) ;
	G_EXCEPTION( InvalidEol , tx("invalid end-of-line specifier") ) ;

	struct Limited /// Overload discrimiator for G::Str::toUWhatever() requesting a range-limited result.
		{} ;

	struct Hex /// Overload discrimiator for G::Str::toUWhatever() indicating hexadecimal strings.
		{} ;

	static bool replace( std::string & s , const std::string & from , const std::string & to ,
		std::size_t * pos_p = nullptr ) ;
			///< Replaces 'from' with 'to', starting at offset '*pos_p'.
			///< Returns true if a substitution was made, and adjusts
			///< '*pos_p' by to.length().

	static bool replace( std::string & s , const char * from , const char * to ,
		std::size_t * pos_p = nullptr ) ;
			///< A c-string overload. Replaces 'from' with 'to', starting at
			///< offset '*pos_p'. Returns true if a substitution was made,
			///< and adjusts '*pos_p' by to.length().

	static bool replace( std::string & s , string_view from , string_view to ,
		std::size_t * pos_p = nullptr ) ;
			///< A string_view overload. Replaces 'from' with 'to', starting at
			///< offset '*pos_p'. Returns true if a substitution was made,
			///< and adjusts '*pos_p' by to.length().

	static void replace( std::string & s , char from , char to ) ;
		///< Replaces all 'from' characters with 'to'.

	static void replace( StringArray & , char from , char to ) ;
		///< Replaces 'from' characters with 'to' in all the strings in the array.

	static unsigned int replaceAll( std::string & s , const std::string & from , const std::string & to ) ;
		///< Does a global replace on string 's', replacing all occurrences
		///< of sub-string 'from' with 'to'. Returns the number of substitutions
		///< made. Consider using in a while loop if 'from' is more than one
		///< character.

	static unsigned int replaceAll( std::string & s , const char * from , const char * to ) ;
		///< A c-string overload.

	static unsigned int replaceAll( std::string & s , string_view from , string_view to ) ;
		///< A string_view overload.

	static std::string replaced( const std::string & s , char from , char to ) ;
		///< Returns the string 's' with all occurrences of 'from' replaced by 'to'.

	static void removeAll( std::string & , char ) ;
		///< Removes all occurrences of the character from the string. See also only().

	static std::string removedAll( const std::string & , char ) ;
		///< Removes all occurrences of the character from the string and returns
		///< the result. See also only().

	static std::string & trimLeft( std::string & s , string_view ws , std::size_t limit = 0U ) ;
		///< Trims the lhs of s, taking off up to 'limit' of the 'ws' characters.
		///< Returns s.

	static std::string & trimRight( std::string & s , string_view ws , std::size_t limit = 0U ) ;
		///< Trims the rhs of s, taking off up to 'limit' of the 'ws' characters.
		///< Returns s.

	static std::string & trim( std::string & s , string_view ws ) ;
		///< Trims both ends of s, taking off any of the 'ws' characters.
		///< Returns s.

	static std::string trimmed( const std::string & s , string_view ws ) ;
		///< Returns a trim()med version of s.

	static std::string trimmed( std::string && s , string_view ws ) ;
		///< Returns a trim()med version of s.

	static bool isNumeric( const std::string & s , bool allow_minus_sign = false ) ;
		///< Returns true if every character is a decimal digit.
		///< Empty strings return true.

	static bool isHex( const std::string & s ) ;
		///< Returns true if every character is a hexadecimal digit.
		///< Empty strings return true.

	static bool isPrintableAscii( const std::string & s ) ;
		///< Returns true if every character is a 7-bit, non-control
		///< character (ie. 0x20<=c<0x7f). Empty strings return true.

	static bool isSimple( const std::string & s ) ;
		///< Returns true if every character is alphanumeric or
		///< "-" or "_". Empty strings return true.

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

	static float toFloat( const std::string & s ) ;
		///< Converts string 's' to a float.
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

	static int toInt( const std::string & s1 , const std::string & s2 ) ;
		///< Overload that converts the first string if it can be converted
		///< without throwing, or otherwise the second string.

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

	static unsigned long toULong( const std::string & s , Hex ) ;
		///< An overload for hexadecimal strings. To avoid exceptions
		///< use isHex() and check the string length.

	static unsigned long toULong( const std::string & s , Hex , Limited ) ;
		///< An overload for hexadecimal strings where overflow
		///< results in the return of the maximum value. To avoid
		///< exceptions use isHex().

	template <typename T> static T toUnsigned( const char * p , const char * end ,
		bool & overflow , bool & invalid ) noexcept ;
			///< Low-level conversion from an unsigned decimal string to a number.
			///< All characters in the range are used; any character
			///< not 0..9 yields an 'invalid' result.

	template <typename T> static T toUnsigned( const char * &p , const char * end ,
		bool & overflow ) noexcept ;
			///< Low-level conversion from an unsigned decimal string to a number.
			///< Consumes characters until the first invalid character.

	static unsigned long toULong( const std::string & s ) ;
		///< Converts string 's' to an unsigned long.
		///<
		///< Exception: Overflow
		///< Exception: InvalidFormat

	static unsigned long toULong( const std::string & s1 , const std::string & s2 ) ;
		///< Overload that converts the first string if it can be converted
		///< without throwing, or otherwise the second string.

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
		///< Replaces all Latin-1 lower-case characters in string 's' by
		///< upper-case characters.

	static void toLower( std::string & s ) ;
		///< Replaces all Latin-1 upper-case characters in string 's' by
		///< lower-case characters.

	static std::string upper( const std::string & s ) ;
		///< Returns a copy of 's' in which all Latin-1 lower-case characters
		///< have been replaced by upper-case characters.

	static std::string lower( const std::string & s ) ;
		///< Returns a copy of 's' in which all Latin-1 upper-case characters
		///< have been replaced by lower-case characters.

	static std::string toPrintableAscii( const std::string & in , char escape = '\\' ) ;
		///< Returns a 7-bit printable representation of the given input string.

	static std::string toPrintableAscii( const std::wstring & in , wchar_t escape = L'\\' ) ;
		///< Returns a 7-bit printable representation of the given wide input string.

	static std::string printable( const std::string & in , char escape = '\\' ) ;
		///< Returns a printable representation of the given input string, using
		///< chacter code ranges 0x20 to 0x7e and 0xa0 to 0xfe inclusive.
		///< Typically used to prevent escape sequences getting into log files.

	static std::string printable( std::string && in , char escape = '\\' ) ;
		///< Returns a printable representation of the given input string, using
		///< chacter code ranges 0x20 to 0x7e and 0xa0 to 0xfe inclusive.
		///< Typically used to prevent escape sequences getting into log files.

	static std::string printable( G::string_view in , char escape = '\\' ) ;
		///< Returns a printable representation of the given input string, using
		///< chacter code ranges 0x20 to 0x7e and 0xa0 to 0xfe inclusive.
		///< Typically used to prevent escape sequences getting into log files.

	static std::string only( string_view allow_chars , const std::string & s ) ;
		///< Returns the 's' with all occurrences of the characters not appearing in
		///< the first string deleted.

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
		///< Overload for 'normal' unescaping where the string has backslash escaping
		///< of whitespace.

	static std::string unescaped( const std::string & s ) ;
		///< Returns the unescape()d version of s.

	static string_view meta() ;
		///< Returns a list of shell meta-characters with a tilde as the
		///< first character. Does not contain the nul character. This is
		///< typically used with escape().

	static std::string readLineFrom( std::istream & stream , const std::string & eol = {} ) ;
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

	static void readLineFrom( std::istream & stream , string_view eol , std::string & result ,
		bool pre_erase_result = true ) ;
			///< An overload using string_view for eol.

	static void readLineFrom( std::istream & stream , const char * eol , std::string & result ,
		bool pre_erase_result = true ) ;
			///< An overload using c-string for eol.

	static void readLineFrom( std::istream & stream , string_view eol , std::vector<char> & result ,
		bool pre_erase_result = true ) ;
			///< An overload for a vector buffer.

	static void splitIntoTokens( const std::string & in , StringArray & out , string_view ws , char esc = '\0' ) ;
		///< Splits the string into 'ws'-delimited tokens. The behaviour is like
		///< strtok() in that adjacent delimiters count as one and leading and
		///< trailing delimiters are ignored. The output array is _not_ cleared
		///< first; new tokens are appended to the output list. If the escape
		///< character is supplied then it can be used to escape whitespace
		///< characters, preventing a split, with those escape characters being
		///< consumed in the process. For shell-like tokenising use dequote()
		///< before splitIntoTokens(), and revert the non-breaking spaces
		///< afterwards.

	static StringArray splitIntoTokens( const std::string & in , string_view ws = Str::ws() , char esc = '\0' ) ;
		///< Overload that returns by value.

	static void splitIntoFields( const std::string & in , StringArray & out ,
		char sep , char escape = '\0' ,
		bool remove_escapes = true ) ;
			///< Splits the string into fields. Duplicated, leading and trailing
			///< separator characters are all significant. The output array is
			///< cleared first.
			///<
			///< If a non-null escape character is given then escaped separators
			///< do not result in a split. If the 'remove-escapes' parameter is
			///< true then all unescaped escapes are removed from the output; this
			///< is generally the preferred behaviour for nested escaping. If the
			///< 'remove-escapes' parameter is false then escapes are not removed;
			///< unescaped escapes are used to prevent splitting but they remain
			///< in the output.

	static StringArray splitIntoFields( const std::string & in , char sep ) ;
		///< Overload that returns by value.

	static std::string dequote( const std::string & , char qq = '\"' , char esc = '\\' ,
		string_view ws = Str::ws() , string_view nbws = Str::ws() ) ;
			///< Dequotes a string by removing unescaped quotes and escaping
			///< quoted whitespace, so "qq-aaa-esc-qq-bbb-ws-ccc-qq" becomes
			///< "aaa-qq-bbb-esc-ws-ccc". Escaped whitespace characters within
			///< quotes can optionally be converted to non-breaking equivalents.

	static std::string join( const std::string & sep , const StringArray & strings ) ;
		///< Concatenates an array of strings with separators.

	static std::string join( const std::string & sep , const std::set<std::string> & strings ) ;
		///< Concatenates a set of strings with separators.

	static std::string join( const std::string & sep , const std::string & s1 , const std::string & s2 ,
		const std::string & s3 = {} , const std::string & s4 = {} , const std::string & s5 = {} ,
		const std::string & s6 = {} , const std::string & s7 = {} , const std::string & s8 = {} ,
		const std::string & s9 = {} ) ;
			///< Concatenates a small number of strings with separators.
			///< In this overload empty strings are ignored.

	static std::string join( const std::string & sep , const StringMap & ,
		const std::string & eq = std::string(1U,'=') , const std::string & tail = {} ) ;
			///< Concatenates entries in a map, where an entry is "<key><eq><value><tail>".

	static std::set<std::string> keySet( const StringMap & string_map ) ;
		///< Extracts the keys from a map of strings.

	static StringArray keys( const StringMap & string_map ) ;
		///< Extracts the keys from a map of strings.

	static std::string head( const std::string & in , std::size_t pos ,
		const std::string & default_ = {} ) ;
			///< Returns the first part of the string up to just before the given position.
			///< The character at pos is not returned. Returns the supplied default
			///< if pos is npos. Returns the whole string if pos is one-or-more
			///< off the end.

	static std::string head( const std::string & in , const std::string & sep , bool default_empty = true ) ;
		///< Overload taking a separator string, and with the default
		///< as either the input string or the empty string. If the
		///< separator occurs more than once in the input then only the
		///< first occurrence is relevant.

	static string_view head( string_view in , std::size_t pos , string_view default_ ) ;
		///< Overload with string-views.

	static string_view head( string_view in , string_view sep , bool default_empty = true ) ;
		///< Overload with string-views.

	static std::string tail( const std::string & in , std::size_t pos ,
		const std::string & default_ = {} ) ;
			///< Returns the last part of the string after the given position.
			///< The character at pos is not returned. Returns the supplied default
			///< if pos is npos. Returns the empty string if pos is one-or-more
			///< off the end.

	static std::string tail( const std::string & in , const std::string & sep , bool default_empty = true ) ;
		///< Overload taking a separator string, and with the default
		///< as either the input string or the empty string. If the
		///< separator occurs more than once in the input then only the
		///< first occurrence is relevant.

	static bool match( const std::string & , const std::string & ) ;
		///< Returns true if the two strings are the same.

	static bool match( const StringArray & , const std::string & ) ;
		///< Returns true if any string in the array matches the given string.

	static bool iless( const std::string & , const std::string & ) ;
		///< Returns true if the first string is lexicographically less
		///< than the first, after Latin-1 lower-case letters have been
		///< folded to upper-case.

	static bool imatch( char , char ) ;
		///< Returns true if the two characters are the same, ignoring Latin-1 case.

	static bool imatch( const std::string & , const std::string & ) ;
		///< Returns true if the two strings are the same, ignoring Latin-1 case.
		///< The locale is ignored.

	static bool imatch( const std::string & , const char * , std::size_t ) ;
		///< Returns true if the two strings are the same, ignoring Latin-1 case.
		///< The locale is ignored.

	static bool imatch( const char * , std::size_t , string_view ) ;
		///< Returns true if the two strings are the same, ignoring Latin-1 case.
		///< The locale is ignored.

	static bool imatch( const StringArray & , const std::string & ) ;
		///< Returns true if any string in the array matches the given string, ignoring
		///< Latin-1 case. The locale is ignored.

	static std::size_t ifind( const std::string & s , const std::string & key ) ;
			///< Returns the position of the key in 's' using a Latin-1 case-insensitive
			///< search. Returns std::string::npos if not found. The locale is ignored.

	static std::size_t ifind( G::string_view s , G::string_view key ) ;
			///< Returns the position of the key in 's' using a Latin-1 case-insensitive
			///< search. Returns std::string::npos if not found. The locale is ignored.

	static std::size_t ifindat( const std::string & s , const std::string & key , std::size_t pos ) ;
			///< Returns the position of the key in 's' at or after position 'pos'
			///< using a Latin-1 case-insensitive search. Returns std::string::npos
			///< if not found. The locale is ignored.

	static std::size_t ifindat( G::string_view s , G::string_view key , std::size_t pos ) ;
			///< Returns the position of the key in 's' at of after position 'pos'
			///< using a Latin-1 case-insensitive search. Returns std::string::npos
			///< if not found. The locale is ignored.

	static bool tailMatch( const std::string & in , const std::string & ending ) ;
		///< Returns true if the string has the given ending (or the given ending is empty).

	static bool tailMatch( const StringArray & in , const std::string & ending ) ;
		///< Returns true if any string in the array has the given ending
		///< (or the given ending is empty).

	static bool headMatch( const std::string & in , const std::string & head ) ;
		///< Returns true if the string has the given start (or head is empty).

	static bool headMatch( const std::string & in , const char * head ) ;
		///< A c-string overload.

	static bool headMatch( const StringArray & in , const std::string & head ) ;
		///< Returns true if any string in the array has the given start
		///< (or head is empty).

	static std::string headMatchResidue( const StringArray & in , const std::string & head ) ;
		///< Returns the unmatched part of the first string in the array that has
		///< the given start. Returns the empty string if nothing matches or if
		///< the first match is an exact match.

	static string_view ws() ;
		///< Returns a string of standard whitespace characters.

	static string_view alnum() ;
		///< Returns a string of seven-bit alphanumeric characters, ie A-Z, a-z and 0-9.

	static string_view alnum_() ;
		///< Returns alnum() with an additional trailing underscore character.

	static std::string positive() ;
		///< Returns a default positive string. See isPositive().

	static std::string negative() ;
		///< Returns a default negative string. See isNegative().

	static bool isPositive( const std::string & ) ;
		///< Returns true if the string has a positive meaning, such as "1", "true", "yes".

	static bool isNegative( const std::string & ) ;
		///< Returns true if the string has a negative meaning, such as "0", "false", "no".

	static std::string unique( const std::string & s , char c , char r ) ;
		///< Returns a string with repeated 'c' characters replaced by
		///< one 'r' character. Single 'c' characters are not replaced.

	static std::string unique( const std::string & s , char c ) ;
		///< An overload that replaces repeated 'c' characters by
		///< one 'c' character.

	static StringArray::iterator keepMatch( StringArray::iterator begin , StringArray::iterator end ,
		const StringArray & match_list , bool ignore_case = false ) ;
			///< Removes items in the begin/end list that do not match any of the
			///< elements in the match-list (whitelist), but keeps everything (by
			///< returning 'end') if the match-list is empty. Returns an iterator
			///< for erase().

	static StringArray::iterator removeMatch( StringArray::iterator begin , StringArray::iterator end ,
		const StringArray & match_list , bool ignore_case = false ) ;
			///< Removes items in the begin/end list that match one of the elements
			///< in the match-list (blocklist). (Removes nothing if the match-list is
			///< empty.) Returns an iterator for erase().

	static constexpr std::size_t truncate = (~(static_cast<std::size_t>(0U))) ;
		///< A special value for the G::Str::strncpy_s() 'count' parameter.

	static errno_t strncpy_s( char * dst , std::size_t n_dst , const char * src , std::size_t count ) noexcept ;
		///< Does the same as windows strncpy_s(). Copies count characters
		///< from src to dst and adds a terminator character, but fails
		///< if dst is too small. If the count is 'truncate' then as
		///< much of src is copied as will fit in dst, allowing for the
		///< terminator character. Returns zero on success or on
		///< truncation (unlike windows strncpy_s()).

public:
	Str() = delete ;
} ;

inline
std::string G::Str::fromInt( int i )
{
	return std::to_string( i ) ;
}

inline
std::string G::Str::fromLong( long l )
{
	return std::to_string( l ) ;
}

inline
std::string G::Str::fromShort( short s )
{
	return std::to_string( s ) ;
}

inline
std::string G::Str::fromUInt( unsigned int ui )
{
	return std::to_string( ui ) ;
}

inline
std::string G::Str::fromULong( unsigned long ul )
{
	return std::to_string( ul ) ;
}

inline
std::string G::Str::fromUShort( unsigned short us )
{
	return std::to_string( us ) ;
}

template <typename T>
T G::Str::toUnsigned( const char * p , const char * end , bool & overflow , bool & invalid ) noexcept
{
	if( p == nullptr || end == nullptr || p == end )
	{
		invalid = true ;
		overflow = false ;
		return 0UL ;
	}
	T result = toUnsigned<T>( p , end , overflow ) ;
	if( p != end )
		invalid = true ;
	return result ;
}

template <typename T>
T G::Str::toUnsigned( const char * &p , const char * end , bool & overflow ) noexcept
{
	static_assert( std::is_integral<T>::value , "" ) ;
	T result = 0 ;
	for( ; p != end ; p++ )
	{
		T n = 0 ;
		if( *p == '0' ) n = 0 ;
		else if( *p == '1' ) n = 1 ;
		else if( *p == '2' ) n = 2 ;
		else if( *p == '3' ) n = 3 ;
		else if( *p == '4' ) n = 4 ;
		else if( *p == '5' ) n = 5 ;
		else if( *p == '6' ) n = 6 ;
		else if( *p == '7' ) n = 7 ;
		else if( *p == '8' ) n = 8 ;
		else if( *p == '9' ) n = 9 ;
		else break ;

		auto old = result ;
		result = result * 10 ;
		result += n ;
		if( result < old )
			overflow = true ;
	}
	return result ;
}

#endif
