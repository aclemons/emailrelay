//
// Copyright (C) 2001-2020 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gsocket.h"
#include "gtest.h"
#include "gsleep.h"
#include "gmsg.h"
#include "gstr.h"
#include "glog.h"

GNet::SocketBase::SocketBase( int domain , int type , int protocol ) :
	m_reason(0) ,
	m_domain(domain)
{
	if( !create(domain,type,protocol ) )
		throw SocketError( "cannot create socket" , m_reason_string ) ;

	if( !prepare(false) )
	{
		destroy() ;
		throw SocketError( "cannot prepare socket" , m_reason_string ) ;
	}
}

GNet::SocketBase::SocketBase( int domain , Descriptor fd , const Accepted & ) :
	m_reason(0) ,
	m_domain(domain),
	m_fd(fd)
{
	if( !prepare(true) )
	{
		destroy() ;
		throw SocketError( "cannot prepare socket" , m_reason_string ) ;
	}
}

GNet::SocketBase::~SocketBase()
{
	drop() ;
	destroy() ;
}

void GNet::SocketBase::drop() noexcept
{
	dropReadHandler() ;
	dropWriteHandler() ;
	dropOtherHandler() ;
}

void GNet::SocketBase::clearReason()
{
	m_reason = 0 ;
	m_reason_string.clear() ;
}

void GNet::SocketBase::saveReason() const
{
	const_cast<GNet::SocketBase*>(this)->saveReason() ;
}

GNet::SocketBase::ssize_type GNet::SocketBase::writeImp( const char * buffer , size_type length )
{
	if( static_cast<ssize_type>(length) < 0 )
		G_WARNING( "GNet::SocketBase::writeImp: too big" ) ; // should get EMSGSIZE from ::send()

	ssize_type nsent = G::Msg::send( m_fd.fd() , buffer , length , MSG_NOSIGNAL ) ;
	if( sizeError(nsent) ) // if -1
	{
		saveReason() ;
		G_DEBUG( "GNet::SocketBase::writeImp: write error " << m_reason ) ;
		return -1 ;
	}
	else if( nsent < 0 || static_cast<size_type>(nsent) < length )
	{
		saveReason() ;
	}
	return nsent;
}

void GNet::SocketBase::addReadHandler( EventHandler & handler , ExceptionSink es )
{
	G_DEBUG( "GNet::SocketBase::addReadHandler: fd " << m_fd ) ;
	EventLoop::instance().addRead( m_fd , handler , es ) ;
}

void GNet::SocketBase::addWriteHandler( EventHandler & handler , ExceptionSink es )
{
	G_DEBUG( "GNet::SocketBase::addWriteHandler: fd " << m_fd ) ;
	EventLoop::instance().addWrite( m_fd , handler , es ) ;
}

void GNet::SocketBase::addOtherHandler( EventHandler & handler , ExceptionSink es )
{
	G_DEBUG( "GNet::SocketBase::addOtherHandler: fd " << m_fd ) ;
	EventLoop::instance().addOther( m_fd , handler , es ) ;
}

void GNet::SocketBase::dropReadHandler() noexcept
{
	if( EventLoop::ptr() )
		EventLoop::ptr()->dropRead( m_fd ) ;
}

void GNet::SocketBase::dropWriteHandler() noexcept
{
	if( EventLoop::ptr() )
		EventLoop::ptr()->dropWrite( m_fd ) ;
}

void GNet::SocketBase::dropOtherHandler() noexcept
{
	if( EventLoop::ptr() )
		EventLoop::ptr()->dropOther( m_fd ) ;
}

SOCKET GNet::SocketBase::fd() const
{
	return m_fd.fd() ;
}

int GNet::SocketBase::domain() const
{
	return m_domain ;
}

std::string GNet::SocketBase::reason() const
{
	if( m_reason == 0 ) return std::string() ;
	return reasonString( m_reason ) ;
}

std::string GNet::SocketBase::asString() const
{
	std::ostringstream ss ;
	ss << m_fd ;
	return ss.str() ;
}

// ==

GNet::Socket::Socket( int domain , int type , int protocol ) :
	SocketBase(domain,type,protocol)
{
}

GNet::Socket::Socket( int domain , Descriptor s , const Accepted & a ) :
	SocketBase(domain,s,a)
{
}

void GNet::Socket::bind( const Address & local_address )
{
	G_DEBUG( "Socket::bind: binding " << local_address.displayString() << " on fd " << fd() ) ;

	if( local_address.domain() != domain() )
		throw SocketBindError( "address family does not match the socket domain" ) ;

	setOptionsOnBind( local_address.family() == Address::Family::ipv6 ) ;

	int rc = ::bind( fd() , local_address.address() , local_address.length() ) ;
	if( error(rc) )
	{
		saveReason() ;
		throw SocketBindError( local_address.displayString() , reason() ) ;
	}
	m_bound_scope_id = local_address.scopeId() ;
}

