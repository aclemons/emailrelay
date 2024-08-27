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
#include <limits>
#include <array>
#include <algorithm>
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
	G_EXCEPTION_CLASS( Overflow , tx("string conversion error: over/underflow") )
	G_EXCEPTION_CLASS( InvalidFormat, tx("string conversion error: invalid format") )
	G_EXCEPTION_CLASS( NotEmpty, tx("internal error: string container not empty") )
	G_EXCEPTION( InvalidEol , tx("invalid end-of-line specifier") )

	struct Limited /// Overload discrimiator for G::Str::toUWhatever() requesting a range-limited result.
		{} ;

	struct Hex /// Overload discrimiator for G::Str::toUWhatever() indicating hexadecimal strings.
		{} ;

	enum class Eol // See G::Str::readLine().
	{
		CrLf ,
		Cr_Lf_CrLf
	} ;

	static bool replace( std::string & s , std::string_view from , std::string_view to ,
		std::size_t * pos_p = nullptr ) ;
			///< A std::string_view overload. Replaces 'from' with 'to', starting at
			///< offset '*pos_p'. Returns true if a substitution was made,
			///< and adjusts '*pos_p' by to.length().

	static void replace( std::string & s , char from , char to ) ;
		///< Replaces all 'from' characters with 'to'.

	static void replace( StringArray & , char from , char to ) ;
		///< Replaces 'from' characters with 'to' in all the strings in the array.

	static unsigned int replaceAll( std::string & s , std::string_view from , std::string_view to ) ;
		///< Does a global replace on string 's', replacing all occurrences
		///< of sub-string 'from' with 'to'. Returns the number of substitutions
		///< made. Consider using in a while loop if 'from' is more than one
		///< character.

	static std::string replaced( const std::string & s , char from , char to ) ;
		///< Returns the string 's' with all occurrences of 'from' replaced by 'to'.

	static void removeAll( std::string & , char ) ;
		///< Removes all occurrences of the character from the string. See also only().

	static std::string removedAll( const std::string & , char ) ;
		///< Removes all occurrences of the character from the string and returns
		///< the result. See also only().

	static std::string & trimLeft( std::string & s , std::string_view ws , std::size_t limit = 0U ) ;
		///< Trims the lhs of s, taking off up to 'limit' of the 'ws' characters.
		///< Returns s.

	static std::string_view trimLeftView( std::string_view , std::string_view ws , std::size_t limit = 0U ) noexcept ;
		///< Trims the lhs of s, taking off up to 'limit' of the 'ws' characters.
		///< Returns a view into the input string.

	static std::string & trimRight( std::string & s , std::string_view ws , std::size_t limit = 0U ) ;
		///< Trims the rhs of s, taking off up to 'limit' of the 'ws' characters.
		///< Returns s.

	static std::string_view trimRightView( std::string_view sv , std::string_view ws , std::size_t limit = 0U ) noexcept ;
		///< Trims the rhs of s, taking off up to 'limit' of the 'ws' characters.
		///< Returns a view into the input string.

	static std::string & trim( std::string & s , std::string_view ws ) ;
		///< Trims both ends of s, taking off any of the 'ws' characters.
		///< Returns s.

	static std::string trimmed( const std::string & s , std::string_view ws ) ;
		///< Returns a trim()med version of s.

	static std::string_view trimmedView( std::string_view s , std::string_view ws ) noexcept ;
		///< Returns a trim()med view of the input view.

	static bool isNumeric( std::string_view s , bool allow_minus_sign = false ) noexcept ;
		///< Returns true if every character is a decimal digit.
		///< Empty strings return true.

	static bool isHex( std::string_view s ) noexcept ;
		///< Returns true if every character is a hexadecimal digit.
		///< Empty strings return true.

	static bool isPrintableAscii( std::string_view s ) noexcept ;
		///< Returns true if every character is between 0x20 and 0x7e
		///< inclusive. Empty strings return true.

	static bool isPrintable( std::string_view s ) noexcept ;
		///< Returns true if every character is 0x20 or above but
		///< not 0x7f. Empty strings return true.

	static bool isSimple( std::string_view s ) noexcept ;
		///< Returns true if every character is alphanumeric or
		///< "-" or "_". Empty strings return true.

	static bool isUShort( std::string_view s ) noexcept ;
		///< Returns true if the string can be converted into
		///< an unsigned short without throwing an exception.

	static bool isUInt( std::string_view s ) noexcept ;
		///< Returns true if the string can be converted into
		///< an unsigned integer without throwing an exception.

	static bool isULong( std::string_view s ) noexcept ;
		///< Returns true if the string can be converted into
		///< an unsigned long without throwing an exception.

	static bool isInt( std::string_view s ) noexcept ;
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

	static std::string fromULong( unsigned long , const Hex & ) ;
		///< Converts an unsigned value to a lower-case
		///< hex string with no leading zeros.

	static std::string fromULongLong( unsigned long long , const Hex & ) ;
		///< Converts an unsigned value to a lower-case
		///< hex string with no leading zeros.

	static std::string_view fromULongToHex( unsigned long , char * out ) noexcept ;
		///< Low-level conversion from an unsigned value
		///< to a lower-case hex string with no leading zeros.
		///< The output buffer must be sizeof(long)*2.

	static std::string_view fromULongLongToHex( unsigned long long , char * out ) noexcept ;
		///< Low-level conversion from an unsigned value
		///< to a lower-case hex string with no leading zeros.
		///< The output buffer must be sizeof(long long)*2.

	static bool toBool( std::string_view s ) ;
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

	static int toInt( std::string_view s ) ;
		///< Converts string 's' to an int.
		///<
		///< Exception: Overflow
		///< Exception: InvalidFormat

	static long toLong( std::string_view s ) ;
		///< Converts string 's' to a long.
		///<
		///< Exception: Overflow
		///< Exception: InvalidFormat

	static short toShort( std::string_view s ) ;
		///< Converts string 's' to a short.
		///<
		///< Exception: Overflow
		///< Exception: InvalidFormat

	static unsigned int toUInt( std::string_view s ) ;
		///< Converts string 's' to an unsigned int.
		///<
		///< Exception: Overflow
		///< Exception: InvalidFormat

	static int toInt( std::string_view s1 , std::string_view s2 ) ;
		///< Overload that converts the first string if it can be converted
		///< without throwing, or otherwise the second string.

	static unsigned int toUInt( std::string_view s , Limited ) ;
		///< Converts string 's' to an unsigned int.
		///<
		///< Very large numeric strings are limited to the maximum
		///< value of the numeric type, without an Overflow exception.
		///<
		///< Exception: InvalidFormat

	static unsigned int toUInt( std::string_view s1 , std::string_view s2 ) ;
		///< Overload that converts the first string if it can be converted
		///< without throwing, or otherwise the second string.

	static unsigned int toUInt( std::string_view s1 , unsigned int default_ ) noexcept ;
		///< Overload that converts the string if it can be converted
		///< without throwing, or otherwise returns the default value.

	static unsigned long toULong( std::string_view s , Limited ) ;
		///< Converts string 's' to an unsigned long.
		///<
		///< Very large numeric strings are limited to the maximum value
		///< of the numeric type, without an Overflow exception.
		///<
		///< Exception: InvalidFormat

	static unsigned long toULong( std::string_view s , Hex ) ;
		///< An overload for hexadecimal strings. To avoid exceptions
		///< use isHex() and check the string length.

	static unsigned long toULong( std::string_view s , Hex , Limited ) ;
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

	static unsigned long toULong( std::string_view s ) ;
		///< Converts string 's' to an unsigned long.
		///<
		///< Exception: Overflow
		///< Exception: InvalidFormat

	static unsigned long toULong( std::string_view s1 , std::string_view s2 ) ;
		///< Overload that converts the first string if it can be converted
		///< without throwing, or otherwise the second string.

	static unsigned short toUShort( std::string_view s , Limited ) ;
		///< Converts string 's' to an unsigned short.
		///<
		///< Very large numeric strings are limited to the maximum value
		///< of the numeric type, without an Overflow exception.
		///<
		///< Exception: InvalidFormat

	static unsigned short toUShort( std::string_view s ) ;
		///< Converts string 's' to an unsigned short.
		///<
		///< Exception: Overflow
		///< Exception: InvalidFormat

	static void toUpper( std::string & s ) ;
		///< Replaces all seven-bit lower-case characters in string 's' by
		///< upper-case characters.

	static void toLower( std::string & s ) ;
		///< Replaces all seven-bit upper-case characters in string 's' by
		///< lower-case characters.

	static std::string upper( std::string_view ) ;
		///< Returns a copy of 's' in which all seven-bit lower-case characters
		///< have been replaced by upper-case characters.

	static std::string lower( std::string_view ) ;
		///< Returns a copy of 's' in which all seven-bit upper-case characters
		///< have been replaced by lower-case characters.

	static std::string toPrintableAscii( const std::string & in , char escape = '\\' ) ;
		///< Returns a 7-bit printable representation of the given input string.

	static std::string toPrintableAscii( const std::wstring & in , wchar_t escape = L'\\' ) ;
		///< Returns a 7-bit printable representation of the given input string.

	static std::string printable( const std::string & in , char escape = '\\' ) ;
		///< Returns a printable representation of the given input string, using
		///< chacter code ranges 0x20 to 0x7e and 0xa0 to 0xfe inclusive.
		///< Typically used to prevent escape sequences getting into log files.

	static std::string printable( std::string_view in , char escape = '\\' ) ;
		///< Returns a printable representation of the given input string, using
		///< chacter code ranges 0x20 to 0x7e and 0xa0 to 0xfe inclusive.
		///< Typically used to prevent escape sequences getting into log files.

	static std::string only( std::string_view allow_chars , std::string_view s ) ;
		///< Returns the 's' with all occurrences of the characters not appearing in
		///< the first string deleted.

	static void escape( std::string & s , char c_escape , std::string_view specials_in ,
		std::string_view specials_out ) ;
			///< Prefixes each occurrence of one of the special-in characters with
			///< the escape character and its corresponding special-out character.
			///<
			///< The specials-in and specials-out strings must be the same size.
			///<
			///< The specials-in string should normally include the escape character
			///< itself, otherwise unescaping will not recover the original.

	static void escape( std::string & s ) ;
		///< Overload for 'normal' backslash escaping of whitespace.

	static std::string escaped( std::string_view , char c_escape , std::string_view specials_in ,
		std::string_view specials_out ) ;
			///< Returns the escape()d string.

	static std::string escaped( std::string_view ) ;
		///< Returns the escape()d string.

	static void unescape( std::string & s , char c_escape , std::string_view specials_in , std::string_view specials_out ) ;
		///< Unescapes the string by replacing e-e with e, e-special-in with
		///< special-out, and e-other with other. The specials-in and
		///< specials-out strings must be the same length.

	static void unescape( std::string & s ) ;
		///< Overload for 'normal' unescaping where the string has backslash escaping
		///< of whitespace.

	static std::string unescaped( const std::string & s ) ;
		///< Returns the unescape()d version of s.

	static std::string_view meta() noexcept ;
		///< Returns a list of shell meta-characters with a tilde as the
		///< first character. Does not contain the nul character. This is
		///< typically used with escape().

	static std::string readLineFrom( std::istream & stream , std::string_view eol = {} ) ;
		///< Reads a line from the stream using the given line terminator.
		///< The line terminator is not part of the returned string.
		///< The terminator defaults to the newline.
		///<
		///< Note that alternatives in the standard library such as
		///< std::istream::getline() or std::getline(stream,string)
		///< in the standard "string" header are limited to a single
		///< character as the terminator.
		///<
		///< The side-effects on the stream state follow std::getline():
		///< reading an unterminated line at the end of the stream sets
		///< eof, but a fully-terminated line does not; if no characters
		///< are read (including terminators) (eg. already eof) then
		///< failbit is set.
		///<
		///< A read loop that processes even incomplete lines at the end
		///< of the stream should do:
		///< \code
		///< while(stream.good()){read(stream);if(stream)process(line)} // or
		///< while(read(stream)){process(line)}
		///< \endcode
		///< or to process only complete lines:
		///< \code
		///< while(stream){read(stream);if(stream.good())process(line)} // or
		///< while(read(stream)&&stream.good()){process(line)}
		///< \endcode

	static std::istream & readLine( std::istream & stream , std::string & result ,
		std::string_view eol = {} , bool pre_erase_result = true , std::size_t limit = 0U ) ;
			///< Reads a line from the stream using the given line
			///< terminator, which may be multi-character. Behaves like
			///< std::getline() when the trailing parameters are defaulted.
			///< Also behaves like std::getline() by not clearing the
			///< result if the stream is initially not good(), even if
			///< the pre-erase flag is set.

	static std::istream & readLine( std::istream & stream , std::string & result ,
		Eol , bool pre_erase_result = true , std::size_t limit = 0U ) ;
			///< An overload where lines are terminated with some
			///< enumerated combination of CR, LF or CRLF.

	static void splitIntoTokens( const std::string & in , StringArray & out , std::string_view ws , char esc = '\0' ) ;
		///< Splits the string into 'ws'-delimited tokens. The behaviour is like
		///< strtok() in that adjacent delimiters count as one and leading and
		///< trailing delimiters are ignored. The output array is _not_ cleared
		///< first; new tokens are appended to the output list. If the escape
		///< character is supplied then it can be used to escape whitespace
		///< characters, preventing a split, with those escape characters being
		///< consumed in the process. For shell-like tokenising use dequote()
		///< before splitIntoTokens() (using the same escape character), and
		///< revert the non-breaking spaces afterwards.

	static StringArray splitIntoTokens( const std::string & in , std::string_view ws = Str::ws() , char esc = '\0' ) ;
		///< Overload that returns by value.

	static void splitIntoFields( std::string_view in , StringArray & out ,
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

	static StringArray splitIntoFields( std::string_view in , char sep ) ;
		///< Overload that returns by value.

	static std::string dequote( const std::string & , char qq = '\"' , char esc = '\\' ,
		std::string_view ws = Str::ws() , std::string_view nbws = Str::ws() ) ;
			///< Dequotes a string by removing unescaped quotes and escaping
			///< quoted whitespace, so "qq-aaa-esc-qq-bbb-ws-ccc-qq" becomes
			///< "aaa-qq-bbb-esc-ws-ccc". Escaped whitespace characters within
			///< quotes can optionally be converted to non-breaking equivalents.

	static std::string join( std::string_view sep , const StringArray & strings ) ;
		///< Concatenates an array of strings with separators.

	static std::string join( std::string_view sep , std::string_view s1 , std::string_view s2 ,
		std::string_view s3 = {} , std::string_view s4 = {} , std::string_view s5 = {} ,
		std::string_view s6 = {} , std::string_view s7 = {} , std::string_view s8 = {} ,
		std::string_view s9 = {} ) ;
			///< Concatenates a small number of strings with separators.
			///< In this overload empty strings are ignored.

	static std::string join( std::string_view sep , const StringMap & , std::string_view eq ,
		std::string_view tail = {} ) ;
			///< Concatenates entries in a map, where an entry is "<key><eq><value><tail>".

	static StringArray keys( const StringMap & string_map ) ;
		///< Extracts the keys from a map of strings.

	static std::string head( std::string_view in , std::size_t pos , std::string_view default_ = {} ) ;
			///< Returns the first part of the string up to just before the given position.
			///< The character at pos is not returned. Returns the supplied default
			///< if pos is npos. Returns the whole string if pos is one-or-more
			///< off the end.

	static std::string head( std::string_view , std::string_view sep , bool default_empty = true ) ;
		///< Overload taking a separator string, and with the default
		///< as either the input string or the empty string. If the
		///< separator occurs more than once in the input then only the
		///< first occurrence is relevant.

	static std::string_view headView( std::string_view in , std::size_t pos , std::string_view default_ = {} ) noexcept ;
		///< Like head() but returning a view into the input string.

	static std::string_view headView( std::string_view in , std::string_view sep , bool default_empty = true ) noexcept ;
		///< Like head() but returning a view into the input string.

	static std::string tail( std::string_view in , std::size_t pos , std::string_view default_ = {} ) ;
			///< Returns the last part of the string after the given position.
			///< The character at pos is not returned. Returns the supplied default
			///< if pos is npos. Returns the empty string if pos is one-or-more
			///< off the end.

	static std::string tail( std::string_view in , std::string_view sep , bool default_empty = true ) ;
		///< Overload taking a separator string, and with the default
		///< as either the input string or the empty string. If the
		///< separator occurs more than once in the input then only the
		///< first occurrence is relevant.

	static std::string_view tailView( std::string_view in , std::size_t pos , std::string_view default_ = {} ) noexcept ;
		///< Like tail() but returning a view into the input string.

	static std::string_view tailView( std::string_view in , std::string_view sep , bool default_empty = true ) noexcept ;
		///< Like tail() but returning a view into the input string.

	static bool match( std::string_view , std::string_view ) noexcept ;
		///< Returns true if the two strings are the same.

	static bool iless( std::string_view , std::string_view ) noexcept ;
		///< Returns true if the first string is lexicographically less
		///< than the first, after seven-bit lower-case letters have been
		///< folded to upper-case.

	static bool imatch( char , char ) noexcept ;
		///< Returns true if the two characters are the same, ignoring seven-bit case.

	static bool imatch( std::string_view , std::string_view ) noexcept ;
		///< Returns true if the two strings are the same, ignoring seven-bit case.
		///< The locale is ignored.

	static std::size_t ifind( std::string_view s , std::string_view key ) ;
			///< Returns the position of the key in 's' using a seven-bit case-insensitive
			///< search. Returns std::string::npos if not found. The locale is ignored.

	static std::size_t ifindat( std::string_view s , std::string_view key , std::size_t pos ) ;
			///< Returns the position of the key in 's' at of after position 'pos'
			///< using a seven-bit case-insensitive search. Returns std::string::npos
			///< if not found. The locale is ignored.

	static bool tailMatch( std::string_view in , std::string_view ending ) noexcept ;
		///< Returns true if the string has the given ending (or the given ending is empty).

	static bool headMatch( std::string_view in , std::string_view head ) noexcept ;
		///< Returns true if the string has the given start (or head is empty).

	static std::string_view ws() noexcept ;
		///< Returns a string of standard whitespace characters.

	static std::string_view alnum() noexcept ;
		///< Returns a string of seven-bit alphanumeric characters, ie A-Z, a-z and 0-9.

	static std::string_view alnum_() noexcept ;
		///< Returns alnum() with an additional trailing underscore character.

	static std::string positive() ;
		///< Returns a default positive string. See isPositive().

	static std::string negative() ;
		///< Returns a default negative string. See isNegative().

	static bool isPositive( std::string_view ) noexcept ;
		///< Returns true if the string has a positive meaning, such as "1", "true", "yes".

	static bool isNegative( std::string_view ) noexcept ;
		///< Returns true if the string has a negative meaning, such as "0", "false", "no".

	static std::string unique( const std::string & s , char c , char r ) ;
		///< Returns a string with repeated 'c' characters replaced by
		///< one 'r' character. Single 'c' characters are not replaced.

	static std::string unique( const std::string & s , char c ) ;
		///< An overload that replaces repeated 'c' characters by
		///< one 'c' character.

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

inline
std::string G::Str::fromULong( unsigned long u , const Hex & )
{
	std::array <char,sizeof(u)*2U> buffer ; // NOLINT cppcoreguidelines-pro-type-member-init
	return sv_to_string( fromULongToHex( u , buffer.data() ) ) ;
}

inline
std::string G::Str::fromULongLong( unsigned long long u , const Hex & )
{
	std::array <char,sizeof(u)*2U> buffer ; // NOLINT cppcoreguidelines-pro-type-member-init
	return sv_to_string( fromULongLongToHex( u , buffer.data() ) ) ;
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
