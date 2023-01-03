//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "glocation.h"
#include "gsaslclientsecrets.h"
#include "glinebuffer.h"
#include "gclient.h"
#include "gclientptr.h"
#include "gsmtpclientprotocol.h"
#include "gmessagestore.h"
#include "gstoredmessage.h"
#include "gfilterfactorybase.h"
#include "gfilter.h"
#include "gcall.h"
#include "gsocket.h"
#include "gslot.h"
#include "gtimer.h"
#include "gstringarray.h"
#include "gexception.h"
#include <memory>
#include <iostream>

namespace GSmtp
{
	class Client ;
	class ClientProtocol ;
}

//| \class GSmtp::Client
/// A class which acts as an SMTP client, extracting messages from a
/// message store and forwarding them to a remote SMTP server.
///
class GSmtp::Client : public GNet::Client , private ClientProtocol::Sender
{
public:
	struct Config /// A structure containing GSmtp::Client configuration parameters.
	{
		GNet::StreamSocket::Config stream_socket_config ;
		ClientProtocol::Config client_protocol_config ;
		Filter::Config filter_config ;
		FilterFactoryBase::Spec filter_spec ;
		bool bind_local_address {false} ;
		GNet::Address local_address ;
		unsigned int connection_timeout {0U} ;
		unsigned int secure_connection_timeout {0U} ;
		bool secure_tunnel {false} ;
		std::string sasl_client_config ;
		std::string client_tls_profile ;
		bool with_routing {true} ;

		Config() ;
		Config & set_stream_socket_config( const GNet::StreamSocket::Config & ) ;
		Config & set_client_protocol_config( const ClientProtocol::Config & ) ;
		Config & set_filter_config( const Filter::Config & ) ;
		Config & set_filter_spec( const FilterFactoryBase::Spec & ) ;
		Config & set_bind_local_address( bool = true ) noexcept ;
		Config & set_local_address( const GNet::Address & ) ;
		Config & set_connection_timeout( unsigned int ) noexcept ;
		Config & set_secure_connection_timeout( unsigned int ) noexcept ;
		Config & set_secure_tunnel( bool = true ) noexcept ;
		Config & set_sasl_client_config( const std::string & ) ;
		Config & set_client_tls_profile( const std::string & ) ;
		Config & set_with_routing( bool = true ) noexcept ;
	} ;

	Client( GNet::ExceptionSink , GStore::MessageStore & ,
		FilterFactoryBase & , const GNet::Location & remote ,
		const GAuth::SaslClientSecrets & , const Config & config ) ;
			///< Constructor. Starts connecting immediately and
			///< sends messages from the store once connected.
			///<
			///< Once all messages have been sent the client will
			///< throw GNet::Done. See GNet::ClientPtr.
			///<
			///< Do not use sendMessage(). The messageDoneSignal()
			///< is not emitted.

	Client( GNet::ExceptionSink ,
		FilterFactoryBase & , const GNet::Location & remote ,
		const GAuth::SaslClientSecrets & , const Config & config ) ;
			///< Constructor. Starts connecting immediately and
			///< expects sendMessage() immediately after construction.
			///<
			///< A messageDoneSignal() is emitted when the message
			///< has been sent, allowing the next sendMessage().
			///< Use quitAndFinish() at the end.

	~Client() override ;
		///< Destructor.

	void sendMessage( std::unique_ptr<GStore::StoredMessage> message ) ;
		///< Starts sending the given message. Cannot be called
		///< if there is a message already in the pipeline.
		///<
		///< The messageDoneSignal() is used to indicate that the
		///< message filtering has finished or failed.
		///<
		///< The message is fail()ed if it cannot be sent. If this
		///< Client object is deleted before the message is sent
		///< the message is neither fail()ed or destroy()ed.
		///<
		///< Does nothing if there are no message recipients.

	void quitAndFinish() ;
		///< Finishes a sendMessage() sequence.

