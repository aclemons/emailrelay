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
// gmultiserver.cpp
//

#include "gdef.h"
#include "gmultiserver.h"
#include "glog.h"
#include "gassert.h"
#include <list>

GNet::MultiServer::MultiServer( ExceptionSink es , const AddressList & address_list , ServerPeerConfig server_peer_config ) :
	m_es(es)
{
	G_ASSERT( ! address_list.empty() ) ;
	init( address_list , server_peer_config ) ;
}

GNet::MultiServer::~MultiServer()
{
	serverCleanup() ;
}

void GNet::MultiServer::serverCleanup()
{
	for( ServerList::iterator p = m_server_list.begin() ; p != m_server_list.end() ; ++p )
	{
		(*p)->cleanup() ;
	}
}

void GNet::MultiServer::init( const AddressList & address_list , ServerPeerConfig server_peer_config )
{
	G_ASSERT( ! address_list.empty() ) ;
	for( AddressList::const_iterator p = address_list.begin() ; p != address_list.end() ; ++p )
	{
		init( *p , server_peer_config ) ;
	}
}

void GNet::MultiServer::init( const Address & address , ServerPeerConfig server_peer_config )
{
	ServerPtr server_ptr( new MultiServerImp(*this,m_es,address,server_peer_config) ) ;
	m_server_list.push_back( server_ptr ) ;
}

bool GNet::MultiServer::canBind( const AddressList & address_list , bool do_throw )
{
	for( AddressList::const_iterator p = address_list.begin() ; p != address_list.end() ; ++p )
	{
		if( ! Server::canBind( *p , do_throw ) )
			return false ;
	}
	return true ;
}

GNet::MultiServer::AddressList GNet::MultiServer::addressList( const Address & address )
{
	AddressList result ;
	result.reserve( 1U ) ;
	result.push_back( address ) ;
	return result ;
}

GNet::MultiServer::AddressList GNet::MultiServer::addressList( const AddressList & list , unsigned int port )
{
	AddressList result ;
	if( list.empty() )
	{
		result.reserve( 2U ) ;
		if( Address::supports(Address::Family::ipv4) ) result.push_back( Address(Address::Family::ipv4,port) ) ;
		if( Address::supports(Address::Family::ipv6) ) result.push_back( Address(Address::Family::ipv6,port) ) ;
	}
	else
	{
		result = list ;
		for( AddressList::iterator p = result.begin() ; p != result.end() ; ++p )
			(*p).setPort( port ) ;
	}
	G_ASSERT( !result.empty() ) ;
	return result ;
}

GNet::MultiServer::AddressList GNet::MultiServer::addressList( const G::StringArray & list , unsigned int port )
{
	AddressList result ;
	if( list.empty() )
	{
		result.reserve( 2U ) ;
		if( Address::supports(Address::Family::ipv4) ) result.push_back( Address(Address::Family::ipv4,port) ) ;
		if( Address::supports(Address::Family::ipv6) ) result.push_back( Address(Address::Family::ipv6,port) ) ;
	}
	else
	{
		result.reserve( list.size() ) ;
		for( G::StringArray::const_iterator p = list.begin() ; p != list.end() ; ++p )
		{
			const bool is_inaddrany = (*p).empty() ;
			if( is_inaddrany )
			{
				if( Address::supports(Address::Family::ipv4) ) result.push_back( Address(Address::Family::ipv4,port) ) ;
				if( Address::supports(Address::Family::ipv6) ) result.push_back( Address(Address::Family::ipv6,port) ) ; // moot
			}
			else
			{
				result.push_back( Address(*p,port) ) ;
			}
		}
	}
	G_ASSERT( !result.empty() ) ;
	return result ;
}

void GNet::MultiServer::serverReport( const std::string & type ) const
{
	for( ServerList::const_iterator p = m_server_list.begin() ; p != m_server_list.end() ; ++p )
	{
		const Server & server = *(*p).get() ;
		G_LOG_S( "GNet::MultiServer: " << type << " server on " << server.address().displayString() ) ;
	}
}

unique_ptr<GNet::ServerPeer> GNet::MultiServer::doNewPeer( ExceptionSinkUnbound esu , ServerPeerInfo pi , ServerInfo si )
{
	return newPeer( esu , pi , si ) ;
}

bool GNet::MultiServer::hasPeers() const
{
	for( ServerList::const_iterator server_p = m_server_list.begin() ; server_p != m_server_list.end() ; ++server_p )
	{
		if( (*server_p)->hasPeers() )
			return true ;
	}
	return false ;
}

std::vector<weak_ptr<GNet::ServerPeer> > GNet::MultiServer::peers()
{
	typedef std::vector<weak_ptr<ServerPeer> > List ;
	List result ;
	for( ServerList::iterator server_p = m_server_list.begin() ; server_p != m_server_list.end() ; ++server_p )
	{
		List list = (*server_p)->peers() ;
		result.insert( result.end() , list.begin() , list.end() ) ;
	}
	return result ;
}

// ==

GNet::MultiServerImp::MultiServerImp( MultiServer & ms , ExceptionSink es , const Address & address , ServerPeerConfig server_peer_config ) :
	GNet::Server(es,address,server_peer_config) ,
	m_ms(ms) ,
	m_address(address)
{
}

GNet::MultiServerImp::~MultiServerImp()
{
}

void GNet::MultiServerImp::cleanup()
{
	serverCleanup() ;
}

unique_ptr<GNet::ServerPeer> GNet::MultiServerImp::newPeer( ExceptionSinkUnbound esu , ServerPeerInfo peer_info )
{
	MultiServer::ServerInfo server_info ;
	server_info.m_address = address() ; // GNet::Server::address()
	return m_ms.doNewPeer( esu , peer_info , server_info ) ;
}

// ==

GNet::MultiServer::ServerInfo::ServerInfo() :
	m_address(Address::defaultAddress())
{
}
/// \file gmultiserver.cpp
