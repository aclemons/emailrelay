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
/// \file gaddresslocal_none.cpp
///

#include "gdef.h"
#include "gaddresslocal.h"

unsigned short GNet::AddressLocal::af() noexcept
{
	return 0 ;
}

int GNet::AddressLocal::domain() noexcept
{
	return 0 ;
}

GNet::AddressLocal::AddressLocal( std::nullptr_t ) :
	m_size(0U)
{
	// avoid clang 'unused field' warnings...
	GDEF_IGNORE_VARIABLE( m_size ) ;
	GDEF_IGNORE_VARIABLE( m_local ) ;
}

GNet::AddressLocal::AddressLocal( unsigned int /*port*/ ) :
	m_size(0U)
{
}

GNet::AddressLocal::AddressLocal( unsigned int /*port*/ , int /*loopback_overload*/ ) :
	m_size(0U)
{
}

GNet::AddressLocal::AddressLocal( const sockaddr * /*addr*/ , socklen_t /*len*/ ) :
	m_size(0U)
{
}

GNet::AddressLocal::AddressLocal( const std::string & /*host_part*/ ) :
	m_size(0U)
{
}

void GNet::AddressLocal::setPort( unsigned int /*port*/ )
{
}

bool GNet::AddressLocal::setZone( const std::string & /*ipv6_zone_name_or_scope_id*/ )
{
	return true ;
}

void GNet::AddressLocal::setScopeId( unsigned long /*ipv6_scope_id*/ )
{
}

std::string GNet::AddressLocal::path() const
{
	return std::string() ;
}

std::string GNet::AddressLocal::displayString( bool /*ipv6_with_scope*/ ) const
{
	return path() ;
}

std::string GNet::AddressLocal::hostPartString() const
{
	return displayString() ;
}

std::string GNet::AddressLocal::queryString() const
{
	return std::string() ;
}

bool GNet::AddressLocal::validData( const sockaddr * /*addr*/ , socklen_t /*len*/ )
{
	return false ;
}

bool GNet::AddressLocal::validString( const std::string & /*path*/ , std::string * reason_p )
{
	if( reason_p )
		*reason_p = "not implemented" ;
	return false ;
}

bool GNet::AddressLocal::validStrings( const std::string & /*host_part*/ , const std::string & /*port_part*/ ,
	std::string * reason_p )
{
	return validString( std::string() , reason_p ) ;
}

bool GNet::AddressLocal::validPort( unsigned int /*port*/ )
{
	return false ;
}

bool GNet::AddressLocal::same( const AddressLocal & /*other*/ , bool /*ipv6_compare_with_scope*/ ) const
{
	return false ;
}

bool GNet::AddressLocal::sameHostPart( const AddressLocal & /*other*/ ) const
{
	return false ;
}

unsigned int GNet::AddressLocal::port() const
{
	return 0U ;
}

unsigned long GNet::AddressLocal::scopeId( unsigned long default_ ) const
{
	return default_ ;
}

const sockaddr * GNet::AddressLocal::address() const
{
	return nullptr ;
}

sockaddr * GNet::AddressLocal::address()
{
	return nullptr ;
}

socklen_t GNet::AddressLocal::length() const noexcept
{
	return 0 ;
}

G::StringArray GNet::AddressLocal::wildcards() const
{
	return {} ;
}

bool GNet::AddressLocal::format( const std::string & )
{
	return true ;
}

bool GNet::AddressLocal::isLocal( std::string & ) const
{
	return false ;
}

bool GNet::AddressLocal::isLoopback() const
{
	return false ;
}

bool GNet::AddressLocal::isLinkLocal() const
{
	return false ;
}

bool GNet::AddressLocal::isUniqueLocal() const
{
	return false ;
}

bool GNet::AddressLocal::isMulticast() const
{
	return false ;
}

bool GNet::AddressLocal::isAny() const
{
	return false ;
}

unsigned int GNet::AddressLocal::bits() const
{
	return 0U ;
}