	G::Slot::Signal<const std::string&> & messageDoneSignal() ;
		///< Returns a signal that indicates that sendMessage()
		///< has completed or failed.

private: // overrides
	void onConnect() override ; // Override from GNet::Client.
	bool onReceive( const char * , std::size_t , std::size_t , std::size_t , char ) override ; // Override from GNet::Client.
	void onDelete( const std::string & ) override ; // Override from GNet::HeapClient.
	void onSendComplete() override ; // Override from GNet::BufferedClient.
	void onSecure( const std::string & , const std::string & , const std::string & ) override ; // Override from GNet::SocketProtocol.
	bool protocolSend( G::string_view , std::size_t , bool ) override ; // Override from ClientProtocol::Sender.

public:
	Client( const Client & ) = delete ;
	Client( Client && ) = delete ;
	Client & operator=( const Client & ) = delete ;
	Client & operator=( Client && ) = delete ;

private:
	std::shared_ptr<GStore::StoredMessage> message() ;
	void protocolDone( int , const std::string & , const std::string & , const G::StringArray & ) ; // see ClientProtocol::doneSignal()
	void filterStart() ;
	void filterDone( int ) ;
	bool sendNext() ;
	void start() ;
	void messageFail( int = 0 , const std::string & = {} ) ;
	void messageDestroy() ;
	void startSending() ;
	void routingFilterDone( int ) ;
	void routedMessageDone( const std::string & ) ;
	void sendMessage( std::shared_ptr<GStore::StoredMessage> message ) ;
	static GNet::Client::Config netConfig( const Config & smtp_config ) ;

private:
	GNet::ExceptionSink m_es ;
	FilterFactoryBase & m_ff ;
	Config m_config ;
	const GAuth::SaslClientSecrets & m_secrets ;
	GStore::MessageStore * m_store ;
	std::shared_ptr<GStore::StoredMessage> m_message ;
	std::unique_ptr<Filter> m_filter ;
	std::unique_ptr<Filter> m_routing_filter ;
	std::shared_ptr<GStore::MessageStore::Iterator> m_iter ;
	ClientProtocol m_protocol ;
	GNet::ClientPtr<Client> m_routing_client ;
	G::Slot::Signal<const std::string&> m_message_done_signal ;
	unsigned int m_message_count ;
	G::CallStack m_stack ;
} ;

inline GSmtp::Client::Config & GSmtp::Client::Config::set_stream_socket_config( const GNet::StreamSocket::Config & c ) { stream_socket_config = c ; return *this ; }
inline GSmtp::Client::Config & GSmtp::Client::Config::set_client_protocol_config( const ClientProtocol::Config & c ) { client_protocol_config = c ; return *this ; }
inline GSmtp::Client::Config & GSmtp::Client::Config::set_filter_spec( const FilterFactoryBase::Spec & r ) { filter_spec = r ; return *this ; }
inline GSmtp::Client::Config & GSmtp::Client::Config::set_filter_config( const Filter::Config & c ) { filter_config = c ; return *this ; }
inline GSmtp::Client::Config & GSmtp::Client::Config::set_bind_local_address( bool b ) noexcept { bind_local_address = b ; return *this ; }
inline GSmtp::Client::Config & GSmtp::Client::Config::set_local_address( const GNet::Address & a ) { local_address = a ; return *this ; }
inline GSmtp::Client::Config & GSmtp::Client::Config::set_connection_timeout( unsigned int t ) noexcept { connection_timeout = t ; return *this ; }
inline GSmtp::Client::Config & GSmtp::Client::Config::set_secure_connection_timeout( unsigned int t ) noexcept { secure_connection_timeout = t ; return *this ; }
inline GSmtp::Client::Config & GSmtp::Client::Config::set_secure_tunnel( bool b ) noexcept { secure_tunnel = b ; return *this ; }
inline GSmtp::Client::Config & GSmtp::Client::Config::set_sasl_client_config( const std::string & s ) { sasl_client_config = s ; return *this ; }
inline GSmtp::Client::Config & GSmtp::Client::Config::set_client_tls_profile( const std::string & s ) { client_tls_profile = s ; return *this ; }
inline GSmtp::Client::Config & GSmtp::Client::Config::set_with_routing( bool b ) noexcept { with_routing = b ; return *this ; }

#endif
