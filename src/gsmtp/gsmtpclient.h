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
/// \file gsmtpclient.h
///

#ifndef G_SMTP_CLIENT__H
#define G_SMTP_CLIENT__H

#include "gdef.h"
#include "glocation.h"
#include "gsecrets.h"
#include "glinebuffer.h"
#include "gclient.h"
#include "gsmtpclientprotocol.h"
#include "gmessagestore.h"
#include "gstoredmessage.h"
#include "gfilter.h"
#include "gcall.h"
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

/// \class GSmtp::Client
/// A class which acts as an SMTP client, extracting messages from a
/// message store and forwarding them to a remote SMTP server.
///
class GSmtp::Client : public GNet::Client , private ClientProtocol::Sender
{
public:
	struct Config /// A structure containing GSmtp::Client configuration parameters.
	{
		std::string filter_address ;
		unsigned int filter_timeout ;
		bool bind_local_address ;
		GNet::Address local_address ;
		ClientProtocol::Config client_protocol_config ;
		unsigned int connection_timeout ;
		unsigned int secure_connection_timeout ;
		bool secure_tunnel ;
		std::string sasl_client_config ;

		Config( std::string filter_address , unsigned int filter_timeout ,
			bool bind_local_address , const GNet::Address & local_address ,
			const ClientProtocol::Config & protocol_config ,
			unsigned int connection_timeout , unsigned int secure_connection_timeout ,
			bool secure_tunnel , const std::string & sasl_client_config ) ;
	} ;

	Client( GNet::ExceptionSink , const GNet::Location & remote ,
		const GAuth::Secrets & secrets , const Config & config ) ;
			///< Constructor. Starts connecting immediately.
			///<
			///< Use sendMessagesFrom() once, or use sendMessage()
			///< repeatedly. Wait for a messageDoneSignal() between
			///< each sendMessage().

	virtual ~Client() ;
		///< Destructor.

	void sendMessagesFrom( MessageStore & store ) ;
		///< Sends all messages from the given message store once
		///< connected. This must be used immediately after
		///< construction with a non-empty message store.
		///<
		///< Once all messages have been sent the client will throw
		///< GNet::Done. See GNet::ClientPtr.
		///<
		///< The messageDoneSignal() is not used when sending
		///< messages using this method.

	void sendMessage( unique_ptr<StoredMessage> message ) ;
		///< Starts sending the given message. Cannot be called
		///< if there is a message already in the pipeline.
		///<
		///< The messageDoneSignal() is used to indicate that the
		///< message filtering has finished or failed.
		///<
		///< The message is fail()ed if it cannot be sent. If this
		///< Client object is deleted before the message is sent
		///< the message is neither fail()ed or destroy()ed.

	G::Slot::Signal1<std::string> & messageDoneSignal() ;
		///< Returns a signal that indicates that sendMessage()
		///< has completed or failed.

private: // overrides
	virtual void onConnect() override ; // Override from GNet::SimpleClient.
	virtual bool onReceive( const char * , size_t , size_t , size_t , char ) override ; // Override from GNet::Client.
	virtual void onDelete( const std::string & ) override ; // Override from GNet::HeapClient.
	virtual void onSendComplete() override ; // Override from GNet::BufferedClient.
	virtual void onSecure( const std::string & , const std::string & ) override ; // Override from GNet::SocketProtocol.
	virtual bool protocolSend( const std::string & , size_t , bool ) override ; // Override from ClientProtocol::Sender.

private:
	Client( const Client & ) g__eq_delete ;
	void operator=( const Client & ) g__eq_delete ;
	shared_ptr<StoredMessage> message() ;
	void protocolDone( int , std::string , std::string ) ; // see ClientProtocol::doneSignal()
	void filterStart() ;
	void filterDone( int ) ;
	bool sendNext() ;
	void start() ;
	void messageFail( int = 0 , const std::string & = std::string() ) ;
	void messageDestroy() ;
	void startSending() ;
	void quitAndFinish() ;

private:
	MessageStore * m_store ;
	G::CallStack m_stack ;
	unique_ptr<Filter> m_filter ;
	shared_ptr<StoredMessage> m_message ;
	MessageStore::Iterator m_iter ;
	ClientProtocol m_protocol ;
	bool m_secure_tunnel ;
	G::Slot::Signal1<std::string> m_message_done_signal ;
	unsigned int m_message_count ;
} ;

#endif
