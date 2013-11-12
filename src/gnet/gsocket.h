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
/// \file gsocket.h
///

#ifndef G_SOCKET_H 
#define G_SOCKET_H

#include "gdef.h"
#include "gnet.h"
#include "gaddress.h"
#include "gevent.h"
#include "gdescriptor.h"
#include <string>

/// \namespace GNet
namespace GNet
{
	class Socket ;
	class SocketProtocol ;
	class StreamSocket ;
	class DatagramSocket ;
	class AcceptPair ;
}

/// \class GNet::Socket
///
/// The Socket class encapsulates a non-blocking
/// Unix socket file descriptor or a Windows 'SOCKET' handle. 
///
/// (Non-blocking network i/o is particularly appropriate for single-
/// threaded server processes which manage multiple client connections.
/// The main disagvantage is that flow control has to be managed 
/// explicitly: see Socket::write() and Socket::eWouldBlock().)
///
/// Provides bind(), listen(), connect(), write(); derived classes 
/// provide accept() and read(). Also interfaces to the event
/// loop with addReadHandler() and addWriteHandler().
///
/// The raw file descriptor is only exposed to the SocketProtocol
/// class (using the credentials pattern) and to the event loop.
///
/// Exceptions are not used.
///
class GNet::Socket  
{
public:
	typedef size_t size_type ;
	typedef ssize_t ssize_type ;
	/// A credentials class that allows SocketProtocol to call Socket::fd().
	class Credentials 
	{ 
		friend class SocketProtocol ;
		friend class SocketProtocolTest ;
		Credentials( const char * ) {}
	} ;

	virtual ~Socket() ;
		///< Destructor.

	bool valid() const ;
		///< Returns true if the socket handle
		///< is valid (open).

	std::pair<bool,Address> getLocalAddress() const ;
		///< Retrieves local address of the socket.
		///< Pair.first is false on error.

	std::pair<bool,Address> getPeerAddress() const ;
		///< Retrieves address of socket's peer.
		///< 'Pair.first' is false on error.

	bool hasPeer() const ;
		///< Returns true if the socket has a valid
		///< peer. This can be used to see if a
		///< connect succeeded.

	bool bind( const Address & address ) ;
		///< Binds the socket with an INADDR_ANY network address
		///< and the port number taken from the given
		///< address. This is used for listening
		///< sockets.

	bool canBindHint( const Address & address ) ;
		///< Returns true if the socket can probably be bound 
		///< with the given address. Some implementations will 
		///< always return true. This method should be used on 
		///< a temporary socket of the correct dynamic type 
		///< since this socket may become unusable.

	bool connect( const Address & addr , bool *done = NULL ) ;
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

	bool listen( int backlog = 1 ) ;
		///< Starts the socket listening on the bound
		///< address for incoming connections or incoming
		///< datagrams.

	virtual ssize_type write( const char * buf , size_type len ) ;
		///< Sends data. For datagram sockets the datagram
		///< is sent to the address specified in the
		///< previous connect(). Returns the amount
		///< of data submitted. 
		///<
		///< If write() returns -1 then use eWouldBlock() to 
		///< determine whether there was a flow control 
		///< problem. If write() returns -1 and eWouldBlock()
		///< returns false then the connection is lost and
		///< the socket should be closed. If write() returns 
		///< less than 'len' then assume that there was a partial 
		///< flow control problem (do not use eWouldBlock()).
		///<
		///< This method is virtual to allow overloading (sic)
		///< in derived classes.

	bool eWouldBlock() ;
		///< Returns true if the previous socket operation
		///< failed with the EWOULDBLOCK or EGAIN error status.
		///< When writing this indicates a flow control 
		///< problem; when reading it indicates that there 
		///< is nothing to read.

	bool eInProgress() ;
		///< Returns true if the previous socket operation
		///< failed with the EINPROGRESS error status.
		///< When connecting this can be considered a
		///< non-error.

	bool eMsgSize() ;
		///< Returns true if the previous socket operation
		///< failed with the EMSGSIZE error status. When 
		///< writing to a datagram socket this indicates that 
		///< the message was too big to send atomically.

	void addReadHandler( EventHandler & handler ) ;
		///< Adds this socket to the event source list so that 
		///< the given handler receives read events.

	void dropReadHandler();
		///< Reverses addReadHandler().

	void addWriteHandler( EventHandler & handler ) ;
		///< Adds this socket to the event source list so that 
		///< the given handler receives write events when flow 
		///< control is released. (Not used for datagram 
		///< sockets.)

	void dropWriteHandler() ;
		///< Reverses addWriteHandler().

	void addExceptionHandler( EventHandler & handler );
		///< Adds this socket to the event source list so that 
		///< the given handler receives exception events. 
		///< A TCP exception event should be treated as a 
		///< disconnection event. (Not used for datagram 
		///< sockets.)

	void dropExceptionHandler() ;
		///< Reverses addExceptionHandler().

	std::string asString() const ;
		///< Returns the socket handle as a string.
		///< Only used in debugging.

