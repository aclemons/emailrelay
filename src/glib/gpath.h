//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gpath.h
//
	
#ifndef G_PATH_H
#define G_PATH_H

#include "gdef.h"
#include "gstrings.h"
#include <string>
#include <iostream>

namespace G
{
	class Path ;
}

// Class: G::Path
// Description: A Path object represents a file system
// path. The class is concerned with path syntax, not
// file system i/o. This class is necessary because
// of the mess Microsoft made with drive letters (like
// having a cwd associated with each drive).
// See also: G::File, G::Directory, G::FileSystem
//
class G::Path 
{
public:
	Path() ;
		// Default constructor. Creates
		// a zero-length path.
		
	Path( const std::string & path ) ;
		// Constructor.
		
	Path( const char *path ) ;
		// Constructor.
		
	Path( const Path & path , const std::string & tail ) ;
		// Constructor with an implicit pathAppend().

	Path( const Path &other ) ;
		// Copy constructor.
		
	virtual ~Path() ;
		// Virtual destructor.

	bool simple() const ;
		// Returns true if the path is just a 
		// file/directory name without 
		// any separators. Note that if the path
		// is simple() then dirname() will
		// return the empty string.

	std::string str() const ;
		// Returns the path string.

	const char *pathCstr() const ;
		// Returns the path string.

	std::string basename() const ;
		// Returns the path, excluding drive/directory parts.
		// Does nothing with the extension (cf. basename(1)).

	Path dirname() const ;
		// Returns the drive/directory parts of the path. If
		// this path is the top of the tree then the
		// null path is returned.
		//
		// eg. "c:foo\bar.exe" -> "c:foo"
		// eg. "c:\foo\bar.exe" -> "c:\foo"
		// eg. "c:bar.exe" -> "c:"
		// eg. "c:\file" -> "c:\"                              .
		// eg. "c:\" -> ""
		// eg. "c:" -> ""
		// eg. "\foo\bar.exe" -> "\foo"
		// eg. "\" -> ""
		// eg. "foo\bar\bletch" -> "foo\bar"
		// eg. "foo\bar" -> "foo"
		// eg. "bar.exe" -> ""
		// eg. "\\machine\drive\dir\file.cc" -> "\\machine\drive\dir"
		// eg. "\\machine\drive\file" -> "\\machine\drive"
		// eg. "\\machine\drive" -> ""

	std::string extension() const ;
		// Returns the path's original extension, even
		// after removeExtension(). Returns
		// the zero-length string if there is none.
	
	void removeExtension() ;
		// Modifies the path by removing any extension.
		// However, the extension returned by extension() is
		// unchanged.
		
	void setExtension( const std::string & extension ) ;
		// Replaces the extension. Any leading dot in the
		// given string is ignored. (The given extension
		// will be returned by subsequent calls
		// to extension().)

	bool isAbsolute() const ;
		// Returns !isRelative().

	bool isRelative() const ;
		// Returns true if the path is a relative
		// path.

	bool hasDriveLetter() const ;
		// Returns true if the path has a leading
		// drive letter (and the file-system
		// uses drive letters).

	Path & operator=( const Path &other ) ;
		// Assignment operator.
	
	void setDirectory( const std::string & dir ) ;
		// Sets the drive/directory.
		
	void pathAppend( const std::string & tail ) ;
		// Appends a filename to the path. A path separator
		// is added if necessary.

	Strings split( bool no_dot = true ) const ;
		// Spits the path into a list
		// of component parts.

	bool operator==( const Path & path ) const ;
		// Comparison operator.
	
	bool operator!=( const Path & path ) const ;
		// Comparison operator.
	
	void streamOut( std::ostream & stream ) const ;
		// Streams out the path.

private:
	void set( const std::string & path ) ;
	void normalise() ;
	void clear() ;	
	void validate( const char * ) const ;
	bool validPath() const ;
	static std::string slashString() ;
	static std::string doubleSlashString() ;
	std::string driveString() const ;
	size_t slashAt() const ;
	bool noSlash() const ;
	std::string noTail() const ;
	bool hasNetworkDrive() const ;

private:
	std::string m_str ;
	std::string m_extension ;
	const char *m_dot ;
} ;

namespace G
{
	inline
	std::ostream & operator<<( std::ostream & stream , const Path & path )
	{
		path.streamOut( stream ) ;
		return stream ;
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
