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
/// \file gmultiserver.h
///

#ifndef G_NET_MULTISERVER_H
#define G_NET_MULTISERVER_H

#include "gdef.h"
#include "gevent.h"
#include "gserver.h"
#include "gtimer.h"
#include "ginterfaces.h"
#include "gexception.h"
#include <memory>
#include <utility> // std::pair<>
#include <vector>
#include <new> // std::nothrow_t

namespace GNet
{
	class MultiServer ;
	class MultiServerImp ;
}

//| \class GNet::MultiServer
/// A server that listens on more than one address using a facade
/// pattern to multiple GNet::Server instances. Supports dynamic
/// server instantiation based on available network interface
/// addresses (see GNet::InterfacesHandler).
///
class GNet::MultiServer : private InterfacesHandler
{
public:
	G_EXCEPTION( NoListeningAddresses , tx("no listening addresses") )
	G_EXCEPTION( InvalidName , tx("invalid address or interface name") )
	using AddressList = std::vector<Address> ;

	struct ServerInfo /// A structure used in GNet::MultiServer::newPeer().
	{
		ServerInfo() ;
		Address m_address ; ///< The server address that the peer connected to.
	} ;

	MultiServer( EventState es_listener , const G::StringArray & listen_list ,
		unsigned int port , const std::string & server_type ,
		ServerPeer::Config server_peer_config , Server::Config server_config ) ;
			///< Constructor. The server listens on inherited file descriptors
			///< formatted like "fd#3", specific local addresses (eg. "127.0.0.1")
			///< and addresses from named interfaces ("eth0").
			///<
			///< Listens on "0.0.0.0" and "::" if the listen list is
			///< empty.
			///<
			///< Throws if there are no addresses in the list and the
			///< GNet::Interfaces implementation is not active().

	~MultiServer() override ;
		///< Destructor.

	bool hasPeers() const ;
		///< Returns true if peers() is not empty.

	std::vector<std::weak_ptr<ServerPeer>> peers() ;
		///< Returns the list of ServerPeer-derived objects.
		///< The returned ServerPeer objects must not outlive
		///< this MultiServer.

	std::unique_ptr<ServerPeer> doNewPeer( EventStateUnbound , ServerPeerInfo && , const ServerInfo & ) ;
		///< Pseudo-private method used by the pimple class.

protected:
	virtual std::unique_ptr<ServerPeer> newPeer( EventStateUnbound , ServerPeerInfo && , ServerInfo ) = 0 ;
		///< A factory method which creates a ServerPeer-derived
		///< object. See GNet::Server for the details.

	void serverCleanup() ;
		///< Should be called from all derived classes' destructors
		///< so that peer objects can use their Server objects
		///< safely during their own destruction.

	void serverReport( const std::string & group = {} ) const ;
		///< Writes to the system log a summary of the underlying server
		///< objects and their addresses.

private: // overrides
	void onInterfaceEvent( const std::string & ) override ; // GNet::InterfacesHandler

public:
	MultiServer( const MultiServer & ) = delete ;
	MultiServer( MultiServer && ) = delete ;
	MultiServer & operator=( const MultiServer & ) = delete ;
	MultiServer & operator=( MultiServer && ) = delete ;

private:
	using ServerPtr = std::unique_ptr<MultiServerImp> ;
	using ServerList = std::vector<ServerPtr> ;
	bool gotServerFor( Address ) const ;
	bool gotAddressFor( const Listener & , const AddressList & ) const ;
	void onInterfaceEventTimeout() ;
	static bool match( const Address & , const Address & ) ;
	static std::string displayString( const Address & ) ;
	void createServer( Descriptor ) ;
	void createServer( const Address & , bool ) ;
	void createServer( const Address & , bool , std::nothrow_t ) ;
	ServerList::iterator removeServer( ServerList::iterator ) ;

private:
	EventState m_es ;
	G::StringArray m_listener_list ;
	unsigned int m_port ;
	std::string m_server_type ;
	ServerPeer::Config m_server_peer_config ;
	Server::Config m_server_config ;
	Interfaces m_if ;
	ServerList m_server_list ;
	Timer<MultiServer> m_interface_event_timer ;
} ;

//| \class GNet::MultiServerImp
/// A GNet::Server class used in GNet::MultiServer.
///
class GNet::MultiServerImp : public GNet::Server
{
public:
	MultiServerImp( MultiServer & , EventState , bool fixed , const Address & , ServerPeer::Config , Server::Config ) ;
		///< Constructor.

	MultiServerImp( MultiServer & , EventState , Descriptor , ServerPeer::Config , Server::Config ) ;
		///< Constructor.

	~MultiServerImp() override ;
		///< Destructor.

	std::unique_ptr<ServerPeer> newPeer( EventStateUnbound , ServerPeerInfo&& ) final ;
		///< Called by the base class to create a new ServerPeer.

	void cleanup() ;
		///< Calls GNet::Server::serverCleanup().

	bool dynamic() const ;
		///< Returns true if not a fixed address, as passed in to ctor.

public:
	MultiServerImp( const MultiServerImp & ) = delete ;
	MultiServerImp( MultiServerImp && ) = delete ;
	MultiServerImp & operator=( const MultiServerImp & ) = delete ;
	MultiServerImp & operator=( MultiServerImp && ) = delete ;

private:
	MultiServer & m_ms ;
	bool m_fixed ;
} ;

#endif
