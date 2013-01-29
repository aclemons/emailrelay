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
/// \file gpopstore.h
///

#ifndef G_POP_STORE_H
#define G_POP_STORE_H

#include "gdef.h"
#include "gpop.h"
#include "gpath.h"
#include "gexception.h"
#include <string>
#include <iostream>
#include <set>
#include <list>

/// \namespace GPop
namespace GPop
{
	class Store ;
	class StoreLock ;
	class StoreLockEntry ;
}

/// \class GPop::Store
/// A message store. Unlike the SMTP message store the
/// POP message store allows content files to be in the envelope file's
/// parent directory.
///
class GPop::Store 
{
public:
	G_EXCEPTION( InvalidDirectory , "invalid spool directory" ) ;

	Store( G::Path spool_dir , bool by_name , bool allow_delete ) ;
		///< Constructor.

	G::Path dir() const ;
		///< Returns the spool directory path.

	bool allowDelete() const ;
		///< Returns true if files can be deleted.

	bool byName() const ;
		///< Returns true if the spool directory is affected
		///< by the user name.

private:
	Store( const Store & ) ;
	void operator=( const Store & ) ;
	static void checkPath( G::Path , bool , bool ) ;
	static bool valid( G::Path , bool ) ;

private:
	G::Path m_path ;
	bool m_by_name ;
	bool m_allow_delete ;
} ;

/// \class GPop::StoreLockEntry
/// Represents a file in the GPop::Store.
/// \see GPop::StoreLock
///
class GPop::StoreLockEntry  
{
public:
	int id ;
	typedef unsigned long Size ;
	Size size ;
	std::string uidl ;
	StoreLockEntry( int id_ , Size size_ , const std::string & uidl_ ) :
		id(id_) ,
		size(size_) ,
		uidl(uidl_)
	{
	}
} ;

/// \class GPop::StoreLock
/// Represents an exclusive lock on the message store.
/// \see RFC-1939
///
class GPop::StoreLock 
{
public:
	G_EXCEPTION( CannotDelete , "cannot delete message file" ) ;
	G_EXCEPTION( CannotRead , "cannot read message file" ) ;
	typedef StoreLockEntry::Size Size ;
	typedef StoreLockEntry Entry ;
	typedef std::list<Entry> List ;
	typedef void (*Fn)(std::ostream&,const std::string&) ;

	explicit StoreLock( Store & store ) ;
		///< Constructor. Keeps the reference.
		///<
		///< Postcondition: !locked()

	void lock( const std::string & user ) ;
		///< Initialisation.
		///<
		///< Precondition: !user.empty() && !locked()
		///< Postcondition: locked()

	bool locked() const ;
		///< Returns true if locked.

	~StoreLock() ;
		///< Destructor.

	Size messageCount() const ;
		///< Returns the store's message count.

	Size totalByteCount() const ;
		///< Returns the store's total byte count.

	Size byteCount( int id ) const ;
		///< Returns a message size.

	std::string uidl( int id ) const ;
		///< Returns a message's unique id.

	bool valid( int id ) const ;
		///< Validates a message number.

	List list( int id = -1 ) const ;
		///< Lists messages in the store.

	std::auto_ptr<std::istream> get( int id ) const ;
		///< Retrieves the message content.

	void remove( int ) ;
		///< Marks the message for removal.

	void rollback() ;
		///< Rolls back remove()als but retains the lock.
		///<
		///< Precondition: locked()
		///< Postcondition: locked() [sic]

	void commit() ;
		///< Commits remove()als.
		///< Postcondition: !locked()

private:
	/// A private implementation class used by GPop::StoreLock.
	struct File 
	{
		std::string name ; // content name
		StoreLockEntry::Size size ;
		explicit File( const G::Path & ) ;
		File( const std::string & name_ , const std::string & size_string ) ;
		bool operator<( const File & ) const ;
		static StoreLockEntry::Size toSize( const std::string & s ) ;
	} ;
	typedef std::set<File> Set ;

private:
	StoreLock( const StoreLock & ) ; // not implemented
	void operator=( const StoreLock & ) ; // not implemented
	Set::iterator find( const std::string & ) ;
	Set::const_iterator find( int id ) const ;
	Set::iterator find( int id ) ;
	void doCommit( Store & ) const ;
	G::Path path( int id ) const ;
	G::Path path( const std::string & , bool fallback ) const ;
	std::string envelopeName( const std::string & ) const ;
	std::string contentName( const std::string & ) const ;
	G::Path contentPath( const std::string & ) const ;
	G::Path contentPath( const File & ) const ;
	G::Path envelopePath( const File & ) const ;
	void deleteFile( const G::Path & , bool & ) const ;
	bool unlinked( Store & , const File & ) const ;

private:
	Store * m_store ;
	G::Path m_dir ;
	std::string m_user ;
	Set m_initial ;
	Set m_current ;
	Set m_deleted ;
} ;

#endif
