//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#ifndef G_SERVER_H 
#define G_SERVER_H

#include "gdef.h"
#include "gnet.h"
#include "gsocket.h"
#include "gsocketprotocol.h"
#include "gtimer.h"
#include "gconnection.h"
#include "gconnectionlookup.h"
#include "gevent.h"
#include <utility>
#include <list>
#include <memory>
#include <string>

/// \namespace GNet
namespace GNet
{
	class Server ;
	class ServerPeer ;
	class ServerPeerTimer ;
	class ServerPeerHandle ;
}

/// \class GNet::ServerPeerHandle
/// A structure used in the implementation of GNet::Server.
/// The server holds a list of handles which refer to all its peer objects. 
/// When a peer object deletes itself it resets the handle, without changing 
/// the server's list. The server uses its list to delete all peer objects
/// from within its destructor. The server does garbage collection occasionally,
/// deleting handles which have been reset. 
///
class GNet::ServerPeerHandle 
{
public:
	ServerPeerHandle() ;
		///< Default constructor.

	void set( ServerPeer * p ) ;
		///< Sets the pointer.

	void reset() ;
		///< Resets the pointer.

	ServerPeer * peer() ;
		///< Returns the pointer.

	ServerPeer * old() ;
		///< Returns the pointer value before it was reset().
		///< Used in debugging.

private:
	ServerPeer * m_p ;
	ServerPeer * m_old ;
} ;

/// \class GNet::Server
/// A network server class which listens on a specific
/// port and spins off ServerPeer objects for each incoming connection.
/// \see GNet::ServerPeer
///
class GNet::Server : public GNet::EventHandler 
{
public:
	G_EXCEPTION( CannotBind , "cannot bind the listening port" ) ;
	G_EXCEPTION( CannotListen , "cannot listen" ) ;
	G_EXCEPTION( AcceptError , "socket accept() failed" ) ;

	/// A structure used in GNet::Server::newPeer().
	struct PeerInfo 
	{
		std::auto_ptr<StreamSocket> m_socket ;
		Address m_address ;
		std::string m_name ; // for local peers - not always available
		ServerPeerHandle * m_handle ;
		PeerInfo() ;
	} ;

	static bool canBind( const Address & listening_address , bool do_throw ) ;
		///< Checks that the specified address can be
		///< bound. Throws CannotBind if the address cannot 
		///< be bound and 'do_throw' is true.

	explicit Server( unsigned int listening_port , ConnectionLookup * = NULL ) ;
		///< Constructor taking a port number. The server
		///< listens on all local interfaces.

	explicit Server( const Address & listening_address , ConnectionLookup * = NULL ) ;
		///< Constructor. The server listens only on the
		///< specific (local) interface.

	Server() ;
		///< Default constructor. Initialise with init().

	void init( unsigned int listening_port ) ;
		///< Initilisation after default construction.

	void init( const Address & listening_address ) ;
		///< Initilisation after default construction.

	virtual ~Server() ;
		///< Destructor.

	std::pair<bool,Address> address() const ;
		///< Returns the listening address. Pair.first
		///< is false if not properly init()ialised.

	virtual void readEvent() ;
		///< Final override from GNet::EventHandler.

	virtual void writeEvent() ;
		///< Final override from GNet::EventHandler.

	virtual void onException( std::exception & e ) ;
		///< Final override from GNet::EventHandler.

protected:
	virtual ServerPeer * newPeer( PeerInfo ) = 0 ;
		///< A factory method which new()s a ServerPeer-derived 
		///< object. This method is called when a new connection
		///< comes into this server. The new ServerPeer object 
		///< is used to represent the state of the client/server 
		///< connection. 
		///<
		///< The new ServerPeer object manages its own lifetime 
		///< doing a "delete this" when the network connection 
		///< disappears. But the Server also deletes remaining
		///< peers during its destruction.
		///<
		///< The implementation should pass the 'PeerInfo' 
		///< parameter through to the ServerPeer base-class 
		///< constructor.
		///<
		///< Should return NULL for non-fatal errors. It is 
		///< expected that a typical server process will 
		///< terminate if newPeer() throws, so most 
		///< implementations will catch any exceptions and 
		///< return NULL.

