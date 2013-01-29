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
// gsocket_win32.cc
//

#include "gdef.h"
#include "gnet.h"
#include "gdebug.h"
#include "gassert.h"
#include "gsocket.h"
#include "glog.h"
#include <list>
#include <errno.h>

bool GNet::Socket::valid( Descriptor s )
{
	// (beware loosing WSAGetLastError() information, so...)
	// (put no debug here)
	return s.valid() ;
}

int GNet::Socket::reason()
{
	// (put no debug here)
	int r = ::WSAGetLastError() ;
	G_DEBUG( "GNet::Socket::reason: error " << r ) ;
	return r ;
}

void GNet::Socket::doClose()
{
	G_ASSERT( valid() ) ;
	::closesocket( m_socket.fd() );
	m_socket = Descriptor::invalid() ;
}

bool GNet::Socket::error( int rc )
{
	// (put no debug here)
	return rc == SOCKET_ERROR ;
}

bool GNet::Socket::sizeError( ssize_t size )
{
	// (put no debug here)
	return size == SOCKET_ERROR ;
}

bool GNet::Socket::eWouldBlock()
{
	return m_reason == WSAEWOULDBLOCK ;
}
 
bool GNet::Socket::eInProgress()
{
	// (Winsock WSAEINPROGRESS has different semantics to Unix)
	return m_reason == WSAEWOULDBLOCK ; // (sic)
}

bool GNet::Socket::eMsgSize()
{
	return m_reason == WSAEMSGSIZE ;
}
 
bool GNet::Socket::setNonBlock()
{
	unsigned long ul = 1 ;
	return ioctlsocket( m_socket.fd() , FIONBIO , &ul ) != SOCKET_ERROR ;
	// (put no debug here)
}

void GNet::Socket::setFault()
{
	m_reason = WSAEFAULT ;
}

bool GNet::Socket::canBindHint( const Address & )
{
	// rebinding the same port number fails, so a dummy implementation here
	return true ;
}

/// \file gsocket_win32.cpp
