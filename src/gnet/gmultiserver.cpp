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
/// \file gmultiserver.cpp
///

#include "gdef.h"
#include "gmultiserver.h"
#include "gdatetime.h"
#include "gstr.h"
#include "gtest.h"
#include "glog.h"
#include "gassert.h"
#include <list>
#include <algorithm>

GNet::MultiServer::MultiServer( ExceptionSink es , const G::StringArray & listener_list , unsigned int port ,
	const std::string & server_type , ServerPeer::Config server_peer_config , Server::Config server_config ) :
		m_es(es) ,
		m_port(port) ,
		m_server_type(server_type) ,
		m_server_peer_config(server_peer_config) ,
		m_server_config(server_config) ,
		m_if(es,*this) ,
		m_interface_event_timer(*this,&MultiServer::onInterfaceEventTimeout,es)
{
	// parse the listener list
	std::vector<int> fds ;
	Interfaces::Addresses result ; // addresses, used_names, empty_names, bad_names, good_names
	for( const auto & listener : listener_list )
	{
		if( listener.empty() ) continue ;
		int fd = G::is_windows() ? -1 : parseFd( listener ) ;
		if( fd >= 0 )
			fds.push_back( fd ) ;
		else
			m_if.addresses( listener , port , result ) ;
	}
	AddressList & address_list = result.addresses ;

	// fail if any bad names
	if( !result.bad_names.empty() )
		throw InvalidName( result.bad_names.at(0) ) ;

	// apply a default set of two wildcard addresses
	if( fds.empty() && address_list.empty() && result.empty_names.empty() )
	{
		if( Address::supports(Address::Family::ipv4) )
			address_list.push_back( Address(Address::Family::ipv4,port) ) ;
		if( Address::supports(Address::Family::ipv6) && StreamSocket::supports(Address::Family::ipv6) )
			address_list.push_back( Address(Address::Family::ipv6,port) ) ;
		if( address_list.empty() )
			throw NoListeningAddresses() ;
	}

	// fail if no addresses and no prospect of getting any
	if( fds.empty() && address_list.empty() && !Interfaces::active() )
		throw NoListeningAddresses() ;

	// save used and empty names to query G::Interfaces later on
	m_if_names = result.good_names ;

	// warn if no addresses from one or more interface names
	if( !result.empty_names.empty() && !address_list.empty() )
	{
		G_WARNING( "GNet::MultiServer::ctor: no addresses bound to named network interface"
			<< (result.empty_names.size()==1U?"":"s")
			<< " \"" << G::Str::join("\", \"",result.empty_names) << "\"" ) ;
	}

	// warn if doing nothing until an interface come up
	if( address_list.empty() && fds.empty() && !result.empty_names.empty() )
	{
		G_WARNING( "GNet::MultiServer::ctor: " << m_server_type << " server: nothing to do: "
			<< "waiting for interface" << (result.empty_names.size()>1U?"s ":" ")
			<< "[" << G::Str::join("] [",result.empty_names) << "]" ) ;
	}

	// warn if we got addresses from an interface name but won't get dynamic updates
	if( !result.used_names.empty() && !Interfaces::active() )
	{
		G_WARNING_ONCE( "GNet::MultiServer::ctor: named network interfaces "
			"are not being monitored for address updates" ) ;
	}

	// instantiate the servers
	for( const auto & fd : fds )
	{
		createServer( Descriptor(fd) ) ;
	}
	for( const auto & address : address_list )
	{
		createServer( address ) ;
	}
}

GNet::MultiServer::~MultiServer()
{
	serverCleanup() ;
}

int GNet::MultiServer::parseFd( const std::string & listener )
{
	if( listener.size() > 3U && listener.find("fd#") == 0U && G::Str::isUInt(listener.substr(3U)) )
	{
		int fd = G::Str::toInt( listener.substr(3U) ) ;
		if( fd < 0 ) throw InvalidFd( listener ) ;
		return fd ;
	}
	return -1 ;
}

void GNet::MultiServer::createServer( Descriptor fd )
{
	m_server_list.emplace_back( std::make_unique<MultiServerImp>( *this , m_es ,
		fd , m_server_peer_config , m_server_config ) ) ;
}

void GNet::MultiServer::createServer( const Address & address )
{
	m_server_list.emplace_back( std::make_unique<MultiServerImp>( *this , m_es ,
		address , m_server_peer_config , m_server_config ) ) ;
}

void GNet::MultiServer::createServer( const Address & address , std::nothrow_t )
{
	try
	{
		createServer( address ) ;
		G_LOG_S( "GNet::MultiServer::createServer: new " << m_server_type
			<< " server on " << displayString(address) ) ;
	}
	catch( Socket::SocketBindError & e )
	{
		// (can fail here if notified too soon, but succeeds later)
		G_LOG( "GNet::MultiServer::createServer: failed to bind " << displayString(address)
			<< " for new " << m_server_type << " server:"
			<< G::Str::tail(e.what(),G::string_view(e.what()).rfind(':')) ) ;
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
	AddressList address_list = m_if.addresses( m_if_names , m_port ).addresses ;

	// delete old
	for( auto server_iter = m_server_list.begin() ; server_iter != m_server_list.end() ; )
	{
		if( !gotAddressFor( **server_iter , address_list ) )
			server_iter = removeServer( server_iter ) ;
		else
			++server_iter ;
	}

	// create new
	for( const auto & address : address_list )
	{
		G_DEBUG( "GNet::MultiServer::onInterfaceEvent: address: " << displayString(address) ) ;
		if( !gotServerFor(address) )
		{
			createServer( address , std::nothrow ) ;
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

std::unique_ptr<GNet::ServerPeer> GNet::MultiServer::doNewPeer( ExceptionSinkUnbound esu ,
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

GNet::MultiServerImp::MultiServerImp( MultiServer & ms , ExceptionSink es , const Address & address ,
	ServerPeer::Config server_peer_config , Server::Config server_config ) :
		GNet::Server(es,address,server_peer_config,server_config) ,
		m_ms(ms)
{
}

GNet::MultiServerImp::MultiServerImp( MultiServer & ms , ExceptionSink es , Descriptor fd ,
	ServerPeer::Config server_peer_config , Server::Config server_config ) :
		GNet::Server(es,fd,server_peer_config,server_config) ,
		m_ms(ms)
{
}

GNet::MultiServerImp::~MultiServerImp()
= default;

void GNet::MultiServerImp::cleanup()
{
	serverCleanup() ;
}

std::unique_ptr<GNet::ServerPeer> GNet::MultiServerImp::newPeer( ExceptionSinkUnbound esu , ServerPeerInfo && peer_info )
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
