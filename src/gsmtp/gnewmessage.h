//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gnewmessage.h
///

#ifndef G_SMTP_NEW_MESSAGE_H
#define G_SMTP_NEW_MESSAGE_H

#include "gdef.h"
#include "gsmtp.h"

/// \namespace GSmtp
namespace GSmtp
{
	class NewMessage ;
	class MessageStoreImp ;
}

/// \class GSmtp::NewMessage
/// An abstract class to allow the creation
/// of a new message in the message store.
/// \see GSmtp::MessageStore, GSmtp::MessageStore::newMessage()
///
class GSmtp::NewMessage 
{
public:
	virtual void addTo( const std::string & to , bool local ) = 0 ;
		///< Adds a 'to' address.

	virtual bool addText( const std::string & line ) = 0 ;
		///< Adds a line of content. Returns false on overflow.

	virtual std::string prepare( const std::string & auth_id , const std::string & peer_socket_address ,
		const std::string & peer_socket_name , const std::string & peer_certificate ) = 0 ;
			///< Prepares to store the message in the message store.
			///< Returns the location of the pre-commit()ed message.

	virtual void commit() = 0 ;
		///< Commits the prepare()d message to the store.

	virtual unsigned long id() const = 0 ;
		///< Returns the message's unique non-zero identifier.

	virtual ~NewMessage() ;
		///< Destructor. Rolls back any prepare()d storage
		///< if un-commit()ed.

private:
	void operator=( const NewMessage & ) ; // not implemented
} ;

#endif

