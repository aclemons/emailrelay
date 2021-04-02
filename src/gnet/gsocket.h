//
// Copyright (C) 2001-2021 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gsocket.h
///

#ifndef G_NET_SOCKET_H
#define G_NET_SOCKET_H

#include "gdef.h"
#include "gaddress.h"
#include "gexceptionsink.h"
#include "gevent.h"
#include "gdescriptor.h"
#include "greadwrite.h"
#include <string>
#include <new>

namespace GNet
{
	class SocketBase ;
	class Socket ;
	class SocketProtocol ;
	class StreamSocket ;
	class DatagramSocket ;
	class RawSocket ;
	class AcceptPair ;
}

//| \class GNet::SocketBase
/// A socket base class that holds a non-blocking socket file descriptor and
/// interfaces to the event loop.
///
class GNet::SocketBase : public G::ReadWrite
{
public:
	G_EXCEPTION( SocketError , "socket error" ) ;
	G_EXCEPTION_CLASS( SocketCreateError , "socket create error" ) ;
	G_EXCEPTION_CLASS( SocketBindError , "socket bind error" ) ;
	G_EXCEPTION_CLASS( SocketTooMany , "socket accept error" ) ;
	using size_type = G::ReadWrite::size_type ;
	using ssize_type = G::ReadWrite::ssize_type ;
	struct Accepted /// Overload discriminator class for GNet::Socket.
		{} ;

	static bool supports( int domain , int type , int protocol ) ;
		///< Returns true if sockets can be created with the
		///< given parameters.

	~SocketBase() override ;
		///< Destructor. The socket file descriptor is closed and
		///< removed from the event loop.

	SOCKET fd() const noexcept override ;
		///< Returns the socket descriptor. Override from G::ReadWrite.

	bool eWouldBlock() const override ;
		///< Returns true if the previous socket operation
		///< failed because the socket would have blocked.
		///< Override from G::ReadWrite.

	bool eInProgress() const ;
		///< Returns true if the previous socket operation
		///< failed with the EINPROGRESS error status.
		///< When connecting this can be considered a
		///< non-error.

	bool eMsgSize() const ;
		///< Returns true if the previous socket operation
		///< failed with the EMSGSIZE error status. When
		///< writing to a datagram socket this indicates that
		///< the message was too big to send atomically.

	bool eTooMany() const ;
		///< Returns true if the previous socket operation
		///< failed with the EMFILE error status, or similar.

	bool eNotConn() const ;
		///< Returns true if the previous socket operation
		///< failed with the ENOTCONN error status, or similar.

	void addReadHandler( EventHandler & , ExceptionSink ) ;
		///< Adds this socket to the event source list so that
		///< the given handler receives read events.

	void dropReadHandler() noexcept ;
		///< Reverses addReadHandler().

	void addWriteHandler( EventHandler & , ExceptionSink ) ;
		///< Adds this socket to the event source list so that
		///< the given handler receives write events when flow
		///< control is released. (Not used for datagram
		///< sockets.)

	void dropWriteHandler() noexcept ;
		///< Reverses addWriteHandler().

	void addOtherHandler( EventHandler & , ExceptionSink ) ;
		///< Adds this socket to the event source list so that
		///< the given handler receives exception events.
		///< A TCP exception event should be treated as a
		///< disconnection event. (Not used for datagram
		///< sockets.)

	void dropOtherHandler() noexcept ;
		///< Reverses addOtherHandler().

	std::string asString() const ;
		///< Returns the socket handle as a string.
		///< Only used in debugging.

	std::string reason() const ;
		///< Returns the reason for the previous error.

protected:
	SocketBase( int domain , int type , int protocol ) ;
		///< Constructor used by derived classes. Creates the
		///< socket using socket().

	SocketBase( int domain , Descriptor s , const Accepted & ) ;
		///< Constructor which creates a socket object from
		///< a socket handle from accept(). Used only by
		///< StreamSocket::accept().

