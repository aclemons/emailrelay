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
/// \file gsocketprotocol.h
///

#ifndef G_NET_SOCKET_PROTOCOL_H
#define G_NET_SOCKET_PROTOCOL_H

#include "gdef.h"
#include "gsocket.h"
#include "geventhandler.h"
#include "gexception.h"
#include "gstringview.h"
#include "glimits.h"
#include <string>
#include <memory>
#include <utility>
#include <vector>

namespace GNet
{
	class SocketProtocol ;
	class SocketProtocolImp ;
	class SocketProtocolSink ;
}

//| \class GNet::SocketProtocol
/// An interface for implementing a low-level TLS/SSL protocol layer on top
/// of a connected non-blocking socket.
///
/// Provides send() to send data, and onData() in a callback interface to
/// receive data. The TLS/SSL socket protocol session is negotiated with the
/// peer by calling secureConnect() or secureAccept(), and thereafter the
/// interface is half-duplex. If no TLS/SSL session is in effect ('raw') then
/// the protocol layer is transparent down to the socket.
///
/// The interface has read-event and write-event handlers that should be
/// called when events are detected on the socket file descriptor. In raw
/// mode the read handler delivers data via the onData() callback interface
/// and the write handler is used to flush the output pipeline.
///
class GNet::SocketProtocol
{
public:
	using Sink = SocketProtocolSink ;
	G_EXCEPTION_CLASS( ReadError , tx("peer disconnected") ) ;
	G_EXCEPTION( SendError , tx("peer disconnected") ) ;
	G_EXCEPTION( ShutdownError , tx("shutdown error") ) ;
	G_EXCEPTION( SecureConnectionTimeout , tx("secure connection timeout") ) ;
	G_EXCEPTION( Shutdown , tx("peer shutdown") ) ;
	G_EXCEPTION( OtherEventError , tx("network event") ) ;
	G_EXCEPTION( ProtocolError , tx("socket protocol error") ) ;

	struct Config /// A configuration structure for GNet::SocketProtocol.
	{
		std::size_t read_buffer_size {G::Limits<>::net_buffer} ;
		unsigned int secure_connection_timeout {0U} ;
		Config & set_read_buffer_size( std::size_t n ) { read_buffer_size = n ; return *this ; }
		Config & set_secure_connection_timeout( unsigned int t ) { secure_connection_timeout = t ; return *this ; }
	} ;

	SocketProtocol( EventHandler & , ExceptionSink , Sink & ,
		StreamSocket & , const Config & ) ;
			///< Constructor.

	~SocketProtocol() ;
		///< Destructor.

	void readEvent() ;
		///< Called on receipt of a read event. Delivers data via the sink
		///< interface. Throws ReadError on error.

	bool writeEvent() ;
		///< Called on receipt of a write event. Sends more pending data
		///< down the connection. Returns true if all pending data was
		///< sent. Throws SendError on error.

	void otherEvent( EventHandler::Reason ) ;
		///< Called on receipt of an 'other' event. Throws an exception.
		///< For simple socket-close events (on Windows) the read queue
		///< is processed (see SocketProtocolSink::onData()) and the
		///< socket is shutdown() before the exception is thrown.

	bool send( const std::string & data , std::size_t offset ) ;
		///< Sends data. Returns false if flow control asserted before
		///< all the data is sent. Returns true if all the data was sent,
		///< or if the data passed in (taking the offset into account)
		///< is empty. Throws SendError on error.
		///<
		///< If flow control is asserted then the socket write-event
		///< handler is installed and send() returns false. Unsent
		///< portions of the data string are copied internally. When
		///< the subsequent write-event is triggered the user should
		///< call writeEvent(). There should be no new calls to send()
		///< until writeEvent() returns true.

	bool send( G::string_view data ) ;
		///< Overload for string_view.

	bool send( const std::vector<G::string_view> & data , std::size_t offset = 0U ) ;
		///< Overload to send data using scatter-gather segments.
		///< In this overload any unsent residue is not copied
		///< and the segment pointers must stay valid until
		///< writeEvent() returns true.

	void shutdown() ;
		///< Initiates a TLS-close if secure, together with a
		///< Socket::shutdown(1).

	static bool secureConnectCapable() ;
		///< Returns true if the implementation supports TLS/SSL and a
		///< "client" profile has been configured. See also GSsl::enabledAs().

	void secureConnect() ;
		///< Initiates the TLS/SSL handshake, acting as a client.
		///< Any send() data blocked by flow control is discarded.

	static bool secureAcceptCapable() ;
		///< Returns true if the implementation supports TLS/SSL and a
		///< "server" profile has been configured. See also GSsl::enabledAs().

	void secureAccept() ;
		///< Waits for the TLS/SSL handshake protocol, acting as a server.
		///< Any send() data blocked by flow control is discarded.

	bool secure() const ;
		///< Returns true if the connection is currently secure, ie. after
		///< onSecure().

	std::string peerCertificate() const ;
		///< Returns the peer's TLS/SSL certificate or the empty
		///< string.

public:
	SocketProtocol( const SocketProtocol & ) = delete ;
	SocketProtocol( SocketProtocol && ) = delete ;
	void operator=( const SocketProtocol & ) = delete ;
	void operator=( SocketProtocol && ) = delete ;

private:
	std::unique_ptr<SocketProtocolImp> m_imp ;
} ;

//| \class GNet::SocketProtocolSink
/// An interface used by GNet::SocketProtocol to deliver data
/// from a socket.
///
class GNet::SocketProtocolSink
{
public:
	virtual ~SocketProtocolSink() = default ;
		///< Destructor.

	virtual void onData( const char * , std::size_t ) = 0 ;
		///< Called when data is read from the socket.

	virtual void onSecure( const std::string & peer_certificate ,
		const std::string & protocol , const std::string & cipher ) = 0 ;
			///< Called once the secure socket protocol has
			///< been successfully negotiated.
} ;

#endif
