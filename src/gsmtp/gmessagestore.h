//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#ifndef G_SMTP_MESSAGE_STORE_H
#define G_SMTP_MESSAGE_STORE_H

#include "gdef.h"
#include "gexception.h"
#include "gslot.h"
#include "gstringarray.h"
#include "gpath.h"
#include <memory>

namespace GSmtp
{
	class NewMessage ;
	class StoredMessage ;
	class MessageId ;
	class MessageStore ;
}

//| \class GSmtp::MessageId
/// A somewhat opaque identifer for a MessageStore message.
///
class GSmtp::MessageId
{
public:
	explicit MessageId( const std::string & ) ;
		///< Constructor.

	static MessageId none() ;
		///< Returns an in-valid() id.

	bool valid() const ;
		///< Returns true if valid.

	std::string str() const ;
		///< Returns the id string.

private:
	MessageId() = default ;
	explicit MessageId( int ) = delete ;

private:
	std::string m_s ;
} ;

//| \class GSmtp::MessageStore
/// A class which allows SMTP messages to be stored and retrieved.
///
/// \see GSmtp::NewMessage, GSmtp::StoredMessage, GSmtp::ProtocolMessage
///
class GSmtp::MessageStore
{
public:
	struct SmtpInfo /// Information on the SMTP options used when submitted.
	{
		std::string auth ; // AUTH=
		std::string body ; // BODY=
	} ;
	enum class BodyType
	{
		Unknown = -1 ,
		SevenBit ,
		EightBitMime , // RFC-1652
		BinaryMime // RFC-3030
	} ;
	struct Iterator /// A base class for GSmtp::MessageStore iterators.
	{
		virtual std::unique_ptr<StoredMessage> next() = 0 ;
			///< Returns the next stored message or a null pointer.

		virtual ~Iterator() = default ;
			///< Destructor.
	} ;

	static G::Path defaultDirectory() ;
		///< Returns a default spool directory, such as "/var/spool/emailrelay".
		///< (Typically with an os-specific implementation.)

	virtual ~MessageStore() = default ;
		///< Destructor.

	virtual std::unique_ptr<NewMessage> newMessage( const std::string & from ,
		const SmtpInfo & smtp_info , const std::string & from_auth_out ) = 0 ;
			///< Creates a new message.

	virtual bool empty() const = 0 ;
		///< Returns true if the message store is empty.

	virtual std::string location( const MessageId & ) const = 0 ;
		///< Returns the location of the given message.

	virtual std::unique_ptr<StoredMessage> get( const MessageId & id ) = 0 ;
		///< Pulls the specified message out of the store. Throws
		///< execptions on error.
		///<
		///< As a side effect some stored messages may be marked as bad,
		///< or deleted (if they have no recipients).

	virtual std::shared_ptr<Iterator> iterator( bool lock ) = 0 ;
		///< Returns an iterator for stored messages.
		///<
		///< If 'lock' is true then stored messages returned by the
		///< iterator are locked. They can then be deleted by
		///< StoredMessage::destroy() once they have been fully
		///< processed.
		///<
		///< If 'lock' is true invalid messages having no receipients
		///< are not returned by the iterator.
		///<
		///< If 'lock' is true then as a side effect of iteration some
		///< stored messages may be marked as bad, or get deleted
		///< if they have no recipients.

	virtual std::shared_ptr<Iterator> failures() = 0 ;
		///< Returns an iterator for failed messages.

	virtual void unfailAll() = 0 ;
		///< Causes messages marked as failed to be unmarked.

	virtual void rescan() = 0 ;
		///< Requests that a messageStoreRescanSignal() is emitted.

	virtual void updated() = 0 ;
		///< Called by associated classes to indicate that the
		///< store has changed. Implementations must cause the
		///< messageStoreUpdateSignal() signal to be emitted.

	virtual G::Slot::Signal<> & messageStoreUpdateSignal() = 0 ;
		///< Provides a signal which is when something might
		///< have changed in the store.

	virtual G::Slot::Signal<> & messageStoreRescanSignal() = 0 ;
		///< Provides a signal which is emitted when rescan()
		///< is called.
} ;

namespace GSmtp
{
	std::unique_ptr<StoredMessage> operator++( std::shared_ptr<MessageStore::Iterator> & iter ) ;
}

inline GSmtp::MessageId::MessageId( const std::string & s ) : m_s(s) {}
inline GSmtp::MessageId GSmtp::MessageId::none() { return MessageId() ; }
inline std::string GSmtp::MessageId::str() const { return m_s ; }
inline bool GSmtp::MessageId::valid() const { return !m_s.empty() ; }

#endif
