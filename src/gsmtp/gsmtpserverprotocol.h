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
/// \file gsmtpserverprotocol.h
///

#ifndef G_SMTP_SERVER_PROTOCOL__H
#define G_SMTP_SERVER_PROTOCOL__H

#include "gdef.h"
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
	public:
		virtual void protocolSend( const std::string & s , bool go_secure ) = 0 ;
			///< Called when the protocol class wants to send
			///< data down the socket.

		virtual void protocolShutdown() = 0 ;
			///< Called on receipt of a quit command after the quit
			///< response has been sent allowing the socket to be
			///< shut down.

		virtual ~Sender() ;
			///< Destructor.
	} ;

	class Text /// An interface used by ServerProtocol to provide response text strings.
	{
	public:
		virtual std::string greeting() const = 0 ;
			///< Returns a system identifier for the initial greeting.

		virtual std::string hello( const std::string & smtp_peer_name ) const = 0 ;
			///< Returns a hello response.

		virtual std::string received( const std::string & smtp_peer_name , bool auth , bool secure ,
			const std::string & cipher ) const = 0 ;
				///< Returns a complete 'Received' line.

		virtual ~Text() ;
			///< Destructor.
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

	ServerProtocol( GNet::ExceptionSink , Sender & , Verifier & , ProtocolMessage & ,
		const GAuth::Secrets & secrets , const std::string & sasl_server_config ,
		Text & text , GNet::Address peer_address , const Config & config ) ;
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
			///< are delivered to the given exception sink.
			///<
			///< All references are kept.

	void init() ;
		///< Starts the protocol. Use only once after construction.

	virtual ~ServerProtocol() ;
		///< Destructor.

	bool inDataState() const ;
		///< Returns true if currently in the data-transfer state.
		///< This can be used to enable the GNet::LineBuffer
		///< 'fragments' option.

	bool apply( const char * line_data , size_t line_size , size_t eolsize , size_t linesize , char c0 ) ;
		///< Called on receipt of a line of text from the remote
		///< client. As an optimisation this can also be a
		///< GNet::LineBuffer line fragment iff this object is
		///< currently inDataState(). Returns true. Throws
		///< ProtocolDone at the end of the protocol.

	void secure( const std::string & certificate , const std::string & cipher ) ;
		///< To be called when the transport protocol goes
		///< into secure mode.

private:
	g__enum(Event)
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
	} ; g__enum_end(Event)
	g__enum(State)
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
	} ; g__enum_end(State)
	struct EventData /// Contains GNet::LineBuffer callback parameters or a complete input line, passed through the G::StateMachine.
	{
		const char * ptr ;
		size_t size ;
		size_t eolsize ;
		size_t linesize ;
		char c0 ;

		EventData( const char * ptr , size_t size ) ;
		EventData( const char * ptr , size_t size , size_t eolsize , size_t linesize , char c0 ) ;
	} ;
	typedef G::StateMachine<ServerProtocol,State,Event,EventData> Fsm ;

private: // overrides
	virtual void onTimeout() override ; // Override from GNet::TimerBase.

private:
	ServerProtocol( const ServerProtocol & ) g__eq_delete ;
	void operator=( const ServerProtocol & ) g__eq_delete ;
	void send( const char * ) ;
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
	bool isEndOfText( const EventData & ) const ;
	bool isEscaped( const EventData & ) const ;
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
	void sendReadyForTls() ;
	void sendBadMechanism() ;
	void doAuthInvalid( EventData , bool & ) ;
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
	void sendOutOfSequence() ;
	void sendGreeting( const std::string & ) ;
	void sendQuitOk() ;
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
	void sendInvalidArgument() ;
	void sendAuthenticationCancelled() ;
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
	std::string m_cipher ;
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

	static std::string receivedLine( const std::string & smtp_peer_name_from_helo ,
		const std::string & peer_address , const std::string & thishost ,
		bool authenticated , bool secure , const std::string & secure_cipher ) ;
			///< Returns a standard "Received:" line.

private: // overrides
	virtual std::string greeting() const override ; // Override from GSmtp::ServerProtocol::Text.
	virtual std::string hello( const std::string & smtp_peer_name_from_helo ) const override ; // Override from GSmtp::ServerProtocol::Text.
	virtual std::string received( const std::string & smtp_peer_name_from_helo , bool authenticated , bool secure , const std::string & cipher ) const override ; // Override from GSmtp::ServerProtocol::Text.

private:
	std::string m_code_ident ;
	std::string m_thishost ;
	GNet::Address m_peer_address ;
} ;

#endif
