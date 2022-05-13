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
/// \file gserver.h
///

#ifndef G_NET_SERVER_H
#define G_NET_SERVER_H

#include "gdef.h"
#include "ggettext.h"
#include "gserverpeer.h"
#include "gexceptionsink.h"
#include "gexception.h"
#include "gsocket.h"
#include "glistener.h"
#include "glimits.h"
#include "gprocess.h"
#include "gevent.h"
#include <utility>
#include <memory>
#include <string>

namespace GNet
{
	class Server ;
	class ServerPeer ;
	class ServerPeerInfo ;
}

//| \class GNet::Server
/// A network server class which listens on a specific port and spins off
/// ServerPeer objects for each incoming connection.
/// \see GNet::ServerPeer
///
class GNet::Server : public Listener, private EventHandler, private ExceptionHandler
{
public:
	G_EXCEPTION( CannotBind , tx("cannot bind the listening port") ) ;

	struct Config /// A configuration structure for GNet::Server.
	{
		int listen_queue { G::Limits<>::net_listen_queue } ; // Socket::listen() 'backlog'
		bool uds_open_permissions {true} ;
		Config & set_uds_open_permissions( bool b = true ) ;
	} ;

	Server( ExceptionSink , const Address & listening_address , ServerPeer::Config , Config ) ;
		///< Constructor. The server listens on the given address,
		///< which can be the 'any' address. The ExceptionSink
		///< is used for exceptions relating to the listening
		///< socket, not the server peers.

	~Server() override ;
		///< Destructor.

	Address address() const override ;
		///< Returns the listening address.
		///< Override from GNet::Listener.

	static bool canBind( const Address & listening_address , bool do_throw ) ;
		///< Checks that the specified address can be
		///< bound. Throws CannotBind if the address cannot
		///< be bound and 'do_throw' is true.

	std::vector<std::weak_ptr<GNet::ServerPeer>> peers() ;
		///< Returns the list of ServerPeer objects.

	bool hasPeers() const ;
		///< Returns true if peers() is not empty.

protected:
	virtual std::unique_ptr<ServerPeer> newPeer( ExceptionSinkUnbound , ServerPeerInfo && ) = 0 ;
		///< A factory method which new()s a ServerPeer-derived
		///< object. This method is called when a new connection
		///< comes in to this server. The new ServerPeer object
		///< is used to represent the state of the client/server
		///< connection.
		///<
		///< The implementation should std::move the 'ServerPeerInfo'
		///< parameter through to the ServerPeer base-class
		///< constructor.
		///<
		///< The implementation should return nullptr for non-fatal
		///< errors. It is expected that a typical server process will
		///< terminate if newPeer() throws, so most implementations
		///< will catch any exceptions and return nullptr.

	void serverCleanup() ;
		///< Should be called by the most-derived class's
		///< destructor in order to trigger early deletion of
		///< peer objects before the derived part of the server
		///< disappears. This prevents slicing if the destructor
		///< of the most-derived ServerPeer makes use of the
		///< most-derived Server.

private: // overrides
	void readEvent( Descriptor ) override ; // Override from GNet::EventHandler.
	void writeEvent( Descriptor ) override ; // Override from GNet::EventHandler.
	void onException( ExceptionSource * , std::exception & , bool ) override ; // Override from GNet::ExceptionHandler.

public:
	Server( const Server & ) = delete ;
	Server( Server && ) = delete ;
	void operator=( const Server & ) = delete ;
	void operator=( Server && ) = delete ;

private:
	void accept( ServerPeerInfo & ) ;
	static bool unlink( G::SignalSafe , const char * ) noexcept ;

private:
	using PeerList = std::vector<std::shared_ptr<ServerPeer>> ;
	ExceptionSink m_es ;
	ServerPeer::Config m_server_peer_config ;
	StreamSocket m_socket ; // listening socket
	PeerList m_peer_list ;
} ;

//| \class GNet::ServerPeerInfo
/// A move-only structure used in GNet::Server::newPeer() and
/// containing the new socket.
///
class GNet::ServerPeerInfo
{
public:
	std::unique_ptr<StreamSocket> m_socket ;
	Address m_address ;
	ServerPeer::Config m_server_peer_config ;
	Server * m_server ;
	ServerPeerInfo( Server * , ServerPeer::Config ) ;
} ;

inline GNet::Server::Config & GNet::Server::Config::set_uds_open_permissions( bool b ) { uds_open_permissions = b ; return *this ; }

#endif
