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
/// \file unit.h
///

#ifndef G_MAIN_UNIT_H
#define G_MAIN_UNIT_H

#include "gdef.h"
#include "gclientptr.h"
#include "gslot.h"
#include "gsecrets.h"
#include "gfilestore.h"
#include "gsmtpclient.h"
#include "gsmtpserver.h"
#include "gadminserver.h"
#include "gpopserver.h"
#include "gpopstore.h"
#include "configuration.h"
#include <memory>

namespace Main
{
	class Unit ;
	class Run ;
}

//| \class Main::Unit
/// An agglomeration of things that surround a spool directory, including
/// an SMTP server and SMTP client.
///
class Main::Unit
{
public:
	Unit( Run & , unsigned int unit_id , const std::string & version ) ;
		///< Constructor.

	unsigned int id() const ;
		///< Returns the unit id.

	void start() ;
		///< Starts things off.

	bool quitWhenSent() const ;
		///< Returns true if configured to quit after all messages are sent.

	bool nothingToDo() const ;
		///< Returns true if there are is nothing to do.

	bool nothingToSend() const ;
		///< Returns true if there are no messages to send.

	static bool needsTls( const Configuration & ) ;
		///< Returns true if a unit with the given configuration will need TLS.

	static bool prefersTls( const Configuration & ) ;
		///< Returns true if a unit with the given configuration should have TLS.

	bool adminNotification() const ;
		///< Returns if the unit requires event notifications that it will
		///< deliver to remote clients of the admin server.

	void adminNotify( std::string , std::string , std::string , std::string ) ;
		///< Delivers the given event notification to remote clients of
		///< the admin server.

	G::Slot::Signal<unsigned,std::string,bool> & clientDoneSignal() ;
		///< Returns a signal that indicates that a forwarding client
		///< has done its work. The string parameter is a failure reason
		///< or the empty string on success. The boolean parameter indicates
		///< that unit's configuration is such that that program should
		///< now terminate.

	G::Slot::Signal<unsigned,std::string,std::string,std::string> & clientEventSignal() ;
		///< Returns a signal that emits messages like "connecting",
		///< "resolving" "connected", "sending", "sent", "forward start"
		///< and "forward end". See also Main::WinForm.

public:
	Unit( const Unit & ) = delete ;
	Unit( Unit && ) = delete ;
	Unit & operator=( const Unit & ) = delete ;
	Unit & operator=( Unit && ) = delete ;

private:
	void onPollTimeout() ;
	void onRequestForwardingTimeout() ;
	bool logForwarding() const ;
	std::string startForwarding() ;
	void requestForwarding( const std::string & reason = {} ) ;
	void onForwardRequest( const std::string & reason ) ;
	void onServerEvent( const std::string & s1 , const std::string & ) ;
	void onStoreRescanEvent() ;
	void onClientEvent( const std::string & , const std::string & , const std::string & ) ;
	void onClientDone( const std::string & ) ;
	int resolverFamily() const ;
	GStore::MessageStore & store() ;
	const GStore::MessageStore & store() const ;
	std::string serverTlsProfile() const ;
	std::string clientTlsProfile() const ;
	std::string ident() const ;

private:
	Run & m_run ;
	Configuration m_configuration ;
	std::string m_version_number ;
	unsigned int m_unit_id ;
	bool m_serving {false} ;
	bool m_forwarding {false} ;
	int m_resolver_family {AF_UNSPEC} ;
	bool m_quit_when_sent {false} ;
	bool m_forwarding_pending {false} ;
	std::string m_forwarding_reason ;
	G::Slot::Signal<unsigned,std::string,bool> m_client_done_signal ;
	G::Slot::Signal<unsigned,std::string,std::string,std::string> m_client_event_signal ;
	G::Slot::Signal<const std::string&> m_forward_request_signal ;
	std::unique_ptr<GNet::Timer<Unit>> m_forwarding_timer ;
	std::unique_ptr<GNet::Timer<Unit>> m_poll_timer ;
	std::unique_ptr<GStore::FileStore> m_file_store ;
	std::unique_ptr<GSmtp::FilterFactoryBase> m_filter_factory ;
	std::unique_ptr<GSmtp::VerifierFactoryBase> m_verifier_factory ;
	std::unique_ptr<GAuth::Secrets> m_client_secrets ;
	std::unique_ptr<GAuth::Secrets> m_server_secrets ;
	std::unique_ptr<GAuth::Secrets> m_pop_secrets ;
	std::unique_ptr<GSmtp::Server> m_smtp_server ;
	std::unique_ptr<GPop::Store> m_pop_store ;
	std::unique_ptr<GPop::Server> m_pop_server ;
	std::unique_ptr<GSmtp::AdminServer> m_admin_server ;
	GNet::ClientPtr<GSmtp::Client> m_client_ptr ;
} ;

#endif
