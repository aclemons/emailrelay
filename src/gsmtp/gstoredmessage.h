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
/// \file gstoredmessage.h
///

#ifndef G_SMTP_STORED_MESSAGE__H
#define G_SMTP_STORED_MESSAGE__H

#include "gdef.h"
#include "gstrings.h"
#include "gpath.h"
#include <iostream>
#include <fstream>

namespace GSmtp
{
	class StoredMessage ;
	class StoredMessageStub ;
}

/// \class GSmtp::StoredMessage
/// An abstract interface for messages which have come from the store.
/// \see GSmtp::MessageStore, GSmtp::MessageStore::get()
///
class GSmtp::StoredMessage
{
public:
	virtual std::string name() const = 0 ;
		///< Returns a readble identifier for internal logging.

	virtual std::string location() const = 0 ;
		///< Returns a unique identifier for the message.

	virtual std::string from() const = 0 ;
		///< Returns the envelope 'from' field.

	virtual std::string to( size_t ) const = 0 ;
		///< Returns the requested envelope non-local recipient
		///< or the empty string if out of range.

	virtual size_t toCount() const = 0 ;
		///< Returns the number of non-local recipients.

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

	virtual void unfail() = 0 ;
		///< Marks the message as unfailed within the store.

	virtual int eightBit() const = 0 ;
		///< Returns 1 if the message content (header+body)
		///< contains a character with the most significant
		///< bit set, or 0 if no such characters, or -1
		///< if unknown.

	virtual std::string authentication() const = 0 ;
		///< Returns the original session authentication id.

	virtual std::string fromAuthIn() const = 0 ;
		///< Returns the incoming "mail from" auth parameter,
		///< either empty, xtext-encoded or "<>".

	virtual std::string fromAuthOut() const = 0 ;
		///< Returns the outgoing "mail from" auth parameter,
		///< either empty, xtext-encoded or "<>".

	virtual size_t errorCount() const = 0 ;
		///< Returns the number of accumulated processing errors.

	virtual ~StoredMessage() ;
		///< Destructor.
} ;

/// \class GSmtp::StoredMessageStub
/// A StoredMessage class that does nothing and can be used as
/// a placeholder.
///
class GSmtp::StoredMessageStub : public StoredMessage
{
public:
	StoredMessageStub() ;
		///< Constructor.

	virtual ~StoredMessageStub() ;
		///< Destructor.

private: // overrides
	virtual std::string name() const override ;
	virtual std::string location() const override ;
	virtual std::string from() const override ;
	virtual std::string to( size_t ) const override ;
	virtual size_t toCount() const override ;
	virtual std::istream & contentStream() override ;
	virtual void close() override ;
	virtual std::string reopen() override ;
	virtual void destroy() override ;
	virtual void fail( const std::string & reason , int reason_code ) override ;
	virtual void unfail() override ;
	virtual int eightBit() const override ;
	virtual std::string authentication() const override ;
	virtual std::string fromAuthIn() const override ;
	virtual std::string fromAuthOut() const override ;
	virtual size_t errorCount() const override ;

private:
	StoredMessageStub( const StoredMessageStub & ) g__eq_delete ;
	void operator=( const StoredMessageStub & ) g__eq_delete ;

private:
	G::StringArray m_to_list ;
	std::ifstream m_content_stream ;
} ;

inline std::string GSmtp::StoredMessageStub::name() const { return std::string() ; }
inline std::string GSmtp::StoredMessageStub::location() const { return std::string() ; }
inline std::string GSmtp::StoredMessageStub::from() const { return std::string() ; }
inline std::string GSmtp::StoredMessageStub::to( size_t ) const { return std::string() ; }
inline size_t GSmtp::StoredMessageStub::toCount() const { return 0U ; }
inline std::istream & GSmtp::StoredMessageStub::contentStream() { return m_content_stream ; }
inline void GSmtp::StoredMessageStub::close() {}
inline std::string GSmtp::StoredMessageStub::reopen() { return std::string() ; }
inline void GSmtp::StoredMessageStub::destroy() {}
inline void GSmtp::StoredMessageStub::fail( const std::string & , int ) {}
inline void GSmtp::StoredMessageStub::unfail() {}
inline int GSmtp::StoredMessageStub::eightBit() const { return false ; }
inline std::string GSmtp::StoredMessageStub::authentication() const { return std::string() ; }
inline std::string GSmtp::StoredMessageStub::fromAuthIn() const { return std::string() ; }
inline std::string GSmtp::StoredMessageStub::fromAuthOut() const { return std::string() ; }
inline size_t GSmtp::StoredMessageStub::errorCount() const { return 0U ; }

#endif
