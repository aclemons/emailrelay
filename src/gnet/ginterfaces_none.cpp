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
/// \file ginterfaces_none.cpp
///

#include "gdef.h"
#include "ginterfaces.h"
#include "gstr.h"
#include "gassert.h"

GNet::Interfaces::Interfaces( EventState es ) :
	m_es(es)
{
}

GNet::Interfaces::Interfaces( EventState es , InterfacesHandler & ) :
	m_es(es)
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

std::size_t GNet::Interfaces::addresses( std::vector<GNet::Address> & , const std::string & , unsigned int , int ) const
{
	return 0U ;
}

std::vector<GNet::Address> GNet::Interfaces::addresses( const std::string & , unsigned int , int ) const
{
	return {} ;
}

void GNet::Interfaces::readEvent()
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

