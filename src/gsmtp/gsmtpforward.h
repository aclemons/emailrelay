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
/// \file gsmtpforward.h
///

#ifndef G_SMTP_FORWARD_H
#define G_SMTP_FORWARD_H

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
#include "gsmtpclient.h"
#include "gfilter.h"
#include "gsocket.h"
#include "gslot.h"
#include "gtimer.h"
#include "gstringarray.h"
#include "gexception.h"
#include <memory>
#include <iostream>

namespace GSmtp
{
	class Forward ;
}

//| \class GSmtp::Forward
/// A class for forwarding messages from a message store that manages
/// a GSmtp::Client instance, connecting and disconnecting as necessary
/// to do routing and re-authentication.
///
class GSmtp::Forward
{
public:
	using Config = GSmtp::Client::Config ;

	Forward( GNet::ExceptionSink , GStore::MessageStore & store ,
		FilterFactoryBase & , const GNet::Location & forward_to_default ,
		const GAuth::SaslClientSecrets & , const Config & config ) ;
			///< Constructor. Starts sending the first message from the
			///< message store.
			///<
			///< Once all messages have been sent the client will
			///< throw GNet::Done. See GNet::ClientPtr.
			///<
			///< Do not use sendMessage(). The messageDoneSignal()
			///< is not emitted.

	Forward( GNet::ExceptionSink ,
		FilterFactoryBase & , const GNet::Location & forward_to_default ,
		const GAuth::SaslClientSecrets & , const Config & config ) ;
			///< Constructor. Use sendMessage() immediately after
			///< construction.
			///<
			///< A messageDoneSignal() is emitted when the message
			///< has been sent, allowing the next sendMessage().
			///<
			///< Use quitAndFinish() at the end.

	virtual ~Forward() ;
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

	G::Slot::Signal<const Client::MessageDoneInfo&> & messageDoneSignal() noexcept ;
		///< Returns a signal that indicates that sendMessage()
		///< has completed or failed.

	G::Slot::Signal<const std::string&,const std::string&,const std::string&> & eventSignal() noexcept ;
		///< See GNet::Client::eventSignal()

	void doOnDelete( const std::string & reason , bool done ) ;
		///< Used by owning ClientPtr when handling an exception.

	bool finished() const ;
		///< Returns true after quitAndFinish().

	std::string peerAddressString() const ;
		///< Returns the Client's peerAddressString() if currently connected.

public:
	Forward( const Forward & ) = delete ;
	Forward( Forward && ) = delete ;
	Forward & operator=( const Forward & ) = delete ;
	Forward & operator=( Forward && ) = delete ;

private:
	void onErrorTimeout() ;
	void onContinueTimeout() ;
	bool sendNext() ;
	void start( std::unique_ptr<GStore::StoredMessage> ) ;
	void onMessageDoneSignal( const Client::MessageDoneInfo & ) ;
	void onEventSignal( const std::string & , const std::string & , const std::string & ) ;
	void onDelete( const std::string & reason ) ;
	void onDeleteSignal( const std::string & ) ;
	void onDeletedSignal( const std::string & ) ;
	bool updateClient( const GStore::StoredMessage & ) ;
	void newClient( const GStore::StoredMessage & ) ;
	void routingFilterDone( int ) ;
	bool unconnectable( const std::string & ) const ;
	static void insert( G::StringArray & , const std::string & ) ;
	static bool contains( const G::StringArray & , const std::string & ) ;
	static std::string messageInfo( const GStore::StoredMessage & ) ;

private:
	GNet::ExceptionSink m_es ;
	GStore::MessageStore * m_store ;
	FilterFactoryBase & m_ff ;
	GNet::Location m_forward_to_default ;
	GNet::Location m_forward_to_location ;
	std::string m_forward_to_address ;
	G::StringArray m_unconnectable ;
	GNet::ClientPtr<GSmtp::Client> m_client_ptr ;
	const GAuth::SaslClientSecrets & m_secrets ;
	Config m_config ;
	GNet::Timer<Forward> m_error_timer ;
	GNet::Timer<Forward> m_continue_timer ;
	std::string m_error ;
	std::shared_ptr<GStore::MessageStore::Iterator> m_iter ;
	std::unique_ptr<GStore::StoredMessage> m_message ;
	unsigned int m_message_count ;
	std::unique_ptr<Filter> m_routing_filter ;
	std::string m_selector ;
	bool m_has_connected ;
	bool m_finished ;
	G::Slot::Signal<const Client::MessageDoneInfo&> m_message_done_signal ;
	G::Slot::Signal<const std::string&,const std::string&,const std::string&> m_event_signal ;
} ;

#endif
