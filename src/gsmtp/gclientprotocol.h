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
/// \file gclientprotocol.h
///

#ifndef G_SMTP_CLIENT_PROTOCOL_H
#define G_SMTP_CLIENT_PROTOCOL_H

#include "gdef.h"
#include "gnet.h"
#include "gsmtp.h"
#include "gmessagestore.h"
#include "gsaslclient.h"
#include "gsecrets.h"
#include "gslot.h"
#include "gstrings.h"
#include "gtimer.h"
#include "gexception.h"
#include <memory>
#include <iostream>

/// \namespace GSmtp
namespace GSmtp
{
	class ClientProtocol ;
	class ClientProtocolReply ;
}

/// \class GSmtp::ClientProtocolReply
/// A private implementation class used
/// by ClientProtocol.
///
class GSmtp::ClientProtocolReply 
{
public:
	enum Type
	{
		PositivePreliminary = 1 ,
		PositiveCompletion = 2 ,
		PositiveIntermediate = 3 ,
		TransientNegative = 4 ,
		PermanentNegative = 5
	} ;
	enum SubType
	{
		Syntax = 0 ,
		Information = 1 ,
		Connections = 2 ,
		MailSystem = 3 ,
		Invalid_SubType = 4
	} ;
	enum Value
	{
		Internal_2xx = 222 ,
		Internal_2yy = 223 ,
		Internal_2zz = 224 ,
		ServiceReady_220 = 220 ,
		Ok_250 = 250 ,
		Authenticated_235 = 235 ,
		Challenge_334 = 334 ,
		OkForData_354 = 354 ,
		SyntaxError_500 = 500 ,
		SyntaxError_501 = 501 ,
		NotImplemented_502 = 502 ,
		BadSequence_503 = 503 ,
		NotAuthenticated_535 = 535 ,
		NotAvailable_454 = 454 ,
		Invalid = 0
	} ;

	static ClientProtocolReply ok() ;
		///< Factory function for an ok reply.

	static ClientProtocolReply ok( Value ) ;
		///< Factory function for an ok reply with a specific 2xx value.

	static ClientProtocolReply error( const std::string & reason ) ;
		///< Factory function for a generalised error reply.

	explicit ClientProtocolReply( const std::string & line = std::string() ) ;
		///< Constructor for one line of text.

	bool add( const ClientProtocolReply & other ) ;
		///< Adds more lines to this reply. Returns
		///< false if the numeric values are different.

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
		///< Returns the complete text of the reply,
		///< excluding the numeric part, and with
		///< embedded newlines.

	std::string errorText() const ;
		///< Returns the text() string but with the guarantee 
		///< that the returned string is empty if and only
		///< if the reply value is exactly 250.

	bool textContains( std::string s ) const ;
		///< Returns true if the text() contains
		///< the given substring.

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
} ;

/// \class GSmtp::ClientProtocol
/// Implements the client-side SMTP protocol.
///
/// Note that fatal, non-message-specific errors result in 
/// an exception being thrown, possibly out of an event-loop 
/// callback. In practice these will result in the connection 
/// being dropped immediately without failing the message.
///
class GSmtp::ClientProtocol : private GNet::AbstractTimer 
{
public:
	G_EXCEPTION( NotReady , "not ready" ) ;
	G_EXCEPTION( ResponseError , "protocol error: unexpected response" ) ;
	G_EXCEPTION( NoMechanism , "cannot do authentication mandated by remote server" ) ;
	G_EXCEPTION( AuthenticationRequired , "authentication required by the remote smtp server" ) ;
	G_EXCEPTION( AuthenticationNotSupported , "authentication not supported by the remote smtp server" ) ;
	G_EXCEPTION( AuthenticationError , "authentication error" ) ;
	G_EXCEPTION( TlsError , "tls/ssl error" ) ;
	typedef ClientProtocolReply Reply ;

	/// An interface used by ClientProtocol to send protocol messages.
	class Sender 
	{
		public: virtual bool protocolSend( const std::string & , size_t offset , bool go_secure ) = 0 ;
			///< Called by the Protocol class to send network data to 
			///< the peer.
			///<
			///< The offset gives the location of the payload within the 
			///< string buffer.
			///<
			///< Returns false if not all of the string was send due to 
			///< flow control. In this case ClientProtocol::sendDone() should 
			///< be called as soon as the full string has been sent.
			///<
			///< Throws on error, eg. if disconnected.

		private: void operator=( const Sender & ) ; // not implemented
		public: virtual ~Sender() ;
	} ;

	/// A structure containing GSmtp::ClientProtocol configuration parameters.
	struct Config 
	{
		std::string thishost_name ;
		unsigned int response_timeout ;
		unsigned int ready_timeout ;
		unsigned int preprocessor_timeout ;
		bool must_authenticate ;
		bool must_accept_all_recipients ;
		bool eight_bit_strict ;
		Config( const std::string & , unsigned int , unsigned int , unsigned int , bool , bool , bool ) ;
	} ;

