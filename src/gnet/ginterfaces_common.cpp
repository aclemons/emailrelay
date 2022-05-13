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
/// \file ginterfaces_common.cpp
///

#include "gdef.h"
#include "ginterfaces.h"
#include "gstr.h"
#include "gtest.h"
#include <algorithm>

GNet::Interfaces::Interfaces()
= default;

GNet::Interfaces::Interfaces( ExceptionSink es , InterfacesHandler & handler ) :
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

bool GNet::Interfaces::supported()
{
	return true ;
}

bool GNet::Interfaces::loaded() const
{
	return m_loaded ;
}

std::vector<GNet::Address> GNet::Interfaces::find( const std::string & name_in , unsigned int port ,
	bool decoration ) const
{
	std::string name = name_in ;
	int type = 0 ;
	if( decoration )
	{
		if( G::Str::tailMatch(name_in,"-ipv6") )
		{
			name = name_in.substr( 0U , name_in.length()-5U ) ;
			type = 6 ;
		}
		else if( G::Str::tailMatch(name_in,"-ipv4") )
		{
			name = name_in.substr( 0U , name_in.length()-5U ) ;
			type = 4 ;
		}
	}

	if( name.empty() )
		return AddressList() ;

	if( !loaded() )
		const_cast<Interfaces*>(this)->load() ;

	AddressList result ;
	for( const auto & item : m_list )
	{
		if( ( item.name == name || item.altname == name ) && item.up && item.valid_address )
		{
			if( type == 0 ||
				( type == 6 && item.address.is6() ) ||
				( type == 4 && item.address.is4() ) )
			{
				result.push_back( item.address ) ;
				result.back().setPort( port ) ;
			}
		}
	}
	return result ;
}

std::vector<GNet::Address> GNet::Interfaces::addresses( const G::StringArray & names , unsigned int port ,
	G::StringArray & used_names , G::StringArray & empty_names , G::StringArray & bad_names ) const
{
	AddressList result ;
	for( const auto & name : names )
	{
		if( Address::validStrings( name , G::Str::fromUInt(port) ) )
		{
			result.push_back( Address::parse(name,port) ) ;
		}
		else
		{
			// 'name' is not an address so treat it as an interface name having
			// bound addresses -- reject file system paths as 'bad' unless
			// they are under "/dev" (bsd)
			AddressList list = find( name , port , true ) ;
			if( list.empty() && ( name.empty() || ( name.find('/') != std::string::npos && name.find("/dev/") != 0U ) ) )
			{
				bad_names.push_back( name ) ;
			}
			else if( list.empty() )
			{
				empty_names.push_back( name ) ;
			}
			else
			{
				used_names.push_back( name ) ;
			}
			result.insert( result.end() , list.begin() , list.end() ) ;
		}
	}
	return result ;
}

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

GNet::Interfaces::const_iterator GNet::Interfaces::begin() const
{
	return m_list.begin() ;
}

GNet::Interfaces::const_iterator GNet::Interfaces::end() const
{
	return m_list.end() ;
}

void GNet::Interfaces::readEvent( Descriptor )
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

