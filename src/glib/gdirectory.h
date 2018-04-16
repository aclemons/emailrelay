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
/// \file gdirectory.h
///

#ifndef G_DIRECTORY_H
#define G_DIRECTORY_H

#include "gdef.h"
#include "gpath.h"
#include "gexception.h"
#include <string>
#include <vector>
#include <list>
#include <sys/types.h>

namespace G
{
	class DirectoryIteratorImp ;
	class Directory ;
	class DirectoryIterator ;
	class DirectoryList ;
}

/// \class G::Directory
/// An encapsulation of a file system directory that allows for iterating
/// through the set of contained files.
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
		///< Returns true if the object represents a valid directory.
		///<
		///< Does additional checks if the 'for-creating-files'
		///< parameter is true. But note that the answer is not
		///< definitive -- file creation may fail, even if
		///< valid() returns true. For a more accurate test use
		///< writeable().

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

	static std::string tmp() ;
		///< A convenience function for constructing a filename for
		///< writeable(). This is factored-out to this public
		///< interface so that client code can minimise the time
		///< spent with a privileged effective userid.

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

	std::string sizeString() const ;
		///< Returns the file size as a decimal string. The value
		///< may be more than 32 bits. See also class G::Number.

	Path filePath() const ;
		///< Returns the path of the current item.

	std::string fileName() const ;
		///< Returns the name of the current item.

private:
	DirectoryIterator( const DirectoryIterator & ) ; // not implemented
	void operator=( const DirectoryIterator & ) ; // not implemented

private:
	DirectoryIteratorImp * m_imp ;
} ;

/// \class G::DirectoryList
/// A Directory iterator with the same kind of interface as G::DirectoryIterator,
/// but doing all file i/o in one go. This is useful, compared to
/// DirectoryIterator, when temporarily adopting additional process privileges
/// to read a directory.
///
class G::DirectoryList
{
public:
	struct Item /// A directory-entry item for G::DirectoryList.
	{
		bool m_is_dir ;
		Path m_path ;
		std::string m_name ;
	} ;

	DirectoryList() ;
		///< Default constructor for an empty list. Initialise with one
		///< of the two read methods to do all the file i/o in one go.

	void readAll( const Path & dir ) ;
		///< An initialiser that is to be used after default
		///< construction. Reads all files in the directory.

	void readType( const Path & dir , const std::string & suffix , unsigned int limit = 0U ) ;
		///< An initialiser that is to be used after default
		///< construction. Reads all files that have the given
		///< suffix.

	bool more() ;
		///< Returns true if more and advances by one.

	bool isDir() const ;
		///< Returns true if the current item is a directory.

	Path filePath() const ;
		///< Returns the current path.

	std::string fileName() const ;
		///< Returns the current filename.

	void sort() ;
		///< Sorts the files lexicographically.

	static void readAll( const Path & dir , std::vector<Item> & out , bool sorted ) ;
		///< A static overload returning by reference a collection
		///< of Items.

private:
	static bool compare( const Item & , const Item & ) ;

private:
	bool m_first ;
	unsigned int m_index ;
	std::vector<Item> m_list ;
} ;

#endif