	std::string reasonString() const ;
		///< Returns the failure reason as a string.
		///< Only used in debugging.

	void shutdown( bool for_writing = true ) ;
		///< Shuts the socket for writing (or reading).

	int fd( Credentials ) const ;
		///< Returns the socket descriptor as an integer.
		///< Only callable from SocketProtocol.

protected:
	Socket( int domain , int type , int protocol ) ;
		///< Constructor used by derived classes. Opens the 
		///< socket using socket().

	explicit Socket( Descriptor s ) ;
		///< Constructor which creates a socket object from 
		///< an existing socket handle. Used only by 
		///< StreamSocket::accept().

protected:
	static bool valid( Descriptor s ) ;
	static int reason() ;
	static bool error( int rc ) ;
	static bool sizeError( ssize_type size ) ;
	void prepare() ;
	void setFault() ;
	void setNoLinger() ;
	void setReuse() ;
	void setKeepAlive() ;
	std::pair<bool,Address> getAddress( bool ) const ;

private:
	void doClose() ;
	bool setNonBlock() ;

protected:
	int m_reason ;
	Descriptor m_socket ;

private:
	Socket( const Socket & ) ;
	void operator=( const Socket & ) ;
	void drop() ;
} ;

///

/// \class GNet::AcceptPair
/// A class which is used to return a new()ed socket
/// to calling code, together with associated information, and with 
/// auto_ptr style transfer of ownership.
///
class GNet::AcceptPair 
{
public:
	typedef std::auto_ptr<StreamSocket> first_type ;
	typedef Address second_type ;

	first_type first ;
	second_type second ;

	AcceptPair( StreamSocket * new_p , Address a ) ;
		///< Constructor.

	AcceptPair( const AcceptPair & other ) ;
		///< Copy constructor.

	AcceptPair & operator=( const AcceptPair & rhs ) ;
		///< Assignment operator.
} ;

///

/// \class GNet::StreamSocket
/// A derivation of Socket for a stream socket. 
///
class GNet::StreamSocket : public GNet::Socket 
{
public:
	typedef Socket::size_type size_type ;
	typedef Socket::ssize_type ssize_type ;

	StreamSocket() ;
		///< Default constructor. Check with valid().

	explicit StreamSocket( const Address & address_hint ) ;
		///< Constructor with a hint of the bind()/connect()
		///< address to be used later. Check with valid().

	virtual ~StreamSocket() ;
		///< Destructor.

	ssize_type read( char * buffer , size_type buffer_length ) ;
		///< Reads data from the socket stream. 
		///<
		///< Returns 0 if the connection has been lost.
		///< Returns -1 on error, or if there is nothing 
		///< to read (eWouldBlock() true). Note that 
		///< having nothing to read is not an error, 
		///< even after getting a read event.

	AcceptPair accept() ;
		///< Accepts an incoming connection, returning
		///< a new()ed socket and the peer address.

private:
	StreamSocket( const StreamSocket & ) ; // not implemented
	void operator=( const StreamSocket & ) ; // not implemented
	StreamSocket( Descriptor s ) ; // A private constructor used in accept().
} ;

///

/// \class GNet::DatagramSocket
/// A derivation of Socket for a connectionless
/// datagram socket. 
///
class GNet::DatagramSocket : public GNet::Socket 
{
public:
	DatagramSocket() ;
		///< Default constructor.

	explicit DatagramSocket( const Address & address_hint ) ;
		///< Constructor with a hint of a local address.

	virtual ~DatagramSocket() ;
		///< Destructor.

	ssize_type read( void * buffer , size_type len , Address & src ) ; 
		///< Reads a datagram and returns the sender's address
		///< by reference. If connect() has been used then
		///< only datagrams from the address specified in the
		///< connect() call will be received.

	ssize_type write( const char * buffer , size_type len , const Address & dst ) ; 
		///< Sends a datagram to the given address.
		///< This form of write() should be used
		///< if there is no connect() assocation 
		///< in effect.

	ssize_type write( const char * buffer , size_type len ) ;
		///< See Socket::write().

	void disconnect() ;
		///< Releases the association between two
		///< datagram endpoints reversing the effect
		///< of the previous Socket::connect().

private:
	DatagramSocket( const DatagramSocket & ) ; // not implemented
	void operator=( const DatagramSocket & ) ; // not implemented
} ;

///

inline
GNet::Socket::ssize_type GNet::DatagramSocket::write( const char *buf, size_type len ) 
{ 
	return Socket::write(buf,len) ; 
}

///

inline 
GNet::AcceptPair::AcceptPair( StreamSocket * p , Address a ) : 
	first(p) , 
	second(a) 
{
}

inline 
GNet::AcceptPair::AcceptPair( const AcceptPair & other ) : 
	first(const_cast<first_type&>(other.first)) , 
	second(other.second) 
{
}

inline 
GNet::AcceptPair & GNet::AcceptPair::operator=( const AcceptPair & rhs ) 
{ 
	///< (safe for self-assignment)
	first = const_cast<first_type&>(rhs.first) ; 
	second = rhs.second ; 
	return *this ; 
}

#endif

