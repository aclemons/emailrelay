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
// gstoredmessage.h
//

#ifndef G_SMTP_STORED_MESSAGE_H
#define G_SMTP_STORED_MESSAGE_H

#include "gdef.h"
#include "gsmtp.h"
#include "gstrings.h"
#include "gpath.h"

namespace GSmtp
{
	class StoredMessage ;
}

// Class: GSmtp::StoredMessage
// Description: An abstract class for messages which have
// come from the store.
// See also: GSmtp::MessageStore, GSmtp::MessageStore::get()
//
class GSmtp::StoredMessage 
{
public:
	virtual std::string name() const = 0 ;
		// Returns some sort of unique identifier for the message.

	virtual const std::string & from() const = 0 ;
		// Returns the envelope 'from' field.

	virtual const G::Strings & to() const = 0 ;
		// Returns the envelope 'to' fields.

	virtual bool preprocess() = 0 ;
		// Does synchronous pre-processing. Returns false
		// on error.

	virtual std::auto_ptr<std::istream> extractContentStream() = 0 ;
		// Extracts the content stream.
		// Can only be called once.

	virtual void destroy() = 0 ;
		// Deletes the message within the store.

	virtual void fail( const std::string & reason ) = 0 ;
		// Marks the message as failed within the store.

	virtual bool eightBit() const = 0 ;
		// Returns true if the message content (header+body)
		// contains a character with the most significant
		// bit set.

	virtual std::string authentication() const = 0 ;
		// Returns the message authentication string.

	virtual size_t remoteRecipientCount() const = 0 ;
		// Returns the number of non-local recipients.

	virtual size_t errorCount() const = 0 ;
		// Returns the number of accumulated submission errors.

	virtual ~StoredMessage() ;
		// Destructor.

private:
	void operator=( const StoredMessage & ) ; // not implemented
} ;

#endif

