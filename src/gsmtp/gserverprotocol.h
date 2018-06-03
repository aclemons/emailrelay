//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "geventhandler.h"
#include "gaddress.h"
#include "gverifier.h"
#include "gverifierstatus.h"
#include "gsaslserver.h"
#include "gsecrets.h"
#include "gstatemachine.h"
#include "gtimer.h"
#include "gexception.h"
#include <map>
#include <utility>
#include <memory>

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
/// Uses the ServerProtocol::Sender as its "sideways" interface
/// to talk back to the email-sending client.
///
/// \see GSmtp::ProtocolMessage, RFC-2821
///
class GSmtp::ServerProtocol : private GNet::TimerBase
{
public:
	G_EXCEPTION( ProtocolDone , "smtp protocol done" ) ;
	class Sender /// An interface used by ServerProtocol to send protocol replies.
	{
		public: virtual void protocolSend( const std::string & s , bool go_secure ) = 0 ;
			///< Called when the protocol class wants to send
			///< data down the socket.

		public: virtual void protocolShutdown() = 0 ;
			///< Called on receipt of a quit command after the quit
			///< response has been sent allowing the socket to be
			///< shut down.

		public: virtual ~Sender() ;
		private: void operator=( const Sender & ) ; // not implemented
	} ;
	class Text /// An interface used by ServerProtocol to provide response text strings.
	{
		public: virtual std::string greeting() const = 0 ;
		public: virtual std::string hello( const std::string & smtp_peer_name ) const = 0 ;
		public: virtual std::string received( const std::string & smtp_peer_name , bool a , bool s ) const = 0 ;
		public: virtual ~Text() ;
		private: void operator=( const Text & ) ; // not implemented
	} ;
	struct Config /// A structure containing configuration parameters for ServerProtocol.
	{
		bool with_vrfy ;
		unsigned int filter_timeout ;
		size_t max_size ;
		bool authentication_requires_encryption ;
		bool mail_requires_encryption ;
		bool disconnect_on_max_size ;
		bool advertise_tls_if_possible ;
		Config( bool with_vrfy , unsigned int filter_timeout , size_t max_size ,
			bool authentication_requires_encryption ,
			bool mail_requires_encryption ,
			bool advertise_tls_if_possible ) ;
	} ;

	ServerProtocol( GNet::ExceptionHandler & , Sender & , Verifier & , ProtocolMessage & ,
		const GAuth::Secrets & secrets , Text & text , GNet::Address peer_address ,
		Config config ) ;
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
			///< Exceptions thrown out of event-loop and timer callbacks
			///< are delivered to the given exception handler interface.
			///<
			///< All references are kept.

	void init() ;
		///< Starts the protocol. Use only once after construction.

	virtual ~ServerProtocol() ;
		///< Destructor.

	bool apply( const char * line_data , size_t line_size , size_t eolsize ) ;
		///< Called on receipt of a line of text from the remote client
		///< Returns true. Throws ProtocolDone at the end of the
		///< protocol.

	void secure( const std::string & certificate ) ;
		///< To be called when the transport protocol goes
		///< into secure mode.

protected:
	virtual void onTimeout() override ;
		///< Override from GNet::TimerBase.

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
		eStartTls ,
		eSecure ,
		eVrfy ,
		eVrfyReply ,
		eHelp ,
		eAuth ,
		eAuthData ,
		eContent ,
		eEot ,
		eDone ,
		eTimeout ,
		eUnknown
	} ;
	enum State
	{
		sStart ,
		sEnd ,
		sIdle ,
		sGotMail ,
		sGotRcpt ,
		sVrfyStart ,
		sVrfyIdle ,
		sVrfyGotMail ,
		sVrfyGotRcpt ,
		sVrfyTo1 ,
		sVrfyTo2 ,
		sData ,
		sProcessing ,
		sAuth ,
		sStartingTls ,
		sDiscarding ,
		s_Any ,
		s_Same
	} ;
	typedef std::pair<const char *,size_t> EventData ;
	typedef G::StateMachine<ServerProtocol,State,Event,EventData> Fsm ;

