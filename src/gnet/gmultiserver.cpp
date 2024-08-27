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
/// \file gmultiserver.cpp
///

#include "gdef.h"
#include "gmultiserver.h"
#include "glisteners.h"
#include "gdatetime.h"
#include "gstr.h"
#include "gtest.h"
#include "glog.h"
#include "gassert.h"
#include <list>
#include <algorithm>

GNet::MultiServer::MultiServer( EventState es , const G::StringArray & listener_list , unsigned int port ,
	const std::string & server_type , ServerPeer::Config server_peer_config , Server::Config server_config ) :
		m_es(es) ,
		m_listener_list(listener_list) ,
		m_port(port) ,
		m_server_type(server_type) ,
		m_server_peer_config(server_peer_config) ,
		m_server_config(server_config) ,
		m_if(es,*this) ,
		m_interface_event_timer(*this,&MultiServer::onInterfaceEventTimeout,es)
{
	Listeners listeners( m_if , m_listener_list , m_port ) ;

	// fail if any bad names (eg. "foo/bar")
	if( listeners.hasBad() )
		throw InvalidName( listeners.badName() ) ;

	// fail if no addresses and no prospect of getting any
	if( listeners.defunct() )
		throw NoListeningAddresses() ;

	// warn if no addresses from one or more interface names
	if( listeners.hasEmpties() )
	{
		G_WARNING( "GNet::MultiServer::ctor: no addresses bound to named network interface"
			<< listeners.logEmpties() ) ;
	}

	// warn if doing nothing until an interface comes up
	if( listeners.idle() )
	{
		G_WARNING( "GNet::MultiServer::ctor: " << m_server_type << " server: nothing to do: "
			<< "waiting for interface" << listeners.logEmpties() ) ;
	}

	// warn if we got addresses from an interface name but won't get dynamic updates
	if( listeners.noUpdates() )
	{
		G_WARNING_ONCE( "GNet::MultiServer::ctor: named network interfaces "
			"are not being monitored for address updates" ) ;
	}

	// instantiate the servers
	for( const auto & fd : listeners.fds() )
		createServer( Descriptor(fd) ) ;
	for( const auto & a : listeners.fixed() )
		createServer( a , true ) ;
	for( const auto & a : listeners.dynamic() )
		createServer( a , false ) ;
}

GNet::MultiServer::~MultiServer()
{
	serverCleanup() ;
}

void GNet::MultiServer::createServer( Descriptor fd )
{
	m_server_list.emplace_back( std::make_unique<MultiServerImp>( *this , m_es ,
		fd , m_server_peer_config , m_server_config ) ) ;
}

void GNet::MultiServer::createServer( const Address & address , bool fixed )
{
	m_server_list.emplace_back( std::make_unique<MultiServerImp>( *this , m_es ,
		fixed , address , m_server_peer_config , m_server_config ) ) ;
}

void GNet::MultiServer::createServer( const Address & address , bool fixed , std::nothrow_t )
{
	try
	{
		createServer( address , fixed ) ;
		G_LOG_S( "GNet::MultiServer::createServer: new " << m_server_type
			<< " server on " << displayString(address) ) ;
	}
	catch( Socket::SocketBindError & e )
	{
		// (can fail here if notified too soon, but succeeds later)
		G_LOG( "GNet::MultiServer::createServer: failed to bind " << displayString(address)
			<< " for new " << m_server_type << " server:"
			<< G::Str::tail(e.what(),std::string_view(e.what()).rfind(':')) ) ;
	}
}

void GNet::MultiServer::serverCleanup()
{
	for( auto & server : m_server_list )
	{
		server->cleanup() ;
	}
}

void GNet::MultiServer::onInterfaceEvent( const std::string & /*description*/ )
{
	// notifications can be periodic and/or bursty, so minimal logging here
	G_DEBUG( "GNet::MultiServer::onInterfaceEvent: network configuration change event" ) ;
	m_if.load() ;
	m_interface_event_timer.startTimer( 1U , 500000U ) ; // maybe increase for fewer bind warnings
}

void GNet::MultiServer::onInterfaceEventTimeout()
{
	// get a fresh address list
	Listeners listeners( m_if , m_listener_list , m_port ) ;

	// delete old
	for( auto server_iter = m_server_list.begin() ; server_iter != m_server_list.end() ; )
	{
		if( (*server_iter)->dynamic() && !gotAddressFor( **server_iter , listeners.dynamic() ) )
			server_iter = removeServer( server_iter ) ;
		else
			++server_iter ;
	}

	// create new
	for( const auto & address : listeners.dynamic() )
	{
		G_DEBUG( "GNet::MultiServer::onInterfaceEvent: address: " << displayString(address) ) ;
		if( !gotServerFor(address) )
		{
			createServer( address , true , std::nothrow ) ;
		}
	}
}

