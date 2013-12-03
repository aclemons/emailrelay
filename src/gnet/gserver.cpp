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
// gserver.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "glimits.h"
#include "gserver.h"
#include "groot.h"
#include "gmonitor.h"
#include "gdebug.h"
#include "gtest.h"
#include "gassert.h"
#include "gmemory.h"
#include <algorithm> // std::find()

GNet::ServerPeer::ServerPeer( Server::PeerInfo peer_info ) :
	m_address(peer_info.m_address) ,
	m_socket(peer_info.m_socket) ,
	m_sp(*this,*this,*m_socket.get(),0U) ,
	m_handle(peer_info.m_handle) ,
	m_delete_timer(*this,&ServerPeer::onTimeout,*this)
{
	G_ASSERT( m_socket.get() != NULL ) ;
	G_DEBUG( "GNet::ServerPeer::ctor: [" << this << "]: " << logId() ) ;

	m_socket->addReadHandler( *this ) ;
	m_socket->addExceptionHandler( *this ) ;
	if( Monitor::instance() ) Monitor::instance()->addServerPeer(*this) ;
}

GNet::ServerPeer::~ServerPeer()
{
	G_DEBUG( "GNet::ServerPeer::dtor: [" << this << "]: fd " << logId() ) ;
	if( Monitor::instance() ) Monitor::instance()->removeServerPeer(*this) ;
	m_handle->reset() ;
	m_socket->dropReadHandler() ;
}

std::string GNet::ServerPeer::logId() const
{
	return m_address.displayString() + "@" + m_socket->asString() ;
}

void GNet::ServerPeer::sslAccept()
{
	m_sp.sslAccept() ;
}

GNet::StreamSocket & GNet::ServerPeer::socket()
{
	G_ASSERT( m_socket.get() != NULL ) ;
	return *m_socket.get() ;
}

void GNet::ServerPeer::readEvent()
{
	m_sp.readEvent() ;
}

void GNet::ServerPeer::onException( std::exception & e )
{
	G_DEBUG( "ServerPeer::onException: exception: " << e.what() ) ;
	doDelete( e.what() ) ;
}

void GNet::ServerPeer::doDelete( const std::string & reason )
{
	onDelete( reason ) ;
	m_delete_timer.startTimer( 0U ) ;
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

std::string GNet::ServerPeer::peerCertificate() const
{
	return m_sp.peerCertificate() ;
}

void GNet::ServerPeer::onTimeout()
{
	doDeleteThis(1) ;
}

void GNet::ServerPeer::doDeleteThis( int )
{
	delete this ;
}

bool GNet::ServerPeer::send( const std::string & data , std::string::size_type offset )
{
	return m_sp.send( data , offset ) ;
}

void GNet::ServerPeer::writeEvent()
{
	try
	{
		if( m_sp.writeEvent() )
			onSendComplete() ;
	}
	catch( std::exception & e ) // strategy
	{
		G_WARNING( "GNet::ServerPeer::writeEvent: exception: " << e.what() ) ;
		doDelete() ;
	}
}

// ===

GNet::Server::Server( unsigned int listening_port , ConnectionLookup * connection_lookup ) :
	m_connection_lookup(connection_lookup)
{
	init( listening_port ) ;
}

GNet::Server::Server( const Address & listening_address , ConnectionLookup * connection_lookup ) :
	m_connection_lookup(connection_lookup)
{
	init( listening_address ) ;
}

GNet::Server::Server() :
	m_connection_lookup(NULL) ,
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
	m_socket <<= new StreamSocket( listening_address ) ;
	G_DEBUG( "GNet::Server::init: listening on " << listening_address.displayString() ) ;
	G::Root claim_root ;
	if( ! m_socket->bind( listening_address ) )
		throw CannotBind( listening_address.displayString() ) ;
	if( ! m_socket->listen(G::limits::net_listen_queue) )
		throw CannotListen() ;
	m_socket->addReadHandler( *this ) ;
}

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
	catch(...) // dtor
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
			handle.peer()->doDeleteThis(1) ;
		}
	}
}

void GNet::Server::onException( std::exception & e )
{
	G_ERROR( "Server::onException: exception: " << e.what() ) ;
	throw ; // out of the event loop
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

	// take this opportunity to do garbage collection
	collectGarbage() ;

	// accept the connection
	PeerInfo peer_info ;
	accept( peer_info ) ;
	G_DEBUG( "GNet::Server::readEvent: new connection from " << peer_info.m_address.displayString() 
		<< " (" << peer_info.m_name << ")" ) ;

	// keep track of peer objects
	m_peer_list.push_back( ServerPeerHandle() ) ;
	peer_info.m_handle = &m_peer_list.back() ;

	// create the peer object -- newPeer() implementations will normally 
	// catch their exceptions to avoid terminating the server
	//
	ServerPeer * peer = newPeer( peer_info ) ;

	// commit or roll back
	if( peer == NULL )
	{
		m_peer_list.pop_back() ;
		G_DEBUG( "GNet::Server::readEvent: connection rejected from " << peer_info.m_address.displayString() ) ;
	}
	else
	{
		m_peer_list.back().set( peer ) ;
		G_DEBUG( "GNet::Server::readEvent: new connection accepted: " << peer->logId() ) ;
	}
}

void GNet::Server::accept( PeerInfo & peer_info )
{
	{
		G::Root claim_root ;
		AcceptPair accept_pair = m_socket->accept() ;
		peer_info.m_socket = accept_pair.first ; // auto_ptr assignment
		peer_info.m_address = accept_pair.second ;
	}

	if( G::Test::enabled("accept-throws") )
		throw AcceptError() ;
	if( peer_info.m_socket.get() == NULL )
		throw AcceptError() ;

	// optionally enrich the peer info
	if( m_connection_lookup != NULL )
	{
		std::pair<bool,Address> local = peer_info.m_socket->getLocalAddress() ;
		if( local.first )
		{
			ConnectionLookup::Connection c = m_connection_lookup->find( local.second , peer_info.m_address ) ;
			if( c.valid() )
				peer_info.m_name = c.peerName() ;
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

/// \file gserver.cpp
