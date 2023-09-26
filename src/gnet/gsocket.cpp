//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gsocket.cpp
///

#include "gdef.h"
#include "gsocket.h"
#include "gtest.h"
#include "gsleep.h"
#include "glimits.h"
#include "gmsg.h"
#include "gstr.h"
#include "gassert.h"
#include "glog.h"

GNet::SocketBase::SocketBase( Address::Family family , int type , int protocol ) :
	m_reason(0) ,
	m_domain(Address::domain(family)) ,
	m_family(family) ,
	m_read_added(false) ,
	m_write_added(false) ,
	m_other_added(false) ,
	m_accepted(false)
{
	if( !create(m_domain,type,protocol) )
		throw SocketCreateError( "cannot create socket" , reason() ) ;

	if( !prepare(false) )
	{
		destroy() ;
		throw SocketError( "cannot prepare socket" , reason() ) ;
	}
}

GNet::SocketBase::SocketBase( const SocketBase::Raw & , int domain , int type , int protocol ) :
	m_reason(0) ,
	m_domain(domain) ,
	m_family(Address::Family::local) ,
	m_read_added(false) ,
	m_write_added(false) ,
	m_other_added(false) ,
	m_accepted(false)
{
	G_ASSERT( !Address::supports( Address::Domain() , domain ) ) ;

	if( !create(domain,type,protocol) )
		throw SocketCreateError( "cannot create socket" , reason() ) ;

	if( !prepare(false) )
	{
		destroy() ;
		throw SocketError( "cannot prepare socket" , reason() ) ;
	}
}

GNet::SocketBase::SocketBase( Address::Family family , Descriptor fd ) :
	m_reason(0) ,
	m_domain(Address::domain(family)) ,
	m_family(family) ,
	m_fd(fd) ,
	m_read_added(false) ,
	m_write_added(false) ,
	m_other_added(false) ,
	m_accepted(false)
{
	if( !prepare(false) )
	{
		destroy() ;
		throw SocketError( "cannot prepare socket" , reason() ) ;
	}
}

GNet::SocketBase::SocketBase( Address::Family family , Descriptor fd , const Accepted & ) :
	m_reason(0) ,
	m_domain(Address::domain(family)) ,
	m_family(family) ,
	m_fd(fd) ,
	m_read_added(false) ,
	m_write_added(false) ,
	m_other_added(false) ,
	m_accepted(true)
{
	if( !prepare(true) )
	{
		destroy() ;
		throw SocketError( "cannot prepare socket" , reason() ) ;
	}
}

GNet::SocketBase::~SocketBase()
{
	drop() ;
	destroy() ;
}

bool GNet::SocketBase::isFamily( Address::Family family ) const
{
	// note that raw sockets to not have a family supported by
	// GNet::Address and their m_address field is bogus
	return Address::supports(Address::Domain(),m_domain) && family == m_family ;
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
		G_DEBUG( "GNet::SocketBase::writeImp: write error: " << reason() ) ;
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
	if( !m_read_added )
		EventLoop::instance().addRead( m_fd , handler , es ) ;
	m_read_added = true ;
}

void GNet::SocketBase::addWriteHandler( EventHandler & handler , ExceptionSink es )
{
	G_DEBUG( "GNet::SocketBase::addWriteHandler: fd " << m_fd ) ;
	if( !m_write_added )
		EventLoop::instance().addWrite( m_fd , handler , es ) ;
	m_write_added = true ;
}

void GNet::SocketBase::addOtherHandler( EventHandler & handler , ExceptionSink es )
{
	G_DEBUG( "GNet::SocketBase::addOtherHandler: fd " << m_fd ) ;
	if( !m_other_added )
		EventLoop::instance().addOther( m_fd , handler , es ) ;
	m_other_added = true ;
}

void GNet::SocketBase::dropReadHandler() noexcept
{
	if( m_read_added && EventLoop::ptr() )
		EventLoop::ptr()->dropRead( m_fd ) ;
	m_read_added = false ;
}

void GNet::SocketBase::dropWriteHandler() noexcept
{
	if( m_write_added && EventLoop::ptr() )
		EventLoop::ptr()->dropWrite( m_fd ) ;
	m_write_added = false ;
}

void GNet::SocketBase::dropOtherHandler() noexcept
{
	if( m_other_added && EventLoop::ptr() )
		EventLoop::ptr()->dropOther( m_fd ) ;
	m_other_added = false ;
}

SOCKET GNet::SocketBase::fd() const noexcept
{
	return m_fd.fd() ;
}

std::string GNet::SocketBase::reason() const
{
	if( m_reason == 0 ) return {} ;
	return reasonString( m_reason ) ;
}

std::string GNet::SocketBase::asString() const
{
	std::ostringstream ss ;
	ss << m_fd ;
	return ss.str() ;
}

// ==

GNet::Socket::Config::Config()
= default ;

GNet::Socket::Socket( Address::Family af , int type , int protocol , const Config & config ) :
	SocketBase(af,type,protocol) ,
	m_config(config)
{
}

