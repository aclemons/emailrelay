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
//
// gsocket.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gtest.h"
#include "gsleep.h"
#include "gassert.h"
#include "gsocket.h"
#include "gcleanup.h"
#include "gmemory.h"
#include "gdebug.h"

GNet::Socket::Socket( int domain , int type , int protocol ) :
	m_reason( 0 )
{
	m_socket = Descriptor( ::socket( domain , type , protocol ) ) ;
	if( valid() )
	{
		prepare() ;
	}
	else
	{
		m_reason = reason() ;
		G_DEBUG( "GNet::Socket::open: cannot open a socket (" << m_reason << ")" ) ;
	}
}

GNet::Socket::Socket( Descriptor s ) :
	m_reason(0) ,
	m_socket(s)
{
	prepare() ;
}

void GNet::Socket::prepare()
{
	G::Cleanup::init() ; // ignore SIGPIPE
	if( ! setNonBlock() )
	{
		m_reason = reason() ;
		doClose() ;
		G_ASSERT( !valid() ) ;
		G_WARNING( "GNet::Socket::open: cannot make socket non-blocking (" << m_reason << ")" ) ;
	}
}

GNet::Socket::~Socket()
{
	try 
	{ 
		drop() ; 
	} 
	catch(...)  // dtor
	{
	}

	try 
	{ 
		if( valid() ) doClose() ;
		G_ASSERT( !valid() ) ;
	} 
	catch(...) // dtor
	{
	}
}

void GNet::Socket::drop()
{
	dropReadHandler() ;
	dropWriteHandler() ;
	dropExceptionHandler() ;
}

bool GNet::Socket::valid() const
{
	return valid( m_socket ) ;
}

bool GNet::Socket::bind( const Address & local_address )
{
	if( !valid() )
		return false ;

	// optionally allow immediate re-use -- may cause problems
	setReuse() ;

	G_DEBUG( "Socket::bind: binding " << local_address.displayString() << " on fd " << m_socket ) ;

	int rc = ::bind( m_socket.fd(), local_address.address() , local_address.length() ) ;
	if( error(rc) )
	{
		m_reason = reason() ;
		return false ;
	}

	const bool debug = true ;
	if( debug )
	{
		std::pair<bool,Address> pair = getLocalAddress() ;
		if( ! pair.first )
		{
			G_WARNING( "GNet::Socket::bind: cannot get bound address" ) ;
			return false ;
		}

		G_DEBUG( "GNet::Socket::bind: bound " << pair.second.displayString() << " on fd " << m_socket ) ;
	}

	return true ;
}

bool GNet::Socket::connect( const Address & address , bool *done )
{
	if( !valid() )
		return false;

	G_DEBUG( "GNet::Socket::connect: connecting to " << address.displayString() ) ;
	int rc = ::connect( m_socket.fd(), address.address(), address.length() ) ;
	if( error(rc) )
	{
		m_reason = reason() ;

		if( G::Test::enabled( "slow-connect" ) )
			sleep( 1 ) ;

		if( eInProgress() )
		{
			G_DEBUG( "GNet::Socket::connect: connection in progress" ) ;
			if( done != NULL ) *done = false ;
			return true ;
		}

		G_DEBUG( "GNet::Socket::connect: synchronous connect failure: " << m_reason ) ;
		return false;
	}

	if( done != NULL ) *done = true ;
	return true;
}

GNet::Socket::ssize_type GNet::Socket::write( const char * buf , size_type len )
{
	if( static_cast<ssize_type>(len) < 0 )
		G_WARNING( "GNet::Socket::write: too big" ) ; // EMSGSIZE from ::send() ?

	ssize_type nsent = ::send( m_socket.fd() , buf , len , 0 ) ;

	if( sizeError(nsent) ) // if -1
	{
		m_reason = reason() ;
		G_DEBUG( "GNet::Socket::write: write error " << m_reason ) ;
		return -1 ;
	}
	else if( nsent < 0 || static_cast<size_type>(nsent) < len )
	{
		m_reason = reason() ;
	}
	return nsent;
}

