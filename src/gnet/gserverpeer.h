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
/// \file gserverpeer.h
///

#ifndef G_NET_SERVER_PEER__H
#define G_NET_SERVER_PEER__H

#include "gdef.h"
#include "gsocket.h"
#include "gsocketprotocol.h"
#include "gexception.h"
#include "gaddress.h"
#include "glinebuffer.h"
#include "gtimer.h"
#include "gconnection.h"
#include "gexceptionsource.h"
#include "gevent.h"
#include <utility>
#include <memory>
#include <string>

namespace GNet
{
	class Server ;
	class ServerPeer ;
	class ServerPeerConfig ;
	class ServerPeerInfo ;
}

/// \class GNet::ServerPeerConfig
/// A structure that GNet::Server uses to configure its ServerPeer objects.
///
class GNet::ServerPeerConfig
{
public:
	unsigned int idle_timeout ;
	explicit ServerPeerConfig( unsigned int idle_timeout ) ;
} ;

/// \class GNet::ServerPeer
/// An abstract base class for the GNet::Server's connection to a remote
/// client. Instances are created on the heap by the Server::newPeer()
/// override. Exceptions are delivered to the owning Server and result
/// in a call to the relevant ServerPeer's onDelete() method followed
/// by its deletion.
/// \see GNet::Server, GNet::EventHandler
///
class GNet::ServerPeer : private EventHandler , public Connection , private SocketProtocolSink , public ExceptionSource
{
public:
	G_EXCEPTION( IdleTimeout , "idle timeout" ) ;

	ServerPeer( ExceptionSink , ServerPeerInfo , LineBufferConfig ) ;
		///< Constructor. This constructor is only used from within the
		///< override of GNet::Server::newPeer(). The ExceptionSink refers
		///< to the owning Server.

	virtual ~ServerPeer() ;
		///< Destructor.

	bool send( const std::string & data , size_t offset = 0U ) ;
		///< Sends data down the socket to the peer. Returns true if completely
		///< sent; returns false if flow control asserted (see onSendComplete()).
		///< If flow control is asserted then there should be no new calls to
		///< send() until onSendComplete() is triggered.
		///< Throws on error.

	bool send( const std::vector<std::pair<const char *,size_t> > & data ) ;
		///< Overload to send data using scatter-gather segments. If false is
		///< returned then segment data pointers must stay valid until
		///< onSendComplete() is triggered.

	virtual std::pair<bool,Address> localAddress() const override ;
		///< Returns the local address. Pair.first is false on error.
		///< Override from GNet::Connection.

	virtual std::pair<bool,Address> peerAddress() const override ;
		///< Returns the peer address.
		///< Override from GNet::Connection.

	virtual std::string connectionState() const override ;
		///< Returns the connection state display string.
		///< Override from GNet::Connection.

	virtual std::string peerCertificate() const override ;
		///< Returns the peer's TLS certificate.
		///< Override from GNet::Connection.

	void doOnDelete( const std::string & reason , bool done ) ;
		///< Used by the Server class to call onDelete().

	LineBufferState lineBuffer() const ;
		///< Returns information about the state of the internal
		///< line-buffer.

protected:
	virtual void onSendComplete() = 0 ;
		///< Called after flow-control has been released and all
		///< residual data sent.

	virtual bool onReceive( const char * data , size_t size , size_t eolsize , size_t linesize , char c0 ) = 0 ;
		///< Called on receipt of data. See GNet::LineBuffer.

	virtual void onDelete( const std::string & reason ) = 0 ;
		///< Called just before the Server deletes this ServerPeer as
		///< the result of an exception (but not as a result of Server
		///< destruction). The reason is the empty string if caused
		///< by a GNet::Done exception. Consider making the implementation
		///< non-throwing, in the spirit of a destructor, since the
		///< ServerPeer object is about to be deleted.

	void secureAccept() ;
		///< Waits for the peer to start a secure session. Uses a
		///< profile called "server"; see GSsl::Library::addProfile().
		///< The callback GNet::SocketProtocolSink::onSecure() is
		///< triggered when the secure session is established.

	StreamSocket & socket() ;
		///< Returns a reference to the client-server connection
		///< socket.

private: // overrides
	virtual void readEvent() override ; // Override from GNet::EventHandler.
	virtual void writeEvent() override ; // Override from GNet::EventHandler.
	virtual void otherEvent( EventHandler::Reason ) override ; // Override from GNet::EventHandler.

protected:
	virtual void onData( const char * , size_t ) override ;
		///< Override from GNet::SocketProtocolSink.
		///< Protected to allow derived classes to ignore
		///< incoming data for DoS prevention.

private:
	ServerPeer( const ServerPeer & ) g__eq_delete ;
	void operator=( const ServerPeer & ) g__eq_delete ;
	void onIdleTimeout() ;
	bool onDataImp( const char * , size_t , size_t , size_t , char ) ;

private:
	Address m_address ;
	shared_ptr<StreamSocket> m_socket ; // order dependency -- first
	SocketProtocol m_sp ; // order dependency -- second
	LineBuffer m_line_buffer ;
	ServerPeerConfig m_config ;
	Timer<ServerPeer> m_idle_timer ;
} ;

#endif
