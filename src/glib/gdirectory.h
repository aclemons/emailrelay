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
// gdirectory.h
//

#ifndef G_DIRECTORY_H
#define G_DIRECTORY_H

#include "gdef.h"
#include "gpath.h"
#include "gexception.h"
#include <string>
#include <sys/types.h>

namespace G
{
	class DirectoryIteratorImp ;
	class Directory ;
	class DirectoryIterator ;
}

// Class: G::Directory
// Description: An encapsulation of a file system directory
// which allows for iterating through the set of contained
// files.
// See also: G::Path, G::FileSystem, G::File
//
class G::Directory 
{
public:
	Directory() ;
		// Default constructor for the current directory.

	explicit Directory( const char * path ) ;
		// Constructor.
		
	explicit Directory( const Path & path ) ;
		// Constructor.

	explicit Directory( const std::string & path ) ;
		// Constructor.
		
	virtual ~Directory() ;
		// Virtual destructor.

	bool valid( bool for_creating_files = false ) const ;
		// Returns true if the object represents a valid 
		// directory.
		//
		// Does additional checks if the 'for-creating-files' 
		// parameter is true. But note that the answer is not 
		// definitive -- file creation may fail, even if 
		// valid() returns true. For a more accurate test 
		// use writeable().

	bool writeable( std::string probe_filename = tmp() ) const ;
		// Tries to create and then delete an empty test file 
		// in the directory. Returns true on success.
		// Precondition: valid()

	Path path() const ;
		// Returns the directory's path.

	Directory( const Directory & other ) ;
		// Copy constructor.

	Directory &operator=( const Directory & ) ;
		// Assignment operator.

	static Directory root() ;
		// Returns a root directory object. For DOSy file
		// systems this will not contain a drive part.

	static std::string tmp() ;
		// A convenience function for constructing a
		// filename for writeable(). This is factored out
		// so that client code can minimise the time spent 
		// with a privileged effective userid.

private:
	Path m_path ;
} ;

// Class: G::DirectoryIterator
// Description: An iterator for Directory.
// The iteration model is
/// while(iter.more()) { (void)iter.filePath() ; }
//
class G::DirectoryIterator 
{
public:
	explicit DirectoryIterator( const Directory & dir , const std::string & wc = std::string() ) ;
		// Constructor taking a directory reference
		// and an optional wildcard specification.

	~DirectoryIterator() ;
		// Destructor.

	bool error() const ;
		// Returns true on error. The caller should stop the iteration.

	bool more() ;
		// Returns true if more.

	bool isDir() const ;
		// Returns true if the current item is a directory.

	std::string modificationTimeString() const ; 
		// Returns the last-modified time for the file in an undefined 
		// format -- used for comparison.

	std::string sizeString() const ; 
		// Returns the file size as a decimal string. The value
		// may be more than 32 bits. See also class G::Number.

	Path filePath() const ;
		// Returns the path of the current item.

	Path fileName() const ;
		// Returns the name of the current item.

private:
	DirectoryIterator( const DirectoryIterator & ) ;
	void operator=( const DirectoryIterator & ) ;

private:
	DirectoryIteratorImp * m_imp ;
} ;

#endif
