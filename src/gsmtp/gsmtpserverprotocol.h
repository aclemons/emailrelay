//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#ifndef G_SMTP_SERVER_PROTOCOL_H
#define G_SMTP_SERVER_PROTOCOL_H

#include "gdef.h"
#include "gprotocolmessage.h"
#include "gsmtpserverparser.h"
#include "gsmtpserversender.h"
#include "gsmtpserversend.h"
#include "geventhandler.h"
#include "gaddress.h"
#include "gverifier.h"
#include "gverifierstatus.h"
#include "gsaslserver.h"
#include "gsaslserversecrets.h"
#include "gstatemachine.h"
#include "glinebuffer.h"
#include "gstringview.h"
#include "gexception.h"
#include "glimits.h"
#include <utility>
#include <memory>
#include <tuple>

namespace GSmtp
{
	class ServerProtocol ;
}

//| \class GSmtp::ServerProtocol
/// Implements the SMTP server-side protocol.
///
/// Uses the ProtocolMessage class as its down-stream interface, used for
/// assembling and processing the incoming email messages.
///
/// Uses the ServerSender as its "sideways" interface to talk back to
/// the client.
///
/// RFC-2920 PIPELINING suggests that reponses are batched while the protocol
/// is working through a batch of incoming requests. Therefore pipelined
/// requests should be apply()ed one by one with a parameter to indicate
/// last-in-batch.
///
/// The return value from apply() will indicate whether a request has been
/// fully processed. If the request is not immediately fully processed then
/// the batch iteration must be paused until a response is emitted via
/// protocolSend(). The GSmtp::ServerBufferIn class can help with this.
///
/// Some commands (DATA, NOOP, QUIT etc) should only appear at the end of
/// a batch of pipelined requests and the responses to these commands
/// should force any accumulated response batch to be flushed. (See also
/// RFC-2920 3.2 (2) (5) (6) and RFC-3030 (chunking) 4.2.) If the caller
/// implements response batching then the 'flush' parameter on the
/// protocolSend() callback can be used to flush the batch.
///
/// Note that RCPT-TO commands are typically in the middle of a pipelined
/// batch and might be processed asynchronously, but they do not cause the
/// response batch to be flushed.
///
class GSmtp::ServerProtocol : private GSmtp::ServerParser , private GSmtp::ServerSend
{
public:
	G_EXCEPTION_CLASS( Done , tx("smtp protocol done") )
	G_EXCEPTION( Busy , tx("smtp protocol busy") )
	using ApplyArgsTuple = std::tuple<const char *,std::size_t,std::size_t,std::size_t,char,bool> ; // see GNet::LineBuffer

	class Text /// An interface used by GSmtp::ServerProtocol to provide response text strings.
	{
	public:
		virtual std::string greeting() const = 0 ;
			///< Returns a system identifier for the initial greeting.

		virtual std::string hello( const std::string & smtp_peer_name ) const = 0 ;
			///< Returns a hello response.

		virtual std::string received( const std::string & smtp_peer_name , bool auth , bool secure ,
			const std::string & protocol , const std::string & cipher ) const = 0 ;
				///< Returns a complete 'Received' line.

		virtual ~Text() = default ;
			///< Destructor.
	} ;

	struct Config /// A configuration structure for GSmtp::ServerProtocol.
	{
		bool mail_requires_authentication {false} ; // for MAIL or VRFY, unless a trusted address
		bool mail_requires_encryption {false} ;

		bool with_vrfy {false} ;
		bool with_chunking {true} ; // CHUNKING (BDAT) and also advertise BINARYMIME
		bool with_pipelining {true} ;
		bool with_smtputf8 {false} ;
		ServerParser::Config parser_config ;
		bool smtputf8_strict {false} ; // reject non-ASCII characters if no MAIL-FROM SMTPUTF8 parameter

		bool tls_starttls {false} ;
		bool tls_connection {false} ; // smtps
		int shutdown_how_on_quit {1} ;
		unsigned int client_error_limit {8U} ;
		std::size_t max_size {0U} ; // EHLO SIZE
		std::string sasl_server_config ;
		std::string sasl_server_challenge_hostname ;

