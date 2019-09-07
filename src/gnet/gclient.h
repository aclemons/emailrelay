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
/// \file gclient.h
///

#ifndef G_NET_CLIENT__H
#define G_NET_CLIENT__H

#include "gdef.h"
#include "gaddress.h"
#include "gsocks.h"
#include "gconnection.h"
#include "gexception.h"
#include "gexceptionsource.h"
#include "geventhandler.h"
#include "gresolver.h"
#include "glocation.h"
#include "glinebuffer.h"
#include "gcall.h"
#include "gtimer.h"
#include "gsocket.h"
#include "gsocketprotocol.h"
#include "gevent.h"
#include "gslot.h"
#include "gstr.h"
#include <string>

namespace GNet
{
	class Client ;
}

/// \class GNet::Client
/// A class for making an outgoing connection to a remote server, with support
/// for socket-level protocols such as TLS/SSL and SOCKS 4a.
///
/// The class handles name-to-address resolution, deals with connection issues,
/// reads incoming data, and manages flow-control when sending. The
/// implementation uses the SocketProtocol class in order to do TLS/SSL;
/// see secureConnect().
///
/// Name-to-address lookup is performed if the supplied Location object does
/// not contain an address. This can be done synchronously or asynchronously.
/// The results of the lookup can be obtained via the remoteLocation()
/// method and possibly fed back to the next Client that connects to the
/// same host/service in order to implement name lookup cacheing.
///
/// Received data is delivered through a virtual method onReceive(), with
/// optional line-buffering.
///
/// Clients should normally be instantiated on the heap and managed by a
/// ClientPtr so that the onDelete() mechanism works as advertised: the
/// ExceptionHandler passed to the Client constructor via the ExceptionSink
/// object should be the ClientPtr instance. Clients that decide to
/// terminate themselves cleanly should call Client::finish() and then throw
/// a GNet::Done exception.
///
class GNet::Client : private EventHandler, public Connection, private SocketProtocolSink, private Resolver::Callback , public ExceptionSource
{
public:
	G_EXCEPTION( DnsError , "dns error" ) ;
	G_EXCEPTION( ConnectError , "connect failure" ) ;
	G_EXCEPTION( NotConnected , "socket not connected" ) ;
	G_EXCEPTION( ResponseTimeout , "response timeout" ) ;
	G_EXCEPTION( IdleTimeout , "idle timeout" ) ;

	struct Config /// A structure containing GNet::Client configuration parameters.
	{
		Config() ;
		explicit Config( LineBufferConfig ) ;
		Config( LineBufferConfig , unsigned int all_timeouts ) ;
		Config( LineBufferConfig , unsigned int connection_timeout ,
			unsigned int secure_connection_timeout , unsigned int response_timeout , unsigned int idle_timeout ) ;
		Config & setTimeouts( unsigned int all_timeouts ) ;
		Config & setAutoStart( bool auto_start ) ;
		bool sync_dns ;
		bool auto_start ;
		bool bind_local_address ;
		Address local_address ;
		unsigned int connection_timeout ;
		unsigned int secure_connection_timeout ;
		unsigned int response_timeout ;
		unsigned int idle_timeout ;
		LineBufferConfig line_buffer_config ;
	} ;

	Client( ExceptionSink , const Location & remote_location , Config ) ;
		///< Constructor. If not auto-starting then connect()
		///< is required to start connecting. The ExceptionSink
		///< should delete this Client object when an exception is
		///< delivered to it, otherwise the the underlying socket
		///< might continue to raise events.

	void connect() ;
		///< Initiates a connection to the remote server. Calls back
		///< to onConnect() when complete (non-reentrantly). Throws
		///< on immediate failure.

	bool connected() const ;
		///< Returns true if connected to the peer.

	bool hasConnected() const ;
		///< Returns true if ever connected().

	void disconnect() ;
		///< Aborts the connection and destroys the object's internal
		///< state, resulting in a zombie object. After disconnect()
		///< only calls to hasConnected(), finished() and the dtor
		///< are allowed.

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

	Location remoteLocation() const ;
		///< Returns a Location structure, including the result of
		///< name lookup if available.

	bool send( const std::string & data , size_t offset = 0 ) ;
		///< Sends data to the peer and starts the response
		///< timer (if configured). Returns true if all sent.
		///< Returns false if flow control was asserted, in which
		///< case the unsent portion is copied internally and
		///< onSendComplete() called when complete. Throws on error.