GNet::MultiServer::ServerList::iterator GNet::MultiServer::removeServer( ServerList::iterator iter )
{
	auto & ptr = *iter ;
	G_LOG_S( "GNet::MultiServer::removeServer: deleting " << m_server_type
		<< " server on " << displayString(ptr->address()) ) ;
	return m_server_list.erase( iter ) ;
}

bool GNet::MultiServer::match( const Address & interface_address , const Address & server_address )
{
	// both addresses should have a well-defined scope-id, so include scope-ids
	// in the match -- this allows for multiple interfaces to have the same link-local address
	return interface_address.same( server_address , interface_address.scopeId() && server_address.scopeId() ) ;
}

bool GNet::MultiServer::gotAddressFor( const Listener & server , const AddressList & address_list ) const
{
	Address server_address = server.address() ;
	return address_list.end() != std::find_if( address_list.begin() , address_list.end() ,
		[server_address](const Address & a){ return match(a,server_address); } ) ;
}

bool GNet::MultiServer::gotServerFor( Address interface_address ) const
{
	return std::any_of( m_server_list.begin() , m_server_list.end() ,
		[interface_address](const ServerPtr &ptr){ return match(interface_address,ptr->address()); } ) ;
}

std::string GNet::MultiServer::displayString( const Address & address )
{
	return address.displayString( true ) ;
}

void GNet::MultiServer::serverReport( const std::string & group ) const
{
	for( const auto & server : m_server_list )
	{
		G_ASSERT( server.get() != nullptr ) ;
		if( !server ) continue ;
		G_LOG_S( "GNet::MultiServer: " << (group.empty()?"":"[") << group << (group.empty()?"":"] ")
			<< m_server_type << " server on " << displayString(server->address()) ) ;
	}
}

std::unique_ptr<GNet::ServerPeer> GNet::MultiServer::doNewPeer( EventStateUnbound esu ,
	ServerPeerInfo && pi , const ServerInfo & si )
{
	return newPeer( esu , std::move(pi) , si ) ;
}

bool GNet::MultiServer::hasPeers() const
{
	for( const auto & server : m_server_list )
	{
		G_ASSERT( server.get() != nullptr ) ;
		if( !server ) continue ;
		if( server->hasPeers() )
			return true ;
	}
	return false ;
}

std::vector<std::weak_ptr<GNet::ServerPeer>> GNet::MultiServer::peers()
{
	using List = std::vector<std::weak_ptr<ServerPeer>> ;
	List result ;
	for( auto & server : m_server_list )
	{
		G_ASSERT( server.get() != nullptr ) ;
		if( !server ) continue ;
		List list = server->peers() ;
		result.insert( result.end() , list.begin() , list.end() ) ;
	}
	return result ;
}

// ==

GNet::MultiServerImp::MultiServerImp( MultiServer & ms , EventState es , bool fixed , const Address & address ,
	ServerPeer::Config server_peer_config , Server::Config server_config ) :
		GNet::Server(es,address,server_peer_config,server_config) ,
		m_ms(ms) ,
		m_fixed(fixed)
{
}

GNet::MultiServerImp::MultiServerImp( MultiServer & ms , EventState es , Descriptor fd ,
	ServerPeer::Config server_peer_config , Server::Config server_config ) :
		GNet::Server(es,fd,server_peer_config,server_config) ,
		m_ms(ms) ,
		m_fixed(true)
{
}

GNet::MultiServerImp::~MultiServerImp()
= default;

bool GNet::MultiServerImp::dynamic() const
{
	return !m_fixed ;
}

void GNet::MultiServerImp::cleanup()
{
	serverCleanup() ;
}

std::unique_ptr<GNet::ServerPeer> GNet::MultiServerImp::newPeer( EventStateUnbound esu , ServerPeerInfo && peer_info )
{
	MultiServer::ServerInfo server_info ;
	server_info.m_address = address() ; // GNet::Server::address()
	return m_ms.doNewPeer( esu , std::move(peer_info) , server_info ) ;
}

// ==

GNet::MultiServer::ServerInfo::ServerInfo() :
	m_address(Address::defaultAddress())
{
}

