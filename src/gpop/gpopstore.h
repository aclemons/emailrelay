//
// Copyright (C) 2001-2005 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gpopstore.h
//

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

namespace GPop
{
	class Store ;
	class StoreLock ;
}

// Class: GPop::Store
// Description: A message store.
//
class GPop::Store 
{
public:
	Store( G::Path dir , bool by_name , bool allow_delete ) ;
		// Constructor.

	G::Path dir() const ;
		// Returns the directory path.

	bool allowDelete() const ;
		// Returns true if files can be deleted.

	bool byName() const ;
		// Returns true if the spool directory is affected
		// by the user name.

private:
	Store( const Store & ) ;
	void operator=( const Store & ) ;

private:
	G::Path m_path ;
	bool m_by_name ;
	bool m_allow_delete ;
} ;

// Class: GPop::StoreLock
// Description: Represents an exclusive lock on the message store.
// See also: RFC-1939
//
class GPop::StoreLock 
{
public:
	G_EXCEPTION( CannotDelete , "cannot delete message" ) ;
	typedef unsigned long Size ;
	struct Entry
	{
		int id ;
		StoreLock::Size size ;
		std::string uidl ;
		Entry( int id_ , StoreLock::Size size_ , const std::string & uidl_ ) :
			id(id_) ,
			size(size_) ,
			uidl(uidl_)
		{
		}
	} ;
	typedef std::list<Entry> List ;
	typedef void (*Fn)(std::ostream&,const std::string&) ;

	explicit StoreLock( Store & store ) ;
		// Constructor. Keeps the reference.
		//
		// Postcondition: !locked()

	void lock( const std::string & user ) ;
		// Initialisation.
		//
		// Precondition: !user.empty() && !locked()
		// Postcondition: locked()

	bool locked() const ;
		// Returns true if locked.

	~StoreLock() ;
		// Destructor.

	Size messageCount() const ;
		// Returns the store's message count.

	Size totalByteCount() const ;
		// Returns the store's total byte count.

	Size byteCount( int id ) const ;
		// Returns a message size.

	std::string uidl( int id ) const ;
		// Returns a message's unique id.

	bool valid( int id ) const ;
		// Validates a message number.

	List list( int id = -1 ) const ;
		// Lists messages in the store.

	std::auto_ptr<std::istream> get( int id ) const ;
		// Retrieves the message content.

	void remove( int ) ;
		// Marks the message for removal.

	void rollback() ;
		// Rolls back remove()als but retains the lock.
		//
		// Precondition: locked()
		// Postcondition: locked() [sic]

	void commit() ;
		// Commits remove()als.
		// Postcondition: !locked()

private:
	struct File
	{
		std::string name ; // envelope
		StoreLock::Size size ;
		explicit File( const G::Path & ) ;
		File( const std::string & name_ , const std::string & size_string ) ;
		bool operator<( const File & ) const ;
		static StoreLock::Size toSize( const std::string & s ) ;
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
	G::Path path( const std::string & ) const ;
	std::string contentName( const std::string & ) const ;
	G::Path contentPath( const std::string & ) const ;
	G::Path contentPath( const File & ) const ;
	G::Path envelopePath( const File & ) const ;
	void deleteFile( const G::Path & , bool & ) const ;

private:
	Store * m_store ;
	G::Path m_dir ;
	std::string m_user ;
	Set m_initial ;
	Set m_current ;
	Set m_deleted ;
} ;

#endif
