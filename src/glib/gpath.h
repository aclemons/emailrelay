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
/// \file gpath.h
///

#ifndef G_PATH_H
#define G_PATH_H

#include "gdef.h"
#include "gstrings.h"
#include <string>
#include <iostream>

/// \namespace G
namespace G
{
	class Path ;
}

/// \class G::Path
/// A Path object represents a file system path.
/// The class is concerned with path syntax, not file system i/o.
/// This class is necessary because of the mess Microsoft made
/// with drive letters (like having a cwd associated with each
/// drive).
///
/// \see G::File, G::Directory, G::FileSystem
///
class G::Path 
{
public:
	Path() ;
		///< Default constructor. Creates
		///< a zero-length path.

	Path( const std::string & path ) ;
		///< Implicit constructor.

	Path( const char * path ) ;
		///< Implicit constructor.

	Path( const Path & path , const std::string & tail ) ;
		///< Constructor with an implicit pathAppend().

	Path( const Path & path , const std::string & tail_1 , const std::string & tail_2 ) ;
		///< Constructor with two implicit pathAppend()s.

	Path( const Path & other ) ;
		///< Copy constructor.

	~Path() ;
		///< Destructor.

	bool simple() const ;
		///< Returns true if the path is just a file/directory name without
		///< any separators. Note that if the path is simple() then dirname()
		///< will return the empty string.

	std::string str() const ;
		///< Returns the path string.

	std::string basename() const ;
		///< Returns the path, excluding drive/directory parts.
		///< Does nothing with the extension (cf. basename(1)).

	Path dirname() const ;
		///< Returns the drive/directory parts of the path. If this path is
		///< the top of the tree then the null path is returned.
		///<
		///< eg. "c:foo\bar.exe" -> "c:foo"
		///< eg. "c:\foo\bar.exe" -> "c:\foo"
		///< eg. "c:bar.exe" -> "c:"
		///< eg. "c:\file" -> "c:\"                              .
		///< eg. "c:\" -> ""
		///< eg. "c:" -> ""
		///< eg. "\foo\bar.exe" -> "\foo"
		///< eg. "\" -> ""
		///< eg. "foo\bar\bletch" -> "foo\bar"
		///< eg. "foo\bar" -> "foo"
		///< eg. "bar.exe" -> ""
		///< eg. "\\machine\drive\dir\file.cc" -> "\\machine\drive\dir"
		///< eg. "\\machine\drive\file" -> "\\machine\drive"
		///< eg. "\\machine\drive" -> ""

	std::string extension() const ;
		///< Returns the path's filename extension. Returns the 
		///< zero-length string if there is none.

	void removeExtension() ;
		///< Modifies the path by removing any extension.

	bool isAbsolute() const ;
		///< Returns !isRelative().

	bool isRelative() const ;
		///< Returns true if the path is a relative path.

	bool hasDriveLetter() const ;
		///< Returns true if the path has a leading drive letter
		///< (and the operating system uses drive letters).

	Path & operator=( const Path & other ) ;
		///< Assignment operator.

	void pathAppend( const std::string & tail ) ;
		///< Appends a filename to the path. A path separator
		///< is added if necessary.

	static G::Path join( const G::Path & p1 , const G::Path & p2 ) ;
		///< Joins two paths together. The second should normally be
		///< a relative path, although absolute paths are allowed.

	Strings split( bool no_dot = true ) const ;
		///< Spits the path into a list of component parts.
		///< Eliminates "/./" parts if the optional parameter
		///< is true.

	bool operator==( const Path & path ) const ;
		///< Comparison operator.

	bool operator!=( const Path & path ) const ;
		///< Comparison operator.

private:
	void set( const std::string & path ) ;
	void normalise() ;
	void clear() ;
	static std::string slashString() ;
	static std::string doubleSlashString() ;
	std::string driveString() const ;
	std::string::size_type slashAt() const ;
	bool hasNoSlash() const ;
	std::string withoutTail() const ;
	bool hasNetworkDrive() const ;
	std::string dirnameImp() const ;

private:
	std::string m_str ;
	std::string m_extension ;
	std::string::size_type m_dot ;
} ;

/// \namespace G
namespace G
{
	inline
	std::ostream & operator<<( std::ostream & stream , const Path & path )
	{
		return stream << path.str() ;
	}

	inline
	Path & operator+=( Path & p , const std::string & str )
	{
		p.pathAppend( str ) ;
		return p ;
	}

	inline
	Path operator+( const Path & p , const std::string & str )
	{
		Path result( p ) ;
		result.pathAppend( str ) ;
		return result ;
	}
}

#endif