		Config() ;
		Config & set_mail_requires_authentication( bool = true ) noexcept ;
		Config & set_mail_requires_encryption( bool = true ) noexcept ;
		Config & set_with_vrfy( bool = true ) noexcept ;
		Config & set_with_chunking( bool = true ) noexcept ;
		Config & set_with_pipelining( bool = true ) noexcept ;
		Config & set_with_smtputf8( bool = true ) noexcept ;
		Config & set_parser_config( const ServerParser::Config & ) ;
		Config & set_smtputf8_strict( bool = true ) noexcept ;
		Config & set_max_size( std::size_t ) noexcept ;
		Config & set_tls_starttls( bool = true ) noexcept ;
		Config & set_tls_connection( bool = true ) noexcept ;
		Config & set_shutdown_how_on_quit( int ) noexcept ;
		Config & set_client_error_limit( unsigned int ) noexcept ;
		Config & set_sasl_server_config( const std::string & ) ;
		Config & set_sasl_server_challenge_hostname( const std::string & ) ;
	} ;

	ServerProtocol( ServerSender & , Verifier & , ProtocolMessage & ,
		const GAuth::SaslServerSecrets & secrets , Text & text ,
		const GNet::Address & peer_address , const Config & config ,
		bool enabled ) ;
			///< Constructor.
			///<
			///< The ServerSender interface is used to send protocol responses
			///< back to the client.
			///<
			///< The Verifier interface is used to verify recipient
			///< addresses. See GSmtp::Verifier.
			///<
			///< The ProtocolMessage interface is used to assemble and
			///< process an incoming message.
			///<
			///< The Text interface is used to get informational text for
			///< returning to the client.

	void setSender( ServerSender & ) ;
		///< Sets the ServerSender interface, overriding the constructor
		///< parameter.

	void init() ;
		///< Starts the protocol. Use only once after construction.
		///< The implementation uses the ServerSender interface to either
		///< send the plaintext SMTP greeting or start the TLS
		///< handshake.

	~ServerProtocol() override ;
		///< Destructor.

	bool inDataState() const ;
		///< Returns true if currently in a data-transfer state
		///< meaning that the next apply() does not need to
		///< contain a complete line of text. This is typically
		///< used to enable the GNet::LineBuffer 'fragments'
		///< option.

	bool inBusyState() const ;
		///< Returns true if in a state where the protocol is
		///< waiting for an asynchronous filter of address-verifier
		///< to complete. A call to apply() will throw an exception
		///< when in this state.

	bool apply( const ApplyArgsTuple & ) ;
		///< Called on receipt of a complete line of text from the
		///< client, or possibly a line fragment iff this object is
		///< currently inDataState().
		///<
		///< Throws an error if inBusyState().
		///<
		///< Returns false if the protocol is now inBusyState(); the
		///< caller should stop apply()ing any more data until the
		///< next ServerSender::protocolSend() callback.
		///<
		///< Throws Done at the end of the protocol.
		///<
		///< To allow for RFC-2920 PIPELINING the 'more' field
		///< should be set if there is another line that is ready to
		///< be apply()d. This defines an input batch and allows the
		///< ServerSender::protocolSend() callback to ask that the
		///< associated responses also get batched up on output.

	void secure( const std::string & certificate , const std::string & protocol , const std::string & cipher ) ;
		///< To be called when the transport protocol successfully
		///< goes into secure mode. See ServerSender::protocolSend().

