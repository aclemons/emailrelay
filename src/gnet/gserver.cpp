//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gserver.cpp
///

#include "gdef.h"
#include "gserver.h"
#include "gnetdone.h"
#include "gmonitor.h"
#include "geventloggingcontext.h"
#include "gfile.h"
#include "gcleanup.h"
#include "glimits.h"
#include "groot.h"
#include "gscope.h"
#include "gtest.h"
#include "glog.h"
#include "gassert.h"
#include <algorithm>

GNet::Server::Server( EventState es , Descriptor fd ,
	const ServerPeer::Config & server_peer_config , const Config & server_config ) :
		m_es(es) ,
		m_config(server_config) ,
		m_server_peer_config(server_peer_config) ,
		m_socket(StreamSocket::Listener(),fd,m_config.stream_socket_config)
{
	G_DEBUG( "GNet::Server::ctor: listening on socket " << m_socket.asString()
		<< " with address " << m_socket.getLocalAddress().displayString() ) ;
	m_socket.listen() ;
	m_socket.addReadHandler( *this , m_es ) ;
	Monitor::addServer( *this ) ;
}

GNet::Server::Server( EventState es , const Address & listening_address ,
	const ServerPeer::Config & server_peer_config , const Config & server_config ) :
		m_es(es) ,
		m_config(server_config) ,
		m_server_peer_config(server_peer_config) ,
		m_socket(listening_address.family(),StreamSocket::Listener(),m_config.stream_socket_config)
{
	G_DEBUG( "GNet::Server::ctor: listening on socket " << m_socket.asString()
		<< " with address " << listening_address.displayString() ) ;

	bool uds = listening_address.family() == Address::Family::local ;
	if( uds )
	{
		bool open = m_config.uds_open_permissions ;
		using Mode = G::Process::Umask::Mode ;
		G::Root claim_root( false ) ; // group ownership from the effective group-id
		G::Process::Umask set_umask( open ? Mode::Open : Mode::Tighter ) ;
		m_socket.bind( listening_address ) ;
	}
	else
	{
		G::Root claim_root ;
		m_socket.bind( listening_address ) ;
	}

	m_socket.listen() ;
	m_socket.addReadHandler( *this , m_es ) ;
	Monitor::addServer( *this ) ;

	if( uds )
	{
		std::string path = listening_address.hostPartString() ;
		if( path.size() > 1U && path.at(0U) == '/' )
		{
			G::Cleanup::add( G::File::cleanup , G::Cleanup::arg(G::Path(path)) ) ;
		}
	}
}

GNet::Server::~Server()
{
	Monitor::removeServer( *this ) ;
}

GNet::Address GNet::Server::address() const
{
	bool with_scope = true ; // was false
	Address result = m_socket.getLocalAddress() ;
	if( with_scope )
		result.setScopeId( m_socket.getBoundScopeId() ) ;
	return result ;
}

void GNet::Server::readEvent()
{
	// read-event-on-listening-port => new connection to accept
	G_DEBUG( "GNet::Server::readEvent: " << this ) ;

	// accept the connection
	//
	ServerPeerInfo peer_info( this , m_server_peer_config ) ;
	accept( peer_info ) ;
	Address peer_address = peer_info.m_address ;
	G_DEBUG( "GNet::Server::readEvent: new connection from " << peer_address.displayString()
		<< " on " << peer_info.m_socket->asString() ) ;

	// change the logging context asap to reflect the server peer object
	// being created (esp. if it sends an initial server greeting)
	//
	GNet::EventLoggingContext inner( m_es ,
		ServerPeer::eventLoggingString(peer_address,m_server_peer_config) ) ;

	// create the peer object -- newPeer() implementations will normally catch
	// their exceptions and return null to avoid terminating the server -- peer
	// objects are given this object as their exception handler so we get
	// to delete them when they throw -- the EventState tuple is passed as
	// 'unbound' to force the peer object to set themselves as the exception
	// source
	//
	std::unique_ptr<ServerPeer> peer = newPeer( m_es.eh(this).unbound() , std::move(peer_info) ) ;

	// commit or roll back
	if( peer == nullptr )
	{
		G_WARNING( "GNet::Server::readEvent: connection rejected from " << peer_address.displayString() ) ;
	}
	else
	{
		G_DEBUG( "GNet::Server::readEvent: new connection accepted" ) ;
		m_peer_list.push_back( std::shared_ptr<ServerPeer>(peer.release()) ) ;
	}
}

void GNet::Server::accept( ServerPeerInfo & peer_info )
{
	AcceptInfo accept_info ;
	{
		G::Root claim_root ;
		accept_info = m_socket.accept() ;
	}
	peer_info.m_address = accept_info.address ;
	peer_info.m_socket = std::move( accept_info.socket_ptr ) ;
}

void GNet::Server::onException( ExceptionSource * esrc , std::exception & e , bool done )
{
	G_DEBUG( "GNet::Server::onException: exception=[" << e.what() << "] esrc=[" << static_cast<void*>(esrc) << "]" ) ;
	bool handled = false ;
	if( esrc != nullptr )
	{
		for( auto list_p = m_peer_list.begin() ; list_p != m_peer_list.end() ; ++list_p )
		{
			if( (*list_p).get() == esrc ) // implicit ServerPeer/ExceptionSource static cast
			{
				std::shared_ptr<ServerPeer> peer_p = *list_p ;
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

std::vector<std::weak_ptr<GNet::ServerPeer>> GNet::Server::peers()
{
	using Peers = std::vector<std::weak_ptr<ServerPeer>> ;
	Peers result ;
	result.reserve( m_peer_list.size() ) ;
	for( auto & peer : m_peer_list )
		result.push_back( std::weak_ptr<ServerPeer>(peer) ) ;
	return result ;
}

void GNet::Server::writeEvent()
{
	G_DEBUG( "GNet::Server::writeEvent" ) ;
}

// ===

GNet::ServerPeerInfo::ServerPeerInfo( Server * server , ServerPeer::Config server_peer_config ) :
	m_address(Address::defaultAddress()) ,
	m_server_peer_config(server_peer_config) ,
	m_server(server)
{
}