void GNet::Socket::setNoLinger()
{
	struct linger options ;
	options.l_onoff = 0 ;
	options.l_linger = 0 ;
	socklen_t sizeof_options = sizeof(options) ;
	G_IGNORE_RETURN(int) ::setsockopt( m_socket.fd() , SOL_SOCKET ,
		SO_LINGER , reinterpret_cast<char*>(&options) , sizeof_options ) ;
}

void GNet::Socket::setKeepAlive()
{
	int keep_alive = 1 ;
	G_IGNORE_RETURN(int) ::setsockopt( m_socket.fd() , SOL_SOCKET , 
		SO_KEEPALIVE , reinterpret_cast<char*>(&keep_alive) , sizeof(keep_alive) ) ;
}

void GNet::Socket::setReuse()
{
	int on = 1 ;
	G_IGNORE_RETURN(int) ::setsockopt( m_socket.fd() , SOL_SOCKET ,
		SO_REUSEADDR , reinterpret_cast<char*>(&on) , sizeof(on) ) ;
}

bool GNet::Socket::listen( int backlog )
{
	int rc = ::listen( m_socket.fd() , backlog ) ;
	if( error(rc) )
	{
		m_reason = reason() ;
		return false ;
	}
	return true ;
}

std::pair<bool,GNet::Address> GNet::Socket::getAddress( bool local ) const
{
	std::pair<bool,Address> error_pair( false , Address::invalidAddress() ) ;

	if( !valid() )
		return error_pair ;

	AddressStorage address_storage ;
	int rc = 
		local ? 
			::getsockname( m_socket.fd() , address_storage.p1() , address_storage.p2() ) :
			::getpeername( m_socket.fd() , address_storage.p1() , address_storage.p2() ) ;

	if( error(rc) )
	{
		const_cast<Socket*>(this)->m_reason = reason() ;
		return error_pair ;
	}

	return std::pair<bool,Address>( true , Address(address_storage) ) ;
}

std::pair<bool,GNet::Address> GNet::Socket::getLocalAddress() const
{
	return getAddress( true ) ;
}

std::pair<bool,GNet::Address> GNet::Socket::getPeerAddress() const
{
	return getAddress( false ) ;
}

bool GNet::Socket::hasPeer() const
{
	return getPeerAddress().first ;
}

void GNet::Socket::addReadHandler( EventHandler & handler )
{
	G_ASSERT( valid() ) ;
	G_DEBUG( "GNet::Socket::addReadHandler: fd " << m_socket ) ;
	EventLoop::instance().addRead( m_socket , handler ) ;
}

void GNet::Socket::dropReadHandler()
{
	EventLoop::instance().dropRead( m_socket ) ;
}

void GNet::Socket::addWriteHandler( EventHandler & handler )
{
	G_ASSERT( valid() ) ;
	G_DEBUG( "GNet::Socket::addWriteHandler: fd " << m_socket ) ;
	EventLoop::instance().addWrite( m_socket , handler ) ;
}

void GNet::Socket::addExceptionHandler( EventHandler & handler )
{
	G_ASSERT( valid() ) ;
	G_DEBUG( "GNet::Socket::addExceptionHandler: fd " << m_socket ) ;
	EventLoop::instance().addException( m_socket , handler ) ;
}

void GNet::Socket::dropWriteHandler()
{
	EventLoop::instance().dropWrite( m_socket ) ;
}

void GNet::Socket::dropExceptionHandler()
{
	EventLoop::instance().dropException( m_socket ) ;
}

std::string GNet::Socket::asString() const
{
	std::ostringstream ss ;
	ss << m_socket ;
	return ss.str() ;
}

std::string GNet::Socket::reasonString() const
{
	std::ostringstream ss ;
	ss << m_reason ;
	return ss.str() ;
}

void GNet::Socket::shutdown( bool for_writing )
{
	::shutdown( m_socket.fd() , for_writing ? 1 : 0 ) ;
}