	G::Slot::Signal<> & changeSignal() noexcept ;
		///< A signal that is emitted at the end of apply() or whenever
		///< the protocol state might have changed by some other
		///< mechanism (eg. GSmtp::Verifier).

public:
	ServerProtocol( const ServerProtocol & ) = delete ;
	ServerProtocol( ServerProtocol && ) = delete ;
	ServerProtocol & operator=( const ServerProtocol & ) = delete ;
	ServerProtocol & operator=( ServerProtocol && ) = delete ;

private:
	enum class Event
	{
		Unknown ,
		Quit ,
		Helo ,
		Ehlo ,
		Rset ,
		Noop ,
		Expn ,
		Data ,
		DataFail ,
		DataContent ,
		Bdat ,
		BdatLast ,
		BdatLastZero ,
		BdatCheck ,
		BdatContent ,
		Rcpt ,
		RcptReply ,
		Mail ,
		StartTls ,
		Secure ,
		Vrfy ,
		VrfyReply ,
		Help ,
		Auth ,
		AuthData ,
		Eot ,
		Done
	} ;
	enum class State
	{
		Start ,
		End ,
		Idle ,
		GotMail ,
		GotRcpt ,
		VrfyStart ,
		VrfyIdle ,
		VrfyGotMail ,
		VrfyGotRcpt ,
		RcptTo1 ,
		RcptTo2 ,
		Data ,
		BdatData ,
		BdatIdle ,
		BdatDataLast ,
		BdatChecking ,
		MustReset ,
		BdatProcessing ,
		Processing ,
		Auth ,
		StartingTls ,
		s_Any ,
		s_Same
	} ;
	using EventData = std::string_view ;
	using Fsm = G::StateMachine<ServerProtocol,State,Event,EventData> ;

private: // overrides
	bool sendFlush() const override ; // GSmtp::ServerSend

private:
	struct AddressCommand /// mail-from or rcpt-to
	{
		AddressCommand() = default ;
		AddressCommand( const std::string & e ) : error(e) {}
		std::string error ;
		std::string address ;
		std::size_t tailpos {std::string::npos} ;
		std::size_t size {0U} ;
		std::string auth ;
	} ;
	static std::unique_ptr<GAuth::SaslServer> newSaslServer( const GAuth::SaslServerSecrets & , const std::string & , const std::string & ) ;
	static int code( EventData ) ;
	static std::string str( EventData ) ;
	void applyEvent( Event , EventData = {} ) ;
	Event commandEvent( std::string_view ) const ;
	Event dataEvent( std::string_view ) const ;
	Event bdatEvent( std::string_view ) const ;
	G::StringArray mechanisms() const ;
	G::StringArray mechanisms( bool ) const ;
	void clear() ;
	bool messageAddContentFailed() ;
	bool messageAddContentTooBig() ;
	void badClientEvent() ;
	void protocolMessageProcessed( const ProtocolMessage::ProcessedInfo & ) ;
	bool rcptState() const ;
	bool flush() const ;
	bool isEndOfText( const ApplyArgsTuple & ) const ;
	bool isEscaped( const ApplyArgsTuple & ) const ;
	void doNoop( EventData , bool & ) ;
	void doIgnore( EventData , bool & ) ;
	void doHelp( EventData , bool & ) ;
	void doExpn( EventData , bool & ) ;
	void doQuit( EventData , bool & ) ;
	void doEhlo( EventData , bool & ) ;
	void doHelo( EventData , bool & ) ;
	void doAuthInvalid( EventData , bool & ) ;
	void doAuth( EventData , bool & ) ;
	void doAuthData( EventData , bool & ) ;
	void doMail( EventData , bool & ) ;
	void doRcpt( EventData , bool & ) ;
	void doUnknown( EventData , bool & ) ;
	void doRset( EventData , bool & ) ;
	void doData( EventData , bool & ) ;
	void doDataContent( EventData , bool & ) ;
	void doBadDataCommand( EventData , bool & ) ;
	void doBdatOutOfSequence( EventData , bool & ) ;
	void doBdatFirst( EventData , bool & ) ;
	void doBdatFirstLast( EventData , bool & ) ;
	void doBdatFirstLastZero( EventData , bool & ) ;
	void doBdatMore( EventData , bool & ) ;
	void doBdatMoreLast( EventData , bool & ) ;
	void doBdatMoreLastZero( EventData , bool & ) ;
	void doBdatImp( std::string_view , bool & , bool , bool , bool ) ;
	void doBdatContent( EventData , bool & ) ;
	void doBdatContentLast( EventData , bool & ) ;
	void doBdatCheck( EventData , bool & ) ;
	void doBdatComplete( EventData , bool & ) ;
	void doComplete( EventData , bool & ) ;
	void doEot( EventData , bool & ) ;
	void doVrfy( EventData , bool & ) ;
	void doVrfyReply( EventData , bool & ) ;
	void doRcptToReply( EventData , bool & ) ;
	void doNoRecipients( EventData , bool & ) ;
	void doStartTls( EventData , bool & ) ;
	void doSecure( EventData , bool & ) ;
	void doSecureGreeting( EventData , bool & ) ;
	void verifyDone( Verifier::Command , const VerifierStatus & ) ;
	std::string useStartTls() const ;
	void verify( Verifier::Command , const std::string & , const std::string & = {} , const std::string & = {} ) ;
	void warnInvalidSpaces() const ;
	void warnNoBrackets() const ;
	static void warning( const std::string & ) ;

private:
	ServerSender * m_sender ;
	Verifier & m_verifier ;
	Text & m_text ;
	ProtocolMessage & m_pm ;
	std::unique_ptr<GAuth::SaslServer> m_sasl ;
	Config m_config ;
	G::Slot::Signal<> m_change_signal ;
	const ApplyArgsTuple * m_apply_data {nullptr} ;
	bool m_apply_more {false} ;
	Fsm m_fsm ;
	bool m_with_starttls {false} ;
	GNet::Address m_peer_address ;
	bool m_secure {false} ;
	std::string m_verifier_raw_address ;
	std::string m_certificate ;
	std::string m_protocol ;
	std::string m_cipher ;
	unsigned int m_client_error_count {0U} ;
	std::string m_session_peer_name ;
	bool m_session_esmtp {false} ;
	std::size_t m_bdat_arg {0U} ;
	std::size_t m_bdat_sum {0U} ;
	bool m_enabled ;
} ;

