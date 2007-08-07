//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gserverprotocol.h
///

#ifndef G_SMTP_SERVER_PROTOCOL_H
#define G_SMTP_SERVER_PROTOCOL_H

#include "gdef.h"
#include "gsmtp.h"
#include "gprotocolmessage.h"
#include "gaddress.h"
#include "gverifier.h"
#include "gsasl.h"
#include "gsecrets.h"
#include "gstatemachine.h"
#include "gtimer.h"
#include "gexception.h"
#include <map>
#include <utility>

/// \namespace GSmtp
namespace GSmtp
{
	class ServerProtocol ;
	class ServerProtocolText ;
}

/// \class GSmtp::ServerProtocol
/// Implements the SMTP server-side protocol.
///
/// Uses the ProtocolMessage class as its down-stream interface,
/// used for assembling and processing the incoming email
/// messages. 
///
/// Uses the ServerProtocol::Sender as its "sideways"
/// interface to talk back to the email-sending client.
/// 
/// \see GSmtp::ProtocolMessage, RFC2821
///
class GSmtp::ServerProtocol : private GNet::AbstractTimer 
{
public:
	G_EXCEPTION( ProtocolDone , "smtp protocol done" ) ;
	/// An interface used by ServerProtocol to send protocol replies.
	class Sender 
	{
		public: virtual void protocolSend( const std::string & s ) = 0 ;
		public: virtual ~Sender() ;
		private: void operator=( const Sender & ) ; // not implemented
	} ;
	/// An interface used by ServerProtocol to provide response text strings.
	class Text 
	{
		public: virtual std::string greeting() const = 0 ;
		public: virtual std::string hello( const std::string & peer_name ) const = 0 ;
		public: virtual std::string received( const std::string & peer_name ) const = 0 ;
		public: virtual ~Text() ;
		private: void operator=( const Text & ) ; // not implemented
	} ;
	/// A structure containing configuration parameters for ServerProtocol.
	struct Config 
	{
		bool with_vrfy ;
		unsigned int preprocessor_timeout ;
		Config( bool , unsigned int ) ;
	} ;

	ServerProtocol( Sender & sender , Verifier & verifier , ProtocolMessage & pmessage ,
		const Secrets & secrets , Text & text , GNet::Address peer_address , Config config ) ;
			///< Constructor. 
			///<
			///< The Verifier interface is used to verify recipient
			///< addresses. See GSmtp::Verifier.
			///<
			///< The ProtocolMessage interface is used to assemble and
			///< process an incoming message.
			///<
			///< The Sender interface is used to send protocol
			///< replies back to the client.
			///<
			///< The Text interface is used to get informational text
			///< for returning to the client.
			///<
			///< All references are kept.

	void init() ;
		///< Starts the protocol. Use only once after construction.

	virtual ~ServerProtocol() ;
		///< Destructor.

