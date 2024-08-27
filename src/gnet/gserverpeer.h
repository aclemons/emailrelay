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
/// \file gserverpeer.h
///

#ifndef G_NET_SERVER_PEER_H
#define G_NET_SERVER_PEER_H

#include "gdef.h"
#include "gsocket.h"
#include "gsocketprotocol.h"
#include "gexception.h"
#include "gaddress.h"
#include "glinebuffer.h"
#include "gtimer.h"
#include "gconnection.h"
#include "geventlogging.h"
#include "gexceptionsource.h"
#include "gevent.h"
#include "gstringview.h"
#include <utility>
#include <memory>
#include <string>

namespace GNet
{
	class Server ;
	class ServerPeer ;
	class ServerPeerInfo ;
}

//| \class GNet::ServerPeer
/// An abstract base class for the GNet::Server's connection to a remote
/// client. Instances are created on the heap by the Server::newPeer()
/// override. Exceptions thrown from event handlers are delivered to
/// the owning Server and result in a call to the relevant ServerPeer's
/// onDelete() method followed by its deletion. Every ServerPeer can
/// do line buffering, but line-buffering can be effectively disabled
/// by configuring the line buffer as transparent.
///
/// \see GNet::Server, GNet::EventHandler
///
class GNet::ServerPeer : private EventHandler , public Connection , private SocketProtocolSink , public ExceptionSource , private EventLogging
{
public:
	G_EXCEPTION( IdleTimeout , tx("idle timeout") )

	struct Config /// A configuration structure for GNet::ServerPeer.
	{
		SocketProtocol::Config socket_protocol_config ;
		unsigned int idle_timeout {0U} ;
		bool kick_idle_timer_on_send {false} ; // idle timeout when nothing received (false) or sent-or-received (true)
		bool no_throw_on_peer_disconnect {false} ; // see SocketProtocolSink::onPeerDisconnect()
		bool log_address {false} ;
		bool log_port {false} ;
		Config & set_socket_protocol_config( const SocketProtocol::Config & ) ;
		Config & set_idle_timeout( unsigned int ) noexcept ;
		Config & set_kick_idle_timer_on_send( bool = true ) noexcept ;
		Config & set_no_throw_on_peer_disconnect( bool = true ) noexcept ;
		Config & set_all_timeouts( unsigned int ) noexcept ;
		Config & set_log_address( bool = true ) noexcept ;
		Config & set_log_port( bool = true ) noexcept ;
	} ;

	ServerPeer( EventState , ServerPeerInfo && , const LineBuffer::Config & ) ;
		///< Constructor. This constructor is only used from within the
		///< override of GNet::Server::newPeer(). The EventState exception
		///< source refers to the owning Server.

	~ServerPeer() override ;
		///< Destructor.

	bool send( const std::string & data ) ;
		///< Sends data down the socket to the peer. Returns true if completely
		///< sent; returns false if flow control asserted (see onSendComplete()).
		///< If flow control is asserted then there should be no new calls to
		///< send() until onSendComplete() is triggered.
		///< Throws on error.

	bool send( std::string_view data ) ;
		///< Overload for string_view.

	bool send( const std::vector<std::string_view> & data , std::size_t offset = 0U ) ;
		///< Overload to send data using scatter-gather segments. If false is
		///< returned then segment data pointers must stay valid until
		///< onSendComplete() is triggered.

	Address localAddress() const override ;
		///< Returns the local address. Throws on error.
		///< Override from GNet::Connection.

	Address peerAddress() const override ;
		///< Returns the peer address. Throws on error.
		///< Override from GNet::Connection.

	std::string connectionState() const override ;
		///< Returns the connection state display string.
		///< Override from GNet::Connection.

	std::string peerCertificate() const override ;
		///< Returns the peer's TLS certificate.
		///< Override from GNet::Connection.

	LineBufferState lineBuffer() const ;
		///< Returns information about the state of the internal
		///< line-buffer.

	void setIdleTimeout( unsigned int seconds ) ;
		///< Sets the idle timeout.

	void doOnDelete( const std::string & reason , bool done ) ;
		///< Used by the GNet::Server class to call onDelete().

	static std::string eventLoggingString( const Address & , const Config & ) ;
		///< Assembles an event logging string for a new
		///< ServerPeer object.

protected:
	virtual void onSendComplete() = 0 ;
		///< Called after flow-control has been released and all
		///< residual data sent.

