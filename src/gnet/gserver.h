//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// gserver.h
//

#ifndef G_SERVER_H 
#define G_SERVER_H

#include "gdef.h"
#include "gnet.h"
#include "gsocket.h"
#include "gconnection.h"
#include "gevent.h"
#include <utility>
#include <list>
#include <memory>
#include <string>

namespace GNet
{
	class Server ;
	class ServerPeer ;
	class ServerPeerHandle ;
}

// Class: GNet::ServerPeerHandle
// Description: A structure used in the implementation of GNet::Server.
// The server holds a list of handles which refer to all its peer objects. 
// When a peer object deletes itself it resets the handle, without changing 
// the server's list. The server uses its list to delete all peer objects
// from within its destructor. The server does garbage collection occasionally,
// deleting handles which have been reset. 
//
class GNet::ServerPeerHandle 
{
public:
	ServerPeerHandle() ;
		// Default constructor.

	void set( ServerPeer * p ) ;
		// Sets the pointer.

	void reset() ;
		// Resets the pointer.

	ServerPeer * peer() ;
		// Returns the pointer.

	ServerPeer * old() ;
		// Returns the pointer value before it was reset().
		// Used in debugging.

private:
	ServerPeer * m_p ;
	ServerPeer * m_old ;
} ;

// Class: GNet::Server
// Description: A network server class which listens on a specific
// port and spins off ServerPeer objects for each incoming connection.
// See also: GNet::ServerPeer
//
class GNet::Server : public GNet::EventHandler 
{
public:
	G_EXCEPTION( CannotBind , "cannot bind the listening port" ) ;
	G_EXCEPTION( CannotListen , "cannot listen" ) ;

	struct PeerInfo // A structure used in GNet::Server::newPeer().
	{
		std::auto_ptr<StreamSocket> m_socket ;
		Address m_address ;
		ServerPeerHandle * m_handle ;
		PeerInfo() ;
	} ;

	static bool canBind( const Address & listening_address , bool do_throw ) ;
		// Checks that the specified address can be
		// bound. Throws CannotBind if the address cannot 
		// be bound and 'do_throw' is true.

	explicit Server( unsigned int listening_port ) ;
		// Constructor taking a port number. The server
		// listens on all local interfaces.

	explicit Server( const Address & listening_address ) ;
		// Constructor. The server listens only on the
		// specific (local) interface.

	Server() ;
		// Default constructor. Initialise with init().

	void init( unsigned int listening_port ) ;
		// Initilisation after default construction.

	void init( const Address & listening_address ) ;
		// Initilisation after default construction.

	virtual ~Server() ;
		// Destructor.

	std::pair<bool,Address> address() const ;
		// Returns the listening address. Pair.first
		// is false if not properly init()ialised.

protected:
	virtual ServerPeer * newPeer( PeerInfo ) = 0 ;
		// A factory method which new()s a ServerPeer-derived 
		// object. This method is called when a new connection
		// comes into this server. The new ServerPeer object 
		// is used to represent the state of the client/server 
		// connection. 
		//
		// The new ServerPeer object manages its own lifetime 
		// doing a "delete this" when the network connection 
		// disappears. But the Server also deletes remaining
		// peers during its destruction.
		//
		// The implementation should pass the 'PeerInfo' 
		// parameter through to the ServerPeer base-class 
		// constructor.
		//
		// Should return NULL for non-fatal errors. It is 
		// expected that a typical server process will 
		// terminate if newPeer() throws, so most 
		// implementations will catch any exceptions and 
		// return NULL.

	void serverCleanup() ;
		// May be called from the derived class destructor
		// in order to trigger early deletion of peer objects,
		// before the derived part of the server disappears.
		// If this is called from the most-derived Server
		// class then it allows ServerPeer objects to use 
		// their Server object safely during destruction.

private:
	Server( const Server & ) ; // not implemented
	void operator=( const Server & ) ; // not implemented
	virtual void readEvent() ; // from EventHandler
	virtual void writeEvent() ; // from EventHandler
	virtual void exceptionEvent() ; // from EventHandler
	void serverCleanupCore() ;
	void collectGarbage() ;
	void readEventCore() ;

private:
	typedef std::list<ServerPeerHandle> PeerList ;
	std::auto_ptr<StreamSocket> m_socket ;
	PeerList m_peer_list ;
	bool m_cleaned_up ;
} ;

// Class: GNet::ServerPeer
// Description: An abstract base class for the GNet::Server's
// connection to a remote client. Instances are created
// on the heap by the Server::newPeer() override, and they 
// delete themselves when the connection is lost.
// See also: GNet::Server, GNet::EventHandler
//
class GNet::ServerPeer : public GNet::EventHandler , public GNet::Connection 
{
public:
	enum { c_buffer_size = 1500 } ;

	explicit ServerPeer( Server::PeerInfo ) ;
		// Constructor. This constructor is
		// only used from within the 
		// override of GServer::newPeer().

	void doDelete() ; 
		// Does "onDelete(); delete this".

	std::string asString() const ;
		// Returns a string representation of the
		// socket descriptor. Typically used in 
		// log message to destinguish one connection
		// from another.

	virtual std::pair<bool,Address> localAddress() const ;
		// Returns the local address.
		// Pair.first is false on error.

	virtual std::pair<bool,Address> peerAddress() const ;
		// Returns the peer address.

	virtual ~ServerPeer() ;
		// Destructor. Note that objects will delete
		// themselves when they detect that the
		// connection has been lost -- see doDelete().

protected:
	virtual void onDelete() = 0 ;
		// Called just before destruction. (Note 
		// that the object typically deletes itself.)

	virtual void onData( const char * , size_t ) = 0 ;
		// Called on receipt of data.

	StreamSocket & socket() ;
		// Returns a reference to the client-server
		// connection socket.

	Server * server() ;
		// Returns a pointer to the associated server 
		// object. Returns NULL if the server has
		// been destroyed.

private:
	virtual void readEvent() ; // from EventHandler
	virtual void exceptionEvent() ; // from EventHandler
	ServerPeer( const ServerPeer & ) ; // not implemented
	void operator=( const ServerPeer & ) ; // not implemented

private:
	Address m_address ;
	std::auto_ptr<StreamSocket> m_socket ;
	ServerPeerHandle * m_handle ;
} ;

#endif