bool GNet::Socket::bind( const Address & local_address , NoThrow )
{
	G_DEBUG( "Socket::bind: binding " << local_address.displayString() << " on fd " << fd() ) ;
	if( local_address.domain() != domain() ) return false ;

	setOptionsOnBind( local_address.family() == Address::Family::ipv6 ) ;

	int rc = ::bind( fd() , local_address.address() , local_address.length() ) ;
	if( error(rc) )
	{
		saveReason() ;
		return false ;
	}
	m_bound_scope_id = local_address.scopeId() ;
	return true ;
}

unsigned long GNet::Socket::getBoundScopeId() const
{
	return m_bound_scope_id ;
}

bool GNet::Socket::connect( const Address & address , bool *done )
{
	G_DEBUG( "GNet::Socket::connect: connecting to " << address.displayString() ) ;
	if( address.domain() != domain() )
	{
		G_WARNING( "GNet::Socket::connect: cannot connect: address family does not match "
			"the socket domain (" << address.domain() << "," << domain() << ")" ) ;
		return false ;
	}

	setOptionsOnConnect( address.family() == Address::Family::ipv6 ) ;

	int rc = ::connect( fd() , address.address() , address.length() ) ;
	if( error(rc) )
	{
		saveReason() ;

		if( G::Test::enabled("socket-slow-connect") )
			sleep( 1 ) ;

		if( eInProgress() )
		{
			G_DEBUG( "GNet::Socket::connect: connection in progress" ) ;
			if( done != nullptr ) *done = false ;
			return true ;
		}

		G_DEBUG( "GNet::Socket::connect: synchronous connect failure: " << reason() ) ;
		return false;
	}

	if( done != nullptr ) *done = true ;
	return true ;
}

void GNet::Socket::listen( int backlog )
{
	int rc = ::listen( fd() , backlog ) ;
	if( error(rc) )
	{
		saveReason() ;
		throw SocketError( "cannot listen on socket" , reason() ) ;
	}
}

