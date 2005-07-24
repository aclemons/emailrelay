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
// gsocket_win32.cc
//

#include "gdef.h"
#include "gnet.h"
#include "gdebug.h"
#include "gassert.h"
#include "gsocket.h"
#include "glog.h"
#include <errno.h>

bool GNet::Socket::valid( Descriptor s )
{
	// (beware loosing WSAGetLastError() information, so...)
	// (put no debug here)
	return Descriptor__valid( s ) ;
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
	::closesocket( m_socket );
	m_socket = Descriptor__invalid() ;
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
	return ioctlsocket( m_socket , FIONBIO , &ul ) != SOCKET_ERROR ;
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

