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
/// \file ginterfaces_none.cpp
///

#include "gdef.h"
#include "ginterfaces.h"
#include "gstr.h"

GNet::Interfaces::Interfaces()
= default;

GNet::Interfaces::Interfaces( ExceptionSink , InterfacesHandler & )
{
}

GNet::Interfaces::~Interfaces()
= default;

bool GNet::Interfaces::supported()
{
	return false ;
}

bool GNet::Interfaces::active()
{
	return false ;
}

void GNet::Interfaces::load()
{
}

bool GNet::Interfaces::loaded() const
{
	return true ;
}

std::vector<GNet::Address> GNet::Interfaces::find( const std::string & , unsigned int , bool ) const
{
	return AddressList() ;
}

std::vector<GNet::Address> GNet::Interfaces::addresses( const G::StringArray & names , unsigned int port ,
	G::StringArray & used_names , G::StringArray & , G::StringArray & bad_names ) const
{
	AddressList result ;
	for( const auto & name : names )
	{
		if( !Address::validStrings( name , G::Str::fromUInt(port) ) )
		{
			bad_names.push_back( name ) ;
		}
		else
		{
			used_names.push_back( name ) ;
			result.push_back( Address::parse(name,port) ) ;
		}
	}
	return result ;
}

G::StringArray GNet::Interfaces::names( bool ) const
{
	return G::StringArray() ;
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
}

void GNet::Interfaces::onFutureEvent()
{
}

// ==

GNet::Interfaces::Item::Item() :
	address(Address::defaultAddress())
{
}

