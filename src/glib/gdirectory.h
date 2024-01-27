//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include <memory>
#include <list>
#include <sys/types.h>

namespace G
{
	class DirectoryIteratorImp ;
	class Directory ;
	class DirectoryIterator ;
	class DirectoryList ;
}

//| \class G::Directory
/// An encapsulation of a file system directory that works with
/// G::DirectoryIterator.
/// \see G::Path, G::FileSystem, G::File
///
class G::Directory
{
public:
	Directory() ;
		///< Default constructor for the current directory.

	explicit Directory( const Path & path ) ;
		///< Constructor.

	explicit Directory( const std::string & path ) ;
		///< Constructor.

	int usable( bool for_creating_files = false ) const ;
		///< Returns zero if the object represents a valid directory
		///< with permissions that dont disallow reading of any
		///< contained files. Returns a non-zero errno otherwise.
		///<
		///< Does additional checks if the 'for-creating-files'
		///< parameter is true. But note that the answer is not
		///< definitive -- file creation may fail, even if
		///< usable() returns zero. For a more accurate test use
		///< writeable().

	bool valid( bool for_creating_files = false ) const ;
		///< Returns true iff usable() is zero.

	bool writeable( const std::string & probe_filename = tmp() ) const ;
		///< Tries to create and then delete an empty test file
		///< in the directory. Returns true on success.
		///< Precondition: valid()

	Path path() const ;
		///< Returns the directory's path, as passed in to the ctor.

	static std::string tmp() ;
		///< A convenience function for constructing a filename for
		///< writeable(). This is factored-out from writeable() into
		///< this public interface so that client code can minimise
		///< the time spent with a privileged effective userid.

private:
	Path m_path ;
} ;

//| \class G::DirectoryIterator
/// A iterator that returns unsorted filenames in a directory.
/// The iteration model is:
/// \code
/// while(iter.more()) { auto path = iter.filePath() ; }
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

	bool isLink() const ;
		///< Returns true if the current item is a symlink.

	std::string sizeString() const ;
		///< Returns the file size as a decimal string. The value
		///< may be bigger than any integer type can hold.

	Path filePath() const ;
		///< Returns the path of the current item.

	std::string fileName() const ;
		///< Returns the name of the current item. On Windows
		///< any characters that cannot be represented in the
		///< active code page are replaced by '?'.

public:
	DirectoryIterator( const DirectoryIterator & ) = delete ;
	DirectoryIterator( DirectoryIterator && ) noexcept = default ;
	DirectoryIterator & operator=( const DirectoryIterator & ) = delete ;
	DirectoryIterator & operator=( DirectoryIterator && ) noexcept = default ;

private:
	std::unique_ptr<DirectoryIteratorImp> m_imp ;
} ;

//| \class G::DirectoryList
/// A iterator similar to G::DirectoryIterator but doing all file
/// i/o in one go and providing a sorted result. This can be useful
/// when temporarily adopting additional process privileges to
/// read a directory.
///
class G::DirectoryList
{
public:
	struct Item /// A directory-entry item for G::DirectoryList.
	{
		bool m_is_dir{false} ;
		bool m_is_link{false} ;
		Path m_path ;
		std::string m_name ;
		bool operator<( const Item & ) const noexcept ;
	} ;

	DirectoryList() ;
		///< Default constructor for an empty list. Initialise with one
		///< of the three read methods to do all the file i/o in one go.

	std::size_t readAll( const Path & dir ) ;
		///< An initialiser that is to be used after default construction.
		///< Reads all files in the directory.

	std::size_t readType( const Path & dir , string_view suffix , unsigned int limit = 0U ) ;
		///< An initialiser that is to be used after default
		///< construction. Reads all files that have the given
		///< suffix (unsorted).

	std::size_t readDirectories( const Path & dir , unsigned int limit = 0U ) ;
		///< An initialiser that reads all sub-directories.

	bool more() ;
		///< Returns true if more and advances by one.

	bool isDir() const ;
		///< Returns true if the current item is a directory.

	bool isLink() const ;
		///< Returns true if the current item is a symlink.

	Path filePath() const ;
		///< Returns the current path.

	std::string fileName() const ;
		///< Returns the current filename. On Windows any characters
		///< that cannot be represented in the active code page are
		///< replaced by '?'.

	static void readAll( const Path & dir , std::vector<Item> & out ) ;
		///< A static overload returning by reference a collection
		///< of Items, sorted by name.

private:
	void readImp( const Path & , bool , string_view , unsigned int ) ;

private:
	bool m_first{true} ;
	unsigned int m_index{0U} ;
	std::vector<Item> m_list ;
} ;

inline bool G::DirectoryList::Item::operator<( const Item & other ) const noexcept
{
	return m_name < other.m_name ;
}

#endif
