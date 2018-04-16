//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include <algorithm> // std::swap
#include <utility> // std::swap

GNet::MultiServer::MultiServer( ExceptionHandler & eh , const AddressList & address_list ) :
	m_eh(eh)
{
	G_ASSERT( ! address_list.empty() ) ;
	init( address_list ) ;
}

GNet::MultiServer::MultiServer( ExceptionHandler & eh ) :
	m_eh(eh)
{
}

GNet::MultiServer::~MultiServer()
{
	serverCleanup() ;
}

void GNet::MultiServer::init( const AddressList & address_list )
{
	G_ASSERT( ! address_list.empty() ) ;
	for( AddressList::const_iterator p = address_list.begin() ; p != address_list.end() ; ++p )
	{
		init( *p ) ;
	}
}

void GNet::MultiServer::init( const Address & address )
{
	// note that the Ptr class does not have proper value semantics...
	MultiServerPtr ptr( new MultiServerImp(*this,m_eh,address) ) ;
	m_server_list.push_back( MultiServerPtr() ) ; // copy a null pointer into the list
	m_server_list.back().swap( ptr ) ;
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
		if( Address::supports(Address::Family::ipv4()) ) result.push_back( Address(Address::Family::ipv4(),port) ) ;
		if( Address::supports(Address::Family::ipv6()) ) result.push_back( Address(Address::Family::ipv6(),port) ) ;
	}
	else
	{
		result = list ;
		for( AddressList::iterator p = result.begin() ; p != result.end() ; ++p )
			(*p).setPort( port ) ;
	}
	return result ;
}

GNet::MultiServer::AddressList GNet::MultiServer::addressList( const G::StringArray & list , unsigned int port )
{
	AddressList result ;
	if( list.empty() )
	{
		result.reserve( 2U ) ;
		if( Address::supports(Address::Family::ipv4()) ) result.push_back( Address(Address::Family::ipv4(),port) ) ;
		if( Address::supports(Address::Family::ipv6()) ) result.push_back( Address(Address::Family::ipv6(),port) ) ;
	}
	else
	{
		result.reserve( list.size() ) ;
		for( G::StringArray::const_iterator p = list.begin() ; p != list.end() ; ++p )
		{
			const bool is_inaddrany = (*p).empty() ;
			if( is_inaddrany )
			{
				if( Address::supports(Address::Family::ipv4()) ) result.push_back( Address(Address::Family::ipv4(),port) ) ;
				if( Address::supports(Address::Family::ipv6()) ) result.push_back( Address(Address::Family::ipv6(),port) ) ; // moot
			}
			else
			{
				result.push_back( Address(*p,port) ) ;
			}
		}
	}
	return result ;
}

void GNet::MultiServer::serverCleanup()
{
	for( List::iterator p = m_server_list.begin() ; p != m_server_list.end() ; ++p )
	{
		try
		{
			(*p).get()->cleanup() ;
		}
		catch(...) // dtor
		{
		}
	}
}

void GNet::MultiServer::serverReport( const std::string & type ) const
{
	for( List::const_iterator p = m_server_list.begin() ; p != m_server_list.end() ; ++p )
	{
		const Server & server = *((*p).get()) ;
		G_LOG_S( "GNet::MultiServer: " << type << " server on " << server.address().second.displayString() ) ;
	}
}

std::pair<bool,GNet::Address> GNet::MultiServer::firstAddress() const
{
	std::pair<bool,Address> result( false , Address::defaultAddress() ) ;
	for( List::const_iterator p = m_server_list.begin() ; p != m_server_list.end() ; ++p )
	{
		if( (*p).get()->address().first )
		{
			result.first = true ;
			result.second = (*p).get()->address().second ;
			break ;
		}
	}
	return result ;
}

// ==

GNet::MultiServerImp::MultiServerImp( MultiServer & ms , ExceptionHandler & eh , const Address & address ) :
	Server(eh,address) ,
	m_ms(ms)
{
}

GNet::ServerPeer * GNet::MultiServerImp::newPeer( PeerInfo peer_info )
{
	MultiServer::ServerInfo server_info ;
	server_info.m_address = address().first ? address().second : Address::defaultAddress() ;
	return m_ms.newPeer( peer_info , server_info ) ;
}

void GNet::MultiServerImp::cleanup()
{
	serverCleanup() ;
}

// ==

GNet::MultiServerPtr::MultiServerPtr( ServerImp * p ) :
	m_p(p)
{
}

GNet::MultiServerPtr::MultiServerPtr( const MultiServerPtr & other ) :
	m_p(other.m_p)
{
}

GNet::MultiServerPtr::~MultiServerPtr()
{
	delete m_p ;
}

void GNet::MultiServerPtr::operator=( const MultiServerPtr & rhs )
{
	m_p = rhs.m_p ;
}

void GNet::MultiServerPtr::swap( MultiServerPtr & other )
{
	std::swap( other.m_p , m_p ) ;
}

GNet::MultiServerImp * GNet::MultiServerPtr::get()
{
	return m_p ;
}

const GNet::MultiServerImp * GNet::MultiServerPtr::get() const
{
	return m_p ;
}

// ==

GNet::MultiServer::ServerInfo::ServerInfo() :
	m_address(Address::defaultAddress())
{
}

/// \file gmultiserver.cpp