	virtual bool onReceive( const char * data , std::size_t size , std::size_t eolsize , std::size_t linesize , char c0 ) = 0 ;
		///< Called on receipt of a complete line of data.
		///< This method is the sink function for the internal
		///< GNet::LineBuffer. See GNet::LineBuffer::apply().

	virtual void onDelete( const std::string & reason ) = 0 ;
		///< Called just before the Server deletes this ServerPeer as
		///< the result of an exception (but not as a result of Server
		///< destruction). The reason is the empty string if caused
		///< by a GNet::Done exception. Consider making the implementation
		///< non-throwing, in the spirit of a destructor, since the
		///< ServerPeer object is about to be deleted.

	void secureAccept() ;
		///< Waits for the peer to start a secure session. Uses a
		///< profile called "server" by default; see GSsl::Library::addProfile().
		///< The callback GNet::SocketProtocolSink::onSecure() is
		///< triggered when the secure session is established.

	bool secureAcceptCapable() const ;
		///< Returns true if secureAccept() is usable.

	StreamSocket & socket() ;
		///< Returns a reference to the client-server connection
		///< socket.

	void dropReadHandler() ;
		///< Drops the socket() read handler.

	void addReadHandler() ;
		///< Re-adds the socket() read handler.

	void expect( std::size_t ) ;
		///< Modifies the line buffer state so that it delivers
		///< a chunk of non-line-delimited data.

	void finish() ;
		///< Does a socket shutdown(). See also GNet::Client::finish().

private: // overrides
	void readEvent() override ; // GNet::EventHandler
	void writeEvent() override ; // GNet::EventHandler
	void otherEvent( EventHandler::Reason ) override ; // GNet::EventHandler
	void onPeerDisconnect() override ; // GNet::SocketProtocolSink
	std::string_view eventLoggingString() const override ; // GNet::EventLogging

protected:
	void onData( const char * , std::size_t ) override ;
		///< Override from GNet::SocketProtocolSink.
		///<
		///< Protected and non-final to allow derived classes
		///< to ignore incoming data for DoS prevention.

public:
	ServerPeer( const ServerPeer & ) = delete ;
	ServerPeer( ServerPeer && ) = delete ;
	ServerPeer & operator=( const ServerPeer & ) = delete ;
	ServerPeer & operator=( ServerPeer && ) = delete ;
	bool send( const char * , std::size_t offset ) = delete ;
	bool send( const char * ) = delete ;
	bool send( const std::string & , std::size_t offset ) = delete ;

private:
	void onIdleTimeout() ;
	bool onDataImp( const char * , std::size_t , std::size_t , std::size_t , char ) ;

private:
	EventState m_es ;
	Address m_address ;
	std::unique_ptr<StreamSocket> m_socket ; // order dependency -- first
	SocketProtocol m_sp ; // order dependency -- second
	LineBuffer m_line_buffer ;
	Config m_config ;
	Timer<ServerPeer> m_idle_timer ;
	std::string m_event_logging_string ;
} ;

inline GNet::ServerPeer::Config & GNet::ServerPeer::Config::set_idle_timeout( unsigned int t ) noexcept { idle_timeout = t ; return *this ; }
inline GNet::ServerPeer::Config & GNet::ServerPeer::Config::set_kick_idle_timer_on_send( bool b ) noexcept { kick_idle_timer_on_send = b ; return *this ; }
inline GNet::ServerPeer::Config & GNet::ServerPeer::Config::set_all_timeouts( unsigned int t ) noexcept { idle_timeout = t ; socket_protocol_config.secure_connection_timeout = t ; return *this ; }
inline GNet::ServerPeer::Config & GNet::ServerPeer::Config::set_socket_protocol_config( const SocketProtocol::Config & config ) { socket_protocol_config = config ; return *this ; }
inline GNet::ServerPeer::Config & GNet::ServerPeer::Config::set_no_throw_on_peer_disconnect( bool b ) noexcept { no_throw_on_peer_disconnect = b ; return *this ; }
inline GNet::ServerPeer::Config & GNet::ServerPeer::Config::set_log_address( bool b ) noexcept { log_address = b ; return *this ; }
inline GNet::ServerPeer::Config & GNet::ServerPeer::Config::set_log_port( bool b ) noexcept { log_port = b ; return *this ; }

#endif
