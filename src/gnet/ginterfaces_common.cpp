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
/// \file ginterfaces_common.cpp
///

#include "gdef.h"
#include "ginterfaces.h"
#include "gstr.h"
#include "gtest.h"
#include <algorithm>

#ifndef G_LIB_SMALL
GNet::Interfaces::Interfaces( EventState es ) :
	m_es(es)
{
}
#endif

GNet::Interfaces::Interfaces( EventState es , InterfacesHandler & handler ) :
	m_es(es) ,
	m_handler(&handler)
{
}

GNet::Interfaces::~Interfaces()
= default;

void GNet::Interfaces::load()
{
	std::vector<Item> new_list ;
	loadImp( m_es , new_list ) ;
	m_loaded = true ;
	using std::swap ;
	swap( m_list , new_list ) ;
}

#ifndef G_LIB_SMALL
bool GNet::Interfaces::supported()
{
	return true ;
}
#endif

bool GNet::Interfaces::loaded() const
{
	return m_loaded ;
}

#ifndef G_LIB_SMALL
std::vector<GNet::Address> GNet::Interfaces::addresses( const std::string & name , unsigned int port , int af ) const
{
	std::vector<GNet::Address> result ;
	addresses( result , name , port , af ) ;
	return result ;
}
#endif

std::size_t GNet::Interfaces::addresses( std::vector<Address> & out , const std::string & name , unsigned int port , int af ) const
{
	if( !loaded() )
		const_cast<Interfaces*>(this)->load() ;

	std::size_t count = 0U ;
	for( const auto & item : m_list )
	{
		if( !name.empty() && ( item.name == name || item.altname == name ) && item.up && item.valid_address )
		{
			if( af == AF_UNSPEC ||
				( af == AF_INET6 && item.address.is6() ) ||
				( af == AF_INET && item.address.is4() ) )
			{
				count++ ;
				out.push_back( item.address ) ;
				out.back().setPort( port ) ;
			}
		}
	}
	return count ;
}

#ifndef G_LIB_SMALL
G::StringArray GNet::Interfaces::names( bool all ) const
{
	G::StringArray list ;
	for( const auto & iface : *this )
	{
		if( all || iface.up )
			list.push_back( iface.name ) ;
	}
	std::sort( list.begin() , list.end() ) ;
	list.erase( std::unique(list.begin(),list.end()) , list.end() ) ;
	return list ;
}
#endif

GNet::Interfaces::const_iterator GNet::Interfaces::begin() const
{
	return m_list.begin() ;
}

GNet::Interfaces::const_iterator GNet::Interfaces::end() const
{
	return m_list.end() ;
}

void GNet::Interfaces::readEvent()
{
	if( m_notifier )
	{
		std::string s = m_notifier->readEvent() ;
		if( m_handler && !s.empty() )
			m_handler->onInterfaceEvent( s ) ;
	}
}

void GNet::Interfaces::onFutureEvent()
{
	if( m_notifier )
	{
		std::string s = m_notifier->onFutureEvent() ;
		if( m_handler && !s.empty() )
			m_handler->onInterfaceEvent( s ) ;
	}
}

// ==

GNet::Interfaces::Item::Item() :
	address(Address::defaultAddress())
{
}

