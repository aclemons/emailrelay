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
/// \file gsocket_unix.cpp
///

#include "gdef.h"
#include "gsocket.h"
#include "gmsg.h"
#include "gprocess.h"
#include "gstr.h"
#include "gfile.h"
#include "gcleanup.h"
#include "glog.h"
#include <cerrno> // EWOULDBLOCK etc
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

bool GNet::SocketBase::supports( Address::Family af , int type , int protocol )
{
	int fd = ::socket( Address::domain(af) , type , protocol ) ;
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

	if( !setNonBlocking() )
	{
		saveReason() ;
		return false ;
	}
	return true ;
}

void GNet::SocketBase::destroy() noexcept
{
	if( m_domain == PF_UNIX && !m_accepted ) unlink() ;
	::close( m_fd.fd() ) ;
}

void GNet::SocketBase::unlink() noexcept
{
	try
	{
		AddressStorage address_storage ;
		int rc = ::getsockname( m_fd.fd() , address_storage.p1() , address_storage.p2() ) ;
		std::string path = rc == 0 ? Address(address_storage).hostPartString() : std::string() ;
		if( !path.empty() && path.at(0U) == '/' )
		{
			G_DEBUG( "GNet::SocketBase::unlink: deleting unix-domain socket: fd=" << m_fd.fd() << " path=[" << G::Str::printable(path) << "]" ) ;
			G::File::remove( path , std::nothrow ) ; // best-effort -- see also G::Root
		}
	}
	catch(...)
	{
	}
}

bool GNet::SocketBase::error( int rc )
{
	return rc < 0 ;
}

void GNet::SocketBase::saveReason()
{
	m_reason = G::Process::errno_() ;
}

bool GNet::SocketBase::setNonBlocking()
{
	int mode = ::fcntl( m_fd.fd() , F_GETFL ) ; // NOLINT
	if( mode < 0 )
		return false ;

	int rc = ::fcntl( m_fd.fd() , F_SETFL , mode | O_NONBLOCK ) ; // NOLINT
	return rc == 0 ;
}

bool GNet::SocketBase::sizeError( ssize_t size )
{
	return size < 0 ;
}

bool GNet::SocketBase::eNotConn() const
{
	return m_reason == ENOTCONN ;
}

bool GNet::SocketBase::eWouldBlock() const
{
	return m_reason == EWOULDBLOCK || m_reason == EAGAIN || m_reason == EINTR ;
}

bool GNet::SocketBase::eInProgress() const
{
	return m_reason == EINPROGRESS ;
}

#ifndef G_LIB_SMALL
bool GNet::SocketBase::eMsgSize() const
{
	return m_reason == EMSGSIZE ;
}
#endif

bool GNet::SocketBase::eTooMany() const
{
	return m_reason == EMFILE ;
}

std::string GNet::SocketBase::reasonString( int e )
{
	return G::Process::strerror( e ) ;
}

// ==

#ifndef G_LIB_SMALL
std::string GNet::Socket::canBindHint( const Address & address , bool stream , const Config & config )
{
	if( address.family() == Address::Family::ipv4 || address.family() == Address::Family::ipv6 )
	{
		if( stream )
		{
			StreamSocket s( address.family() , StreamSocket::Config(config) ) ;
			return s.bind( address , std::nothrow ) ? std::string() : s.reason() ;
		}
		else
		{
			int protocol = 0 ;
			DatagramSocket s( address.family() , protocol , DatagramSocket::Config(config) ) ;
			return s.bind( address , std::nothrow ) ? std::string() : s.reason() ;
		}
	}
	else
	{
		return {} ; // could do better
	}
}
#endif

void GNet::Socket::setOptionReuse()
{
	// allow bind on TIME_WAIT address -- see also SO_REUSEPORT
	setOption( SOL_SOCKET , "so_reuseaddr" , SO_REUSEADDR , 1 ) ;
}

void GNet::Socket::setOptionExclusive()
{
	// no-op
}

void GNet::Socket::setOptionPureV6()
{
	#if GCONFIG_HAVE_IPV6
		setOption( IPPROTO_IPV6 , "ipv6_v6only" , IPV6_V6ONLY , 1 ) ;
	#else
		throw SocketError( "cannot set socket option for pure ipv6" ) ;
	#endif
}

bool GNet::Socket::setOptionPureV6( std::nothrow_t )
{
	#if GCONFIG_HAVE_IPV6
		return setOption( IPPROTO_IPV6 , "ipv6_v6only" , IPV6_V6ONLY , 1 , std::nothrow ) ;
	#endif
}

bool GNet::Socket::setOptionImp( int level , int op , const void * arg , socklen_t n )
{
	int rc = ::setsockopt( fd() , level , op , arg , n ) ;
	return ! error(rc) ;
}

// ==

GNet::RawSocket::RawSocket( int domain , int type , int protocol ) :
	SocketBase(SocketBase::Raw(),domain,type,protocol)
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

// ==

#ifndef G_LIB_SMALL
std::size_t GNet::DatagramSocket::limit( std::size_t default_in ) const
{
	int value = 0 ;
	socklen_t size = sizeof(int) ;
	int rc = ::getsockopt( fd() , SOL_SOCKET , SO_SNDBUF , &value , &size ) ;
	if( rc == 0 && size == sizeof(int) && value >= 0 && static_cast<std::size_t>(value) > default_in )
		return static_cast<std::size_t>(value) ;
	else
		return default_in ;
}
#endif

#ifndef G_LIB_SMALL
GNet::Socket::ssize_type GNet::DatagramSocket::writeto( const std::vector<G::string_view> & data , const Address & dst )
{
	ssize_type nsent = G::Msg::sendto( fd() , data , MSG_NOSIGNAL , dst.address() , dst.length() ) ;
	if( nsent < 0 )
	{
		saveReason() ;
		G_DEBUG( "GNet::DatagramSocket::write: write error " << reason() ) ;
		return -1 ;
	}
	return nsent ;
}
#endif

