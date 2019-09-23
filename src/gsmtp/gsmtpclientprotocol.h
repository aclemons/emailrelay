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
/// \file gsmtpclientprotocol.h
///

#ifndef G_SMTP_CLIENT_PROTOCOL__H
#define G_SMTP_CLIENT_PROTOCOL__H

#include "gdef.h"
#include "gmessagestore.h"
#include "gstoredmessage.h"
#include "gsaslclient.h"
#include "gsecrets.h"
#include "gslot.h"
#include "gstrings.h"
#include "gtimer.h"
#include "gexception.h"
#include <memory>
#include <iostream>

namespace GSmtp
{
	class ClientProtocol ;
	class ClientProtocolReply ;
}

/// \class GSmtp::ClientProtocolReply
/// A private implementation class used by ClientProtocol.
///
class GSmtp::ClientProtocolReply
{
public:
	g__enum(Type)
	{
		PositivePreliminary = 1 ,
		PositiveCompletion = 2 ,
		PositiveIntermediate = 3 ,
		TransientNegative = 4 ,
		PermanentNegative = 5
	} ; g__enum_end(Type)
	g__enum(SubType)
	{
		Syntax = 0 ,
		Information = 1 ,
		Connections = 2 ,
		MailSystem = 3 ,
		Invalid_SubType = 4
	} ; g__enum_end(SubType)
	g__enum(Value)
	{
		Internal_filter_ok = 222 ,
		Internal_filter_abandon = 223 ,
		Internal_secure = 224 ,
		ServiceReady_220 = 220 ,
		Ok_250 = 250 ,
		Authenticated_235 = 235 ,
		Challenge_334 = 334 ,
		OkForData_354 = 354 ,
		SyntaxError_500 = 500 ,
		SyntaxError_501 = 501 ,
		NotImplemented_502 = 502 ,
		BadSequence_503 = 503 ,
		Internal_filter_error = 590 ,
		NotAuthenticated_535 = 535 ,
		NotAvailable_454 = 454 ,
		Invalid = 0
	} ; g__enum_end(Value)

	static ClientProtocolReply ok() ;
		///< Factory function for an ok reply.

	static ClientProtocolReply ok( Value , const std::string & = std::string() ) ;
		///< Factory function for an ok reply with a specific 2xx value.

	static ClientProtocolReply error( Value , const std::string & response , const std::string & error_reason ) ;
		///< Factory function for an error reply with a specific 5xx value.

	explicit ClientProtocolReply( const std::string & line = std::string() ) ;
		///< Constructor for one line of text.

	bool add( const ClientProtocolReply & other ) ;
		///< Adds more lines to this reply. Returns false if the
		///< numeric values are different.

	bool incomplete() const ;
		///< Returns true if the reply is incomplete.

	bool validFormat() const ;
		///< Returns true if a valid format.

	bool positive() const ;
		///< Returns true if the numeric value of the
		///< reply is less that four hundred.

	bool is( Value v ) const ;
		///< Returns true if the reply value is 'v'.

	int value() const ;
		///< Returns the numeric value of the reply.

	std::string text() const ;
		///< Returns the text of the reply, excluding the numeric part,
		///< and with embedded newlines.

	std::string errorText() const ;
		///< Returns the text() string, plus any error reason, but with
		///< the guarantee that the returned string is empty if and only
		///< if the reply value is 2xx.

	std::string errorReason() const ;
		///< Returns an error reason string, as passed to error().

	bool textContains( std::string s ) const ;
		///< Returns true if the text() contains the given substring.

	std::string textLine( const std::string & prefix ) const ;
		///< Returns a line of text() which starts with
		///< prefix.

	Type type() const ;
		///< Returns the reply type (category).

	SubType subType() const ;
		///< Returns the reply sub-type.

private:
	static bool is_digit( char ) ;

private:
	bool m_complete ;
	bool m_valid ;
	int m_value ;
	std::string m_text ;
	std::string m_reason ; // additional error reason
} ;

/// \class GSmtp::ClientProtocol
/// Implements the client-side SMTP protocol.
///
class GSmtp::ClientProtocol : private GNet::TimerBase
{
public:
	G_EXCEPTION( NotReady , "not ready" ) ;
	G_EXCEPTION( TlsError , "tls/ssl error" ) ;
	G_EXCEPTION_CLASS( SmtpError , "smtp error" ) ;
	typedef ClientProtocolReply Reply ;

	class Sender /// An interface used by ClientProtocol to send protocol messages.
	{
	public:
		virtual bool protocolSend( const std::string & , size_t offset , bool go_secure ) = 0 ;
			///< Called by the Protocol class to send network data to
			///< the peer.
			///<
			///< The offset gives the location of the payload within the
			///< string buffer.
			///<
			///< Returns false if not all of the string was send due to
			///< flow control. In this case ClientProtocol::sendComplete() should
			///< be called as soon as the full string has been sent.
			///<
			///< Throws on error, eg. if disconnected.

		virtual ~Sender() ;
			///< Destructor.
	} ;

