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
#include "gsmtpforward.h"
#include "gstringview.h"
#include "gstringarray.h"
#include "gstringmap.h"
#include <string>
#include <list>
#include <sstream>
#include <utility>
#include <memory>

namespace GSmtp
{
	class AdminServer ;
	class AdminServerImp ;
	class AdminServerPeer ;
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
	AdminServerPeer( GNet::ExceptionSinkUnbound , GNet::ServerPeerInfo && , AdminServerImp & ,
		const std::string & remote , const G::StringMap & info_commands ,
		bool with_terminate ) ;
			///< Constructor.

	~AdminServerPeer() override ;
		///< Destructor.

	bool notifying() const ;
		///< Returns true if the remote user has asked for notifications.

	void notify( const std::string & s0 , const std::string & s1 , const std::string & s2 , const std::string & s3 ) ;
		///< Called when something happens which the remote admin
		///< user might be interested in.

private: // overrides
	void onSendComplete() override ; // GNet::BufferedServerPeer
	bool onReceive( const char * , std::size_t , std::size_t , std::size_t , char ) override ; // GNet::BufferedServerPeer
	void onDelete( const std::string & ) override ; // GNet::ServerPeer
	void onSecure( const std::string & , const std::string & , const std::string & ) override ; // GNet::SocketProtocolSink

public:
	AdminServerPeer( const AdminServerPeer & ) = delete ;
	AdminServerPeer( AdminServerPeer && ) = delete ;
	AdminServerPeer & operator=( const AdminServerPeer & ) = delete ;
	AdminServerPeer & operator=( AdminServerPeer && ) = delete ;

private:
	void clientDone( const std::string & ) ;
	static bool is( G::string_view , G::string_view ) ;
	static std::pair<bool,std::string> find( G::string_view , const G::StringMap & map ) ;
	void flush() ;
	void forward() ;
	void help() ;
	void status() ;
	void sendMessageIds( const std::vector<GStore::MessageId> & ) ;
	void sendLine( std::string && ) ;
	void sendLineCopy( std::string ) ;
	void warranty() ;
	void version() ;
	void copyright() ;
	std::string eol() const ;
	void unfailAll() ;
	void sendImp( const std::string & ) ;

private:
	GNet::ExceptionSink m_es ;
	AdminServerImp & m_server_imp ;
	std::string m_prompt ;
	bool m_blocked {false} ;
	std::string m_remote_address ;
	GNet::ClientPtr<GSmtp::Forward> m_client_ptr ;
	bool m_notifying {false} ;
	G::StringMap m_info_commands ;
	bool m_with_terminate ;
	unsigned int m_error_limit {30U} ;
	unsigned int m_error_count {0U} ;
} ;

//| \class GSmtp::AdminServer
/// A server class which implements the emailrelay administration interface.
///
class GSmtp::AdminServer
{
public:
	G_EXCEPTION( NotImplemented , tx("admin server not implemented") ) ;
	struct Config /// A configuration structure for GSmtp::AdminServer.
	{
		unsigned int port {10026U} ;
		bool with_terminate {false} ;
		bool allow_remote {false} ;
		std::string remote_address ;
		G::StringMap info_commands ;
		Client::Config smtp_client_config ;
		GNet::Server::Config net_server_config ;
		GNet::ServerPeer::Config net_server_peer_config ;

		Config & set_port( unsigned int ) noexcept ;
		Config & set_with_terminate( bool = true ) noexcept ;
		Config & set_allow_remote( bool = true ) noexcept ;
		Config & set_remote_address( const std::string & ) ;
		Config & set_info_commands( const G::StringMap & ) ;
		Config & set_smtp_client_config( const Client::Config & ) ;
		Config & set_net_server_config( const GNet::Server::Config & ) ;
		Config & set_net_server_peer_config( const GNet::ServerPeer::Config & ) ;
	} ;
	enum class Command
	{
		forward ,
		dnsbl ,
		smtp_enable
	} ;

	static bool enabled() ;
		///< Returns true if the server is enabled.

	AdminServer( GNet::ExceptionSink , GStore::MessageStore & store , FilterFactoryBase & ,
		const GAuth::SaslClientSecrets & client_secrets , const G::StringArray & interfaces ,
		const Config & config ) ;
			///< Constructor.

	~AdminServer() ;
		///< Destructor.

	G::Slot::Signal<Command,unsigned int> & commandSignal() ;
		///< Returns a reference to a signal that is emit()ted when the
		///< remote user makes a request.

	void report( const std::string & group = {} ) const ;
		///< Generates helpful diagnostics.

	GStore::MessageStore & store() ;
		///< Returns a reference to the message store, as
		///< passed in to the constructor.

	FilterFactoryBase & ff() ;
		///< Returns a reference to the filter factory, as
		///< passed in to the constructor.

	const GAuth::SaslClientSecrets & clientSecrets() const ;
		///< Returns a reference to the client secrets object, as passed
		///< in to the constructor. This is a client-side secrets file,
		///< used to authenticate ourselves with a remote server.

	void emitCommand( Command , unsigned int ) ;
		///< Emits an asynchronous event on the commandSignal().
		///< Used by AdminServerPeer.

	bool notifying() const ;
		///< Returns true if the remote user has asked for notifications.

	void notify( const std::string & s0 , const std::string & s1 , const std::string & s2 , const std::string & s3 ) ;
		///< Called when something happens which the admin
		///< users might be interested in.

public:
	AdminServer( const AdminServer & ) = delete ;
	AdminServer( AdminServer && ) = delete ;
	AdminServer & operator=( const AdminServer & ) = delete ;
	AdminServer & operator=( AdminServer && ) = delete ;

private:
	std::unique_ptr<AdminServerImp> m_imp ;
} ;

inline GSmtp::AdminServer::Config & GSmtp::AdminServer::Config::set_port( unsigned int n ) noexcept { port = n ; return *this ; }
inline GSmtp::AdminServer::Config & GSmtp::AdminServer::Config::set_with_terminate( bool b ) noexcept { with_terminate = b ; return *this ; }
inline GSmtp::AdminServer::Config & GSmtp::AdminServer::Config::set_allow_remote( bool b ) noexcept { allow_remote = b ; return *this ; }
inline GSmtp::AdminServer::Config & GSmtp::AdminServer::Config::set_remote_address( const std::string & s ) { remote_address = s ; return *this ; }
inline GSmtp::AdminServer::Config & GSmtp::AdminServer::Config::set_info_commands( const G::StringMap & m ) { info_commands = m ; return *this ; }
inline GSmtp::AdminServer::Config & GSmtp::AdminServer::Config::set_smtp_client_config( const Client::Config & c ) { smtp_client_config = c ; return *this ; }
inline GSmtp::AdminServer::Config & GSmtp::AdminServer::Config::set_net_server_config( const GNet::Server::Config & c ) { net_server_config = c ; return *this ; }
inline GSmtp::AdminServer::Config & GSmtp::AdminServer::Config::set_net_server_peer_config( const GNet::ServerPeer::Config & c ) { net_server_peer_config = c ; return *this ; }

#endif