GNet::Socket::Socket( Address::Family af , Descriptor s , const Accepted & a , const Config & config ) :
	SocketBase(af,s,a) ,
	m_config(config)
{
}

GNet::Socket::Socket( Address::Family af , Descriptor s , const Adopted & , const Config & config ) :
	SocketBase(af,s) ,
	m_config(config)
{
}

void GNet::Socket::bind( const Address & local_address )
{
	G_DEBUG( "Socket::bind: binding " << local_address.displayString() << " on fd " << fd() ) ;

	if( !isFamily( local_address.family() ) )
		throw SocketBindError( "address family does not match the socket domain" ) ;

	setOptionsOnBind( local_address.family() ) ;

	int rc = ::bind( fd() , local_address.address() , local_address.length() ) ;
	if( error(rc) )
	{
		saveReason() ;
		throw SocketBindError( local_address.displayString() , reason() ) ;
	}
	m_bound_scope_id = local_address.scopeId() ;
}

bool GNet::Socket::bind( const Address & local_address , std::nothrow_t )
{
	G_DEBUG( "Socket::bind: binding " << local_address.displayString() << " on fd " << fd() ) ;
	if( !isFamily( local_address.family() ) )
		return false ;

	setOptionsOnBind( local_address.family() ) ;

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

bool GNet::Socket::connect( const Address & address , bool * done )
{
	G_DEBUG( "GNet::Socket::connect: connecting to " << address.displayString() ) ;
	if( !isFamily( address.family() ) )
	{
		G_WARNING( "GNet::Socket::connect: cannot connect: address family does not match the socket domain" ) ;
		return false ;
	}

	setOptionsOnConnect( address.family() ) ;

	int rc = ::connect( fd() , address.address() , address.length() ) ;
	if( error(rc) )
	{
		saveReason() ;

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

void GNet::Socket::listen()
{
	int listen_queue = m_config.listen_queue ;
	if( listen_queue <= 0 )
		listen_queue = G::Limits<>::net_listen_queue ;

	int rc = ::listen( fd() , std::max(1,listen_queue) ) ; // see also SOMAXCONN
	if( error(rc) )
	{
		saveReason() ;
		throw SocketError( "cannot listen on socket" , reason() ) ;
	}
}

GNet::Address GNet::Socket::getLocalAddress( Descriptor fd )
{
	AddressStorage address_storage ;
	int rc = ::getsockname( fd.fd() , address_storage.p1() , address_storage.p2() ) ;
	if( error(rc) )
		throw SocketError( std::string("no bound address on fd ").append(std::to_string(fd.fd())) ) ;
	return Address( address_storage ) ;
}

GNet::Address GNet::Socket::getLocalAddress() const
{
	AddressStorage address_storage ;
	int rc = ::getsockname( fd() , address_storage.p1() , address_storage.p2() ) ;
	if( error(rc) )
	{
		saveReason() ;
		throw SocketError( "getsockname" , reason() ) ;
	}
	return Address( address_storage ) ;
}

std::pair<bool,GNet::Address> GNet::Socket::getPeerAddress() const
{
	AddressStorage address_storage ;
	int rc = ::getpeername( fd() , address_storage.p1() , address_storage.p2() ) ;
	if( error(rc) )
	{
		saveReason() ;
		if( eNotConn() )
			return { false , Address::defaultAddress() } ;
		throw SocketError( "getpeername" , reason() ) ;
	}
	return { true , Address(address_storage) } ;
}

void GNet::Socket::shutdown( int how )
{
	if( G::Test::enabled("socket-no-shutdown") ) return ;
	::shutdown( fd() , how ) ;
}

void GNet::Socket::setOptionsOnConnect( Address::Family af )
{
	if( af == Address::Family::ipv4 || af == Address::Family::ipv6 )
	{
		if( af == Address::Family::ipv6 && m_config.connect_pureipv6 )
			setOptionPureV6( std::nothrow ) ; // ignore errors - may fail if already bound
	}
}

void GNet::Socket::setOptionsOnBind( Address::Family af )
{
	if( af == Address::Family::ipv4 || af == Address::Family::ipv6 )
	{
		if( m_config.bind_reuse )
			setOptionReuse() ;
		if( m_config.bind_exclusive )
			setOptionExclusive() ;
		if( m_config.free_bind )
			setOptionFreeBind() ;
		if( af == Address::Family::ipv6 && m_config.bind_pureipv6 )
			setOptionPureV6() ;
	}
}

void GNet::Socket::setOptionKeepAlive()
{
	setOption( SOL_SOCKET , "so_keepalive" , SO_KEEPALIVE , 1 ) ;
}

void GNet::Socket::setOptionFreeBind()
{
	// not tested -- can also use /proc
	//setOption( IPPROTO_IP , "so_freebind" , IP_FREEBIND , 1 ) ;
}

void GNet::Socket::setOptionLinger( int onoff , int time )
{
	struct linger linger_config {} ;
	linger_config.l_onoff = static_cast<decltype(linger::l_onoff)>(onoff) ;
	linger_config.l_linger = static_cast<decltype(linger::l_linger)>(time) ;
	bool ok = setOptionImp( SOL_SOCKET , SO_LINGER , &linger_config , sizeof(linger_config) ) ;
	if( !ok )
	{
		saveReason() ;
		throw SocketError( "cannot set socket linger option" , reason() ) ;
	}
}

bool GNet::Socket::setOption( int level , const char * , int op , int arg , std::nothrow_t )
{
	const void * const vp = static_cast<const void*>(&arg) ;
	bool ok = setOptionImp( level , op , vp , sizeof(int) ) ;
	if( !ok )
		saveReason() ;
	return ok ;
}

void GNet::Socket::setOption( int level , const char * opp , int op , int arg )
{
	if( !setOption( level , opp , op , arg , std::nothrow ) )
		throw SocketError( opp , reason() ) ;
}

//==

GNet::StreamSocket::Config::Config()
= default ;

GNet::StreamSocket::Config::Config( const Socket::Config & base ) :
	Socket::Config(base)
{
}

bool GNet::StreamSocket::supports( Address::Family af )
{
	if( af == Address::Family::ipv6 )
	{
		static bool first = true ;
		static bool result = false ;
		if( first )
		{
			first = false ;
			if( !Address::supports(af) )
				G_WARNING( "GNet::StreamSocket::supports: no ipv6 support built-in" ) ;
			else if( !SocketBase::supports(af,SOCK_STREAM,0) )
				G_WARNING( "GNet::StreamSocket::supports: no ipv6 support detected" ) ;
			else
				result = true ;
		}
		return result ;
	}
	else if( af == Address::Family::local )
	{
		return Address::supports( af ) ;
	}
	else
	{
		return true ; // ipv4 always supported
	}
}

GNet::StreamSocket::StreamSocket( Address::Family af , const Config & config ) :
	Socket(af,SOCK_STREAM,0,config) ,
	m_config(config)
{
	setOptionsOnCreate( af , /*listener=*/false ) ;
}

GNet::StreamSocket::StreamSocket( Address::Family af , const Listener & , const Config & config ) :
	Socket(af,SOCK_STREAM,0,config) ,
	m_config(config)
{
	setOptionsOnCreate( af , /*listener=*/true ) ;
}

GNet::StreamSocket::StreamSocket( const Listener & , Descriptor fd , const Config & config ) :
	Socket(family(fd),fd,Socket::Adopted(),config) ,
	m_config(config)
{
}

GNet::StreamSocket::StreamSocket( Address::Family af , Descriptor fd , const Accepted & accepted , const Config & config ) :
	Socket(af,fd,accepted,config) ,
	m_config(config)
{
	setOptionsOnAccept( af ) ;
}

GNet::Address::Family GNet::StreamSocket::family( Descriptor fd )
{
	return getLocalAddress(fd).family() ;
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

GNet::AcceptInfo GNet::StreamSocket::accept()
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

	AcceptInfo info ;
	info.address = Address( addr ) ;
	info.socket_ptr.reset( new StreamSocket( info.address.family() , new_fd , SocketBase::Accepted() , m_config ) ) ; // 'new' sic

	G_DEBUG( "GNet::StreamSocket::accept: accepted from " << fd()
		<< " to " << new_fd << " (" << info.address.displayString() << ")" ) ;

	return info ;
}

void GNet::StreamSocket::setOptionsOnCreate( Address::Family af , bool /*listener*/ )
{
	if( af == Address::Family::ipv4 || af == Address::Family::ipv6 )
	{
		if( m_config.create_linger_onoff == 1 )
			setOptionLinger( 1 , m_config.create_linger_time ) ;
		else if( m_config.create_linger_onoff == 0 )
			setOptionLinger( 0 , 0 ) ;
		if( m_config.create_keepalive )
			setOptionKeepAlive() ;
	}
}

void GNet::StreamSocket::setOptionsOnAccept( Address::Family af )
{
	if( af == Address::Family::ipv4 || af == Address::Family::ipv6 )
	{
		if( m_config.accept_linger_onoff == 1 )
			setOptionLinger( 1 , m_config.accept_linger_time ) ;
		else if( m_config.accept_linger_onoff == 0 )
			setOptionLinger( 0 , 0 ) ;
		if( m_config.accept_keepalive )
			setOptionKeepAlive() ;
	}
}

//==

GNet::DatagramSocket::Config::Config()
= default ;

GNet::DatagramSocket::Config::Config( const Socket::Config & base ) :
	Socket::Config(base)
{
}

GNet::DatagramSocket::DatagramSocket( Address::Family af , int protocol , const Config & config ) :
	Socket(af,SOCK_DGRAM,protocol,config)
{
}

#ifndef G_LIB_SMALL
void GNet::DatagramSocket::disconnect()
{
	int rc = ::connect( fd() , nullptr , 0 ) ;
	if( error(rc) )
		saveReason() ;
}
#endif

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

