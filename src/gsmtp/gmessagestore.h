//
// Copyright (C) 2001-2004 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gmessagestore.h
//

#ifndef G_SMTP_MESSAGE_STORE_H
#define G_SMTP_MESSAGE_STORE_H

#include "gdef.h"
#include "gsmtp.h"
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

// Class: GSmtp::MessageStore
// Description: A class which allows SMTP messages 
// (envelope+content) to be stored and retrieved.
//
// See also: GSmtp::NewMessage, GSmtp::StoredMessage, GSmtp::ProtocolMessage
//
class GSmtp::MessageStore 
{
public:
	G_EXCEPTION( WriteError , "error writing file" ) ;
	G_EXCEPTION( StorageError , "error storing message" ) ;
	G_EXCEPTION( FormatError , "format error" ) ;
	class IteratorImp // A base class for MessageStore::Iterator implementations.
	{
		public: unsigned long m_ref_count ;
		public: virtual std::auto_ptr<GSmtp::StoredMessage> next() = 0 ;
		public: IteratorImp() ;
		public: virtual ~IteratorImp() ;
		private: IteratorImp( const IteratorImp & ) ;
		private: void operator=( const IteratorImp & ) ;
	} ;
	class Iterator // An iterator class for GSmtp::MessageStore.
	{
		public: std::auto_ptr<StoredMessage> next() ;
		private: IteratorImp * m_imp ;
		public: Iterator() ;
		public: explicit Iterator( IteratorImp * ) ;
		public: ~Iterator() ;
		public: Iterator( const Iterator & ) ;
		public: Iterator & operator=( const Iterator & ) ;
	} ;

	static G::Path defaultDirectory() ;
		// Returns a default spool directory, such as
		// "/usr/local/var/spool/emailrelay". (Typically
		// has an os-specific implementation.)

	virtual ~MessageStore() ;
		// Destructor.

	virtual std::auto_ptr<NewMessage> newMessage( const std::string & from ) = 0 ;
		// Creates a new message.

	virtual bool empty() const = 0 ;
		// Returns true if the message store is empty.

	virtual std::auto_ptr<StoredMessage> get( unsigned long id ) = 0 ;
		// Pulls a message out of the store.
		// Throws execptions on error.
		//
		// See also NewMessage::id().
		//
		// As a side effect some stored messages may be
		// marked as bad, or deleted (if they
		// have no recipients).

	virtual Iterator iterator( bool lock ) = 0 ;
		// Returns an iterator for stored messages. (Note that copies of
		// iterators share state. For independent iterators
		// call iterator() for each.)
		//
		// If 'lock' is true then stored messages returned 
		// by the iterator are locked. Normally they are 
		// then processed (using StoredMessage::extractContentStream()) 
		// and then deleted (by StoredMessage::destroy()).
		//
		// As a side effect of iteration when 'lock' is true,
		// then some stored messages may be marked as bad, 
		// or deleted (if they have no recipients).

	virtual void repoll() = 0 ;
		// Ensures that the next updated() signal() has
		// its parameter set to true.

	virtual void updated() = 0 ;
		// Called by associated classes to indicate that the
		// store has changed. Results in the signal() being
		// emited.

	virtual G::Signal1<bool> & signal() = 0 ;
		// Provides a signal which is activated when something might 
		// have changed in the store. The boolean parameter is used 
		// to indicate that repoll()ing is requested.

private:
	void operator=( const MessageStore & ) ; // not implemented
} ;

#endif

