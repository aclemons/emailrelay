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
// gsocket_unix.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gsocket.h"
#include "gdebug.h"
#include "gassert.h"
#include "glog.h"
#include <cstring>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

bool GNet::Socket::valid( Descriptor s )
{
	return s.valid() ;
}

bool GNet::Socket::setNonBlock()
{
	G_ASSERT( valid() ) ;

	int mode = ::fcntl( m_socket.fd() , F_GETFL ) ;
	if( mode < 0 )
		return false ;

	int rc = ::fcntl( m_socket.fd() , F_SETFL , mode | O_NONBLOCK ) ;
	bool ok = rc >= 0 ;
	if( ok )
	{
		G_ASSERT( ::fcntl(m_socket.fd(),F_GETFL) & O_NONBLOCK ) ;
	}

	return ok ;
}

int GNet::Socket::reason()
{
	int r = errno ;
	G_DEBUG( "GNet::Socket::reason: " << (r==EINPROGRESS?"":"error ") << r << ": " << std::strerror(r) ) ;
	return r ;
}

void GNet::Socket::doClose()
{
	G_ASSERT( valid() ) ;
	::close( m_socket.fd() ) ;
	m_socket = Descriptor::invalid() ;
}

bool GNet::Socket::error( int rc )
{
	return rc < 0 ;
}

bool GNet::Socket::sizeError( ssize_t size )
{
	return size < 0 ;
}

bool GNet::Socket::eWouldBlock()
{
	return m_reason == EWOULDBLOCK || m_reason == EAGAIN ;
}

bool GNet::Socket::eInProgress()
{
	return m_reason == EINPROGRESS ;
}

bool GNet::Socket::eMsgSize()
{
	return m_reason == EMSGSIZE ;
}

void GNet::Socket::setFault()
{
	m_reason = EFAULT ;
}

bool GNet::Socket::canBindHint( const Address & address )
{
	return bind( address ) ;
}

/// \file gsocket_unix.cpp
