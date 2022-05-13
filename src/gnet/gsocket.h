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
/// \file gsocket.h
///

#ifndef G_NET_SOCKET_H
#define G_NET_SOCKET_H

#include "gdef.h"
#include "gaddress.h"
#include "gexceptionsink.h"
#include "gexception.h"
#include "gevent.h"
#include "gdescriptor.h"
#include "greadwrite.h"
#include "gstringview.h"
#include <string>
#include <memory>
#include <new>

namespace GNet
{
	class SocketBase ;
	class Socket ;
	class SocketProtocol ;
	class StreamSocket ;
	class DatagramSocket ;
	class RawSocket ;
	class AcceptInfo ;
}

//| \class GNet::SocketBase
/// A socket base class that holds a non-blocking socket file descriptor and
/// interfaces to the event loop.
///
class GNet::SocketBase : public G::ReadWrite
{
public:
	G_EXCEPTION( SocketError , tx("socket error") ) ;
	G_EXCEPTION_CLASS( SocketCreateError , tx("socket create error") ) ;
	G_EXCEPTION_CLASS( SocketBindError , tx("socket bind error") ) ;
	G_EXCEPTION_CLASS( SocketTooMany , tx("socket accept error") ) ;
	using size_type = G::ReadWrite::size_type ;
	using ssize_type = G::ReadWrite::ssize_type ;
	struct Accepted /// Overload discriminator class for GNet::SocketBase.
		{} ;
	struct Raw /// Overload discriminator class for GNet::SocketBase.
		{} ;

	static bool supports( Address::Family , int type , int protocol ) ;
		///< Returns true if sockets can be created with the
		///< given parameters.

	~SocketBase() override ;
		///< Destructor. The socket file descriptor is closed and
		///< removed from the event loop.

	SOCKET fd() const noexcept override ;
		///< Returns the socket file descriptor.

	bool eWouldBlock() const override ;
		///< Returns true if the previous socket operation
		///< failed because the socket would have blocked.

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
		///< Reverses addReadHandler(). Does nothing if no
		///< read handler is currently installed.

	void addWriteHandler( EventHandler & , ExceptionSink ) ;
		///< Adds this socket to the event source list so that
		///< the given handler receives write events when flow
		///< control is released. (Not used for datagram
		///< sockets.)

	void dropWriteHandler() noexcept ;
		///< Reverses addWriteHandler(). Does nothing if no
		///< write handler is currently installed.

	void addOtherHandler( EventHandler & , ExceptionSink ) ;
		///< Adds this socket to the event source list so that
		///< the given handler receives exception events.
		///< A TCP exception event should be treated as a
		///< disconnection event. (Not used for datagram
		///< sockets.)

	void dropOtherHandler() noexcept ;
		///< Reverses addOtherHandler(). Does nothing if no
		///< 'other' handler is currently installed.

	std::string asString() const ;
		///< Returns the socket handle as a string.
		///< Only used in debugging.

	std::string reason() const ;
		///< Returns the reason for the previous error.

protected:
	SocketBase( Address::Family , int type , int protocol ) ;
		///< Constructor used by derived classes. Creates the
		///< socket using socket() and makes it non-blocking.

	SocketBase( Address::Family , Descriptor s ) ;
		///< Constructor used by derived classes. Creates the
		///< socket object from a newly-created socket handle
		///< and makes it non-blocking.

	SocketBase( Address::Family , Descriptor s , const Accepted & ) ;
		///< Constructor used by StreamSocket::accept() to create
		///< a socket object from a newly accept()ed socket
		///< handle.

	SocketBase( const Raw & , int domain , int type , int protocol ) ;
		///< Constructor for a raw socket.

	ssize_type writeImp( const char * buf , size_type len ) ;
		///< Writes to the socket. This is a default implementation
		///< for write() that can be called from derived classes'
		///< overrides.

	static bool error( int rc ) ;
		///< Returns true if the given return code indicates an
		///< error.

	static bool sizeError( ssize_type size ) ;
		///< Returns true if the given write() return value
		///< indicates an error.

	void clearReason() ;
		///< Clears the saved errno.

	void saveReason() ;
		///< Saves the current errno following error()/sizeError().

	void saveReason() const ;
		///< Saves the current errno following error()/sizeError().

	bool isFamily( Address::Family ) const ;
		///< Returns true if the socket family is as given.

private:
	static std::string reasonString( int ) ;
	bool create( int , int , int ) ;
	bool prepare( bool ) ;
	void drop() noexcept ;
	void destroy() noexcept ;
	void unlink() noexcept ;
	bool setNonBlocking() ;

public:
	SocketBase( const SocketBase & ) = delete ;
	SocketBase( SocketBase && ) = delete ;
	void operator=( const SocketBase & ) = delete ;
	void operator=( SocketBase && ) = delete ;

private:
	int m_reason ;
	int m_domain ;
	Address::Family m_family ; // valid depending on m_domain
	Descriptor m_fd ;
	bool m_read_added ;
	bool m_write_added ;
	bool m_other_added ;
	bool m_accepted ;
} ;