	ClientProtocol( Sender & sender , const GAuth::Secrets & secrets , Config config ) ;
		///< Constructor. The 'sender' and 'secrets' references are kept.
		///<
		///< The Sender interface is used to send protocol messages to 
		///< the peer. 
		///<
		///< The 'thishost_name' parameter is used in the SMTP EHLO 
		///< request. 
		///<
		///< If the 'eight-bit-strict' flag is true then an eight-bit 
		///< message being sent to a seven-bit server will be failed.

	G::Signal2<std::string,int> & doneSignal() ;
		///< Returns a signal that is raised once the protocol has
		///< finished with a given message. The first signal parameter
		///< is the empty string on success or a non-empty reason 
		///< string. The second parameter is a reason code, typically 
		///< the SMTP error value (but see preprocessorDone()).

	G::Signal0 & preprocessorSignal() ;
		///< Returns a signal that is raised when the protocol
		///< needs to do message preprocessing. The callee
		///< must call preprocessorDone().

	void start( const std::string & from , const G::Strings & to , bool eight_bit ,
		std::string authentication , std::string server_name ,
		std::auto_ptr<std::istream> content ) ;
			///< Starts transmission of the given message.
			///<
			///< The doneSignal() is used to indicate that the 
			///< message has been processed.
			///<
			///< The 'server_name' parameter is passed to the SASL 
			///< authentication code. It should be a fully-qualified
			///< domain name where possible.

	void sendDone() ;
		///< To be called when a blocked connection becomes unblocked.
		///< See ClientProtocol::Sender::protocolSend().

	void preprocessorDone( bool ok , const std::string & reason ) ;
		///< To be called when the Preprocessor interface has done
		///< its thing. If not ok with an empty reason then the 
		///< current message is just abandoned, resulting in a
		///< done signal with an empty reason string and a 
		///< reason code of 1.

	void secure() ;
		///< To be called when the secure socket protocol
		///< has been successfully established.

	bool apply( const std::string & rx ) ;
		///< Called on receipt of a line of text from the server.
		///< Returns true if the protocol is done and the doneSignal()
		///< has been emited.

protected:
	virtual void onTimeout() ; 
		///< Final override from GNet::AbstractTimer.

	virtual void onTimeoutException( std::exception & ) ;
		///< Final override from GNet::AbstractTimer.

private:
	void send( const char * ) ;
	void send( const char * , const std::string & ) ;
	void send( const char * , const std::string & , const char * ) ;
	bool send( const std::string & , bool eot , bool sensitive = false ) ;
	bool sendLine( std::string & ) ;
	size_t sendLines() ;
	void sendEhlo() ;
	void sendHelo() ;
	void sendMail() ;
	void sendMailCore() ;
	bool endOfContent() const ;
	static const std::string & crlf() ;
	bool applyEvent( const Reply & event , bool is_start_event = false ) ;
	static bool parseReply( Reply & , const std::string & , std::string & ) ;
	void raiseDoneSignal( const std::string & , int = 0 , bool = false ) ;
	bool serverAuth( const ClientProtocolReply & reply ) const ;
	G::Strings serverAuthMechanisms( const ClientProtocolReply & reply ) const ;
	void startPreprocessing() ;

private:
	enum State { sInit , sStarted , sServiceReady , sSentEhlo , sSentHelo , sAuth1 , sAuth2 , sSentMail , 
		sPreprocessing , sSentRcpt , sSentData , sSentDataStub , sData , sSentDot , sStartTls , sSentTlsEhlo , sDone } ;
	Sender & m_sender ;
	const GAuth::Secrets & m_secrets ;
	std::string m_thishost ;
	State m_state ;
	std::string m_from ;
	G::Strings m_to ;
	size_t m_to_size ;
	size_t m_to_accepted ;
	std::auto_ptr<std::istream> m_content ;
	bool m_server_has_auth ;
	bool m_server_has_8bitmime ;
	bool m_server_has_tls ;
	bool m_message_is_8bit ;
	std::string m_message_authentication ;
	Reply m_reply ;
	bool m_authenticated_with_server ;
	std::string m_auth_mechanism ;
	std::auto_ptr<GAuth::SaslClient> m_sasl ;
	bool m_must_authenticate ;
	bool m_must_accept_all_recipients ;
	bool m_strict ;
	bool m_warned ;
	unsigned int m_response_timeout ;
	unsigned int m_ready_timeout ;
	unsigned int m_preprocessor_timeout ;
	G::Signal2<std::string,int> m_done_signal ;
	G::Signal0 m_preprocessor_signal ;
} ;

#endif
