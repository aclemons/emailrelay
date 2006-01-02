//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gprotocolmessage.h
//

#ifndef G_SMTP_PROTOCOL_MESSAGE_H
#define G_SMTP_PROTOCOL_MESSAGE_H

#include "gdef.h"
#include "gsmtp.h"
#include "gslot.h"
#include "gstrings.h"
#include "gverifier.h"
#include "gexception.h"
#include <string>

namespace GSmtp
{
	class ProtocolMessage ;
}

// Class: GSmtp::ProtocolMessage
// Description: An interface used by the ServerProtocol class
// to assemble and process an incoming message. It implements 
// the three 'buffers' mentioned in RFC2821 (esp. section 4.1.1).
//
// This interface serves to decouple the protocol class from
// the downstream message processing -- hence the name. Derived
// classes implement different types of downstream processing. 
// For store-and-forward behaviour the ProtocolMessageStore
// class uses GSmtp::MessageStore to store messages; for proxying 
// behaviour the ProtocolMessageForward class uses GSmtp::Client 
// to do immediate forwarding.
//
// The interface is used by the protocol class in the following 
// sequence:
// - clear()
// - setFrom()
// - prepare() -> preparedSignal() [async]
// - addTo() [1..n]
// - addReceived() [0..n]
// - addText() [0..n]
// - process() -> doneSignal() [async]
//
// The prepare() and process() methods are asynchronous, but
// note that the completion signals may be emited before the
// initiating call returns.
//
class GSmtp::ProtocolMessage 
{
public:
	G_EXCEPTION( ProcessingError , "error storing message" ) ;

	virtual ~ProtocolMessage() ;
		// Destructor.

	virtual G::Signal3<bool,unsigned long,std::string> & doneSignal() = 0 ;
		// Returns a signal which is raised once process() has
		// completed.
		//
		// The signal parameters are 'success', 'id' and 'reason'.
		//
		// As a special case, if success is true and id is zero then 
		// the message processing was cancelled.

	virtual G::Signal3<bool,bool,std::string> & preparedSignal() = 0 ;
		// Returns a signal which is raised once prepare() has
		// completed.
		//
		// The signal parameters are 'success', 'temporary' and 
		// 'reason'.

	virtual void clear() = 0 ;
		// Clears the message state and terminates
		// any asynchronous message processing.

	virtual bool setFrom( const std::string & from_user ) = 0 ;
		// Sets the message envelope 'from'.
		// Returns false if an invalid user.

	virtual bool prepare() = 0 ;
		// Called to start any asynchronous preparation which is
		// required after setFrom(). Returns true if there
		// is something to do (in which case preparedSignal()
		// must be fired later), or false if there is nothing
		// to do.

	virtual bool addTo( const std::string & to_user , Verifier::Status to_status ) = 0 ;
		// Adds an envelope 'to'.
		//
		// The 'to_status' parameter comes from
		// GSmtp::Verifier.verify().
		//
		// Returns false if an invalid user.
		// Precondition: setFrom() called
		// since clear() or process().

	virtual void addReceived( const std::string & ) = 0 ;
		// Adds a 'received' line to the
		// start of the content.
		// Precondition: at least one 
		// successful addTo() call

	virtual void addText( const std::string & ) = 0 ;
		// Adds text.
		// Precondition: at least one 
		// successful addTo() call

	virtual std::string from() const = 0 ;
		// Returns the setFrom() string.

	virtual void process( const std::string & authenticated_client_id , 
		const std::string & peer_ip_address ) = 0 ;
			// Starts asynchronous processing of the 
			// message. Once processing is complete the
			// message state is cleared and the doneSignal()
			// is raised. The signal may be raised before 
			// process() returns.
			//
			// The client-id parameter is used to propogate
			// authentication information from the SMTP
			// AUTH command into individual messages.
			// It is the empty string for unauthenticated
			// clients. See also GSmtp::Sasl::id().

private:
	void operator=( const ProtocolMessage & ) ; // not implemented
} ;

#endif