	void serverCleanup() ;
		///< May be called from the derived class destructor
		///< in order to trigger early deletion of peer objects,
		///< before the derived part of the server disappears.
		///< If this is called from the most-derived Server
		///< class then it allows ServerPeer objects to use 
		///< their Server object safely during destruction.

private:
	Server( const Server & ) ; // not implemented
	void operator=( const Server & ) ; // not implemented
	void serverCleanupCore() ;
	void collectGarbage() ;
	void accept( PeerInfo & ) ;

private:
	typedef std::list<ServerPeerHandle> PeerList ;
	std::auto_ptr<StreamSocket> m_socket ;
	ConnectionLookup * m_connection_lookup ;
	PeerList m_peer_list ;
	bool m_cleaned_up ;
} ;

/// \class GNet::ServerPeer
/// An abstract base class for the GNet::Server's
/// connection to a remote client. Instances are created
/// on the heap by the Server::newPeer() override, and they 
/// delete themselves when the connection is lost.
/// \see GNet::Server, GNet::EventHandler
///
class GNet::ServerPeer : public GNet::EventHandler , public GNet::Connection , public GNet::SocketProtocolSink 
{
public:
	typedef std::string::size_type size_type ;

	explicit ServerPeer( Server::PeerInfo ) ;
		///< Constructor. This constructor is only used from within the 
		///< override of GServer::newPeer().

	bool send( const std::string & data , std::string::size_type offset = 0U ) ;
		///< Sends data down the socket to the peer. Returns 
		///< false if flow control asserted (see onSendComplete()). 
		///< Throws on error.

	void doDelete( const std::string & = std::string() ) ; 
		///< Does "onDelete(); delete this".

	std::string logId() const ;
		///< Returns an identification string for logging purposes.

	virtual std::pair<bool,Address> localAddress() const ;
		///< Returns the local address. Pair.first is false on error.
		///< Final override from GNet::Connection.

	virtual std::pair<bool,Address> peerAddress() const ;
		///< Returns the peer address.
		///< Final override from GNet::Connection.

	virtual std::string peerCertificate() const ;
		///< Returns the peer's TLS certificate.
		///< Final override from GNet::Connection.

	virtual void readEvent() ; 
		///< Final override from GNet::EventHandler.

	virtual void writeEvent() ;
		///< Final override from GNet::EventHandler.

	virtual void onException( std::exception & ) ;
		///< Final override from GNet::EventHandler.

	void doDeleteThis( int ) ;
		///< Does delete this. Should only be used by the GNet::Server class.

protected:
	virtual ~ServerPeer() ;
		///< Destructor. Note that objects will delete themselves 
		///< when they detect that the connection has been 
		///< lost -- see doDelete().

	virtual void onDelete( const std::string & reason ) = 0 ;
		///< Called just before destruction. (Note that the
		///< object typically deletes itself.)

    virtual void onSendComplete() = 0 ;
        ///< Called after flow-control has been released and all
        ///< residual data sent.
        ///<
        ///< If an exception is thrown in the override then this
        ///< object catches it and deletes iteself by calling
        ///< doDelete().

	void sslAccept() ;
		///< Waits for the peer to start a secure session.
		///< See also GNet::SocketProtocolSink::onSecure().

	StreamSocket & socket() ;
		///< Returns a reference to the client-server connection 
		///< socket.

	Server * server() ;
		///< Returns a pointer to the associated server object. 
		///< Returns NULL if the server has been destroyed.

private:
	ServerPeer( const ServerPeer & ) ; // not implemented
	void operator=( const ServerPeer & ) ; // not implemented
	void onTimeout() ;

private:
	Address m_address ;
	std::auto_ptr<StreamSocket> m_socket ; // order dependency -- first
	SocketProtocol m_sp ; // order dependency -- second
	ServerPeerHandle * m_handle ;
	Timer<ServerPeer> m_delete_timer ;
} ;

#endif