	void apply( const std::string & line ) ;
		///< Called on receipt of a string from the client.
		///< The string is expected to be CR-LF terminated.
		///< Throws ProtocolDone at the end of the protocol.

private:
	enum Event
	{
		eQuit ,
		eHelo ,
		eEhlo ,
		eRset ,
		eNoop ,
		eExpn ,
		eData ,
		eRcpt ,
		eMail ,
		eVrfy ,
		eHelp ,
		eAuth ,
		eAuthData ,
		eContent ,
		eEot ,
		eUnknown
	} ;
	enum State
	{
		sStart ,
		sEnd ,
		sIdle ,
		sGotMail ,
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
	virtual void onTimeout() ; // from AbstractTimer
	virtual void onTimeoutException( std::exception & ) ; // from AbstractTimer
	void send( std::string ) ;
	Event commandEvent( const std::string & ) const ;
	std::string commandWord( const std::string & line ) const ;
	std::string commandLine( const std::string & line ) const ;
	static std::string crlf() ;
	void reset() ;
	void badClientEvent() ;
	void processDone( bool , unsigned long , std::string ) ; // ProtocolMessage::doneSignal()
	void prepareDone( bool , bool , std::string ) ;
	bool isEndOfText( const std::string & ) const ;
	bool isEscaped( const std::string & ) const ;
	void doNoop( const std::string & , bool & ) ;
	void doHelp( const std::string & line , bool & ) ;
	void doExpn( const std::string & line , bool & ) ;
	void doQuit( const std::string & , bool & ) ;
	void doEhlo( const std::string & , bool & ) ;
	void doHelo( const std::string & , bool & ) ;
	void doAuth( const std::string & , bool & ) ;
	void doAuthData( const std::string & , bool & ) ;
	void doMail( const std::string & , bool & ) ;
	void doRcpt( const std::string & , bool & ) ;
	void doUnknown( const std::string & , bool & ) ;
	void doRset( const std::string & , bool & ) ;
	void doData( const std::string & , bool & ) ;
	void doContent( const std::string & , bool & ) ;
	void doEot( const std::string & , bool & ) ;
	void doVrfy( const std::string & , bool & ) ;
	void doNoRecipients( const std::string & , bool & ) ;
	void sendBadFrom( std::string ) ;
	void sendChallenge( const std::string & ) ;
	void sendBadTo( const std::string & , bool ) ;
	void sendOutOfSequence( const std::string & ) ;
	void sendGreeting( const std::string & ) ;
	void sendClosing() ;
	void sendUnrecognised( const std::string & ) ;
	void sendNotImplemented() ;
	void sendHeloReply() ;
	void sendEhloReply() ;
	void sendRsetReply() ;
	void sendMailReply() ;
	void sendRcptReply() ;
	void sendDataReply() ;
	void sendCompletionReply( bool ok , const std::string & ) ;
	void sendAuthRequired() ;
	void sendNoRecipients() ;
	void sendMissingParameter() ;
	void sendVerified( const std::string & ) ;
	void sendNotVerified( const std::string & , bool ) ;
	void sendWillAccept( const std::string & ) ;
	void sendAuthDone( bool ok ) ;
	void sendOk() ;
	std::pair<std::string,std::string> parse( const std::string & ) const ;
	std::pair<std::string,std::string> parseFrom( const std::string & ) const ;
	std::pair<std::string,std::string> parseTo( const std::string & ) const ;
	std::string parseMailbox( const std::string & ) const ;
	std::string parsePeerName( const std::string & ) const ;
	Verifier::Status verify( const std::string & , const std::string & ) const ;

private:
	Sender & m_sender ;
	Verifier & m_verifier ;
	ProtocolMessage & m_pmessage ;
	Text & m_text ;
	GNet::Address m_peer_address ;
	Fsm m_fsm ;
	std::string m_peer_name ;
	bool m_authenticated ;
	SaslServer m_sasl ;
	bool m_with_vrfy ;
	std::string m_buffer ;
	unsigned int m_preprocessor_timeout ;
	unsigned int m_bad_client_count ;
	unsigned int m_bad_client_limit ;
} ;

/// \class GSmtp::ServerProtocolText
/// A default implementation for the 
/// ServerProtocol::Text interface.
///
class GSmtp::ServerProtocolText : public GSmtp::ServerProtocol::Text 
{
public:
	ServerProtocolText( const std::string & ident , const std::string & thishost ,
		const GNet::Address & peer_address ) ;
			///< Constructor.

	virtual std::string greeting() const ;
		///< From ServerProtocol::Text.

	virtual std::string hello( const std::string & peer_name_from_helo ) const ;
		///< From ServerProtocol::Text.

	virtual std::string received( const std::string & peer_name_from_helo ) const ;
		///< From ServerProtocol::Text.

	static std::string receivedLine( const std::string & peer_name_from_helo , const std::string & peer_address , 
		const std::string & thishost ) ;
			///< Returns a standard "Received:" line.

private:
	std::string m_ident ;
	std::string m_thishost ;
	GNet::Address m_peer_address ;
} ;

#endif
