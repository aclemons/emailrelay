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
// gclientprotocol.h
//

#ifndef G_SMTP_CLIENT_PROTOCOL_H
#define G_SMTP_CLIENT_PROTOCOL_H

#include "gdef.h"
#include "gnet.h"
#include "gsmtp.h"
#include "gmessagestore.h"
#include "gsasl.h"
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

// Class: GSmtp::ClientProtocolReply
// Description: A private implementation class used
// by ClientProtocol.
//
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
		ServiceReady_220 = 220 ,
		Ok_250 = 250 ,
		Authenticated_235 = 235 ,
		Challenge_334 = 334 ,
		OkForData_354 = 354 ,
		SyntaxError_500 = 500 ,
		SyntaxError_501 = 501 ,
		NotImplemented_502 = 502 ,
		BadSequence_503 = 503 ,
		Invalid = 0
	} ;

	static ClientProtocolReply ok() ;
		// Factory function for an ok reply.

	static ClientProtocolReply error( const std::string & reason ) ;
		// Factory function for a generalised error reply.

	explicit ClientProtocolReply( const std::string & line = std::string() ) ;
		// Constructor for one line of text.

	bool add( const ClientProtocolReply & other ) ;
		// Adds more lines to this reply. Returns
		// false if the numeric values are different.

	bool incomplete() const ;
		// Returns true if the reply is incomplete.

	bool validFormat() const ;
		// Returns true if a valid format.

	bool positive() const ;
		// Returns true if the numeric value of the
		// reply is less that four hundred.

	bool is( Value v ) const ;
		// Returns true if the reply value is 'v'.

	unsigned int value() const ;
		// Returns the numeric value of the reply.

	std::string text() const ;
		// Returns the complete text of the reply,
		// excluding the numeric part, and with
		// embedded newlines.

	bool textContains( std::string s ) const ;
		// Returns true if the text() contains
		// the given substring.

	std::string textLine( const std::string & prefix ) const ;
		// Returns a line of text() which starts with
		// prefix.

	Type type() const ;
		// Returns the reply type (category).

	SubType subType() const ;
		// Returns the reply sub-type.

private:
	static bool is_digit( char ) ;

private:
	bool m_complete ;
	bool m_valid ;
	unsigned int m_value ;
	std::string m_text ;
} ;

// Class: GSmtp::ClientProtocol
// Description: Implements the client-side SMTP protocol.
//
class GSmtp::ClientProtocol : private GNet::Timer 
{
public:
	G_EXCEPTION( NotReady , "not ready" ) ;
	G_EXCEPTION( NoRecipients , "no recipients" ) ;
	typedef ClientProtocolReply Reply ;

	class Sender // An interface used by ClientProtocol to send protocol messages.
	{
		public: virtual bool protocolSend( const std::string & , size_t offset ) = 0 ;
			// Called by the Protocol class to send
			// network data to the peer.
			//
			// The offset gives the location of the
			// payload within the string buffer.
			//
			// Returns false if not all of the string
			// was sent, either due to flow control
			// or disconnection. After false is returned
			// ClientProtocol::sendDone() should be called
			// as soon as the full string has been sent.

		private: void operator=( const Sender & ) ; // not implemented
		public: virtual ~Sender() ;
	} ;

	struct Config // A structure containing GSmtp::ClientProtocol configuration parameters.
	{
		std::string thishost_name ;
		unsigned int response_timeout ;
		unsigned int ready_timeout ;
		unsigned int preprocessor_timeout ;
		bool must_authenticate ;
		bool eight_bit_strict ;
		Config( const std::string & , unsigned int , unsigned int , unsigned int , bool , bool ) ;
	} ;

	ClientProtocol( Sender & sender , const Secrets & secrets , Config config ) ;
		// Constructor. The 'sender' and 'secrets' references 
		// are kept.
		//
		// The Sender interface is used to send protocol 
		// messages to the peer. 
		//
		// The 'thishost_name' parameter is used in the
		// SMTP EHLO request. 
		//
		// If the 'eight-bit-strict' flag is true then
		// an eight-bit message being sent to a 
		// seven-bit server will be failed.

	G::Signal3<bool,bool,std::string> & doneSignal() ;
		// Returns a signal that is raised once the protocol has
		// finished with a given message. The signal parameters 
		// are 'ok', 'abort' and 'reason'.
		//
		// If 'ok' is false then 'abort' indicates
		// whether there is any point in trying to 
		// send more messages to the same server.
		// The 'abort' parameter will be true if,
		// for example, authentication failed -- if
		// it failed for one message then it will
		// fail for all the others.

	G::Signal0 & preprocessorSignal() ;
		// Returns a signal that is raised when the protocol
		// needs to do message preprocessing. The callee
		// must call preprocessorDone().

	void start( const std::string & from , const G::Strings & to , bool eight_bit ,
		std::string authentication , std::string server_name ,
		std::auto_ptr<std::istream> content ) ;
			// Starts transmission of the given message.
			//
			// The doneSignal() is used to indicate that the 
			// message has been processed.
			//
			// The 'server_name' parameter is passed to the SASL 
			// authentication code. It should be a fully-qualified
			// domain name where possible.

	void sendDone() ;
		// To be called when a blocked connection becomes unblocked.
		// See ClientProtocol::Sender::protocolSend().

	void preprocessorDone( const std::string & reason ) ;
		// To be called when the Preprocessor interface has done
		// its thing. The reason string should be empty
		// on success.

	bool apply( const std::string & rx ) ;
		// Called on receipt of a line of text from the server.
		// Returns true if the protocol is done and the doneSignal()
		// has been emited.

private:
	bool send( const std::string & , bool eot = false , bool log = true ) ;
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
	void raiseDoneSignal( bool , bool , const std::string & ) ;
	G::Strings serverAuthMechanisms( const ClientProtocolReply & reply ) const ;
	void startPreprocessing() ;
	void onTimeout() ;

private:
	enum State { sInit , sStarted , sServiceReady , sSentEhlo , sSentHelo , sAuth1 , sAuth2 , sSentMail , 
		sPreprocessing , sSentRcpt , sSentData , sData , sSentDot , sDone } ;
	Sender & m_sender ;
	const Secrets & m_secrets ;
	std::string m_thishost ;
	State m_state ;
	std::string m_from ;
	G::Strings m_to ;
	std::auto_ptr<std::istream> m_content ;
	bool m_server_has_8bitmime ;
	bool m_message_is_8bit ;
	std::string m_message_authentication ;
	Reply m_reply ;
	bool m_authenticated_with_server ;
	std::string m_auth_mechanism ;
	std::auto_ptr<SaslClient> m_sasl ;
	bool m_must_authenticate ;
	bool m_strict ;
	bool m_warned ;
	unsigned int m_response_timeout ;
	unsigned int m_ready_timeout ;
	unsigned int m_preprocessor_timeout ;
	G::Signal3<bool,bool,std::string> m_done_signal ;
	G::Signal0 m_preprocessor_signal ;
	bool m_signalled ;
} ;

#endif