	struct Config /// A structure containing GSmtp::ClientProtocol configuration parameters.
	{
		std::string thishost_name ; // EHLO parameter
		unsigned int response_timeout ;
		unsigned int ready_timeout ;
		unsigned int filter_timeout ;
		bool use_starttls_if_possible ;
		bool must_use_tls ;
		bool must_authenticate ;
		bool anonymous ; // MAIL..AUTH=
		bool must_accept_all_recipients ;
		bool eight_bit_strict ; // fail 8bit messages to 7bit server
		Config( const std::string & name , unsigned int response_timeout ,
			unsigned int ready_timeout , unsigned int filter_timeout ,
			bool use_starttls_if_possible , bool must_use_tls ,
			bool must_authenticate , bool anonymous ,
			bool must_accept_all_recipients , bool eight_bit_strict ) ;
	} ;

	ClientProtocol( GNet::ExceptionSink , Sender & sender ,
		const GAuth::Secrets & secrets , const std::string & sasl_client_config ,
		const Config & config , bool in_secure_tunnel ) ;
			///< Constructor. The Sender interface is used to send protocol
			///< messages to the peer. The references are kept.

	G::Slot::Signal3<int,std::string,std::string> & doneSignal() ;
		///< Returns a signal that is raised once the protocol has finished
		///< with a given message. The first signal parameter is the SMTP response
		///< value, or 0 for an internal error, or -1 for filter-abandon, or -2
		///< for a filter-fail. The second parameter is the empty string on success
		///< or a non-empty response string. The third parameter contains
		///< any additional error reason text.

	G::Slot::Signal0 & filterSignal() ;
		///< Returns a signal that is raised when the protocol
		///< needs to do message filtering. The callee must call
		///< filterDone() when finished.

	void start( weak_ptr<StoredMessage> ) ;
		///< Starts transmission of the given message. The doneSignal()
		///< is used to indicate that the message has been processed
		///< and the shared object should remain valid until then.

	void finish() ;
		///< Called after the last message has been sent. Sends a quit
		///< command and shuts down the socket.

	void sendComplete() ;
		///< To be called when a blocked connection becomes unblocked.
		///< See ClientProtocol::Sender::protocolSend().

	void filterDone( bool ok , const std::string & response , const std::string & reason ) ;
		///< To be called when the Filter interface has done its thing.
		///< If ok then the message processing continues; if not ok
		///< then the message processing fails with a done signal
		///< code of -1 if the response is empty, or -2.

	void secure() ;
		///< To be called when the secure socket protocol has been
		///< successfully established.

	bool apply( const std::string & rx ) ;
		///< Called on receipt of a line of text from the remote server.
		///< Returns true if the protocol is done and the doneSignal()
		///< has been emited.

private:
	struct AuthError : public SmtpError
	{
		AuthError( const GAuth::SaslClient & , const ClientProtocolReply & ) ;
		std::string str() const ;
	} ;

private: // overrides
	virtual void onTimeout() override ; // Override from GNet::TimerBase.

private:
	shared_ptr<StoredMessage> message() ;
	void send( const char * ) ;
	void send( const char * , const std::string & ) ;
	void send( const char * , const std::string & , const std::string & ) ;
	bool send( const std::string & , bool eot , bool sensitive = false ) ;
	bool sendLine( std::string & ) ;
	size_t sendLines() ;
	void sendEhlo() ;
	void sendHelo() ;
	void sendMail() ;
	void sendMailCore() ;
	bool endOfContent() ;
	bool applyEvent( const Reply & event , bool is_start_event = false ) ;
	static bool parseReply( Reply & , const std::string & , std::string & ) ;
	void raiseDoneSignal( int , const std::string & , const std::string & = std::string() ) ;
	bool serverAuth( const ClientProtocolReply & reply ) const ;
	G::StringArray serverAuthMechanisms( const ClientProtocolReply & reply ) const ;
	void startFiltering() ;
	static std::string initialResponse( const GAuth::SaslClient & ) ;

private:
	g__enum(State) {
		sInit ,
		sStarted ,
		sServiceReady ,
		sSentEhlo ,
		sSentHelo ,
		sAuth ,
		sSentMail ,
		sFiltering ,
		sSentRcpt ,
		sSentData ,
		sSentDataStub ,
		sData ,
		sSentDot ,
		sStartTls ,
		sSentTlsEhlo ,
		sMessageDone ,
		sQuitting
	} ; g__enum_end(State)

private:
	Sender & m_sender ;
	const GAuth::Secrets & m_secrets ;
	unique_ptr<GAuth::SaslClient> m_sasl ;
	weak_ptr<StoredMessage> m_message ;
	Config m_config ;
	State m_state ;
	size_t m_to_index ;
	size_t m_to_accepted ;
	bool m_server_has_starttls ;
	bool m_server_has_auth ;
	bool m_server_secure ;
	bool m_server_has_8bitmime ;
	G::StringArray m_server_auth_mechanisms ;
	bool m_authenticated_with_server ;
	std::string m_auth_mechanism ;
	bool m_in_secure_tunnel ;
	bool m_warned ;
	Reply m_reply ;
	G::Slot::Signal3<int,std::string,std::string> m_done_signal ;
	G::Slot::Signal0 m_filter_signal ;
} ;

#endif