private:
	ServerProtocol( const ServerProtocol & ) ; // not implemented
	void operator=( const ServerProtocol & ) ; // not implemented
	void send( std::string , bool = false ) ;
	Event commandEvent( const std::string & ) const ;
	std::string commandWord( const std::string & line ) const ;
	std::string commandLine( const std::string & line ) const ;
	static const std::string & crlf() ;
	bool authenticationRequiresEncryption() const ;
	void reset() ;
	void badClientEvent() ;
	void processDone( bool , unsigned long , std::string , std::string ) ; // ProtocolMessage::doneSignal()
	void prepareDone( bool , bool , std::string ) ;
	bool isEndOfText( const char * , size_t ) const ;
	bool isEscaped( const char * , size_t ) const ;
	void doNoop( EventData , bool & ) ;
	void doIgnore( EventData , bool & ) ;
	void doNothing( EventData , bool & ) ;
	void doDiscarded( EventData , bool & ) ;
	void doDiscard( EventData , bool & ) ;
	void doHelp( EventData , bool & ) ;
	void doExpn( EventData , bool & ) ;
	void doQuit( EventData , bool & ) ;
	void doEhlo( EventData , bool & ) ;
	void doHelo( EventData , bool & ) ;
	void doAuth( EventData , bool & ) ;
	void doAuthData( EventData , bool & ) ;
	void doMail( EventData , bool & ) ;
	void doRcpt( EventData , bool & ) ;
	void doUnknown( EventData , bool & ) ;
	void doRset( EventData , bool & ) ;
	void doData( EventData , bool & ) ;
	void doContent( EventData , bool & ) ;
	void doComplete( EventData , bool & ) ;
	void doEot( EventData , bool & ) ;
	void doVrfy( EventData , bool & ) ;
	void doVrfyReply( EventData , bool & ) ;
	void doVrfyToReply( EventData , bool & ) ;
	void doNoRecipients( EventData , bool & ) ;
	void doStartTls( EventData , bool & ) ;
	void doSecure( EventData , bool & ) ;
	void verifyDone( std::string , VerifierStatus status ) ;
	void sendBadFrom( std::string ) ;
	void sendTooBig( bool disconnecting = false ) ;
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
	void sendEncryptionRequired() ;
	void sendNoRecipients() ;
	void sendMissingParameter() ;
	void sendVerified( const std::string & ) ;
	void sendNotVerified( const std::string & , bool ) ;
	void sendWillAccept( const std::string & ) ;
	void sendAuthDone( bool ok ) ;
	void sendOk() ;
	std::pair<std::string,std::string> parseAddress( const std::string & ) const ;
	std::pair<std::string,std::string> parseMailFrom( const std::string & ) const ;
	std::string parseMailParameter( const std::string & , const std::string & ) const ;
	size_t parseMailSize( const std::string & ) const ;
	std::string parseMailAuth( const std::string & ) const ;
	std::pair<std::string,std::string> parseRcptTo( const std::string & ) const ;
	std::string parseRcptParameter( const std::string & ) const ;
	std::string parsePeerName( const std::string & ) const ;
	void verify( const std::string & , const std::string & ) ;

private:
	Sender & m_sender ;
	Verifier & m_verifier ;
	Text & m_text ;
	ProtocolMessage & m_message ;
	unique_ptr<GAuth::SaslServer> m_sasl ;
	Config m_config ;
	Fsm m_fsm ;
	bool m_with_starttls ;
	GNet::Address m_peer_address ;
	bool m_secure ;
	std::string m_certificate ;
	unsigned int m_bad_client_count ;
	unsigned int m_bad_client_limit ;
	std::string m_session_peer_name ;
	bool m_session_authenticated ;
	std::string m_buffer ;
} ;

/// \class GSmtp::ServerProtocolText
/// A default implementation for the
/// ServerProtocol::Text interface.
///
class GSmtp::ServerProtocolText : public ServerProtocol::Text
{
public:
	ServerProtocolText( const std::string & code_ident , const std::string & thishost ,
		const GNet::Address & peer_address ) ;
			///< Constructor.

	virtual std::string greeting() const override ;
		///< Override from GSmtp::ServerProtocol::Text.

	virtual std::string hello( const std::string & smtp_peer_name_from_helo ) const override ;
		///< Override from GSmtp::ServerProtocol::Text.

	virtual std::string received( const std::string & smtp_peer_name_from_helo ,
		bool authenticated , bool secure ) const override ;
			///< Override from GSmtp::ServerProtocol::Text.

	static std::string receivedLine( const std::string & smtp_peer_name_from_helo ,
		const std::string & peer_address , const std::string & thishost ,
		bool authenticated , bool secure ) ;
			///< Returns a standard "Received:" line.

private:
	std::string m_code_ident ;
	std::string m_thishost ;
	GNet::Address m_peer_address ;
} ;

#endif
