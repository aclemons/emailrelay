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
// gserver.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gserver.h"
#include "groot.h"
#include "gmonitor.h"
#include "gdebug.h"
#include "gassert.h"
#include "gmemory.h"
#include <algorithm> // std::find()

GNet::ServerPeer::ServerPeer( Server::PeerInfo peer_info ) :
	m_address(peer_info.m_address) ,
	m_socket(peer_info.m_socket) ,
	m_handle(peer_info.m_handle)
{
	G_ASSERT( m_socket.get() != NULL ) ;
	G_DEBUG( "GNet::ServerPeer::ctor: [" << this << "]: fd " << asString() << ": " << m_address.displayString() ) ;
	m_socket->addReadHandler( *this ) ;
	m_socket->addExceptionHandler( *this ) ;
	if( Monitor::instance() ) Monitor::instance()->add(*this) ;
}

GNet::ServerPeer::~ServerPeer()
{
	G_DEBUG( "GNet::ServerPeer::dtor: [" << this << "]: fd " << asString() ) ;
	if( Monitor::instance() ) Monitor::instance()->remove(*this) ;
	m_handle->reset() ;
	m_socket->dropReadHandler() ;
}

std::string GNet::ServerPeer::asString() const
{
	return m_socket->asString() ; // ie. the fd
}

GNet::StreamSocket & GNet::ServerPeer::socket()
{
	G_ASSERT( m_socket.get() != NULL ) ;
	return *m_socket.get() ;
}

void GNet::ServerPeer::readEvent()
{
	//G_DEBUG( "GNet::ServerPeer::readEvent: peer=" << this ) ;

	char buffer[500] ;
	buffer[0] = '\0' ;
	size_t buffer_size = sizeof(buffer) ;
	ssize_t rc = m_socket->read( buffer , buffer_size ) ;

	if( rc == 0 || (rc == -1 && !m_socket->eWouldBlock()) )
	{
		doDelete() ;
	}
	else if( rc != -1 )
	{
		size_t n = rc ;
		onData( buffer , n ) ;
	}
	else 
	{ 
		; // no-op (windows)
	}
}

void GNet::ServerPeer::exceptionEvent()
{
	G_DEBUG( "GNet::Server::exceptionEvent: peer=" << this ) ;
	doDelete() ;
}

void GNet::ServerPeer::doDelete()
{
	onDelete() ;
	delete this ;
}

std::pair<bool,GNet::Address> GNet::ServerPeer::localAddress() const
{
	G_ASSERT( m_socket.get() != NULL ) ;
	return m_socket->getLocalAddress() ;
}

std::pair<bool,GNet::Address> GNet::ServerPeer::peerAddress() const
{
	return std::pair<bool,Address>( true , m_address ) ;
}

// ===

GNet::Server::Server( unsigned int listening_port )
{
	init( listening_port ) ;
}

GNet::Server::Server( const Address & listening_address )
{
	init( listening_address ) ;
}

GNet::Server::Server() :
	m_cleaned_up(false)
{
}

void GNet::Server::init( unsigned int listening_port )
{
	init( Address(listening_port) ) ;
}

void GNet::Server::init( const Address & listening_address )
{
	m_cleaned_up = false ;
	m_socket <<= new StreamSocket ;
	G_DEBUG( "GNet::Server::init: listening on " << listening_address.displayString() ) ;
	G::Root claim_root ;
	if( ! m_socket->bind( listening_address ) )
		throw CannotBind( listening_address.displayString() ) ;
	if( ! m_socket->listen(3) )
		throw CannotListen() ;
	m_socket->addReadHandler( *this ) ;
}

//static
bool GNet::Server::canBind( const Address & address , bool do_throw )
{
	G::Root claim_root ;
	StreamSocket s ;
	bool ok = s.canBindHint( address ) ;
	if( !ok && do_throw )
		throw CannotBind( address.displayString() ) ;
	return ok ;
}

GNet::Server::~Server()
{
	serverCleanup() ;
}

void GNet::Server::serverCleanup()
{
	try
	{
		if( ! m_cleaned_up )
		{
			m_cleaned_up = true ;
			serverCleanupCore() ;
		}
	}
	catch(...) // since always called from dtor
	{
	}
}

void GNet::Server::serverCleanupCore()
{
	for( PeerList::iterator p = m_peer_list.begin() ; p != m_peer_list.end() ; ++p )
	{
		ServerPeerHandle handle = *p ;
		if( handle.peer() != NULL )
		{
			G_DEBUG( "GNet::Server::serverCleanupCore: deleting peer: [" << handle.peer() << "]" ) ;
			delete handle.peer() ;
		}
	}
}

std::pair<bool,GNet::Address> GNet::Server::address() const
{
	std::pair<bool,Address> result( false , Address::invalidAddress() ) ;
	if( m_socket.get() != NULL )
		result = m_socket->getLocalAddress() ;
	return result ;
}

void GNet::Server::readEvent()
{
	// read-event-on-listening-port => new connection to accept

	G_DEBUG( "GNet::Server::readEvent: " << this ) ;
	G_ASSERT( m_socket.get() != NULL ) ;

	collectGarbage() ;

	G::Root claim_root ;
	AcceptPair pair = m_socket->accept() ;
	if( pair.first.get() == NULL )
	{
		G_WARNING( "GNet::Server::readEvent: accept error" ) ;
	}
	else
	{
		G_DEBUG( "GNet::Server::readEvent: new connection from " << pair.second.displayString() ) ;
		m_peer_list.push_back( ServerPeerHandle() ) ;

		PeerInfo peer_info ;
		peer_info.m_socket = pair.first ; // auto_ptr
		peer_info.m_address = pair.second ;
		peer_info.m_handle = &m_peer_list.back() ;

		ServerPeer * peer = newPeer(peer_info) ;
		if( peer == NULL )
		{
			m_peer_list.pop_back() ;
		}
		else
		{
			m_peer_list.back().set( peer ) ;
			G_DEBUG( "GNet::Server::readEvent: new connection accepted onto fd " << peer->asString() ) ;
		}
	}
}

void GNet::Server::collectGarbage()
{
	// cleanup empty handles, where peer objects have deleted themselves
	G_DEBUG( "GNet::Server::collectGarbage" ) ;
	for( PeerList::iterator p = m_peer_list.begin() ; p != m_peer_list.end() ; )
	{
		ServerPeerHandle handle = *p ;
		if( handle.peer() == NULL )
		{
			G_DEBUG( "GNet::Server::collectGarbage: [" << handle.old() << "]" ) ;
			p = m_peer_list.erase( p ) ;
		}
		else
		{
			++p ;
		}
	}
}

void GNet::Server::writeEvent()
{
	G_DEBUG( "GNet::Server::writeEvent" ) ;
}

void GNet::Server::exceptionEvent()
{
	G_DEBUG( "GNet::Server::exceptionEvent" ) ;
}

// ===

GNet::Server::PeerInfo::PeerInfo() :
	m_address(Address::invalidAddress()) ,
	m_handle(NULL)
{
}

// ===

GNet::ServerPeerHandle::ServerPeerHandle() :
	m_p(NULL) ,
	m_old(NULL)
{
}

void GNet::ServerPeerHandle::reset()
{
	m_p = NULL ;
}

GNet::ServerPeer * GNet::ServerPeerHandle::peer()
{
	return m_p ;
}

GNet::ServerPeer * GNet::ServerPeerHandle::old()
{
	return m_old ;
}

void GNet::ServerPeerHandle::set( ServerPeer * p )
{
	m_p = p ;
	m_old = p ;
}

