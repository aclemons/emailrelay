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
// gsmtpclient.h
//

#ifndef G_SMTP_CLIENT_H
#define G_SMTP_CLIENT_H

#include "gdef.h"
#include "gnet.h"
#include "gsmtp.h"
#include "gsecrets.h"
#include "glinebuffer.h"
#include "gclient.h"
#include "gclientprotocol.h"
#include "gmessagestore.h"
#include "gstoredmessage.h"
#include "gsocket.h"
#include "gslot.h"
#include "gtimer.h"
#include "gstrings.h"
#include "gexception.h"
#include <memory>
#include <iostream>

namespace GSmtp
{
	class Client ;
	class ClientProtocol ;
}

// Class: GSmtp::Client
// Description: A class which acts as an SMTP client, extracting
// messages from a message store and forwarding them to
// a remote SMTP server.
//
class GSmtp::Client : private GNet::Client , private GNet::TimeoutHandler , private GSmtp::ClientProtocol::Sender 
{
public:
	G_EXCEPTION( NotConnected , "not connected" ) ;

	Client( MessageStore & store , const Secrets & secrets , 
		bool quit_on_disconnect , unsigned int response_timeout ) ;
			// Constructor. The 'store' and 'secrets' 
			// references are kept.
			//
			// The doneSignal() is used to indicate that
			// all message processing has finished 
			// or that the server connection has
			// been lost.

	Client( std::auto_ptr<StoredMessage> message , const Secrets & secrets , 
		unsigned int response_timeout ) ;
			// Constructor for sending a single message.
			// The 'secrets' reference is kept.
			//
			// The doneSignal() is used to indicate that all
			// message processing has finished or that the 
			// server connection has been lost.
			//
			// With this constructor (designed for proxying) the 
			// message is fail()ed if the connection to the 
			// downstream server cannot be made.

	G::Signal1<std::string> & doneSignal() ;
		// Returns a signal which indicates that client processing
		// is complete.
		//
		// The signal parameter is a failure reason, or the
		// empty string on success.

	G::Signal2<std::string,std::string> & eventSignal() ;
		// Returns a signal which indicates something interesting.
		//
		// The first signal parameter is one of "connecting",
		// "failed", "connected", "sending", or "done".

	std::string startSending( const std::string & server_address_string , unsigned int connection_timeout ) ;
		// Starts the sending process. Messages are extracted
		// from the message store (as passed in the ctor) and 
		// forwarded on to the specified server.
		//
		// To be called once (only) after construction.
		//
		// Returns an error string if there are no messages
		// to be sent, or if the network connection 
		// cannot be initiated. Returns the empty
		// string on success. The error string can
		// be partially interpreted by calling 
		// nothingToSend().

	static bool nothingToSend( const std::string & reason ) ;
		// Returns true if the given reason string -- obtained 
		// from startSending() -- is the fairly benign 
		// 'no messages to send'.

	bool busy() const ;
		// Returns true after construction and while
		// message processing is going on. Returns 
		// false once the doneSignal() has been emited.

private:
	virtual void onConnect( GNet::Socket & socket ) ; // GNet::Client
	virtual void onDisconnect() ; // GNet::Client
	virtual void onData( const char * data , size_t size ) ; // GNet::Client
	virtual void onWriteable() ; // GNet::Client
	virtual void onError( const std::string & error ) ; // GNet::Client
	virtual bool protocolSend( const std::string & ) ; // ClientProtocol::Sender
	void protocolDone( bool , bool , std::string ) ; // ClientProtocol::doneSignal()
	virtual void onTimeout( GNet::Timer & ) ; // GNet::TimeoutHandler
	std::string init( const std::string & , const std::string & , unsigned int ) ;
	GNet::Socket & socket() ;
	static std::string crlf() ;
	bool sendNext() ;
	void start( StoredMessage & ) ;
	void raiseDoneSignal( const std::string & ) ;
	void raiseEventSignal( const std::string & , const std::string & ) ;
	void finish( const std::string & reason = std::string() , bool do_disconnect = true ) ;
	void messageFail( const std::string & reason ) ;
	void messageDestroy() ;
	static std::string none() ;

private:
	MessageStore * m_store ;
	std::auto_ptr<StoredMessage> m_message ;
	MessageStore::Iterator m_iter ;
	GNet::LineBuffer m_buffer ;
	ClientProtocol m_protocol ;
	GNet::Socket * m_socket ;
	std::string m_pending ;
	G::Signal1<std::string> m_done_signal ;
	G::Signal2<std::string,std::string> m_event_signal ;
	std::string m_host ;
	GNet::Timer m_connect_timer ;
	unsigned int m_message_index ;
	bool m_busy ;
	bool m_force_message_fail ;
} ;

#endif
