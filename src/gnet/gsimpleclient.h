//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gsimpleclient.h
///

#ifndef G_NET_SIMPLE_CLIENT__H
#define G_NET_SIMPLE_CLIENT__H

#include "gdef.h"
#include "gaddress.h"
#include "gsocks.h"
#include "gconnection.h"
#include "gexception.h"
#include "geventhandler.h"
#include "gresolver.h"
#include "glocation.h"
#include "gtimer.h"
#include "gsocket.h"
#include "gsocketprotocol.h"
#include "gevent.h"
#include <string>

namespace GNet
{
	class SimpleClient ;
}

/// \class GNet::SimpleClient
/// A class for making an outgoing connection to a remote server, with
/// support for socket-level protocols such as TLS/SSL and SOCKS 4a.
///
/// The class handles name-to-address resolution, deals with
/// connection issues, reads incoming data, and manages flow-control
/// when sending. The implementation uses the SocketProtocol class
/// in order to do TLS/SSL; see secureConnect().
///
/// Name-to-address lookup is performed if the supplied Location
/// object does not contain an address. This can be done synchronously
/// or asynchronously. The results of the lookup can be obtained via
/// the remoteLocation() method and possibly fed back to the next
/// SimpleClient that connects to the same host/service in order to
/// implement name lookup cacheing (see GNet::ClientPtr). However,
/// most operating systems implement their own name lookup cacheing,
/// so this is not terribly useful in practice.
///
class GNet::SimpleClient : public EventHandler, public Connection, private SocketProtocolSink, private Resolver::Callback
{
public:
	enum ConnectStatus { Success , Failure , ImmediateSuccess } ;
    enum State { Idle , Resolving , Connecting , Connected , Socksing , Testing } ;
	G_EXCEPTION( DnsError , "dns error" ) ;
	G_EXCEPTION( ConnectError , "connect failure" ) ;
	G_EXCEPTION( NotConnected , "socket not connected" ) ;
	typedef std::string::size_type size_type ;

	SimpleClient( ExceptionHandler & ,
		const Location & remote_info , bool bind_local_address = false ,
		const Address & local_address = Address::defaultAddress() ,
		bool sync_dns = synchronousDnsDefault() ,
		unsigned int secure_connection_timeout = 0U ) ;
			///< Constructor. Call connect() to start connecting.

	void connect() ;
		///< Initates a connection to the remote server. Calls back
		///< to onConnect() when complete (non-reentrantly). Throws
		///< on error.

	bool connected() const ;
		///< Returns true if connected to the peer.

	virtual std::pair<bool,Address> localAddress() const override ;
		///< Override from Connection. Returns the local
		///< address. Pair.first is false on error.
		///< Override from GNet::Connection.

	virtual std::pair<bool,Address> peerAddress() const override ;
		///< Override from Connection. Returns the peer
		///< address. Pair.first is false on error.
		///< Override from GNet::Connection.

	virtual std::string connectionState() const override ;
		///< Returns the connection state display string.
		///< Override from GNet::Connection.

	virtual std::string peerCertificate() const override ;
		///< Returns the peer's TLS certificate.
		///< Override from GNet::Connection.

	static bool synchronousDnsDefault() ;
		///< Returns true if DNS queries should normally be
		///< synchronous on this platform. Used to default the
		///< relevant constructor parameter.

	Location remoteLocation() const ;
		///< Returns a Location structure containing the
		///< result of host() and service() name lookup if
		///< available.

	void updateLocation( const Location & ) ;
		///< Updates the constructor's Location object with
		///< the given one as long as both objects have the
		///< same host and service name. This is only useful
		///< immediately after construction and before
		///< re-entering the event loop.

	virtual void readEvent() override ;
		///< Override from GNet::EventHandler.

	virtual void writeEvent() override ;
		///< Override from GNet::EventHandler.

	virtual void otherEvent( EventHandler::Reason ) override ;
		///< Override from GNet::EventHandler.

	bool send( const std::string & data , std::string::size_type offset = 0 ) ;
		///< Sends data to the peer. Returns true if all sent,
		///< or false if flow control was asserted. Throws on
		///< error.

protected:
	virtual ~SimpleClient() ;
		///< Destructor.

	StreamSocket & socket() ;
		///< Returns a reference to the socket. Throws if not connected.

	const StreamSocket & socket() const ;
		///< Returns a const reference to the socket. Throws if not connected.

	virtual void onConnect() = 0 ;
		///< Called once connected.

	virtual void onConnectImp() ;
		///< An alternative to onConnect() for private implementation
		///< classes. The default implementation does nothing.

	virtual void onSendComplete() = 0 ;
		///< Called when all residual data from send() has been sent.

	virtual void onSendImp() ;
		///< Called from within send().

	void secureConnect() ;
		///< Starts TLS/SSL client-side negotiation. Uses a profile
		///< called "client"; see GSsl::Library::addProfile().
		///< The callback GNet::SocketProtocolSink::onSecure() is
		///< triggered when the secure session is established.

	static bool connectError( const std::string & reason ) ;
		///< Returns true if the reason string implies the
		///< SimpleClient::connect() failed.

	std::string logId() const ;
		///< Returns a identification string for logging purposes.
		///< Not guaranteed to stay the same during the lifetime
		///< of the object.

private:
	SimpleClient( const SimpleClient& ) ; // not implemented
	void operator=( const SimpleClient& ) ; // not implemented
	void close() ;
	void startConnecting() ;
	void bindLocalAddress( const Address & ) ;
	void setState( State ) ;
	void logFlowControlAsserted() const ;
	void logFlowControlReleased() const ;
	virtual void onResolved( std::string , Location ) override ; // Override from GNet::Resolver.
	void onConnectTimer() ;
	void onWriteable() ;

private:
	ExceptionHandler & m_eh ;
	unique_ptr<StreamSocket> m_socket ;
	unique_ptr<SocketProtocol> m_sp ;
	unique_ptr<Socks> m_socks ;
	Resolver m_resolver ;
	Location m_remote_location ;
	bool m_bind_local_address ;
	Address m_local_address ;
	State m_state ;
	bool m_sync_dns ;
	unsigned int m_secure_connection_timeout ;
	Timer<SimpleClient> m_on_connect_timer ; // zero-length timer
} ;

#endif
