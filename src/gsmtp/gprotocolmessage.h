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
/// \file gprotocolmessage.h
///

#ifndef G_SMTP_PROTOCOL_MESSAGE__H
#define G_SMTP_PROTOCOL_MESSAGE__H

#include "gdef.h"
#include "gslot.h"
#include "gstrings.h"
#include "gverifier.h"
#include "gexception.h"
#include <string>

namespace GSmtp
{
	class ProtocolMessage ;
}

/// \class GSmtp::ProtocolMessage
/// An interface used by the ServerProtocol class to assemble and
/// process an incoming message. It implements the three 'buffers'
/// mentioned in RFC-2821 (esp. section 4.1.1).
///
/// This interface serves to decouple the protocol class from
/// the downstream message processing -- hence the name. Derived
/// classes implement different types of downstream processing.
/// For store-and-forward behaviour the ProtocolMessageStore
/// class uses GSmtp::MessageStore to store messages; for proxying
/// behaviour the ProtocolMessageForward class uses GSmtp::Client
/// to do immediate forwarding.
///
/// The interface is used by the protocol class in the following
/// sequence:
/// - clear()
/// - setFrom()
/// - addTo() [1..n]
/// - addReceived() [0..n]
/// - addText() [0..n]
/// - process() -> doneSignal() [async]
///
/// The process() method is asynchronous, but note that the
/// completion signal may be emitted before the initiating call
/// returns.
///
class GSmtp::ProtocolMessage
{
public:
	virtual ~ProtocolMessage() ;
		///< Destructor.

	virtual G::Slot::Signal4<bool,unsigned long,std::string,std::string> & doneSignal() = 0 ;
		///< Returns a signal which is raised once process() has
		///< completed.
		///<
		///< The signal parameters are 'success', 'id', 'short-response' and
		///< 'full-reason'. As a special case, if success is true and id
		///< is zero then the message processing was either abandoned
		///< or it only had local-mailbox recipients.

	virtual void reset() = 0 ;
		///< Resets the object state as if just constructed.

	virtual void clear() = 0 ;
		///< Clears the message state and terminates any asynchronous
		///< message processing.

	virtual bool setFrom( const std::string & from_user , const std::string & from_auth ) = 0 ;
		///< Sets the message envelope 'from'. Returns false if an
		///< invalid user.

	virtual bool addTo( const std::string & to_user , VerifierStatus to_status ) = 0 ;
		///< Adds an envelope 'to'. The 'to_status' parameter comes
		///< from GSmtp::Verifier.verify(). Returns false if an
		///< invalid user.
		///<
		///< Precondition: setFrom() called since clear() or process().

	virtual void addReceived( const std::string & ) = 0 ;
		///< Adds a 'received' line to the start of the content.
		///< Precondition: at least one successful addTo() call

	virtual bool addText( const char * , size_t ) = 0 ;
		///< Adds text. The text should normally end in CR-LF. Returns
		///< false on error, typically because a size limit is reached.
		///<
		///< Precondition: at least one successful addTo() call

	bool addTextLine( const std::string & ) ;
		///< A convenience function that calls addText() taking
		///< a string parameter and adding CR-LF.

	virtual std::string from() const = 0 ;
		///< Returns the setFrom() string.

	virtual void process( const std::string & session_auth_id , const std::string & peer_socket_address ,
		const std::string & peer_certificate ) = 0 ;
			///< Starts asynchronous processing of the message. Once processing
			///< is complete the message state is cleared and the doneSignal()
			///< is raised. The signal may be raised before process() returns.
			///<
			///< The session-auth-id parameter is used to propagate authentication
			///< information from the SMTP AUTH command into individual messages.
			///< It is the empty string for unauthenticated clients.
			///< See also GAuth::SaslServer::id().
} ;

#endif
