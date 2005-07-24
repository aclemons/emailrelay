//
// Copyright (C) 2001-2005 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// gsocket.h
//

#ifndef G_SOCKET_H 
#define G_SOCKET_H

#include "gdef.h"
#include "gnet.h"
#include "gaddress.h"
#include "gevent.h"
#include "gdescriptor.h"
#include <string>

namespace GNet
{
	class Socket ;
	class StreamSocket ;
	class DatagramSocket ;
	class AcceptPair ;
}

// Class: GNet::Socket
//
// Description: The Socket class encapsulates an asynchronous 
// (ie. non-blocking) Unix socket file descriptor or a Windows 
// 'SOCKET' handle. The class hides all differences between BSD 
// sockets and WinSock.
//
// (Non-blocking network i/o is particularly appropriate for single-
// threaded server processes which manage multiple client connections.
// The main disagvantage is that flow control has to be managed 
// explicitly: see Socket::write() and Socket::eWouldBlock().)
//
class GNet::Socket  
{
public:
	virtual ~Socket() ;
		// Destructor.

	bool valid() const ;
		// Returns true if the socket handle
		// is valid (open).

	std::pair<bool,Address> getLocalAddress() const ;
		// Retrieves local address of the socket.
		// Pair.first is false on error.

	std::pair<bool,Address> getPeerAddress() const ;
		// Retrieves address of socket's peer.
		// 'Pair.first' is false on error.

	bool hasPeer() const ;
		// Returns true if the socket has a valid
		// peer. This can be used to see if a
		// connect succeeded.

	virtual void close() ;
		// Closes the socket.
		//
		// Postcondition: !valid()

	virtual bool reopen() = 0 ;
		// Reopens a closed socket. The new socket is
		// _not_ registered with the event source to 
		// receive read and write events.
		//
		// Returns false on error.
		//
		// Postcondition: !valid()

	bool bind( const Address &address ) ;
		// Binds the socket with an INADDR_ANY network address
		// and the port number taken from the given
		// address. This is used for listening
		// sockets.

	bool bind() ;
		// Binds the socket with an INADDR_ANY network address
		// and a zero port number. This is used to
		// initialise the socket prior to connect().

	bool canBindHint( const Address & address ) ;
		// Returns true if the socket can probably be 
		// bound with the given address. Some
		// implementations will always return
		// true. This method should be used on a 
		// temporary socket of the correct dynamic 
		// type.

	bool connect( const Address &addr , bool *done = NULL ) ;
		// Initiates a connection to (or association 
		// with) the given address. Returns false on 
		// error.
		//
		// If successful, a 'done' flag is returned by 
		// reference indicating whether the connect completed 
		// immediately. Normally a stream socket connection 
		// will take some time to complete so the 'done' flag 
		// will be false: the completion will be indicated by 
		// a write event some time later.
		//
		// For datagram sockets this sets up an association 
		// between two addresses. The socket should first be 
		// bound with a local address.

	bool listen( int backlog = 1 );
		// Starts the socket listening on the bound
		// address for incoming connections or incoming
		// datagrams.

	virtual ssize_t write( const char *buf, size_t len ) ;
		// Sends data. For datagram sockets the datagram
		// is sent to the address specified in the
		// previous connect(). Returns the amount
		// of data submitted. 
		//
		// If write() returns -1 then use eWouldBlock() to 
		// determine whether there was a flow control 
		// problem. If write() returns -1 and eWouldBlock()
		// returns false then the connection is lost and
		// the socket should be closed. If write() returns 
		// less than 'len' then assume that there was a partial 
		// flow control problem (do not use eWouldBlock()).
		//
		// (This is virtual to allow overloading --
		//  not overriding -- in derived classes.)

	bool eWouldBlock() ;
		// Returns true if the previous socket operation
		// failed with the EWOULDBLOCK or EGAIN error status.
		// When writing this indicates a flow control 
		// problem; when reading it indicates that there 
		// is nothing to read.

	bool eInProgress() ;
		// Returns true if the previous socket operation
		// failed with the EINPROGRESS error status.
		// When connecting this can be considered a
		// non-error.

	bool eMsgSize() ;
		// Returns true if the previous socket operation
		// failed with the EMSGSIZE error status.
		// When writing to a datagram socket this
		// indicates that the message was too big
		// to send atomically.

	void addReadHandler( EventHandler & handler ) ;
		// Adds this socket to the event source list
		// so that the given handler receives read
		// events.

	void dropReadHandler();
		// Reverses addReadHandler().

	void addWriteHandler( EventHandler & handler ) ;
		// Adds this socket to the event source list
		// so that the given handler receives write
		// events when flow control is released.
		// (Not used for datagram sockets.)

	void dropWriteHandler() ;
		// Reverses addWriteHandler().

