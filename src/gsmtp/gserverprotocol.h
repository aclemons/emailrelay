//
// Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gserverprotocol.h
//

#ifndef G_SMTP_SERVER_PROTOCOL_H
#define G_SMTP_SERVER_PROTOCOL_H

#include "gdef.h"
#include "gsmtp.h"
#include "gprotocolmessage.h"
#include "gaddress.h"
#include "gverifier.h"
#include "gsasl.h"
#include "gstatemachine.h"
#include <map>

namespace GSmtp
{
	class ServerProtocol ;
}

// Class: GSmtp::ServerProtocol
// Description: Implements the SMTP server-side protocol.
//
// Uses the ProtocolMessage class as its down-stream interface,
// used for assembling and processing the incoming email
// messages. 
//
// Uses the ServerProtocol::Sender as its "sideways"
// interface to talk back to the email-sending client.
// 
// A special feature of the implementation (which is
// important to the ServerPeer class) is that once
// Sender::protocolDone() has been called (from within
// ServerProtocol::apply()) no non-static method of 
// ServerProtocol is called and no non-static data-member 
// is accessed. This allows the ServerProtocol object to 
// be deleted from within Sender::protocolDone(), and therefore
// allows the ServerPeer implementation to contain an
// instance of ServerProtocol as a data member.
//
// See also: ProtocolMessage, RFC2821
//
class GSmtp::ServerProtocol 
{
public:
	class Sender // An interface used by ServerProtocol to send protocol replies.
	{
		public: virtual void protocolSend( const std::string & s ) = 0 ;
		public: virtual void protocolDone() = 0 ;
		public: virtual ~Sender() ;
		private: void operator=( const Sender & ) ; // not implemented
	} ;

	ServerProtocol( Sender & sender , Verifier & verifier , ProtocolMessage & pmessage ,
		const Secrets & secrets , const std::string & thishost , GNet::Address peer_address ) ;
			// Constructor. 
			//
			// The Verifier interface is used to verify recipient
			// addresses. See GSmtp::Verifier.
			//
			// The ProtocolMessage interface is used to assemble and
			// process an incoming message.
			//
			// The Sender interface is used to send protocol
			// replies back to the client.
			//
			// The 'thishost' string is used in HELO replies (etc).
			//
			// The peer address string is put into the "Received:" 
			// trace lines.

	void init( const std::string & ident ) ;
		// Starts the protocol. The 'ident' string is issued 
		// to the client.

	virtual ~ServerProtocol() ;
		// Destructor.

	bool apply( const std::string & line ) ;
		// Called on receipt of a string from the client.
		// The string is expected to be <CRLF> terminated.
		// Returns true if the protocol has completed
		// and Sender::protocolDone() has been called.

private:
	enum Event
	{
		eQuit ,
		eHelo ,
		eEhlo ,
		eRset ,
		eNoop ,
		eData ,
		eRcpt ,
		eMail ,
		ePrepared ,
		eVrfy ,
		eHelp ,
		eAuth ,
		eAuthData ,
		eUnknown
	} ;
	enum State
	{
		sStart ,
		sEnd ,
		sIdle ,
		sGotMail ,
		sPrepare ,
		sGotRcpt ,
		sData ,
		sProcessing ,
		sAuth ,
		s_Any ,
		s_Same
	} ;
	typedef G::StateMachine<ServerProtocol,State,Event> Fsm ;

private:
	ServerProtocol( const ServerProtocol & ) ; // not implemented
	void operator=( const ServerProtocol & ) ; // not implemented
	void send( std::string ) ;
	Event commandEvent( const std::string & ) const ;
	std::string commandWord( const std::string & line ) const ;
	std::string commandLine( const std::string & line ) const ;
	static std::string crlf() ;
	void processDone( bool , unsigned long , std::string ) ; // ProtocolMessage::doneSignal()
	void prepareDone( bool , bool , std::string ) ;
	bool isEndOfText( const std::string & ) const ;
	bool isEscaped( const std::string & ) const ;
	void doNoop( const std::string & , bool & ) ;
	void doQuit( const std::string & , bool & ) ;
	void doEhlo( const std::string & , bool & ) ;
	void doHelo( const std::string & , bool & ) ;
	void doAuth( const std::string & , bool & ) ;
	void doAuthData( const std::string & , bool & ) ;
	void doMailPrepare( const std::string & line , bool & ) ;
	void doMail( const std::string & line , bool & ) ;
	void doRcpt( const std::string & line , bool & ) ;
	void doUnknown( const std::string & line , bool & ) ;
	void doRset( const std::string & line , bool & ) ;
	void doData( const std::string & line , bool & ) ;
	void doVrfy( const std::string & line , bool & ) ;
	void doNoRecipients( const std::string & line , bool & ) ;
	void sendBadFrom( const std::string & line ) ;
	void sendChallenge( const std::string & line ) ;
	void sendBadTo( const std::string & line ) ;
	void sendOutOfSequence( const std::string & line ) ;
	void sendGreeting( const std::string & , const std::string & ) ;
	void sendClosing() ;
	void sendUnrecognised( const std::string & ) ;
	void sendHeloReply( const std::string & ) ;
	void sendEhloReply( const std::string & ) ;
	void sendRsetReply() ;
	void sendMailReply() ;
	void sendMailError( const std::string & , bool ) ;
	void sendRcptReply() ;
	void sendDataReply() ;
	void sendCompletionReply( bool ok , const std::string & ) ;
	void sendAuthRequired() ;
	void sendNoRecipients() ;
	void sendMissingParameter() ;
	void sendVerified( const std::string & ) ;
	void sendNotVerified( const std::string & ) ;
	void sendWillAccept( const std::string & ) ;
	void sendAuthDone( bool ok ) ;
	void sendOk() ;
	std::string parseFrom( const std::string & ) const ;
	std::string parseTo( const std::string & ) const ;
	std::string parseMailbox( const std::string & ) const ;
	std::string parsePeerName( const std::string & ) const ;
	std::string parse( const std::string & ) const ;
	std::string receivedLine() const ;
	Verifier::Status verify( const std::string & , const std::string & ) const ;

private:
	Sender & m_sender ;
	ProtocolMessage & m_pmessage ;
	Verifier & m_verifier ;
	Fsm m_fsm ;
	std::string m_thishost ;
	std::string m_peer_name ;
	GNet::Address m_peer_address ;
	bool m_authenticated ;
	SaslServer m_sasl ;
} ;

#endif
