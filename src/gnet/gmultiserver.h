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
/// \file gmultiserver.h
///

#ifndef G_NET_MULTISERVER__H
#define G_NET_MULTISERVER__H

#include "gdef.h"
#include "gevent.h"
#include "gserver.h"
#include <vector>
#include <utility> // std::pair<>

namespace GNet
{
	class MultiServer ;
	class MultiServerImp ;
}

/// \class GNet::MultiServer
/// A server that listens on more than one address using a facade
/// pattern to multiple GNet::Server instances.
///
class GNet::MultiServer
{
public:
	typedef std::vector<Address> AddressList ;

	struct ServerInfo /// A structure used in GNet::MultiServer::newPeer().
	{
		ServerInfo() ;
		Address m_address ; ///< The server address that the peer connected to.
	} ;

	MultiServer( ExceptionSink listener_exception_sink , const AddressList & address_list ,
		ServerPeerConfig server_peer_config ) ;
			///< Constructor. The server listens on on the specific
			///< (local) interfaces.
			///<
			///< Precondition: !address_list.empty()

	virtual ~MultiServer() ;
		///< Destructor.

	bool hasPeers() const ;
		///< Returns true if peers() is not empty.

	std::vector<weak_ptr<ServerPeer> > peers() ;
		///< Returns the list of ServerPeer-derived objects.

	static bool canBind( const AddressList & listening_address_list , bool do_throw ) ;
		///< Checks that the specified addresses can be
		///< bound. Throws CannotBind if an address cannot
		///< be bound and 'do_throw' is true.

	static AddressList addressList( const Address & ) ;
		///< A trivial convenience fuction that returns the given
		///< address as a single-element list.

	static AddressList addressList( const AddressList & , unsigned int port ) ;
		///< Returns the given list of addresses with the port set
		///< correctly. If the given list is empty then a single
		///< 'any' address is returned.

	static AddressList addressList( const G::StringArray & , unsigned int port ) ;
		///< A convenience function that returns a list of
		///< listening addresses given a list of listening
		///< interfaces and a port number. If the list of
		///< interfaces is empty then a single 'any' address
		///< is returned.

	unique_ptr<ServerPeer> doNewPeer( ExceptionSinkUnbound , ServerPeerInfo , ServerInfo ) ;
		///< Pseudo-private method used by the pimple class.

protected:
	virtual unique_ptr<ServerPeer> newPeer( ExceptionSinkUnbound , ServerPeerInfo , ServerInfo ) = 0 ;
		///< A factory method which new()s a ServerPeer-derived
		///< object. See GNet::Server for the details.

	void serverCleanup() ;
		///< Should be called from all derived classes' destructors
		///< so that peer objects can use their Server objects
		///< safely during their own destruction.

	void serverReport( const std::string & server_type ) const ;
		///< Writes to the system log a summary of the underlying server
		///< objects and their addresses.

private:
	friend class GNet::MultiServerImp ;
	MultiServer( const MultiServer & ) g__eq_delete ;
	void operator=( const MultiServer & ) g__eq_delete ;
	void init( const Address & , ServerPeerConfig ) ;
	void init( const AddressList & address_list , ServerPeerConfig ) ;

private:
	typedef shared_ptr<MultiServerImp> ServerPtr ;
	typedef std::vector<ServerPtr> ServerList ;
	ExceptionSink m_es ;
	ServerList m_server_list ;
} ;

/// \class GNet::MultiServerImp
/// A GNet::Server class used in GNet::MultiServer.
///
class GNet::MultiServerImp : public GNet::Server
{
public:
	MultiServerImp( MultiServer & , ExceptionSink , const Address & , ServerPeerConfig ) ;
		///< Constructor.

	virtual ~MultiServerImp() ;
		///< Destructor.

	unique_ptr<ServerPeer> newPeer( ExceptionSinkUnbound , ServerPeerInfo ) g__final override ;
		///< Called by the base class to create a new ServerPeer.

	void cleanup() ;
		///< Calls GNet::Server::serverCleanup().

private:
	MultiServerImp( const MultiServerImp & ) g__eq_delete ;
	void operator=( const MultiServerImp & ) g__eq_delete ;

private:
	MultiServer & m_ms ;
	Address m_address ;
} ;

#endif