	void addExceptionHandler( EventHandler & handler );
		// Adds this socket to the event source list
		// so that the given handler receives exception
		// events. An exception event should
		// be treated as a disconnection event.
		// (Not used for datagram sockets.)

	void dropExceptionHandler() ;
		// Reverses addExceptionHandler().

	std::string asString() const ;
		// Returns the socket handle as a string.
		// Only used in debugging.

	std::string reasonString() const ;
		// Returns the failure reason as a string.
		// Only used in debugging.

protected:
	Socket( int domain, int type, int protocol = 0 ) ;
		// Constructor used by derived classes.
		// Opens the socket using ::socket().

	bool open( int domain , int type , int protocol = 0 ) ;
		// Used by derived classes to implement reopen().
		// Opens the socket using ::socket().
		// Returns false on error.

	Socket( Descriptor s ) ;
		// Constructor which creates a socket object from 
		// an existing socket handle. Used only by 
		// StreamSocket::accept().

protected:
	static bool valid( Descriptor s ) ;
	static int reason() ;
	static bool error( int rc ) ;
	static bool sizeError( ssize_t size ) ;
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
	Socket( const Socket& );
	void operator=( const Socket& );
	void drop() ;
};

// ===

// Class: GNet::AcceptPair
// Description: A class which behaves like std::pair<std::auto_ptr<StreamSocket>,Address>.
//
// (The standard pair<> template cannot be used because gcc's auto_ptr<> has
// a non-const copy constructor and assignment operator -- the pair<> op=() 
// fails to compile because the rhs of the 'first' assignment is const, 
// not matching any op=() in auto_ptr<>. Note the use of const_cast<>() 
// in the implementation.)
//
class GNet::AcceptPair 
{
public:
	typedef std::auto_ptr<StreamSocket> first_type ;
	typedef Address second_type ;

	first_type first ;
	second_type second ;

	AcceptPair( StreamSocket * new_p , Address a ) ;
		// Constructor.

	AcceptPair( const AcceptPair & other ) ;
		// Copy constructor.

	AcceptPair & operator=( const AcceptPair & rhs ) ;
		// Assignment operator.
} ;

// ===

// Class: GNet::StreamSocket
// Description: A derivation of Socket for a stream socket. 
//
class GNet::StreamSocket : public GNet::Socket 
{
public:
	StreamSocket() ;
		// Default constructor.

	virtual ~StreamSocket() ;
		// Destructor.

	ssize_t read( char *buffer , size_t buffer_length ) ;
		// Reads from the TCP stream. Returns
		// 0 if the connection has been lost.
		// Returns -1 on error, or if there is 
		// nothing to read (eWouldBlock() true). 
		// Note that under Windows there can 
		// be nothing to read even after receiving 
		// a read event.

	AcceptPair accept() ;
		// Accepts an incoming connection, returning
		// a new()ed socket and the peer address.

	virtual bool reopen() ;
		// Inherited from Socket.

private:
	StreamSocket( const StreamSocket & ) ;
		// Copy constructor. Not implemented.

	void operator=( const StreamSocket & ) ;
		// Assignment operator. Not implemented.

	StreamSocket( Descriptor s ) ;
		// A private constructor used in accept().
};

// ===

// Class: GNet::DatagramSocket
// Description: A derivation of Socket for a connectionless
// datagram socket. 
//
class GNet::DatagramSocket : public GNet::Socket 
{
public:
	DatagramSocket();
		// Default constructor.

	virtual ~DatagramSocket() ;
		// Destructor.

	ssize_t read( void * buffer , size_t len , Address & src ) ; 
		// Reads a datagram and returns the sender's address
		// by reference. If connect() has been used then
		// only datagrams from the address specified in the
		// connect() call will be received.

	ssize_t write( const char * buffer , size_t len , const Address & dst ) ; 
		// Sends a datagram to the given address.
		// This form of write() should be used
		// if there is no connect() assocation 
		// in effect.

	ssize_t write( const char * buffer , size_t len ) ;
		// See Socket::write().

	void disconnect() ;
		// Releases the association between two
		// datagram endpoints reversing the effect
		// of the previous Socket::connect().

	virtual bool reopen() ;
		// Inherited from Socket.
 
private:
	DatagramSocket( const DatagramSocket & ) ;
		// Copy constructor. Not implemented.

	void operator=( const DatagramSocket & ) ;
		// Assignment operator. Not implemented.
} ;

// ===

inline
ssize_t GNet::DatagramSocket::write( const char *buf, size_t len ) 
{ 
	return Socket::write(buf,len) ; 
}

// ===

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
	first = const_cast<first_type&>(rhs.first) ; 
	second = rhs.second ; return *this ; 
}

#endif

