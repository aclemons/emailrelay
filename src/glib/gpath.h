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
/// \file gpath.h
///

#ifndef G_PATH_H
#define G_PATH_H

#include "gdef.h"
#include "gstringarray.h"
#include "gstringview.h"
#ifdef G_WINDOWS
#include "gconvert.h"
#endif
#include <string>
#include <iostream>
#include <initializer_list>

namespace G
{
	class Path ;
}

//| \class G::Path
/// A Path object represents a file system path. The class is concerned with
/// path syntax, not file system i/o.
///
/// A full path is made up of a root, a set of directories, and a filename. The
/// posix root is just a forward slash, but on Windows the root can be complex,
/// possibly including non-splitting separator characters. The filename may have
/// an extension part, which is to the right of the right-most dot.
///
/// The path separator is used between directories and filename, but only
/// between the root and the first directory if the root does not itself end in
/// a separator character.
///
/// A windows drive-letter root may end with a separator character or not; if
/// there is no separator character at the end of the drive-letter root then
/// the path is relative to the drive's current working directory.
///
/// Path components of "." are ignored by simple(), basename(), and dirname().
/// Path components of ".." are retained but can be eliminated if they are
/// collapsed(). Path components of "." are eliminated by split(), except
/// in the degenerate case.
///
/// This class is agnostic on the choice of UTF-8 or eight-bit characters since
/// the delimiters are all seven-bit ascii. Wide characters are not used,
/// following "utf8everywhere.org" rather than std::filesystem::path (but
/// see G_ANSI as a temporary deprecated feature).
///
/// Most file operations should be handled in o/s-aware source (see G::File,
/// G::Environment, G::Process etc) so that the Path character encoding is
/// opaque. However, std::fstream objects can be initialised directly by
/// using G::Path::iopath().
///
/// Both posix and windows behaviours are available at run-time; the default
/// behaviour is the native behaviour, but this can be overridden, typically
/// for testing purposes.
///
/// The posix path separator character is the forward-slash; on Windows it is a
/// back-slash, but with all forward-slashes converted to back-slashes
/// immediately on input.
///
/// \see G::File, G::Directory
///
class G::Path
{
public:
	using value_type = char ;
	using string_type = std::string ;
	#if defined(G_WINDOWS) && !defined(G_ANSI)
		using iopath_char_type = wchar_t ;
	#else
		using iopath_char_type = char ;
	#endif

	Path() noexcept(noexcept(std::string())) ;
		///< Default constructor for a zero-length path.
		///< Postcondition: empty()

	Path( const std::string & path ) ;
		///< Implicit constructor from a string.

	Path( std::string_view path ) ;
		///< Implicit constructor from a string view.

	Path( const char * path ) ;
		///< Implicit constructor from a c string.

	Path( const Path & path , const std::string & tail ) ;
		///< Constructor with an implicit pathAppend().

	Path( const Path & path , const std::string & tail_1 , const std::string & tail_2 ) ;
		///< Constructor with two implicit pathAppend()s.

	Path( const Path & path , const std::string & tail_1 , const std::string & tail_2 , const std::string & tail_3 ) ;
		///< Constructor with three implicit pathAppend()s.

	bool empty() const noexcept ;
		///< Returns true if the path is empty.

	std::string str() const ;
		///< Returns the path string.

	const iopath_char_type * iopath() const ;
		///< Returns the path's string with a type that is suitable for
		///< initialising std::fstreams.

	const value_type * cstr() const noexcept ;
		///< Returns the path's c-string. Typically used by o/s-aware
		///< code such as G::File.

	bool simple() const ;
		///< Returns true if the path has a single component (ignoring "." parts),
		///< ie. the dirname() is empty.

	std::string basename() const ;
		///< Returns the rightmost part of the path, ignoring "." parts.
		///< For a directory path this may be "..", but see also collapsed().

	Path dirname() const ;
		///< Returns the path without the rightmost part, ignoring "." parts.
		///< For simple() paths the empty path is returned.

	std::string extension() const ;
		///< Returns the path's basename extension, ie. anything
		///< after the rightmost dot. Returns the zero-length
		///< string if there is none.

	Path withExtension( const std::string & ext ) const ;
		///< Returns the path with the new basename extension.
		///< Any previous extension is replaced. The extension
		///< should not normally have a leading dot and it
		///< should not be the empty string.

