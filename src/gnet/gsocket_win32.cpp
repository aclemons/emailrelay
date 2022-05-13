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
/// \file gsocket_win32.cpp
///

#include "gdef.h"
#include "gsocket.h"
#include "gprocess.h"
#include "gstr.h"
#include "gassert.h"
#include <errno.h>

bool GNet::SocketBase::supports( Address::Family af , int type , int protocol )
{
	SOCKET fd = ::socket( Address::domain(af) , type , protocol ) ;
	if( fd == INVALID_SOCKET )
		return false ;
	::closesocket( fd ) ;
	return true ;
}

bool GNet::SocketBase::create( int domain , int type , int protocol )
{
	m_fd = Descriptor( ::socket( domain , type , protocol ) , 0 ) ;
	if( !m_fd.valid() )
	{
		saveReason() ;
		return false ;
	}

	m_fd = Descriptor( m_fd.fd() , WSACreateEvent() ) ;
	if( m_fd.h() == HNULL )
	{
		saveReason() ;
		::closesocket( m_fd.fd() ) ;
		return false ;
	}
	return true ;
}

bool GNet::SocketBase::prepare( bool accepted )
{
	if( accepted )
	{
		G_ASSERT( m_fd.h() == HNULL ) ;
		HANDLE h = WSACreateEvent() ;
		if( h == HNULL )
		{
			saveReason() ;
			return false ;
		}
		m_fd = Descriptor( m_fd.fd() , h ) ;
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
	if( m_fd.h() != HNULL )
		WSACloseEvent( m_fd.h() ) ;

	if( m_fd.valid() )
		::closesocket( m_fd.fd() ) ;
}

bool GNet::SocketBase::error( int rc )
{
	return rc == SOCKET_ERROR ;
}

void GNet::SocketBase::saveReason()
{
	m_reason = WSAGetLastError() ;
}

bool GNet::SocketBase::sizeError( ssize_t size )
{
	return size == SOCKET_ERROR ;
}

bool GNet::SocketBase::eNotConn() const
{
	return m_reason == WSAENOTCONN ;
}

bool GNet::SocketBase::eWouldBlock() const
{
	return m_reason == WSAEWOULDBLOCK ;
}

bool GNet::SocketBase::eInProgress() const
{
	return m_reason == WSAEWOULDBLOCK ; // sic -- WSAEINPROGRESS has different semantics wrt. Unix
}

bool GNet::SocketBase::eMsgSize() const
{
	return m_reason == WSAEMSGSIZE ;
}

bool GNet::SocketBase::eTooMany() const
{
	return m_reason == WSAEMFILE ; // or WSAENOBUFS ?
}

bool GNet::SocketBase::setNonBlocking()
{
	unsigned long ul = 1 ;
	return ioctlsocket( m_fd.fd() , FIONBIO , &ul ) != SOCKET_ERROR ;
}

std::string GNet::SocketBase::reasonString( int e )
{
	return G::Process::strerror( e ) ;
}

// ==

std::string GNet::Socket::canBindHint( const Address & , bool )
{
	return std::string() ; // rebinding the same port number fails, so a dummy implementation here
}

void GNet::Socket::setOptionReuse()
{
	setOption( SOL_SOCKET , "so_reuseaddr" , SO_REUSEADDR , 1 ) ;
}

void GNet::Socket::setOptionExclusive()
{
	setOption( SOL_SOCKET , "so_exclusiveaddruse" , SO_EXCLUSIVEADDRUSE , 1 ) ;
}

void GNet::Socket::setOptionPureV6()
{
	// no-op
}

bool GNet::Socket::setOptionPureV6( std::nothrow_t )
{
	return true ; // no-op
}

bool GNet::Socket::setOptionImp( int level , int op , const void * arg , socklen_t n )
{
	const char * cp = static_cast<const char*>(arg) ;
	int rc = ::setsockopt( fd() , level , op , cp , n ) ;
	return ! error(rc) ;
}

