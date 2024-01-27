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
/// \file gpopstore.h
///

#ifndef G_POP_STORE_H
#define G_POP_STORE_H

#include "gdef.h"
#include "gpath.h"
#include "gexception.h"
#include <string>
#include <memory>
#include <iostream>
#include <set>
#include <vector>

namespace GPop
{
	class Store ;
	class StoreMessage ;
	class StoreUser ;
	class StoreList ;
}

//| \class GPop::Store
/// A message store. Unlike the SMTP message store the POP message
/// store allows content files to be in the envelope file's parent
/// directory.
///
class GPop::Store
{
public:
	G_EXCEPTION( InvalidDirectory , tx("invalid spool directory") ) ;

	Store( const G::Path & spool_dir , bool by_name , bool allow_delete ) ;
		///< Constructor. Throws InvalidDirectory.

	G::Path dir() const ;
		///< Returns the spool directory path.

	bool allowDelete() const ;
		///< Returns true if files can be deleted.

	bool byName() const ;
		///< Returns true if the spool directory is affected
		///< by the user name.

public:
	~Store() = default ;
	Store( const Store & ) = delete ;
	Store( Store && ) = delete ;
	Store & operator=( const Store & ) = delete ;
	Store & operator=( Store && ) = delete ;

private:
	static bool accessible( const G::Path & dir , bool ) ;

private:
	G::Path m_path ;
	bool m_by_name ;
	bool m_allow_delete ;
} ;

//| \class GPop::StoreMessage
/// A structure representing a pop message.
///
class GPop::StoreMessage
{
public:
	using Size = unsigned long ;
	StoreMessage( const std::string & name , Size size , bool in_parent ) ;
	G::Path epath( const G::Path & edir ) const ;
	G::Path cpath( const G::Path & edir , const G::Path & sdir ) const ;
	G::Path cpath( const G::Path & ) const ;
	std::string uidl() const ;
	static StoreMessage invalid() ;

public:
	std::string name ;
	Size size ;
	bool in_parent ;
	bool deleted {false} ;
} ;

//| \class GPop::StoreUser
/// Holds the list of messages available to a particular pop user.
///
class GPop::StoreUser
{
public:
	StoreUser( Store & , const std::string & user ) ;
		///< Constructor.

public:
	~StoreUser() = default ;
	StoreUser( const StoreUser & ) = delete ;
	StoreUser( StoreUser && ) = delete ;
	StoreUser & operator=( const StoreUser & ) = delete ;
	StoreUser & operator=( StoreUser && ) = delete ;

private:
	friend class GPop::StoreList ;
	Store & m_store ;
	std::string m_user ;
	G::Path m_edir ;
	G::Path m_sdir ;
	std::vector<StoreMessage> m_list ;
} ;

//| \class GPop::StoreList
/// Represents the protocol's view of the pop store having 1-based
/// message ids. Messages can be marked as deleted and then
/// actually deleted by commit().
/// \see RFC-1939
///
class GPop::StoreList
{
public:
	G_EXCEPTION( CannotDelete , tx("cannot delete message file") ) ;
	G_EXCEPTION( CannotRead , tx("cannot read message file") ) ;
	using Size = StoreMessage::Size ;
	using List = std::vector<StoreMessage> ;

	StoreList() ;
		///< Constructor for an empty list.

	StoreList( const StoreUser & , bool allow_delete ) ;
		///< Constructor.

	List::const_iterator cbegin() const ;
		///< Returns the begin iterator.

	List::const_iterator cend() const ;
		///< Returns the end iterator.

	List::value_type get( int id ) const ;
		///< Returns the item with index id-1.

	Size messageCount() const ;
		///< Returns the store's message count.

	Size totalByteCount() const ;
		///< Returns the store's total byte count.

	std::string uidl( int id ) const ;
		///< Returns a message's unique 1-based id.

	bool valid( int id ) const ;
		///< Validates a message id.

	Size byteCount( int id ) const ;
		///< Returns the message size.

	std::unique_ptr<std::istream> content( int id ) const ;
		///< Retrieves the message content.

	void remove( int ) ;
		///< Marks the message files for deletion.

	void rollback() ;
		///< Rolls back remove()als.

	void commit() ;
		///< Commits remove()als. Messages remain marked
		///< for deletion so another commit() will emit
		///< 'cannot delete' error messages.

public:
	~StoreList() = default ;
	StoreList( const StoreList & ) = delete ;
	StoreList( StoreList && ) = default ;
	StoreList & operator=( const StoreList & ) = delete ;
	StoreList & operator=( StoreList && ) = default ;

private:
	void deleteFile( const G::Path & , bool & ) const ;
	bool shared( const StoreMessage & ) const ;

private:
	bool m_allow_delete {false} ;
	G::Path m_edir ;
	G::Path m_sdir ;
	std::vector<StoreMessage> m_list ;
} ;

#endif
