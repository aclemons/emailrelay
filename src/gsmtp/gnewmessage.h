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
// gnewmessage.h
//

#ifndef G_SMTP_NEW_MESSAGE_H
#define G_SMTP_NEW_MESSAGE_H

#include "gdef.h"
#include "gsmtp.h"

namespace GSmtp
{
	class NewMessage ;
	class MessageStoreImp ;
}

// Class: GSmtp::NewMessage
// Description: An abstract class to allow the creation
// of a new message in the message store.
// See also: MessageStore, MessageStore::newMessage()
//
class GSmtp::NewMessage 
{
public:
	virtual void addTo( const std::string & to , bool local ) = 0 ;
		// Adds a 'to' address.

	virtual void addText( const std::string & line ) = 0 ;
		// Adds a line of content.

	virtual bool store( const std::string & auth_id , const std::string & client_ip ) = 0 ;
		// Stores the message in the message store.
		// Returns true if storage was deliberately
		// cancelled.

	virtual unsigned long id() const = 0 ;
		// Returns the message's unique non-zero identifier.

	virtual ~NewMessage() ;
		// Destructor.

private:
	void operator=( const NewMessage & ) ; // not implemented
} ;

#endif

