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
/// \file gdirectory.h
///

#ifndef G_DIRECTORY_H
#define G_DIRECTORY_H

#include "gdef.h"
#include "gpath.h"
#include "gexception.h"
#include <string>
#include <vector>
#include <sys/types.h>

/// \namespace G
namespace G
{
	class DirectoryIteratorImp ;
	class Directory ;
	class DirectoryIterator ;
	class DirectoryList ;
}

/// \class G::Directory
/// An encapsulation of a file system directory
/// which allows for iterating through the set of contained
/// files.
/// \see G::Path, G::FileSystem, G::File
///
class G::Directory 
{
public:
	Directory() ;
		///< Default constructor for the current directory.

	explicit Directory( const char * path ) ;
		///< Constructor.
		
	explicit Directory( const Path & path ) ;
		///< Constructor.

	explicit Directory( const std::string & path ) ;
		///< Constructor.
		
	~Directory() ;
		///< Destructor.

	bool valid( bool for_creating_files = false ) const ;
		///< Returns true if the object represents a valid 
		///< directory.
		///<
		///< Does additional checks if the 'for-creating-files' 
		///< parameter is true. But note that the answer is not 
		///< definitive -- file creation may fail, even if 
		///< valid() returns true. For a more accurate test 
		///< use writeable().

	bool writeable( std::string probe_filename = tmp() ) const ;
		///< Tries to create and then delete an empty test file 
		///< in the directory. Returns true on success.
		///< Precondition: valid()

	Path path() const ;
		///< Returns the directory's path.

	Directory( const Directory & other ) ;
		///< Copy constructor.

	Directory & operator=( const Directory & ) ;
		///< Assignment operator.

	static Directory root() ;
		///< Returns a root directory object. For DOSy file
		///< systems this will not contain a drive part.

	static std::string tmp() ;
		///< A convenience function for constructing a
		///< filename for writeable(). This is factored out
		///< so that client code can minimise the time spent 
		///< with a privileged effective userid.

private:
	Path m_path ;
} ;

/// \class G::DirectoryIterator
/// A Directory iterator. The iteration model is
/// \code
/// while(iter.more()) { (void)iter.filePath() ; }
/// \endcode
///
class G::DirectoryIterator 
{
public:
	DirectoryIterator( const Directory & dir , const std::string & wc ) ;
		///< Constructor taking a directory reference and
		///< a wildcard specification. Iterates over all 
		///< matching files in the directory.
		///<
		///< This constructor overload may not be implemented
		///< on all platforms, so prefer DirectoryList::readType()
		///< where possible.

	explicit DirectoryIterator( const Directory & dir ) ;
		///< Constructor taking a directory reference.
		///< Iterates over all files in the directory.

	~DirectoryIterator() ;
		///< Destructor.

	bool error() const ;
		///< Returns true on error. The caller should stop the iteration.

	bool more() ;
		///< Returns true if more and advances by one.

	bool isDir() const ;
		///< Returns true if the current item is a directory.

	std::string modificationTimeString() const ; 
		///< Returns the last-modified time for the file in an undefined 
		///< format -- used for comparison.

	std::string sizeString() const ; 
		///< Returns the file size as a decimal string. The value
		///< may be more than 32 bits. See also class G::Number.

	Path filePath() const ;
		///< Returns the path of the current item.

	Path fileName() const ;
		///< Returns the name of the current item.

private:
	DirectoryIterator( const DirectoryIterator & ) ; // not implemented
	void operator=( const DirectoryIterator & ) ; // not implemented

private:
	DirectoryIteratorImp * m_imp ;
} ;

/// \class G::DirectoryList
/// A Directory iterator that does all file i/o in one go.
/// This is useful, compared to DirectoryIterator, while temporarily 
/// adopting additional process privileges to read a directory.
/// The implementation uses DirectoryIterator.
///
class G::DirectoryList 
{
public:
	DirectoryList() ;
		///< Default constructor for an empty list. Initialise with a
		///< read method.

	void readAll( const Path & dir ) ;
		///< An initialiser that is to be used after default 
		///< construction. Reads all files in the directory.
		///< All file i/o is done in readAll()/readType().

	void readType( const Path & dir , const std::string & suffix , unsigned int limit = 0U ) ;
		///< An initialiser that is to be used after default 
		///< construction. Reads all files that have the given 
		///< suffix. All file i/o is done in readAll()/readType().

	bool more() ;
		///< Returns true if more and advances by one.

	bool isDir() const ;
		///< Returns true if the current item is a directory.

	G::Path filePath() const ;
		///< Returns the current path.

	G::Path fileName() const ;
		///< Returns the current filename.

private:
	DirectoryList( const DirectoryList & ) ;
	void operator=( const DirectoryList & ) ;

private:
	bool m_first ;
	unsigned int m_index ;
	std::vector<int> m_is_dir ;
	std::vector<G::Path> m_path ;
	std::vector<G::Path> m_name ;
} ;

#endif
