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
#include "gstringview.h"
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
class GSmtp::Client : public GNet::Client , private ClientProtocol::Sender , private GNet::EventLogging
{
public:
	struct Config /// A structure containing GSmtp::Client configuration parameters.
	{
		ClientProtocol::Config client_protocol_config ;
		GNet::Client::Config net_client_config ;
		Filter::Config filter_config ;
		FilterFactoryBase::Spec filter_spec ;
		bool secure_tunnel {false} ;
		std::string sasl_client_config ;
		bool fail_if_no_remote_recipients {true} ; // used by GSmtp::Forward
		bool log_msgid {false} ;
		Config & set_client_protocol_config( const ClientProtocol::Config & ) ;
		Config & set_net_client_config( const GNet::Client::Config & ) ;
		Config & set_filter_config( const Filter::Config & ) ;
		Config & set_filter_spec( const FilterFactoryBase::Spec & ) ;
		Config & set_secure_tunnel( bool = true ) noexcept ;
		Config & set_sasl_client_config( const std::string & ) ;
		Config & set_fail_if_no_remote_recipients( bool = true ) noexcept ;
		Config & set_log_msgid( bool = true ) noexcept ;
	} ;

	struct MessageDoneInfo /// Signal parameters for GNet::Client::messageDoneSignal()
	{
		int response_code ; // smtp response code, or 0 for an internal non-smtp error
		std::string response ; // response text, empty iff sent successfully
		bool filter_special ;
	} ;

	Client( GNet::EventState ,
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

	static std::string eventLoggingString( const GStore::StoredMessage * , const Config & ) ;
		///< Returns an event logging string for the given message.

private: // overrides
	void onConnect() override ; // GNet::Client
	bool onReceive( const char * , std::size_t , std::size_t , std::size_t , char ) override ; // GNet::Client
	void onDelete( const std::string & ) override ; // GNet::Client
	void onSendComplete() override ; // GNet::Client
	void onSecure( const std::string & , const std::string & , const std::string & ) override ; // GNet::SocketProtocol
	bool protocolSend( std::string_view , std::size_t , bool ) override ; // ClientProtocol::Sender
	std::string_view eventLoggingString() const noexcept override ; // GNet::EventLogging

public:
	Client( const Client & ) = delete ;
	Client( Client && ) = delete ;
	Client & operator=( const Client & ) = delete ;
	Client & operator=( Client && ) = delete ;

private:
	GNet::EventState m_es ;
	std::shared_ptr<GStore::StoredMessage> message() ;
	void protocolDone( const ClientProtocol::DoneInfo & ) ; // GSmtp::ClientProtocol::doneSignal()
	void filterStart() ;
	void filterDone( int ) ;
	bool ready() const ;
	void start() ;
	void messageFail( int = 0 , const std::string & = {} ) ;
	void messageDestroy() ;
	void onNoFilterTimeout() ;
	static GNet::Client::Config normalise( GNet::Client::Config ) ;

private:
	Config m_config ;
	GNet::Timer<Client> m_nofilter_timer ;
	std::shared_ptr<GStore::StoredMessage> m_message ;
	std::unique_ptr<Filter> m_filter ;
	ClientProtocol m_protocol ;
	G::Slot::Signal<const MessageDoneInfo&> m_message_done_signal ;
	bool m_secure {false} ;
	bool m_filter_special {false} ;
	G::CallStack m_stack ;
	std::string m_event_logging_string ;
} ;

inline GSmtp::Client::Config & GSmtp::Client::Config::set_client_protocol_config( const ClientProtocol::Config & c ) { client_protocol_config = c ; return *this ; }
inline GSmtp::Client::Config & GSmtp::Client::Config::set_net_client_config( const GNet::Client::Config & c ) { net_client_config = c ; return *this ; }
inline GSmtp::Client::Config & GSmtp::Client::Config::set_filter_spec( const FilterFactoryBase::Spec & r ) { filter_spec = r ; return *this ; }
inline GSmtp::Client::Config & GSmtp::Client::Config::set_filter_config( const Filter::Config & c ) { filter_config = c ; return *this ; }
inline GSmtp::Client::Config & GSmtp::Client::Config::set_secure_tunnel( bool b ) noexcept { secure_tunnel = b ; return *this ; }
inline GSmtp::Client::Config & GSmtp::Client::Config::set_sasl_client_config( const std::string & s ) { sasl_client_config = s ; return *this ; }
inline GSmtp::Client::Config & GSmtp::Client::Config::set_fail_if_no_remote_recipients( bool b ) noexcept { fail_if_no_remote_recipients = b ; return *this ; }
inline GSmtp::Client::Config & GSmtp::Client::Config::set_log_msgid( bool b ) noexcept { log_msgid = b ; return *this ; }

#endif
