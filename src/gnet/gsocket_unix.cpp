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
/// \file gsocket_unix.cpp
///

#include "gdef.h"
#include "gsocket.h"
#include "gmsg.h"
#include "gprocess.h"
#include "gstr.h"
#include "gcleanup.h"
#include "glog.h"
#include <cerrno> // EWOULDBLOCK etc
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

bool GNet::SocketBase::supports( int domain , int type , int protocol )
{
	int fd = ::socket( domain , type , protocol ) ;
	if( fd < 0 )
		return false ;
	::close( fd ) ;
	return true ;
}

bool GNet::SocketBase::create( int domain , int type , int protocol )
{
	m_fd = Descriptor( ::socket(domain,type,protocol) ) ;
	if( m_fd == Descriptor::invalid() )
	{
		saveReason() ;
		return false ;
	}
	return true ;
}

bool GNet::SocketBase::prepare( bool /*accepted*/ )
{
	static bool first = true ;
	if( first )
	{
		first = false ;
		G::Cleanup::init() ; // ignore SIGPIPE
	}

	if( !setNonBlock() )
	{
		saveReason() ;
		return false ;
	}
	return true ;
}

void GNet::SocketBase::destroy() noexcept
{
	::close( m_fd.fd() ) ;
}

bool GNet::SocketBase::error( int rc )
{
	return rc < 0 ;
}

void GNet::SocketBase::saveReason()
{
	m_reason = G::Process::errno_() ;
	m_reason_string = reasonString( m_reason ) ;
}

bool GNet::SocketBase::setNonBlock()
{
	int mode = ::fcntl( m_fd.fd() , F_GETFL ) ;
	if( mode < 0 )
		return false ;

	int rc = ::fcntl( m_fd.fd() , F_SETFL , mode | O_NONBLOCK ) ;
	return rc == 0 ;
}

bool GNet::SocketBase::sizeError( ssize_t size )
{
	return size < 0 ;
}

bool GNet::SocketBase::eWouldBlock() const
{
	return m_reason == EWOULDBLOCK || m_reason == EAGAIN || m_reason == EINTR ;
}

bool GNet::SocketBase::eInProgress() const
{
	return m_reason == EINPROGRESS ;
}

bool GNet::SocketBase::eMsgSize() const
{
	return m_reason == EMSGSIZE ;
}

bool GNet::SocketBase::eTooMany() const
{
	return m_reason == EMFILE ;
}

std::string GNet::SocketBase::reasonString( int e )
{
	return G::Str::lower(G::Process::strerror(e)) ;
}

// ==

bool GNet::Socket::canBindHint( const Address & address )
{
	return bind( address , std::nothrow ) ;
}

void GNet::Socket::setOptionReuse()
{
	// allow bind on TIME_WAIT address -- see also SO_REUSEPORT
	setOption( SOL_SOCKET , "so_reuseaddr" , SO_REUSEADDR , 1 ) ;
}

void GNet::Socket::setOptionExclusive()
{
	// no-op
}

void GNet::Socket::setOptionPureV6( bool active )
{
	#if GCONFIG_HAVE_IPV6
		if( active )
			setOption( IPPROTO_IPV6 , "ipv6_v6only" , IPV6_V6ONLY , 1 ) ;
	#else
		if( active )
			throw SocketError( "cannot set socket option for pure ipv6" ) ;
	#endif
}

bool GNet::Socket::setOptionPureV6( bool active , std::nothrow_t )
{
	#if GCONFIG_HAVE_IPV6
		return active ? setOption( IPPROTO_IPV6 , "ipv6_v6only" , IPV6_V6ONLY , 1 , std::nothrow ) : true ;
	#else
		return !active ;
	#endif
}

bool GNet::Socket::setOptionImp( int level , int op , const void * arg , socklen_t n )
{
	int rc = ::setsockopt( fd() , level , op , arg , n ) ;
	return ! error(rc) ;
}

// ==

GNet::RawSocket::RawSocket( int domain , int protocol ) :
	SocketBase(domain,SOCK_RAW,protocol)
{
}

GNet::SocketBase::ssize_type GNet::RawSocket::read( char * buffer , size_type length )
{
	if( length == 0 ) return 0 ;
	clearReason() ;
	ssize_type nread = G::Msg::recv( fd() , buffer , length , 0 ) ;
	if( sizeError(nread) )
	{
		saveReason() ;
		G_DEBUG( "GNet::RawSocket::read: cannot read from " << fd() ) ;
		return -1 ;
	}
	return nread ;
}

GNet::SocketBase::ssize_type GNet::RawSocket::write( const char * buffer , size_type length )
{
	return writeImp( buffer , length ) ; // SocketBase
}