//| \class GNet::Socket
/// An internet-protocol socket class. Provides bind(), listen(),
/// and connect(); the base classes provide write(); and derived
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

	static std::string canBindHint( const Address & address , bool stream_socket = true ) ;
		///< Returns the empty string if a socket could probably be
		///< bound with the given address or a failure reason.
		///< Some implementations will always return the empty
		///< string.

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

	void setOptionLinger( int onoff , int time ) ;
		///< Sets the linger socket option.

	void setOptionNoLinger() ;
		///< Sets the linger socket option to no-linger (setOptionLinger(0,0)).

	void setOptionKeepAlive() ;
		///< Sets the keep-alive socket option.

public:
	~Socket() override = default ;
	Socket( const Socket & ) = delete ;
	Socket( Socket && ) = delete ;
	void operator=( const Socket & ) = delete ;
	void operator=( Socket && ) = delete ;

protected:
	Socket( Address::Family , int type , int protocol ) ;
	Socket( Address::Family , Descriptor s , const Accepted & ) ;
	std::pair<bool,Address> getLocalAddress( std::nothrow_t ) const ;
	void setOption( int , const char * , int , int ) ;
	bool setOption( int , const char * , int , int , std::nothrow_t ) ;
	bool setOptionImp( int , int , const void * , socklen_t ) ;
	void setOptionsOnBind( Address::Family ) ;
	void setOptionsOnConnect( Address::Family ) ;
	void setOptionReuse() ;
	void setOptionExclusive() ;
	void setOptionPureV6() ;
	bool setOptionPureV6( std::nothrow_t ) ;
	void setOptionFreeBind() ;

private:
	unsigned long m_bound_scope_id{0UL} ;
} ;

//| \class GNet::AcceptInfo
/// A move-only class which is used to return a new()ed socket to calling
/// code, together with associated address information.
///
class GNet::AcceptInfo
{
public:
	std::unique_ptr<StreamSocket> socket_ptr ;
	Address address ;
	AcceptInfo() : address(Address::defaultAddress()) {}
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

	explicit StreamSocket( Address::Family ) ;
		///< Constructor.

	StreamSocket( Address::Family , const Listener & ) ;
		///< Constructor overload specifically for a listening
		///< socket, which might need slightly different socket
		///< options.

	ssize_type read( char * buffer , size_type buffer_length ) override ;
		///< Override from ReadWrite::read().

	ssize_type write( const char * buf , size_type len ) override ;
		///< Override from Socket::write().

	AcceptInfo accept() ;
		///< Accepts an incoming connection, returning a new()ed
		///< socket and the peer address.

public:
	~StreamSocket() override = default ;
	StreamSocket( const StreamSocket & ) = delete ;
	StreamSocket( StreamSocket && ) = delete ;
	void operator=( const StreamSocket & ) = delete ;
	void operator=( StreamSocket && ) = delete ;

private:
	StreamSocket( Address::Family , Descriptor s , const Accepted & ) ;
	void setOptionsOnCreate( Address::Family , bool listener ) ;
	void setOptionsOnAccept( Address::Family ) ;
} ;

//| \class GNet::DatagramSocket
/// A derivation of GNet::Socket for a datagram socket.
///
class GNet::DatagramSocket : public Socket
{
public:
	explicit DatagramSocket( Address::Family , int protocol = 0 ) ;
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

	ssize_type writeto( const std::vector<G::string_view> & , const Address & dst ) ;
		///< Sends a datagram to the given address, overloaded for
		///< scatter-gather data chunks.

	void disconnect() ;
		///< Releases the association between two datagram endpoints
		///< reversing the effect of the previous Socket::connect().

	std::size_t limit() const ;
		///< Returns the maximum datagram size or 1024, whichever is
		///< larger.
		/// \see SO_SNDBUF, /proc/sys/net/core/wmem_default

public:
	~DatagramSocket() override = default ;
	DatagramSocket( const DatagramSocket & ) = delete ;
	DatagramSocket( DatagramSocket && ) = delete ;
	void operator=( const DatagramSocket & ) = delete ;
	void operator=( DatagramSocket && ) = delete ;
} ;

//| \class GNet::RawSocket
/// A derivation of GNet::SocketBase for a raw socket, typically of
/// type AF_NETLINK or PF_ROUTE.
///
class GNet::RawSocket : public SocketBase
{
public:
	RawSocket( int domain , int type , int protocol ) ;
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
