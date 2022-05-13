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
/// \file gstoredmessage.h
///

#ifndef G_SMTP_STORED_MESSAGE_H
#define G_SMTP_STORED_MESSAGE_H

#include "gdef.h"
#include "gstringarray.h"
#include "gmessagestore.h"
#include "gpath.h"
#include <iostream>
#include <fstream>

namespace GSmtp
{
	class StoredMessage ;
	class StoredMessageStub ;
}

//| \class GSmtp::StoredMessage
/// An abstract interface for messages which have come from the store.
/// \see GSmtp::MessageStore, GSmtp::MessageStore::get()
///
class GSmtp::StoredMessage
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

	virtual void edit( const G::StringArray & new_to_list ) = 0 ;
		///< Edits the message by updating the list of
		///< non-local recipients to the given non-empty list.

	virtual void fail( const std::string & reason , int reason_code ) = 0 ;
		///< Marks the message as failed within the store.

	virtual void unfail() = 0 ;
		///< Marks the message as unfailed within the store.

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

	virtual bool utf8Mailboxes() const = 0 ;
		///< Returns true if the mail-from command should
		///< have SMTPUTF8 (RFC-6531).

	virtual ~StoredMessage() = default ;
		///< Destructor.
} ;

//| \class GSmtp::StoredMessageStub
/// A StoredMessage class that does nothing and can be used as
/// a placeholder.
///
class GSmtp::StoredMessageStub : public StoredMessage
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
	void edit( const G::StringArray & ) override ;
	void fail( const std::string & reason , int reason_code ) override ;
	void unfail() override ;
	MessageStore::BodyType bodyType() const override ;
	std::string authentication() const override ;
	std::string fromAuthIn() const override ;
	std::string fromAuthOut() const override ;
	bool utf8Mailboxes() const override ;

public:
	StoredMessageStub( const StoredMessageStub & ) = delete ;
	StoredMessageStub( StoredMessageStub && ) = delete ;
	void operator=( const StoredMessageStub & ) = delete ;
	void operator=( StoredMessageStub && ) = delete ;

private:
	G::StringArray m_to_list ;
	std::ifstream m_content_stream ;
} ;

inline GSmtp::MessageId GSmtp::StoredMessageStub::id() const { return MessageId::none() ; }
inline std::string GSmtp::StoredMessageStub::location() const { return {} ; }
inline std::string GSmtp::StoredMessageStub::from() const { return {} ; }
inline std::string GSmtp::StoredMessageStub::to( std::size_t ) const { return {} ; }
inline std::size_t GSmtp::StoredMessageStub::toCount() const { return 0U ; }
inline std::size_t GSmtp::StoredMessageStub::contentSize() const { return 0U ; }
inline std::istream & GSmtp::StoredMessageStub::contentStream() { return m_content_stream ; }
inline void GSmtp::StoredMessageStub::close() {}
inline std::string GSmtp::StoredMessageStub::reopen() { return {} ; }
inline void GSmtp::StoredMessageStub::destroy() {}
inline void GSmtp::StoredMessageStub::edit( const G::StringArray & ) {}
inline void GSmtp::StoredMessageStub::fail( const std::string & , int ) {}
inline void GSmtp::StoredMessageStub::unfail() {}
inline GSmtp::MessageStore::BodyType GSmtp::StoredMessageStub::bodyType() const { return MessageStore::BodyType::Unknown ; }
inline std::string GSmtp::StoredMessageStub::authentication() const { return {} ; }
inline std::string GSmtp::StoredMessageStub::fromAuthIn() const { return {} ; }
inline std::string GSmtp::StoredMessageStub::fromAuthOut() const { return {} ; }
inline bool GSmtp::StoredMessageStub::utf8Mailboxes() const { return false ; }

#endif