	Path withoutExtension() const ;
		///< Returns a path without the basename extension, if any.
		///< Returns this path if there is no dot in the basename.
		///< As a special case, a basename() like ".foo" ends up as
		///< "."; prefer withExtension() where appropriate to avoid
		///< this.

	Path withoutRoot() const ;
		///< Returns a path without the root part. This has no effect
		///< if the path isRelative().

	bool isRoot() const noexcept ;
		///< Returns true if the path is a root, like "/", "c:",
		///< "c:/", "\\server\volume" etc.

	bool isAbsolute() const noexcept ;
		///< Returns !isRelative().

	bool isRelative() const noexcept ;
		///< Returns true if the path is a relative path or empty().

	Path & pathAppend( const std::string & tail ) ;
		///< Appends a filename or a relative path to this path.

	bool replace( const std::string_view & from , const std::string_view & to , bool ex_root = false ) ;
		///< Replaces the first occurrence of 'from' with 'to',
		///< optionally excluding the root part. Returns true
		///< if replaced.

	StringArray split() const ;
		///< Spits the path into a list of component parts (ignoring "." parts
		///< unless the whole path is ".").

	static Path join( const StringArray & parts ) ;
		///< Builds a path from a set of parts. Note that part boundaries
		///< are not necessarily preserved once they have been join()ed
		///< into a path.

	static Path join( const Path & p1 , const Path & p2 ) ;
		///< Joins two paths together. The second should be a relative path.

	static Path difference( const Path & p1 , const Path & p2 ) ;
		///< Returns the relative path from p1 to p2. Returns the empty
		///< path if p2 is not under p1. Returns "." if p1 and p2 are the
		///< same. Input paths are collapsed(). Empty input paths are
		///< treated as ".".

	Path collapsed() const ;
		///< Returns the path with "foo/.." and "." parts removed, so far
		///< as is possible without changing the meaning of the path.
		///< Parts like "../foo" at the beginning of the path, or immediately
		///< following the root, are not removed.

	static Path nullDevice() ;
		///< Returns the path of the "/dev/null" special file, or equivalent.

	void swap( Path & other ) noexcept ;
		///< Swaps this with other.

	bool operator==( const Path & path ) const noexcept(noexcept(std::string().compare(std::string()))) ;
		///< Comparison operator.

	bool operator!=( const Path & path ) const noexcept(noexcept(std::string().compare(std::string()))) ;
		///< Comparison operator.

	static void setPosixStyle() ;
		///< Sets posix mode for testing purposes.

	static void setWindowsStyle() ;
		///< Sets windows mode for testing purposes.

	static bool less( const Path & a , const Path & b ) ;
		///< Compares two paths, with simple eight-bit lexicographical
		///< comparisons of each path component. This is slightly different
		///< from a lexicographical comparison of the compete strings
		///< (eg. "a/b" compared to "a./b"), and it is not suitable for
		///< UTF-8 paths.

private:
	std::string m_str ;
	#if defined(G_WINDOWS) && !defined(G_ANSI)
	mutable std::wstring m_wstr ;
	#endif
} ;

inline
bool G::Path::empty() const noexcept
{
	return m_str.empty() ;
}

inline
std::string G::Path::str() const
{
	return m_str ;
}

inline
const char * G::Path::cstr() const noexcept
{
	return m_str.c_str() ;
}

inline
const G::Path::iopath_char_type * G::Path::iopath() const
{
	#if defined(G_WINDOWS) && !defined(G_ANSI)
		m_wstr = Convert::widen( m_str ) ;
		return m_wstr.c_str() ;
	#else
		return m_str.c_str() ;
	#endif
}

namespace G
{
	inline
	std::ostream & operator<<( std::ostream & stream , const Path & path )
	{
		return stream << path.str() ;
	}

	inline
	Path & operator/=( Path & p , const std::string & str )
	{
		p.pathAppend( str ) ;
		return p ;
	}

	inline
	Path operator/( const Path & p , const std::string & str )
	{
		return Path( p , str ) ; // NOLINT not return {...}
	}

	Path & operator+=( Path & , const std::string & ) = delete ;
	Path & operator+( const Path & , const std::string & ) = delete ;

	inline
	void swap( Path & p1 , Path & p2 ) noexcept
	{
		p1.swap( p2 ) ;
	}
}

#endif
