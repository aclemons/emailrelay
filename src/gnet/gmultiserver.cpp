//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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

namespace GNet
{
	class Listeners ;
}

class GNet::Listeners /// Used by GNet::MultiServer to represent a set of listening inputs (fd, interface or address).
{
public:
	Listeners( Interfaces & , const G::StringArray & , unsigned int port ) ;
	bool empty() const ; // no inputs
	bool defunct() const ; // no inputs and static
	bool idle() const ; // no inputs but some interfaces might come up
	bool hasBad() const ; // one or more invalid inputs
	std::string badName() const ; // first invalid input
	bool hasEmpties() const ; // some interfaces have no addresses
	std::string logEmpties() const ; // log line snippet for hasEmpties()
	bool noUpdates() const ; // some inputs are interfaces but static
	const std::vector<int> & fds() const ; // fd inputs
	const std::vector<Address> & fixed() const ; // address inputs
	const std::vector<Address> & dynamic() const ; // interface addresses

private:
	void addWildcards( unsigned int ) ;
	static int parseFd( const std::string & ) ;
	static bool isAddress( const std::string & , unsigned int ) ;
	static Address address( const std::string & , unsigned int ) ;
	static int af( const std::string & ) ;
	static std::string basename( const std::string & ) ;
	static bool isBad( const std::string & ) ;

private:
	std::string m_bad ;
	G::StringArray m_empties ;
	G::StringArray m_used ;
	std::vector<Address> m_fixed ;
	std::vector<Address> m_dynamic ;
	std::vector<int> m_fds ;
} ;

GNet::MultiServer::MultiServer( ExceptionSink es , const G::StringArray & listener_list , unsigned int port ,
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

GNet::MultiServerImp::MultiServerImp( MultiServer & ms , ExceptionSink es , bool fixed , const Address & address ,
	ServerPeer::Config server_peer_config , Server::Config server_config ) :
		GNet::Server(es,address,server_peer_config,server_config) ,
		m_ms(ms) ,
		m_fixed(fixed)
{
}

GNet::MultiServerImp::MultiServerImp( MultiServer & ms , ExceptionSink es , Descriptor fd ,
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

// ==

GNet::Listeners::Listeners( Interfaces & if_ , const G::StringArray & listener_list , unsigned int port )
{
	// listeners are file-descriptors, addresses or interface names (possibly decorated)
	for( const auto & listener : listener_list )
	{
		int fd = G::is_windows() ? -1 : parseFd( listener ) ;
		if( fd >= 0 )
		{
			m_fds.push_back( fd ) ;
		}
		else if( isAddress(listener,port) )
		{
			m_fixed.push_back( address(listener,port) ) ;
		}
		else
		{
			std::size_t n = if_.addresses( m_dynamic , basename(listener) , port , af(listener) ) ;
			if( n == 0U && isBad(listener) )
				m_bad = listener ;
			(n?m_used:m_empties).push_back( listener ) ;
		}
	}
	if( empty() )
		addWildcards( port ) ;
}

int GNet::Listeners::af( const std::string & s )
{
	if( G::Str::tailMatch(s,"-ipv6") )
		return AF_INET6 ;
	else if( G::Str::tailMatch(s,"-ipv4") )
		return AF_INET ;
	else
		return AF_UNSPEC ;
}

std::string GNet::Listeners::basename( const std::string & s )
{
	return
		G::Str::tailMatch(s,"-ipv6") || G::Str::tailMatch(s,"-ipv4") ?
			s.substr( 0U , s.length()-5U ) :
			s ;
}

int GNet::Listeners::parseFd( const std::string & listener )
{
    if( listener.size() > 3U && listener.find("fd#") == 0U && G::Str::isUInt(listener.substr(3U)) )
    {
        int fd = G::Str::toInt( listener.substr(3U) ) ;
        if( fd < 0 ) throw MultiServer::InvalidFd( listener ) ;
        return fd ;
    }
    return -1 ;
}

void GNet::Listeners::addWildcards( unsigned int port )
{
	if( StreamSocket::supports(Address::Family::ipv4) )
		m_fixed.emplace_back( Address::Family::ipv4 , port ) ;

	if( StreamSocket::supports(Address::Family::ipv6) )
		m_fixed.emplace_back( Address::Family::ipv6 , port ) ;
}

bool GNet::Listeners::isAddress( const std::string & s , unsigned int port )
{
	return Address::validStrings( s , G::Str::fromUInt(port) ) ;
}

GNet::Address GNet::Listeners::address( const std::string & s , unsigned int port )
{
	return Address::parse( s , port ) ;
}

bool GNet::Listeners::empty() const
{
	return m_fds.empty() && m_fixed.empty() && m_dynamic.empty() ;
}

bool GNet::Listeners::defunct() const
{
	return empty() && !Interfaces::active() ;
}

bool GNet::Listeners::idle() const
{
	return empty() && hasEmpties() && Interfaces::active() ;
}

bool GNet::Listeners::noUpdates() const
{
	return !m_used.empty() && !Interfaces::active() ;
}

bool GNet::Listeners::isBad( const std::string & s )
{
	// the input is not an address and not an interface-with-addresses so
	// report it as bad if clearly not an interface-with-no-addresses --
	// a slash is not normally allowed in an interface name, but allow "/dev/..."
	// because of bsd
	return s.empty() || ( s.find('/') != std::string::npos && s.find("/dev/") != 0U ) ;
}

bool GNet::Listeners::hasBad() const
{
	return !m_bad.empty() ;
}

std::string GNet::Listeners::badName() const
{
	return m_bad ;
}

bool GNet::Listeners::hasEmpties() const
{
	return !m_empties.empty() ;
}

std::string GNet::Listeners::logEmpties() const
{
	return std::string(m_empties.size()==1U?" \"":"s \"").append(G::Str::join("\", \"",m_empties)).append(1U,'"') ;
}

const std::vector<int> & GNet::Listeners::fds() const
{
	return m_fds ;
}

const std::vector<GNet::Address> & GNet::Listeners::fixed() const
{
	return m_fixed ;
}

const std::vector<GNet::Address> & GNet::Listeners::dynamic() const
{
	return m_dynamic ;
}

