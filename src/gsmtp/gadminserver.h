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
/// \file gadminserver.h
///

#ifndef G_SMTP_ADMIN_H
#define G_SMTP_ADMIN_H

#include "gdef.h"
#include "gmultiserver.h"
#include "gtimer.h"
#include "gstr.h"
#include "glinebuffer.h"
#include "gsmtpserverprotocol.h"
#include "gclientptr.h"
#include "gsmtpclient.h"
#include "gstringarray.h"
#include "gstringmap.h"
#include <string>
#include <list>
#include <sstream>
#include <utility>
#include <memory>

namespace GSmtp
{
	class AdminServerPeer ;
	class AdminServer ;
}

//| \class GSmtp::AdminServerPeer
/// A derivation of ServerPeer for the administration interface.
///
/// The AdminServerPeer instantiates its own Smtp::Client in order
/// to implement the "flush" command.
///
/// \see GSmtp::AdminServer
///
class GSmtp::AdminServerPeer : public GNet::ServerPeer
{
public:
	AdminServerPeer( GNet::ExceptionSinkUnbound , GNet::ServerPeerInfo && , AdminServer & ,
		const std::string & remote , const G::StringMap & info_commands ,
		const G::StringMap & config_commands , bool with_terminate ) ;
			///< Constructor.

	~AdminServerPeer() override ;
		///< Destructor.

	bool notifying() const ;
		///< Returns true if the remote user has asked for notifications.

	void notify( const std::string & s0 , const std::string & s1 ,
		const std::string & s2 , const std::string & s4 ) ;
			///< Called when something happens which the admin
			///< user might be interested in.

private: // overrides
	void onSendComplete() override ; // Override from GNet::BufferedServerPeer.
	bool onReceive( const char * , std::size_t , std::size_t , std::size_t , char ) override ; // Override from GNet::BufferedServerPeer.
	void onDelete( const std::string & ) override ; // Override from GNet::ServerPeer.
	void onSecure( const std::string & , const std::string & , const std::string & ) override ; // Override from GNet::SocketProtocolSink.

public:
	AdminServerPeer( const AdminServerPeer & ) = delete ;
	AdminServerPeer( AdminServerPeer && ) = delete ;
	void operator=( const AdminServerPeer & ) = delete ;
	void operator=( AdminServerPeer && ) = delete ;

private:
	void clientDone( const std::string & ) ;
	static bool is( const std::string & , const std::string & ) ;
	static std::pair<bool,std::string> find( const std::string & line , const G::StringMap & map ) ;
	static std::string argument( const std::string & ) ;
	void flush() ;
	void forward() ;
	void help() ;
	void status() ;
	std::shared_ptr<MessageStore::Iterator> spooled() ;
	std::shared_ptr<MessageStore::Iterator> failures() ;
	void sendList( std::shared_ptr<MessageStore::Iterator> ) ;
	void sendLine( std::string , bool = true ) ;
	void warranty() ;
	void version() ;
	void copyright() ;
	std::string eol() const ;
	void unfailAll() ;
	void send_( const std::string & ) ;

private:
	GNet::ExceptionSink m_es ;
	AdminServer & m_server ;
	std::string m_prompt ;
	bool m_blocked ;
	std::string m_remote_address ;
	GNet::ClientPtr<GSmtp::Client> m_client_ptr ;
	bool m_notifying ;
	G::StringMap m_info_commands ;
	G::StringMap m_config_commands ;
	bool m_with_terminate ;
} ;

//| \class GSmtp::AdminServer
/// A server class which implements the emailrelay administration interface.
///
class GSmtp::AdminServer : public GNet::MultiServer
{
public:
	AdminServer( GNet::ExceptionSink , MessageStore & store , FilterFactory & ,
		G::Slot::Signal<const std::string&> & forward_request ,
		const GNet::ServerPeer::Config & net_server_peer_config ,
		const GNet::Server::Config & net_server_config ,
		const GSmtp::Client::Config & smtp_client_config ,
		const GAuth::SaslClientSecrets & client_secrets ,
		const G::StringArray & interfaces , unsigned int port , bool allow_remote ,
		const std::string & remote_address , unsigned int connection_timeout ,
		const G::StringMap & info_commands , const G::StringMap & config_commands ,
		bool with_terminate ) ;
			///< Constructor.

	~AdminServer() override ;
		///< Destructor.

	void report() const ;
		///< Generates helpful diagnostics.

	MessageStore & store() ;
		///< Returns a reference to the message store, as
		///< passed in to the constructor.

	FilterFactory & ff() ;
		///< Returns a reference to the filter factory, as
		///< passed in to the constructor.

	const GAuth::SaslClientSecrets & clientSecrets() const ;
		///< Returns a reference to the client secrets object, as passed
		///< in to the constructor. This is a client-side secrets file,
		///< used to authenticate ourselves with a remote server.

	GSmtp::Client::Config clientConfig() const ;
		///< Returns the client configuration.

	unsigned int connectionTimeout() const ;
		///< Returns the connection timeout, as passed in to the
		///< constructor.

	void forward() ;
		///< Called to trigger asynchronous forwarding.

	bool notifying() const ;
		///< Returns true if the remote user has asked for notifications.

	void notify( const std::string & s0 , const std::string & s1 ,
		const std::string & s2 , const std::string & s3 ) ;
			///< Called when something happens which the admin
			///< users might be interested in.

protected:
	std::unique_ptr<GNet::ServerPeer> newPeer( GNet::ExceptionSinkUnbound , GNet::ServerPeerInfo && , GNet::MultiServer::ServerInfo ) override ;
		///< Override from GNet::MultiServer.

public:
	AdminServer( const AdminServer & ) = delete ;
	AdminServer( AdminServer && ) = delete ;
	void operator=( const AdminServer & ) = delete ;
	void operator=( AdminServer && ) = delete ;

private:
	void onForwardTimeout() ;

private:
	GNet::Timer<AdminServer> m_forward_timer ;
	MessageStore & m_store ;
	FilterFactory & m_ff ;
	G::Slot::Signal<const std::string&> & m_forward_request ;
	GSmtp::Client::Config m_smtp_client_config ;
	const GAuth::SaslClientSecrets & m_client_secrets ;
	bool m_allow_remote ;
	std::string m_remote_address ;
	unsigned int m_connection_timeout ;
	G::StringMap m_info_commands ;
	G::StringMap m_config_commands ;
	bool m_with_terminate ;
} ;

#endif
