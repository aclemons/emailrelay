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
/// \file gclient.h
///

#ifndef G_NET_CLIENT_H
#define G_NET_CLIENT_H

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
#include "gstringview.h"
#include "gcall.h"
#include "gtimer.h"
#include "gsocket.h"
#include "gsocketprotocol.h"
#include "gevent.h"
#include "gslot.h"
#include "gstr.h"
#include <string>
#include <memory>

namespace GNet
{
	class Client ;
}

//| \class GNet::Client
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
	G_EXCEPTION( DnsError , tx("dns error") ) ;
	G_EXCEPTION( ConnectError , tx("connect failure") ) ;
	G_EXCEPTION( NotConnected , tx("socket not connected") ) ;
	G_EXCEPTION( ResponseTimeout , tx("response timeout") ) ;
	G_EXCEPTION( IdleTimeout , tx("idle timeout") ) ;

	struct Config /// A structure containing GNet::Client configuration parameters.
	{
		LineBufferConfig line_buffer_config {LineBufferConfig::transparent()} ;
		SocketProtocol::Config socket_protocol_config ; // inc. secure_connection_timeout
		Address local_address {Address::defaultAddress()} ;
		bool sync_dns {false} ;
		bool auto_start {true} ;
		bool bind_local_address {false} ;
		unsigned int connection_timeout {0U} ;
		unsigned int response_timeout {0U} ;
		unsigned int idle_timeout {0U} ;

		Config & set_line_buffer_config( const LineBufferConfig & ) ;
		Config & set_socket_protocol_config( const SocketProtocol::Config & ) ;
		Config & set_sync_dns( bool = true ) ;
		Config & set_auto_start( bool = true ) ;
		Config & set_bind_local_address( bool = true ) ;
		Config & set_local_address( const Address & ) ;
		Config & set_connection_timeout( unsigned int ) ;
		Config & set_secure_connection_timeout( unsigned int ) ;
		Config & set_response_timeout( unsigned int ) ;
		Config & set_idle_timeout( unsigned int ) ;
		Config & set_all_timeouts( unsigned int ) ;
	} ;

	Client( ExceptionSink , const Location & remote_location , const Config & ) ;
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

	Address localAddress() const override ;
		///< Returns the local address.
		///< Override from GNet::Connection.

	Address peerAddress() const override ;
		///< Returns the peer address. Throws if not connected().
		///< Override from GNet::Connection.

	std::string connectionState() const override ;
		///< Returns the connection state display string.
		///< Override from GNet::Connection.

	std::string peerCertificate() const override ;
		///< Returns the peer's TLS certificate.
		///< Override from GNet::Connection.

	Location remoteLocation() const ;
		///< Returns a Location structure, including the result of
		///< name lookup if available.

	bool send( const std::string & data ) ;
		///< Sends data to the peer and starts the response
		///< timer (if configured). Returns true if all sent.
		///< Returns false if flow control was asserted, in which
		///< case the unsent portion is copied internally and
		///< onSendComplete() called when complete. Throws on error.

	bool send( G::string_view data ) ;
		///< Overload for string_view.

	bool send( const std::vector<G::string_view> & data , std::size_t offset = 0 ) ;
		///< Overload for scatter/gather segments.

	G::Slot::Signal<const std::string&,const std::string&,const std::string&> & eventSignal() noexcept ;
		///< Returns a signal that indicates that something interesting
		///< has happened. The first signal parameter is one of
		///< "resolving", "connecting", or "connected", but other
		///< classes may inject the own events into this channel.

	void doOnDelete( const std::string & reason , bool done ) ;
		///< Called by ClientPtr (or equivalent) to call onDelete(),
		///< just before this client object is deleted.

	bool finished() const ;
		///< Returns true if finish()ed or disconnect()ed.

	LineBufferState lineBuffer() const ;
		///< Returns information about the state of the internal
		///< line-buffer.

	~Client() override ;
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

	virtual bool onReceive( const char * data , std::size_t size , std::size_t eolsize , std::size_t linesize , char c0 ) = 0 ;
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
	void readEvent( Descriptor ) override ; // Override from GNet::EventHandler.
	void writeEvent( Descriptor ) override ; // Override from GNet::EventHandler.
	void otherEvent( Descriptor , EventHandler::Reason ) override ; // Override from GNet::EventHandler.
	void onResolved( std::string , Location ) override ; // Override from GNet::Resolver.
	void onData( const char * , std::size_t ) override ; // Override from GNet::SocketProtocolSink.

public:
	Client( const Client & ) = delete ;
	Client( Client && ) = delete ;
	void operator=( const Client & ) = delete ;
	void operator=( Client && ) = delete ;
	bool send( const char * , std::size_t ) = delete ;
	bool send( const char * ) = delete ;
	bool send( const std::string & , std::size_t ) = delete ;

private:
	enum class State
	{
		Idle ,
		Resolving ,
		Connecting ,
		Connected ,
		Socksing ,
		Disconnected ,
		Testing
	} ;
	bool onDataImp( const char * , std::size_t , std::size_t , std::size_t , char ) ;
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
	std::unique_ptr<StreamSocket> m_socket ;
	std::unique_ptr<SocketProtocol> m_sp ;
	std::unique_ptr<Socks> m_socks ;
	LineBuffer m_line_buffer ;
	std::unique_ptr<Resolver> m_resolver ;
	Location m_remote_location ;
	bool m_bind_local_address ;
	Address m_local_address ;
	SocketProtocol::Config m_socket_protocol_config ; // secure_connection_timeout
	bool m_sync_dns ;
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
	G::Slot::Signal<const std::string&,const std::string&,const std::string&> m_event_signal ;
} ;

inline GNet::Client::Config & GNet::Client::Config::set_line_buffer_config( const LineBufferConfig & cfg ) { line_buffer_config = cfg ; return *this ; }
inline GNet::Client::Config & GNet::Client::Config::set_socket_protocol_config( const SocketProtocol::Config & cfg ) { socket_protocol_config = cfg ; return *this ; }
inline GNet::Client::Config & GNet::Client::Config::set_sync_dns( bool b ) { sync_dns = b ; return *this ; }
inline GNet::Client::Config & GNet::Client::Config::set_auto_start( bool b ) { auto_start = b ; return *this ; }
inline GNet::Client::Config & GNet::Client::Config::set_bind_local_address( bool b ) { bind_local_address = b ; return *this ; }
inline GNet::Client::Config & GNet::Client::Config::set_local_address( const Address & a ) { local_address = a ; return *this ; }
inline GNet::Client::Config & GNet::Client::Config::set_connection_timeout( unsigned int t ) { connection_timeout = t ; return *this ; }
inline GNet::Client::Config & GNet::Client::Config::set_secure_connection_timeout( unsigned int t ) { socket_protocol_config.secure_connection_timeout = t ; return *this ; }
inline GNet::Client::Config & GNet::Client::Config::set_response_timeout( unsigned int t ) { response_timeout = t ; return *this ; }
inline GNet::Client::Config & GNet::Client::Config::set_idle_timeout( unsigned int t ) { idle_timeout = t ; return *this ; }

#endif
