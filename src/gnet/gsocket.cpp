//
// Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gsocket.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gassert.h"
#include "gsocket.h"
#include "gmemory.h"
#include "gdebug.h"

GNet::Socket::Socket( int domain, int type, int protocol ) :
	m_reason( 0 )
{
	G_IGNORE open( domain , type , protocol ) ;
}

bool GNet::Socket::open( int domain, int type, int protocol )
{
	m_socket = ::socket( domain, type, protocol ) ;
	if( !valid() )
	{
		m_reason = reason() ;
		G_DEBUG( "GNet::Socket::open: cannot open a socket (" << m_reason << ")" ) ;
		return false ;
	}

	if( !setNonBlock() )
	{
		m_reason = reason() ;
		doClose() ;
		G_ASSERT( !valid() ) ;
		G_DEBUG( "GNet::Socket::open: cannot make socket non-blocking (" << m_reason << ")" ) ;
		return false ;
	}

	return true ;
}

GNet::Socket::Socket( Descriptor s ) :
	m_reason(0) ,
	m_socket(s)
{
	;
}

GNet::Socket::~Socket()
{
	try
	{
		close() ;
	}
	catch(...)
	{
	}
}

void GNet::Socket::drop()
{
	dropReadHandler() ;
	dropWriteHandler() ;
	dropExceptionHandler() ;
}

void GNet::Socket::close()
{
	drop() ;
	if( valid() ) 
		doClose() ;

	G_ASSERT( !valid() ) ;
}

bool GNet::Socket::valid() const
{
	return valid( m_socket ) ;
}

bool GNet::Socket::bind()
{
	Address address( 0U ) ;
	return bind( address ) ;
}

bool GNet::Socket::bind( const Address & local_address )
{
	if( !valid() )
		return false ;

	// optionally allow immediate re-use -- may cause problems
	setReuse() ;

	G_DEBUG( "Socket::bind: binding " << local_address.displayString() << " on fd " << m_socket ) ;

	int rc = ::bind( m_socket, local_address.address() , local_address.length() ) ;
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

bool GNet::Socket::connect( const Address &addr , bool *done )
{
	if( !valid() )
		return false;

	G_DEBUG( "GNet::Socket::connect: connecting to " << addr.displayString() ) ;
	int rc = ::connect( m_socket, addr.address(), addr.length() ) ;
	if( error(rc) )
	{
		m_reason = reason() ;
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

ssize_t GNet::Socket::write( const char *buf, size_t len )
{
	if( static_cast<ssize_t>(len) < 0 )
		G_WARNING( "GNet::Socket::write: too big" ) ; // EMSGSIZE from ::send() ?

	ssize_t nsent = ::send( m_socket, buf, len, 0 ) ;

	if( sizeError(nsent) ) // if -1
	{
		m_reason = reason() ;
		G_DEBUG( "GNet::Socket::write: write error " << m_reason ) ;
		return -1 ;
	}
	else if( nsent < 0 || static_cast<size_t>(nsent) < len )
	{
		m_reason = reason() ;
	}

	//G_DEBUG( "GNet::Socket::write: wrote " << nsent << "/" << len << " byte(s)" ) ;
	return nsent;
}

void GNet::Socket::setNoLinger()
{
	struct linger options ;
	options.l_onoff = 0 ;
	options.l_linger = 0 ;
	socklen_t sizeof_options = sizeof(options) ;
	G_IGNORE ::setsockopt( m_socket , SOL_SOCKET , SO_LINGER , 
		reinterpret_cast<char*>(&options) , sizeof_options ) ;
}

void GNet::Socket::setKeepAlive()
{
	int keep_alive = 1 ;
	G_IGNORE ::setsockopt( m_socket , SOL_SOCKET , 
		SO_KEEPALIVE , reinterpret_cast<char*>(&keep_alive) , sizeof(keep_alive) ) ;
}

void GNet::Socket::setReuse()
{
	int on = 1 ;
	G_IGNORE ::setsockopt( m_socket , SOL_SOCKET , 
		SO_REUSEADDR , reinterpret_cast<char*>(&on) , sizeof(on) ) ;
}

bool GNet::Socket::listen( int backlog )
{
	int rc = ::listen( m_socket, backlog ) ;
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

	static sockaddr saddr_zero ;
	sockaddr saddr( saddr_zero ) ;
	socklen_t addr_length = sizeof(saddr) ;
	int rc = 
		local ? 
			::getsockname( m_socket , &saddr , &addr_length ) :
			::getpeername( m_socket , &saddr , &addr_length ) ;

	if( error(rc) )
	{
		const_cast<Socket*>(this)->m_reason = reason() ;
		return error_pair ;
	}

	return std::pair<bool,Address>( true , Address( &saddr, addr_length ) ) ;
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

//===================================================================

GNet::StreamSocket::StreamSocket() : 
	Socket( PF_INET , SOCK_STREAM )
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

bool GNet::StreamSocket::reopen()
{
	G_ASSERT( !valid() ) ;
	if( valid() )
		close() ;

	return open( PF_INET , SOCK_STREAM ) ;
}

ssize_t GNet::StreamSocket::read( char *buf , size_t len )
{
	if( len == 0 ) return 0 ;
	G_ASSERT( valid() ) ;
	ssize_t nread = ::recv( m_socket , buf , len , 0 ) ;
	if( sizeError(nread) )
	{
		m_reason = reason() ;
		G_DEBUG( "GNet::StreamSocket::read: fd " << m_socket << ": read error " << m_reason ) ;
		return -1 ;
	}

	//G_DEBUG( "GNet::StreamSocket::read: fd " << m_socket << ": read " << nread << " bytes(s)" ) ;
	return nread ;
}

GNet::AcceptPair GNet::StreamSocket::accept()
{
	AcceptPair pair( NULL , Address::invalidAddress() ) ;

	sockaddr addr ;
	socklen_t addr_length = sizeof(addr) ;
	Descriptor new_socket = ::accept( m_socket, &addr, &addr_length ) ;
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

//===================================================================

GNet::DatagramSocket::DatagramSocket() : 
	Socket( PF_INET , SOCK_DGRAM )
{
}

GNet::DatagramSocket::~DatagramSocket()
{
}

void GNet::DatagramSocket::disconnect()
{
	int rc = ::connect( m_socket , 0 , 0 ) ;
	if( error(rc) )
		m_reason = reason() ;
}

bool GNet::DatagramSocket::reopen()
{
	G_ASSERT( !valid() ) ;
	if( valid() )
		close() ;

	return open( PF_INET , SOCK_DGRAM ) ;
}

ssize_t GNet::DatagramSocket::read( void *buf , size_t len , Address & src_address )
{
	sockaddr sender ;
	socklen_t sender_len = sizeof(sender) ;
	ssize_t nread = ::recvfrom( m_socket, reinterpret_cast<char*>(buf), len, 0, &sender, &sender_len ) ;
	if( sizeError(nread) )
	{
		m_reason = reason() ;
		return -1 ;
	}

	src_address = Address( &sender , sender_len ) ;
	return nread ;
}

ssize_t GNet::DatagramSocket::write( const char *buf , size_t len , const Address &dst )
{
	G_DEBUG( "GNet::DatagramSocket::write: sending " << len << " bytes to " 
		<< dst.displayString() ) ;

	ssize_t nsent = ::sendto( m_socket, buf, 
		len, 0, dst.address(), dst.length() ) ;

	if( nsent < 0 )
	{
		m_reason = reason() ;
		G_DEBUG( "GNet::DatagramSocket::write: write error " << m_reason ) ;
		return -1 ;
	}
	return nsent ;
}


