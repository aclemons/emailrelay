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
// gsocket_win32.cpp
//

#include "gdef.h"
#include "gsocket.h"
#include "gconvert.h"
#include "gstr.h"
#include "gassert.h"
#include <errno.h>

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
		HANDLE h = WSACreateEvent() ; // handle errors in the event loop
		m_fd = Descriptor( m_fd.fd() , h ) ;
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
	m_reason_string = reasonString( m_reason ) ;
}

bool GNet::SocketBase::sizeError( ssize_t size )
{
	return size == SOCKET_ERROR ;
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

bool GNet::SocketBase::setNonBlock()
{
	unsigned long ul = 1 ;
	return ioctlsocket( m_fd.fd() , FIONBIO , &ul ) != SOCKET_ERROR ;
}

std::string GNet::SocketBase::reasonString( int e )
{
	//if( e == WSANOTINITIALISED )
	if( e == WSAENETDOWN ) return "network down" ;
	//if( e == WSAEFAULT )
	//if( e == WSAENOTCONN )
	//if( e == WSAEINTR )
	//if( e == WSAEINPROGRESS )
	if( e == WSAENETRESET ) return "network reset" ;
	//if( e == WSAENOTSOCK )
	//if( e == WSAEOPNOTSUPP )
	if( e == WSAESHUTDOWN ) return "already shut down" ;
	//if( e == WSAEWOULDBLOCK )
	//if( e == WSAEMSGSIZE )
	//if( e == WSAEINVAL )
	if( e == WSAECONNABORTED ) return "aborted" ;
	if( e == WSAETIMEDOUT ) return "timed out" ;
	if( e == WSAECONNRESET ) return "connection reset by peer" ;

	std::string result = "unknown error" ;
	DWORD size_limit = 128U ;
	TCHAR * buffer = nullptr ;
	DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS ;
	DWORD rc = FormatMessage( flags , nullptr , e , 0 , reinterpret_cast<LPSTR>(&buffer) , size_limit , nullptr ) ;
	if( buffer == nullptr || rc == 0U ) return result ;
	G::Convert::tstring tmessage( buffer , static_cast<std::size_t>(rc) ) ;
	::LocalFree( buffer ) ;
	try
	{
		G::Convert::convert( result , tmessage , G::Convert::ThrowOnError() ) ;
	}
	catch( G::Convert::Error & )
	{
	}
	G::Str::removeAll( result , '\r' ) ;
	G::Str::trimRight( result , ".\n" ) ;
	G::Str::replaceAll( result , "\n" , " " ) ;
	G_DEBUG( "GNet::SocketBase::reasonString: " << e << " -> [" << G::Str::printable(result) << "]" ) ;
	return G::Str::lower(G::Str::printable(result)) ;
}

// ==

bool GNet::Socket::canBindHint( const Address & )
{
	return true ; // rebinding the same port number fails, so a dummy implementation here
}

void GNet::Socket::setOptionReuse()
{
	setOption( SOL_SOCKET , "so_reuseaddr" , SO_REUSEADDR , 1 ) ;
}

void GNet::Socket::setOptionExclusive()
{
	setOption( SOL_SOCKET , "so_exclusiveaddruse" , SO_EXCLUSIVEADDRUSE , 1 ) ;
}

void GNet::Socket::setOptionPureV6( bool )
{
	// no-op
}

bool GNet::Socket::setOptionPureV6( bool , NoThrow )
{
	return true ; // no-op
}

bool GNet::Socket::setOptionImp( int level , int op , const void * arg , socklen_t n )
{
	const char * cp = reinterpret_cast<const char*>(arg) ;
	int rc = ::setsockopt( fd() , level , op , cp , n ) ;
	return ! error(rc) ;
}

/// \file gsocket_win32.cpp
