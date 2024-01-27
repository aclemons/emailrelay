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
/// \file gprotocolmessage.h
///

#ifndef G_SMTP_PROTOCOL_MESSAGE_H
#define G_SMTP_PROTOCOL_MESSAGE_H

#include "gdef.h"
#include "gmessagestore.h"
#include "gnewmessage.h"
#include "gslot.h"
#include "gstringarray.h"
#include "gverifier.h"
#include "gexception.h"
#include <string>

namespace GSmtp
{
	class ProtocolMessage ;
}

//| \class GSmtp::ProtocolMessage
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
/// - addContent() [0..n]
/// - process() -> processedSignal() [async]
///
/// The process() method is asynchronous, but note that the
/// completion signal may be emitted before the initiating call
/// returns.
///
class GSmtp::ProtocolMessage
{
public:
	struct ProcessedInfo /// Parameters for GSmtp::ProtocolMessage::processedSignal()
	{
		bool success ;
		GStore::MessageId id ; // message id, not valid() with 'success' true if abandoned
		int response_code ; // response code (400..599) to send to remote client if not 'success', or zero to default
		std::string response ; // response to send to remote client (no tabs), empty if 'success'
		std::string reason ; // log string typically from filter output
	} ;
	using ProcessedSignal = G::Slot::Signal<const ProcessedInfo&> ;
	struct FromInfo /// Extra information from the SMTP MAIL-FROM command passed to GSmtp::ProtocolMessage::setFrom().
	{
		std::string auth ; // RFC-2554 MAIL-FROM with AUTH= ie. 'auth-in' (xtext or "<>")
		std::string body ; // RFC-1652 MAIL-FROM with BODY={7BIT|8BITMIME|BINARYMIME}
		bool smtputf8 {true} ; // RFC-6531 MAIL-FROM with SMTPUTF8
		bool utf8address {false} ; // GSmtp::ServerParser::MailboxStyle
	} ;
	struct ToInfo /// Extra information passed to GSmtp::ProtocolMessage::addTo().
	{
		explicit ToInfo( const VerifierStatus & ) ;
		VerifierStatus status ;
		bool utf8address ; // GSmtp::ServerParser::MailboxStyle
	} ;

	virtual ~ProtocolMessage() = default ;
		///< Destructor.

	virtual ProcessedSignal & processedSignal() = 0 ;
		///< Returns a signal which is raised once process() has completed.

	virtual void reset() = 0 ;
		///< Clears the message state, terminates any asynchronous message
		///< processing and resets the object as if just constructed.
		///< (In practice this is clear() plus the disconnection of any
		///< forwarding client).

	virtual void clear() = 0 ;
		///< Clears the message state and terminates any asynchronous
		///< message processing.

	virtual GStore::MessageId setFrom( const std::string & from_user , const FromInfo & ) = 0 ;
		///< Sets the message envelope 'from' address etc. Returns a unique
		///< message id.

	virtual bool addTo( const ToInfo & to ) = 0 ;
		///< Adds an envelope 'to'. See also GSmtp::Verifier::verify().
		///< Returns false if an invalid user.
		///<
		///< Precondition: setFrom() called since clear() or process().

	virtual void addReceived( const std::string & ) = 0 ;
		///< Adds a 'received' line to the start of the content.
		///< Precondition: at least one successful addTo() call

	virtual GStore::NewMessage::Status addContent( const char * , std::size_t ) = 0 ;
		///< Adds content. The text should normally end in CR-LF. Returns
		///< an error enum, but error processing can be deferred
		///< until a final addContent(0) or until process().
		///<
		///< Precondition: at least one successful addTo() call

	void addContentLine( const std::string & ) ;
		///< A convenience function that calls addContent() taking
		///< a string parameter and adding CR-LF.

	virtual std::size_t contentSize() const = 0 ;
		///< Returns the current content size. Returns the maximum
		///< std::size_t value on overflow.

	virtual std::string from() const = 0 ;
		///< Returns the setFrom() user string.

	virtual FromInfo fromInfo() const = 0 ;
		///< Returns the setFrom() extra info.

	virtual std::string bodyType() const = 0 ;
		///< Returns the setFrom() body type, fromInfo().body.

	virtual void process( const std::string & session_auth_id , const std::string & peer_socket_address ,
		const std::string & peer_certificate ) = 0 ;
			///< Starts asynchronous processing of the message. Once processing
			///< is complete the message state is cleared and the processedSignal()
			///< is raised. All errors are also signalled via the processedSignal().
			///< The processedSignal() may be emitted before process() returns.
			///<
			///< The session-auth-id parameter is used to propagate authentication
			///< information from the SMTP AUTH command into individual messages.
			///< It is the empty string for unauthenticated clients.
			///< See also GAuth::SaslServer::id().
} ;

#endif
