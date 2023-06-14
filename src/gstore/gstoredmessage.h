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
/// \file gstoredmessage.h
///

#ifndef G_SMTP_STORED_MESSAGE_H
#define G_SMTP_STORED_MESSAGE_H

#include "gdef.h"
#include "gstringarray.h"
#include "gmessagestore.h"
#include "genvelope.h"
#include "gpath.h"
#include <functional>
#include <iostream>
#include <fstream>

namespace GStore
{
	class StoredMessage ;
	class StoredMessageStub ;
}

//| \class GStore::StoredMessage
/// An abstract interface for messages which have come from the store.
/// \see GStore::MessageStore, GStore::MessageStore::get()
///
class GStore::StoredMessage
{
public:
	virtual MessageId id() const = 0 ;
		///< Returns the message identifier.

	virtual std::string location() const = 0 ;
		///< Returns the message location.

	virtual std::string from() const = 0 ;
		///< Returns the envelope 'from' field.

	virtual std::string to( std::size_t ) const = 0 ;
		///< Returns the requested envelope non-local recipient
		///< or the empty string if out of range.

	virtual std::size_t toCount() const = 0 ;
		///< Returns the number of non-local recipients.

	virtual std::size_t contentSize() const = 0 ;
		///< Returns the content size.

	virtual std::istream & contentStream() = 0 ;
		///< Returns a reference to the content stream.

	virtual void close() = 0 ;
		///< Releases the message to allow external editing.

	virtual std::string reopen() = 0 ;
		///< Reverses a close(), returning the empty string
		///< on success or an error reason.

	virtual void destroy() = 0 ;
		///< Deletes the message within the store.

	virtual void fail( const std::string & reason , int reason_code ) = 0 ;
		///< Marks the message as failed within the store.

	virtual MessageStore::BodyType bodyType() const = 0 ;
		///< Returns the message body type.

	virtual std::string authentication() const = 0 ;
		///< Returns the original session authentication id.

	virtual std::string fromAuthIn() const = 0 ;
		///< Returns the incoming "mail from" auth parameter,
		///< either empty, xtext-encoded or "<>".

	virtual std::string fromAuthOut() const = 0 ;
		///< Returns the outgoing "mail from" auth parameter,
		///< either empty, xtext-encoded or "<>".

	virtual std::string forwardTo() const = 0 ;
		///< Returns the routing override or the empty string.

	virtual std::string forwardToAddress() const = 0 ;
		///< Returns the forwardTo() address or the empty string.

	virtual bool utf8Mailboxes() const = 0 ;
		///< Returns true if the mail-from command should
		///< have SMTPUTF8 (RFC-6531).

	virtual void editRecipients( const G::StringArray & ) = 0 ;
		///< Updates the message's remote recipients, typically to
		///< the sub-set that have not received it successfully.

	virtual ~StoredMessage() = default ;
		///< Destructor.
} ;

//| \class GStore::StoredMessageStub
/// A StoredMessage class that does nothing and can be used as
/// a placeholder.
///
class GStore::StoredMessageStub : public StoredMessage
{
public:
	StoredMessageStub() ;
		///< Constructor.

	~StoredMessageStub() override ;
		///< Destructor.

private: // overrides
	MessageId id() const override ;
	std::string location() const override ;
	std::string from() const override ;
	std::string to( std::size_t ) const override ;
	std::size_t toCount() const override ;
	std::size_t contentSize() const override ;
	std::istream & contentStream() override ;
	void close() override ;
	std::string reopen() override ;
	void destroy() override ;
	void fail( const std::string & reason , int reason_code ) override ;
	MessageStore::BodyType bodyType() const override ;
	std::string authentication() const override ;
	std::string fromAuthIn() const override ;
	std::string fromAuthOut() const override ;
	std::string forwardTo() const override ;
	std::string forwardToAddress() const override ;
	bool utf8Mailboxes() const override ;
	void editRecipients( const G::StringArray & ) override ;

public:
	StoredMessageStub( const StoredMessageStub & ) = delete ;
	StoredMessageStub( StoredMessageStub && ) = delete ;
	StoredMessageStub & operator=( const StoredMessageStub & ) = delete ;
	StoredMessageStub & operator=( StoredMessageStub && ) = delete ;

private:
	G::StringArray m_to_list ;
	std::ifstream m_content_stream ;
} ;

inline GStore::MessageId GStore::StoredMessageStub::id() const { return MessageId::none() ; }
inline std::string GStore::StoredMessageStub::location() const { return {} ; }
inline std::string GStore::StoredMessageStub::from() const { return {} ; }
inline std::string GStore::StoredMessageStub::to( std::size_t ) const { return {} ; }
inline std::size_t GStore::StoredMessageStub::toCount() const { return 0U ; }
inline std::size_t GStore::StoredMessageStub::contentSize() const { return 0U ; }
inline std::istream & GStore::StoredMessageStub::contentStream() { return m_content_stream ; }
inline void GStore::StoredMessageStub::close() {}
inline std::string GStore::StoredMessageStub::reopen() { return {} ; }
inline void GStore::StoredMessageStub::destroy() {}
inline void GStore::StoredMessageStub::fail( const std::string & , int ) {}
inline GStore::MessageStore::BodyType GStore::StoredMessageStub::bodyType() const { return MessageStore::BodyType::Unknown ; }
inline std::string GStore::StoredMessageStub::authentication() const { return {} ; }
inline std::string GStore::StoredMessageStub::fromAuthIn() const { return {} ; }
inline std::string GStore::StoredMessageStub::fromAuthOut() const { return {} ; }
inline std::string GStore::StoredMessageStub::forwardTo() const { return {} ; }
inline std::string GStore::StoredMessageStub::forwardToAddress() const { return {} ; }
inline bool GStore::StoredMessageStub::utf8Mailboxes() const { return false ; }
inline void GStore::StoredMessageStub::editRecipients( const G::StringArray & ) {}

#endif