	ssize_type writeImp( const char * buf , size_type len ) ;
		///< Writes to the socket. This is a default implementation
		///< for write() that can be called from derived classes'
		///< overrides.

protected:
	static std::string reasonString( int ) ;
	static bool error( int rc ) ;
	static bool sizeError( ssize_type size ) ;
	bool create( int , int , int ) ;
	void clearReason() ;
	void saveReason() ;
	void saveReason() const ;
	int domain() const ;
	bool prepare( bool ) ;

private:
	void drop() noexcept ;
	void destroy() noexcept ;
	bool setNonBlock() ;

public:
	SocketBase( const SocketBase & ) = delete ;
	SocketBase( SocketBase && ) = delete ;
	void operator=( const SocketBase & ) = delete ;
	void operator=( SocketBase && ) = delete ;

private:
	int m_reason ;
	std::string m_reason_string ;
	int m_domain ;
	Descriptor m_fd ;
	bool m_added ;
} ;

//| \class GNet::Socket
/// An internet-protocol socket class. Provides bind(), listen(),
/// and connect(); the base class provide write(); and derived
/// classes provide accept() and read().
///
class GNet::Socket : public SocketBase
{
public:
	Address getLocalAddress() const ;
		///< Retrieves local address of the socket.

	std::pair<bool,Address> getPeerAddress() const ;
		///< Retrieves address of socket's peer.
		///< Returns false in 'first' if none, ie. not yet
		///< connected.

	void bind( const Address & ) ;
		///< Binds the socket with the given address.

	bool bind( const Address & , std::nothrow_t ) ;
		///< No-throw overload. Returns false on error.

	bool canBindHint( const Address & address ) ;
		///< Returns true if the socket can probably be bound
		///< with the given address. Some implementations will
		///< always return true. This method should be used on
		///< a temporary socket of the correct dynamic type
		///< since this socket may become unusable.

	unsigned long getBoundScopeId() const ;
		///< Returns the scope-id of the address last successfully
		///< bind()ed. Note that getLocalAddress() has a zero
		///< scope-id even after bind()ing an address with
		///< a non-zero scope-id.

	bool connect( const Address & addr , bool *done = nullptr ) ;
		///< Initiates a connection to (or association with)
		///< the given address. Returns false on error.
		///<
		///< If successful, a 'done' flag is returned by
		///< reference indicating whether the connect completed
		///< immediately. Normally a stream socket connection
		///< will take some time to complete so the 'done' flag
		///< will be false: the completion will be indicated by
		///< a write event some time later.
		///<
		///< For datagram sockets this sets up an association
		///< between two addresses. The socket should first be
		///< bound with a local address.

	void listen( int backlog = 1 ) ;
		///< Starts the socket listening on the bound
		///< address for incoming connections or incoming
		///< datagrams.

	void shutdown( int how = 1 ) ;
		///< Modifies the local socket state so that so that new
		///< sends (1 or 2) and/or receives (0 or 2) will fail.
		///<
		///< If receives are shut-down then anything received
		///< will be rejected with a RST.
		///<
		///< If sends are shut-down then the transmit queue is
		///< drained and a final empty FIN packet is sent when
		///< fully acknowledged. See also RFC-793 3.5.
		///<
		///< Errors are ignored.

public:
	~Socket() override = default ;
	Socket( const Socket & ) = delete ;
	Socket( Socket && ) = delete ;
	void operator=( const Socket & ) = delete ;
	void operator=( Socket && ) = delete ;

protected:
	Socket( int domain , int type , int protocol ) ;
	Socket( int domain , Descriptor s , const Accepted & ) ;
	std::pair<bool,Address> getAddress( bool ) const ;
	void setOption( int , const char * , int , int ) ;
	bool setOption( int , const char * , int , int , std::nothrow_t ) ;
	bool setOptionImp( int , int , const void * , socklen_t ) ;
	void setOptionsOnBind( bool ) ;
	void setOptionsOnConnect( bool ) ;
	void setOptionLingerImp( int , int ) ;
	void setOptionNoLinger() ;
	void setOptionReuse() ;
	void setOptionExclusive() ;
	void setOptionPureV6( bool ) ;
	bool setOptionPureV6( bool , std::nothrow_t ) ;
	void setOptionKeepAlive() ;

private:
	unsigned long m_bound_scope_id{0UL} ;
} ;