inline GSmtp::ServerProtocol::Config & GSmtp::ServerProtocol::Config::set_with_vrfy( bool b ) noexcept { with_vrfy = b ; return *this ; }
inline GSmtp::ServerProtocol::Config & GSmtp::ServerProtocol::Config::set_with_chunking( bool b ) noexcept { with_chunking = b ; return *this ; }
inline GSmtp::ServerProtocol::Config & GSmtp::ServerProtocol::Config::set_max_size( std::size_t n ) noexcept { max_size = n ; return *this ; }
inline GSmtp::ServerProtocol::Config & GSmtp::ServerProtocol::Config::set_mail_requires_authentication( bool b ) noexcept { mail_requires_authentication = b ; return *this ; }
inline GSmtp::ServerProtocol::Config & GSmtp::ServerProtocol::Config::set_mail_requires_encryption( bool b ) noexcept { mail_requires_encryption = b ; return *this ; }
inline GSmtp::ServerProtocol::Config & GSmtp::ServerProtocol::Config::set_tls_starttls( bool b ) noexcept { tls_starttls = b ; return *this ; }
inline GSmtp::ServerProtocol::Config & GSmtp::ServerProtocol::Config::set_tls_connection( bool b ) noexcept { tls_connection = b ; return *this ; }
inline GSmtp::ServerProtocol::Config & GSmtp::ServerProtocol::Config::set_with_pipelining( bool b ) noexcept { with_pipelining = b ; return *this ; }
inline GSmtp::ServerProtocol::Config & GSmtp::ServerProtocol::Config::set_with_smtputf8( bool b ) noexcept { with_smtputf8 = b ; return *this ; }
inline GSmtp::ServerProtocol::Config & GSmtp::ServerProtocol::Config::set_parser_config( const ServerParser::Config & c ) { parser_config = c ; return *this ; }
inline GSmtp::ServerProtocol::Config & GSmtp::ServerProtocol::Config::set_shutdown_how_on_quit( int i ) noexcept { shutdown_how_on_quit = i ; return *this ; }
inline GSmtp::ServerProtocol::Config & GSmtp::ServerProtocol::Config::set_client_error_limit( unsigned int n ) noexcept { client_error_limit = n ; return *this ; }
inline GSmtp::ServerProtocol::Config & GSmtp::ServerProtocol::Config::set_smtputf8_strict( bool b ) noexcept { smtputf8_strict = b ; return *this ; }
inline GSmtp::ServerProtocol::Config & GSmtp::ServerProtocol::Config::set_sasl_server_config( const std::string & s ) { sasl_server_config = s ; return *this ; }
inline GSmtp::ServerProtocol::Config & GSmtp::ServerProtocol::Config::set_sasl_server_challenge_hostname( const std::string & s ) { sasl_server_challenge_hostname = s ; return *this ; }

#endif
