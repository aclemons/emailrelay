//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gmessagestore.h
///

#ifndef G_SMTP_MESSAGE_STORE__H
#define G_SMTP_MESSAGE_STORE__H

#include "gdef.h"
#include "gnewmessage.h"
#include "gstoredmessage.h"
#include "gexception.h"
#include "gslot.h"
#include "gstrings.h"
#include "gpath.h"

namespace GSmtp
{
	class MessageStore ;
}

/// \class GSmtp::MessageStore
/// A class which allows SMTP messages to be stored and retrieved.
///
/// \see GSmtp::NewMessage, GSmtp::StoredMessage, GSmtp::ProtocolMessage
///
class GSmtp::MessageStore
{
public:
	class IteratorImp /// A base class for MessageStore::Iterator implementations.
	{
	public:
		IteratorImp() ;
		virtual unique_ptr<StoredMessage> next() = 0 ;
		virtual ~IteratorImp() ;

	public:
		unsigned long m_ref_count ;

	private:
		IteratorImp( const IteratorImp & ) g__eq_delete ;
		void operator=( const IteratorImp & ) g__eq_delete ;
	} ;

	class Iterator /// An iterator class for GSmtp::MessageStore.
	{
	public:
		Iterator() ;
		explicit Iterator( IteratorImp * ) ;
		~Iterator() ;
		Iterator( const Iterator & ) ;
		Iterator & operator=( const Iterator & ) ;
		unique_ptr<StoredMessage> next() ;
		void last() ;

	private:
		IteratorImp * m_imp ;
	} ;

	static G::Path defaultDirectory() ;
		///< Returns a default spool directory, such as "/var/spool/emailrelay".
		///< (Typically with an os-specific implementation.)

	virtual ~MessageStore() ;
		///< Destructor.

	virtual unique_ptr<NewMessage> newMessage( const std::string & from ,
		const std::string & from_auth_in , const std::string & from_auth_out ) = 0 ;
			///< Creates a new message.

	virtual bool empty() const = 0 ;
		///< Returns true if the message store is empty.

	virtual unique_ptr<StoredMessage> get( unsigned long id ) = 0 ;
		///< Pulls the specified message out of the store.
		///< Throws execptions on error.
		///<
		///< See also NewMessage::id().
		///<
		///< As a side effect some stored messages may be marked as bad,
		///< or deleted (if they have no recipients).

	virtual Iterator iterator( bool lock ) = 0 ;
		///< Returns an iterator for stored messages. (Note that copies
		///< of iterators share state. For independent iterators call
		///< iterator() for each.)
		///<
		///< If 'lock' is true then stored messages returned by the
		///< iterator are locked. Normally they are then processed
		///< (using StoredMessage::extractContentStream()) and then
		///< deleted (by StoredMessage::destroy()).
		///<
		///< As a side effect of iteration when 'lock' is true some
		///< stored messages may be marked as bad, or get deleted
		///< (if they have no recipients).

	virtual Iterator failures() = 0 ;
		///< Returns an iterator for failed messages.

	virtual void unfailAll() = 0 ;
		///< Causes messages marked as failed to be unmarked.

	virtual void rescan() = 0 ;
		///< Requests that a messageStoreRescanSignal() is emitted.

	virtual void updated() = 0 ;
		///< Called by associated classes to indicate that the
		///< store has changed. Implementations must cause the
		///< messageStoreUpdateSignal() signal to be emitted.

	virtual G::Slot::Signal0 & messageStoreUpdateSignal() = 0 ;
		///< Provides a signal which is when something might
		///< have changed in the store.

	virtual G::Slot::Signal0 & messageStoreRescanSignal() = 0 ;
		///< Provides a signal which is emitted when rescan()
		///< is called.
} ;

#endif