//| \class GNet::AcceptPair
/// A class which is used to return a new()ed socket to calling code, together
/// with associated address information.
///
class GNet::AcceptPair
{
public:
	std::shared_ptr<StreamSocket> first ;
	Address second ;
	AcceptPair() : second(Address::defaultAddress()) {}
} ;

//| \class GNet::StreamSocket
/// A derivation of GNet::Socket for a stream socket.
///
class GNet::StreamSocket : public Socket
{
public:
	using size_type = Socket::size_type ;
	using ssize_type = Socket::ssize_type ;
	struct Listener /// Overload discriminator class for GNet::StreamSocket.
		{} ;

	static bool supports( Address::Family ) ;
		///< Returns true if stream sockets can be created with the
		///< given the address family. This is a one-off run-time
		///< check on socket creation, with a warning if it fails.
		///< Note that a run-time check is useful when running a
		///< new binary on an old operating system.

	explicit StreamSocket( int address_domain ) ;
		///< Constructor.

	StreamSocket( int address_domain , const Listener & ) ;
		///< Constructor overload specifically for a listening
		///< socket. This can be used modify the socket options.

	ssize_type read( char * buffer , size_type buffer_length ) override ;
		///< Override from ReadWrite::read().

	ssize_type write( const char * buf , size_type len ) override ;
		///< Override from Socket::write().

	AcceptPair accept() ;
		///< Accepts an incoming connection, returning a new()ed
		///< socket and the peer address.

public:
	~StreamSocket() override = default ;
	StreamSocket( const StreamSocket & ) = delete ;
	StreamSocket( StreamSocket && ) = delete ;
	void operator=( const StreamSocket & ) = delete ;
	void operator=( StreamSocket && ) = delete ;

private:
	StreamSocket( int , Descriptor s , const Socket::Accepted & ) ;
	void setOptionsOnCreate( bool ) ;
	void setOptionsOnAccept() ;
} ;

//| \class GNet::DatagramSocket
/// A derivation of GNet::Socket for a datagram socket.
///
class GNet::DatagramSocket : public Socket
{
public:
	explicit DatagramSocket( int address_domain , int protocol = 0 ) ;
		///< Constructor.

	ssize_type read( char * buffer , size_type len ) override ;
		///< Override from ReadWrite::read().

	ssize_type write( const char * buffer , size_type len ) override ;
		///< Override from Socket::write().

	ssize_type readfrom( char * buffer , size_type len , Address & src ) ;
		///< Reads a datagram and returns the sender's address by reference.
		///< If connect() has been used then only datagrams from the address
		///< specified in the connect() call will be received.

	ssize_type writeto( const char * buffer , size_type len , const Address & dst ) ;
		///< Sends a datagram to the given address. This should be used
		///< if there is no connect() assocation in effect.

	void disconnect() ;
		///< Releases the association between two datagram endpoints
		///< reversing the effect of the previous Socket::connect().

public:
	~DatagramSocket() override = default ;
	DatagramSocket( const DatagramSocket & ) = delete ;
	DatagramSocket( DatagramSocket && ) = delete ;
	void operator=( const DatagramSocket & ) = delete ;
	void operator=( DatagramSocket && ) = delete ;
} ;

//| \class GNet::RawSocket
/// A derivation of GNet::SocketBase for a raw socket.
///
class GNet::RawSocket : public SocketBase
{
public:
	explicit RawSocket( int domain , int protocol = 0 ) ;
		///< Constructor.

	ssize_type read( char * buffer , size_type buffer_length ) override ;
		///< Reads from the socket.

	ssize_type write( const char * buf , size_type len ) override ;
		///< Writes to the socket.

public:
	~RawSocket() override = default ;
	RawSocket( const RawSocket & ) = delete ;
	RawSocket( RawSocket && ) = delete ;
	void operator=( const RawSocket & ) = delete ;
	void operator=( RawSocket && ) = delete ;
} ;

#endif
