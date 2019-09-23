//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gserver.h"
#include "gnetdone.h"
#include "glimits.h"
#include "groot.h"
#include "glog.h"
#include "gassert.h"

GNet::Server::Server( ExceptionSink es , const Address & listening_address ,
	ServerPeerConfig server_peer_config ) :
		m_es(es) ,
		m_server_peer_config(server_peer_config) ,
		m_socket(listening_address.domain(),StreamSocket::Listener())
{
	G_DEBUG( "GNet::Server::ctor: listening on socket " << m_socket.asString() << " with address " << listening_address.displayString() ) ;
	{
		G::Root claim_root ;
		m_socket.bind( listening_address ) ;
	}
	m_socket.listen( G::limits::net_listen_queue ) ;
	m_socket.addReadHandler( *this , m_es ) ;
}

GNet::Server::~Server()
{
}

bool GNet::Server::canBind( const Address & address , bool do_throw )
{
	StreamSocket socket( address.domain() ) ;
	bool ok = false ;
	{
		G::Root claim_root ;
		ok = socket.canBindHint( address ) ;
	}
	if( !ok && do_throw )
		throw CannotBind( address.displayString() , socket.reason() ) ;
	return ok ;
}

GNet::Address GNet::Server::address() const
{
	G_ASSERT( m_socket.getLocalAddress().first ) ;
	std::pair<bool,Address> pair = m_socket.getLocalAddress() ;
	return pair.second ;
}

void GNet::Server::readEvent()
{
	// read-event-on-listening-port => new connection to accept
	G_DEBUG( "GNet::Server::readEvent: " << this ) ;

	// accept the connection
	ServerPeerInfo peer_info( this , m_server_peer_config ) ;
	accept( peer_info ) ;
	G_DEBUG( "GNet::Server::readEvent: new connection from " << peer_info.m_address.displayString()
		<< " on " << peer_info.m_socket->asString() ) ;

	// create the peer object -- newPeer() implementations will normally catch
	// their exceptions and return null to avoid terminating the server -- peer
	// objects are given this object as their exception sink so we get
	// to delete them
	//
	unique_ptr<ServerPeer> peer = newPeer( ExceptionSinkUnbound(this) , peer_info ) ;

	// commit or roll back
	if( peer.get() == nullptr )
	{
		G_WARNING( "GNet::Server::readEvent: connection rejected from " << peer_info.m_address.displayString() ) ;
	}
	else
	{
		G_DEBUG( "GNet::Server::readEvent: new connection accepted" ) ;
		m_peer_list.push_back( shared_ptr<ServerPeer>(peer.release()) ) ;
	}
}

void GNet::Server::accept( ServerPeerInfo & peer_info )
{
	AcceptPair accept_pair ;
	{
		G::Root claim_root ;
		accept_pair = m_socket.accept() ;
	}
	peer_info.m_socket = accept_pair.first ;
	peer_info.m_address = accept_pair.second ;
}

void GNet::Server::onException( ExceptionSource * esrc , std::exception & e , bool done )
{
	G_DEBUG( "GNet::Server::onException: exception=[" << e.what() << "] esrc=[" << static_cast<void*>(esrc) << "]" ) ;
	bool handled = false ;
	if( esrc != nullptr )
	{
		for( PeerList::iterator list_p = m_peer_list.begin() ; list_p != m_peer_list.end() ; ++list_p )
		{
			if( (*list_p).get() == esrc ) // implicit ServerPeer/ExceptionSource static cast
			{
				shared_ptr<ServerPeer> peer_p = *list_p ;
				m_peer_list.erase( list_p ) ; // remove first, in case onDelete() throws
				(*peer_p).doOnDelete( e.what() , done ) ;
				handled = true ;
				break ; // ServerPeer deleted here
			}
		}
	}
	if( !handled )
	{
		G_WARNING( "GNet::Server::onException: unhandled exception: " << e.what() ) ;
		throw ; // should never get here -- rethrow just in case
	}
}

void GNet::Server::serverCleanup()
{
	m_peer_list.clear() ;
}

bool GNet::Server::hasPeers() const
{
	return !m_peer_list.empty() ;
}

std::vector<weak_ptr<GNet::ServerPeer> > GNet::Server::peers()
{
	typedef std::vector<weak_ptr<ServerPeer> > Peers ;
	Peers result ;
	result.reserve( m_peer_list.size() ) ;
	for( PeerList::iterator peer_p = m_peer_list.begin() ; peer_p != m_peer_list.end() ; ++peer_p )
		result.push_back( weak_ptr<ServerPeer>(*peer_p) ) ;
	return result ;
}

void GNet::Server::writeEvent()
{
	G_DEBUG( "GNet::Server::writeEvent" ) ;
}

// ===

GNet::ServerPeerInfo::ServerPeerInfo( Server * server , ServerPeerConfig config ) :
	m_address(Address::defaultAddress()) ,
	m_config(config) ,
	m_server(server)
{
}

/// \file gserver.cpp