std::pair<bool,GNet::Address> GNet::Socket::getAddress( bool local ) const
{
	std::pair<bool,Address> error_pair( false , Address::defaultAddress() ) ;
	AddressStorage address_storage ;
	int rc =
		local ?
			::getsockname( fd() , address_storage.p1() , address_storage.p2() ) :
			::getpeername( fd() , address_storage.p1() , address_storage.p2() ) ;

	if( error(rc) )
	{
		saveReason() ;
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

void GNet::Socket::shutdown( int how )
{
	if( G::Test::enabled("socket-no-shutdown") ) return ;
	::shutdown( fd() , how ) ;
}

//==

GNet::StreamSocket::StreamSocket( int address_domain ) :
	Socket(address_domain,SOCK_STREAM,0)
{
	setOptionsOnCreate( false ) ;
}

GNet::StreamSocket::StreamSocket( int address_domain , const Listener & ) :
	Socket(address_domain,SOCK_STREAM,0)
{
	setOptionsOnCreate( true ) ;
}

GNet::StreamSocket::StreamSocket( int domain , Descriptor s , const Socket::Accepted & accepted ) :
	Socket(domain,s,accepted)
{
	setOptionsOnAccept() ;
}

GNet::Socket::ssize_type GNet::StreamSocket::read( char * buffer , size_type length )
{
	if( length == 0 ) return 0 ;
	clearReason() ;
	ssize_type nread = G::Msg::recv( fd() , buffer , length , 0 ) ;
	if( sizeError(nread) )
	{
		saveReason() ;
		G_DEBUG( "GNet::StreamSocket::read: cannot read from " << fd() ) ;
		return -1 ;
	}
	return nread ;
}

GNet::Socket::ssize_type GNet::StreamSocket::write( const char * buffer , size_type length )
{
	return writeImp( buffer , length ) ; // SocketBase
}

GNet::AcceptPair GNet::StreamSocket::accept()
{
	AddressStorage addr ;
	Descriptor new_fd( ::accept(fd(),addr.p1(),addr.p2()) ) ;
	if( ! new_fd.valid() )
	{
		saveReason() ;
		if( eTooMany() )
			throw SocketTooMany( "cannot accept on listening socket" , reason() ) ;
		else
			throw SocketError( "cannot accept on listening socket" , reason() ) ;
	}

	if( G::Test::enabled("socket-accept-throws") )
		throw SocketError( "testing" ) ;

	AcceptPair pair ;
	pair.second = Address( addr.p() , addr.n() ) ;
	pair.first.reset( new StreamSocket(domain(),new_fd,Socket::Accepted()) ) ;

	G_DEBUG( "GNet::StreamSocket::accept: accepted from " << fd()
		<< " to " << new_fd << " (" << pair.second.displayString() << ")" ) ;

	return pair ;
}

//==

GNet::DatagramSocket::DatagramSocket( int address_domain , int protocol ) :
	Socket( address_domain , SOCK_DGRAM , protocol )
{
}

void GNet::DatagramSocket::disconnect()
{
	int rc = ::connect( fd() , nullptr , 0 ) ;
	if( error(rc) )
		saveReason() ;
}

GNet::Socket::ssize_type GNet::DatagramSocket::read( char * buffer , size_type length )
{
	if( length == 0 ) return 0 ;
	sockaddr sender {} ; // not used
	socklen_t sender_len = sizeof(sender) ;
	ssize_type nread = G::Msg::recvfrom( fd() , buffer , length , 0 , &sender , &sender_len ) ;
	if( sizeError(nread) )
	{
		saveReason() ;
		return -1 ;
	}
	return nread ;
}

GNet::Socket::ssize_type GNet::DatagramSocket::readfrom( char * buffer , size_type length , Address & src_address )
{
	if( length == 0 ) return 0 ;
	sockaddr sender {} ;
	socklen_t sender_len = sizeof(sender) ;
	ssize_type nread = G::Msg::recvfrom( fd() , buffer , length , 0 , &sender , &sender_len ) ;
	if( sizeError(nread) )
	{
		saveReason() ;
		return -1 ;
	}
	src_address = Address( &sender , sender_len ) ;
	return nread ;
}

GNet::Socket::ssize_type GNet::DatagramSocket::writeto( const char * buffer , size_type length , const Address & dst )
{
	ssize_type nsent = G::Msg::sendto( fd() , buffer , length , MSG_NOSIGNAL , dst.address() , dst.length() ) ;
	if( nsent < 0 )
	{
		saveReason() ;
		G_DEBUG( "GNet::DatagramSocket::write: write error " << reason() ) ;
		return -1 ;
	}
	return nsent ;
}

GNet::Socket::ssize_type GNet::DatagramSocket::write( const char * buffer , size_type length )
{
	return writeImp( buffer , length ) ; // SocketBase
}

// ==

void GNet::StreamSocket::setOptionsOnCreate( bool /*listener*/ )
{
	if( G::Test::enabled("socket-linger-zero") )
		setOptionLingerImp( 1 , 0 ) ;
	else if( G::Test::enabled("socket-linger-default") )
		;
	else
		setOptionNoLinger() ;

	if( G::Test::enabled("socket-keep-alive") )
		setOptionKeepAlive() ;
}

void GNet::StreamSocket::setOptionsOnAccept()
{
	if( G::Test::enabled("socket-linger-zero") )
		setOptionLingerImp( 1 , 0 ) ;
	else if( G::Test::enabled("socket-linger-default") )
		;
	else
		setOptionNoLinger() ;

	if( G::Test::enabled("socket-keep-alive") )
		setOptionKeepAlive() ;
}

void GNet::Socket::setOptionsOnConnect( bool ipv6 )
{
	setOptionPureV6( ipv6 , NoThrow() ) ; // ignore errors - may fail if already bound
}

void GNet::Socket::setOptionsOnBind( bool ipv6 )
{
	if( G::Test::enabled("socket-no-reuse") )
		;
	else
		setOptionReuse() ; // allow us to rebind another socket's (eg. time-wait zombie's) address

	//setOptionExclusive() ; // don't allow anyone else to bind our address
	setOptionPureV6( ipv6 ) ;
}

void GNet::Socket::setOptionKeepAlive()
{
	setOption( SOL_SOCKET , "so_keepalive" , SO_KEEPALIVE , 1 ) ;
}

void GNet::Socket::setOptionNoLinger()
{
	setOptionLingerImp( 0 , 0 ) ;
}

void GNet::Socket::setOptionLingerImp( int onoff , int time )
{
	struct linger options {} ;
	options.l_onoff = onoff ;
	options.l_linger = time ;
	bool ok = setOptionImp( SOL_SOCKET , SO_LINGER , reinterpret_cast<char*>(&options) , sizeof(options) ) ;
	if( !ok )
	{
		saveReason() ;
		throw SocketError( "cannot set no_linger" , reason() ) ;
	}
}

bool GNet::Socket::setOption( int level , const char * , int op , int arg , NoThrow )
{
	const void * const vp = reinterpret_cast<const void*>(&arg) ;
	bool ok = setOptionImp( level , op , vp , sizeof(int) ) ;
	if( !ok )
		saveReason() ;
	return ok ;
}

void GNet::Socket::setOption( int level , const char * opp , int op , int arg )
{
	if( !setOption( level , opp , op , arg , NoThrow() ) )
		throw SocketError( opp , reason() ) ;
}

/// \file gsocket.cpp