int GNet::Socket::fd( Credentials ) const
{
	return static_cast<int>( m_socket.fd() ) ;
}

//==

GNet::StreamSocket::StreamSocket() : 
	Socket( Address::defaultDomain() , SOCK_STREAM , 0 )
{
	setNoLinger() ;
	setKeepAlive() ;
}

GNet::StreamSocket::StreamSocket( const Address & address_hint ) : 
	Socket( address_hint.domain() , SOCK_STREAM , 0 )
{
	setNoLinger() ;
	setKeepAlive() ;
}

GNet::StreamSocket::StreamSocket( Descriptor s ) : 
	Socket( s )
{
}

GNet::StreamSocket::~StreamSocket()
{
}

GNet::Socket::ssize_type GNet::StreamSocket::read( char * buf , size_type len )
{
	if( len == 0 ) return 0 ;
	G_ASSERT( valid() ) ;
	ssize_type nread = ::recv( m_socket.fd() , buf , len , 0 ) ;
	if( sizeError(nread) )
	{
		m_reason = reason() ;
		G_DEBUG( "GNet::StreamSocket::read: fd " << m_socket << ": read error " << m_reason ) ;
		return -1 ;
	}
	if( nread == 0 )
	{
		G_DEBUG( "GNet::StreamSocket::read: fd " << m_socket << ": read zero bytes" ) ;
	}
	return nread ;
}

GNet::AcceptPair GNet::StreamSocket::accept()
{
	AcceptPair pair( NULL , Address::invalidAddress() ) ;

	sockaddr addr ;
	socklen_t addr_length = sizeof(addr) ;
	Descriptor new_socket( ::accept( m_socket.fd() , &addr, &addr_length ) ) ;
	if( valid(new_socket) )
	{
		pair.second = Address( &addr , addr_length ) ;
		G_DEBUG( "GNet::StreamSocket::accept: " << pair.second.displayString() ) ;
		pair.first <<= new StreamSocket(new_socket) ;
		pair.first.get()->setNoLinger() ;
	}
	else
	{
		m_reason = reason() ;
		G_DEBUG( "GNet::StreamSocket::accept: failure" ) ;
	}
	return pair ;
}

//==

GNet::DatagramSocket::DatagramSocket() : 
	Socket( Address::defaultDomain() , SOCK_DGRAM , 0 )
{
}

GNet::DatagramSocket::DatagramSocket( const Address & address_hint ) : 
	Socket( address_hint.domain() , SOCK_DGRAM , 0 )
{
}

GNet::DatagramSocket::~DatagramSocket()
{
}

void GNet::DatagramSocket::disconnect()
{
	int rc = ::connect( m_socket.fd() , 0 , 0 ) ;
	if( error(rc) )
		m_reason = reason() ;
}

GNet::Socket::ssize_type GNet::DatagramSocket::read( void *buf , size_type len , Address & src_address )
{
	sockaddr sender ;
	socklen_t sender_len = sizeof(sender) ;
	ssize_type nread = ::recvfrom( m_socket.fd() , reinterpret_cast<char*>(buf) , len , 0 , &sender , &sender_len ) ;
	if( sizeError(nread) )
	{
		m_reason = reason() ;
		return -1 ;
	}

	src_address = Address( &sender , sender_len ) ;
	return nread ;
}

GNet::Socket::ssize_type GNet::DatagramSocket::write( const char *buf , size_type len , const Address &dst )
{
	G_DEBUG( "GNet::DatagramSocket::write: sending " << len << " bytes to " << dst.displayString() ) ;

	ssize_type nsent = ::sendto( m_socket.fd() , buf, len, 0, dst.address(), dst.length() ) ;
	if( nsent < 0 )
	{
		m_reason = reason() ;
		G_DEBUG( "GNet::DatagramSocket::write: write error " << m_reason ) ;
		return -1 ;
	}
	return nsent ;
}

/// \file gsocket.cpp
