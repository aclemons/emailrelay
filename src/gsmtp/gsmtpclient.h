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
/// \file gsmtpclient.h
///

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
#include "gprocessor.h"
#include "gsocket.h"
#include "gslot.h"
#include "gtimer.h"
#include "gstrings.h"
#include "gexecutable.h"
#include "gexception.h"
#include <memory>
#include <iostream>

/// \namespace GSmtp
namespace GSmtp
{
	class Client ;
	class ClientProtocol ;
}

/// \class GSmtp::Client
/// A class which acts as an SMTP client, extracting
/// messages from a message store and forwarding them to
/// a remote SMTP server.
///
class GSmtp::Client : public GNet::Client , private GSmtp::ClientProtocol::Sender 
{
public:
	G_EXCEPTION( NotConnected , "not connected" ) ;

	/// A structure containing GSmtp::Client configuration parameters.
	struct Config 
	{
		std::string processor_address ;
		unsigned int processor_timeout ;
		GNet::Address local_address ;
		ClientProtocol::Config client_protocol_config ;
		unsigned int connection_timeout ;
		unsigned int secure_connection_timeout ;
		bool secure_tunnel ;
		Config( std::string , unsigned int , GNet::Address , ClientProtocol::Config , unsigned int , unsigned int , bool ) ;
	} ;

	Client( const GNet::ResolverInfo & remote , const GAuth::Secrets & secrets , Config config ) ;
		///< Constructor. Starts connecting immediately.
		///<
		///< All Client instances must be on the heap since they 
		///< delete themselves after raising the done signal.

	void sendMessagesFrom( MessageStore & store ) ;
		///< Sends all messages from the given message store once 
		///< connected. This must be used immediately after 
		///< construction with a non-empty message store.
		///<
		///< Once all messages have been sent the client will delete
		///< itself (see HeapClient).
		///<
		///< The base class doneSignal() can be used as an indication
		///< that all messages have been sent and the object is about
		///< to delete itself.

	void sendMessage( std::auto_ptr<StoredMessage> message ) ;
		///< Starts sending the given message. Cannot be called
		///< if there is a message already in the pipeline.
		///<
		///< The messageDoneSignal() is used to indicate that the message 
		///< processing has finished or failed.
		///<
		///< The message is fail()ed if it cannot be sent. If this
		///< Client object is deleted before the message is sent
		///< the message is neither fail()ed or destroy()ed.

	G::Signal1<std::string> & messageDoneSignal() ;
		///< Returns a signal that indicates that sendMessage()
		///< has completed or failed.

protected:
	virtual ~Client() ;
		///< Destructor.

	virtual void onConnect() ; 
		///< Final override from GNet::SimpleClient.

	virtual bool onReceive( const std::string & ) ; 
		///< Final override from GNet::Client.

	virtual void onDelete( const std::string & , bool ) ; 
		///< Final override from GNet::HeapClient.

	virtual void onSendComplete() ; 
		///< Final override from GNet::BufferedClient.

	virtual void onSecure( const std::string & ) ;
		///< Final override from GNet::SocketProtocol.

private:
	virtual bool protocolSend( const std::string & , size_t , bool ) ; // override from private base class
	void protocolDone( std::string , int ) ; // see ClientProtocol::doneSignal()
	void preprocessorStart() ;
	void preprocessorDone( bool ) ;
	static const std::string & crlf() ;
	bool sendNext() ;
	void start( StoredMessage & ) ;
	void messageFail( const std::string & , int = 0 ) ;
	void messageDestroy() ;
	void doOnConnect() ;
	void logCertificate( const std::string & ) ;

private:
	MessageStore * m_store ;
	std::auto_ptr<Processor> m_processor ;
	std::auto_ptr<StoredMessage> m_message ;
	MessageStore::Iterator m_iter ;
	ClientProtocol m_protocol ;
	bool m_secure_tunnel ;
	G::Signal1<std::string> m_message_done_signal ;
} ;

#endif