	G::Slot::Signal3<std::string,std::string,std::string> & eventSignal() ;
		///< Returns a signal that indicates that something interesting
		///< has happened. The first signal parameter is one of
		///< "resolving", "connecting", or "connected", but other
		///< classes may inject the own events into this channel.

	void doOnDelete( const std::string & reason , bool done ) ;
		///< Called by ClientPtr (or equivalent) when handling an
		///< exception, just before the Client is deleted,
		///< triggering onDelete().

	bool finished() const ;
		///< Returns true if finish()ed or disconnect()ed.

	LineBufferState lineBuffer() const ;
		///< Returns information about the state of the internal
		///< line-buffer.

	virtual ~Client() ;
		///< Destructor.

protected:
	StreamSocket & socket() ;
		///< Returns a reference to the socket. Throws if not connected.

	const StreamSocket & socket() const ;
		///< Returns a const reference to the socket. Throws if not connected.

	void finish( bool with_socket_shutdown ) ;
		///< Indicates that the last data has been sent and the client
		///< is expecting a peer disconnect. Any subsequent onDelete()
		///< callback from doOnDelete() will have an empty reason
		///< string.

	void clearInput() ;
		///< Clears the input LineBuffer and cancels the response
		///< timer if running.

	virtual bool onReceive( const char * data , size_t size , size_t eolsize , size_t linesize , char c0 ) = 0 ;
		///< Called with received data. If configured with no line
		///< buffering then only the first two parameters are
		///< relevant. The implementation should return false if
		///< it needs to stop further onReceive() calls being
		///< generated from data already received and buffered.

	virtual void onConnect() = 0 ;
		///< Called once connected.

	virtual void onSendComplete() = 0 ;
		///< Called when all residual data from send() has been sent.

	virtual void onDelete( const std::string & reason ) = 0 ;
		///< Called just before ClientPtr destroys the Client as the
		///< result of handling an exception. The reason is the empty
		///< string if caused by a GNet::Done exception, or after
		///< finish() or disconnect(). Consider making the
		///< implementation non-throwing, in the spirit of a
		///< destructor, since the Client object is about to be
		///< deleted.

	void secureConnect() ;
		///< Starts TLS/SSL client-side negotiation. Uses a profile
		///< called "client"; see GSsl::Library::addProfile().
		///< The callback GNet::SocketProtocolSink::onSecure() is
		///< triggered when the secure session is established.

private: // overrides
	virtual void readEvent() override ; // Override from GNet::EventHandler.
	virtual void writeEvent() override ; // Override from GNet::EventHandler.
	virtual void otherEvent( EventHandler::Reason ) override ; // Override from GNet::EventHandler.
	virtual void onResolved( std::string , Location ) override ; // Override from GNet::Resolver.
	virtual void onData( const char * , size_t ) override ; // Override from GNet::SocketProtocolSink.

private:
	g__enum(State) { Idle , Resolving , Connecting , Connected , Socksing , Disconnected , Testing } ; g__enum_end(State)
	Client( const Client& ) g__eq_delete ;
	void operator=( const Client& ) g__eq_delete ;
	bool onDataImp( const char * , size_t , size_t , size_t , char ) ;
	void emit( const std::string & ) ;
	void startConnecting() ;
	void bindLocalAddress( const Address & ) ;
	void setState( State ) ;
	void onStartTimeout() ;
	void onConnectTimeout() ;
	void onConnectedTimeout() ;
	void onResponseTimeout() ;
	void onIdleTimeout() ;
	void onWriteable() ;
	void doOnConnect() ;

private:
	ExceptionSink m_es ;
	G::CallStack m_call_stack ;
	unique_ptr<StreamSocket> m_socket ;
	unique_ptr<SocketProtocol> m_sp ;
	unique_ptr<Socks> m_socks ;
	LineBuffer m_line_buffer ;
	unique_ptr<Resolver> m_resolver ;
	Location m_remote_location ;
	bool m_bind_local_address ;
	Address m_local_address ;
	bool m_sync_dns ;
	unsigned int m_secure_connection_timeout ;
	unsigned int m_connection_timeout ;
	unsigned int m_response_timeout ;
	unsigned int m_idle_timeout ;
	State m_state ;
	bool m_finished ;
	bool m_has_connected ;
	Timer<Client> m_start_timer ;
	Timer<Client> m_connect_timer ;
	Timer<Client> m_connected_timer ;
	Timer<Client> m_response_timer ;
	Timer<Client> m_idle_timer ;
	G::Slot::Signal3<std::string,std::string,std::string> m_event_signal ;
} ;

#endif
