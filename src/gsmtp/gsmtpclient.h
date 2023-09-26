//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// A class which acts as an SMTP client, sending messages to a remote
/// SMTP server.
///
/// A GSmtp::Client is-a GNet::Client so when it is destroyed as the
/// result of an exception the owner should call GNet::Client::doOnDelete()
/// (like GNet::ClientPtr does). The onDelete() implementation in this
/// class fails the current message if the GNet::Client was not
/// finish()ed (see quitAndFinish()) and the exception was not GNet::Done.
///
/// \see GSmtp::Forward
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
		bool fail_if_no_remote_recipients {true} ; // used by GSmtp::Forward

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
		Config & set_fail_if_no_remote_recipients( bool = true ) noexcept ;
	} ;

	struct MessageDoneInfo /// Signal parameters for GNet::Client::messageDoneSignal()
	{
		int response_code ; // smtp response code, or 0 for an internal non-smtp error
		std::string response ; // response text, empty iff sent successfully
		bool filter_special ;
	} ;

	Client( GNet::ExceptionSink ,
		FilterFactoryBase & , const GNet::Location & remote ,
		const GAuth::SaslClientSecrets & , const Config & config ) ;
			///< Constructor. Expects sendMessage() immediately after
			///< construction.
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
		///< Finishes a sendMessage() sequence. Sends a QUIT command and
		///< finish()es the GNet::Client.

	G::Slot::Signal<const MessageDoneInfo&> & messageDoneSignal() noexcept ;
		///< Returns a signal that indicates that sendMessage()
		///< has completed or failed.

private: // overrides
	void onConnect() override ; // GNet::Client
	bool onReceive( const char * , std::size_t , std::size_t , std::size_t , char ) override ; // GNet::Client
	void onDelete( const std::string & ) override ; // GNet::Client
	void onSendComplete() override ; // GNet::Client
	void onSecure( const std::string & , const std::string & , const std::string & ) override ; // GNet::SocketProtocol
	bool protocolSend( G::string_view , std::size_t , bool ) override ; // ClientProtocol::Sender

public:
	Client( const Client & ) = delete ;
	Client( Client && ) = delete ;
	Client & operator=( const Client & ) = delete ;
	Client & operator=( Client && ) = delete ;

private:
	std::shared_ptr<GStore::StoredMessage> message() ;
	void protocolDone( const ClientProtocol::DoneInfo & ) ; // GSmtp::ClientProtocol::doneSignal()
	void filterStart() ;
	void filterDone( int ) ;
	bool ready() const ;
	void start() ;
	void messageFail( int = 0 , const std::string & = {} ) ;
	void messageDestroy() ;
	void onNoFilterTimeout() ;
	static GNet::Client::Config netConfig( const Config & smtp_config ) ;

private:
	Config m_config ;
	GNet::Timer<Client> m_nofilter_timer ;
	std::shared_ptr<GStore::StoredMessage> m_message ;
	std::unique_ptr<Filter> m_filter ;
	ClientProtocol m_protocol ;
	G::Slot::Signal<const MessageDoneInfo&> m_message_done_signal ;
	bool m_secure ;
	bool m_filter_special ;
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
inline GSmtp::Client::Config & GSmtp::Client::Config::set_fail_if_no_remote_recipients( bool b ) noexcept { fail_if_no_remote_recipients = b ; return *this ; }

#endif
