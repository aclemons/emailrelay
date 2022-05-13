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
/// \file gaddresslocal_unix.cpp
///

#include "gdef.h"
#include "gaddresslocal.h"
#include "gstr.h"
#include "gassert.h"
#include <cstddef> // offsetof
#include <cstring> // std::memcpy()
#include <sys/types.h>
#include <sys/un.h>

namespace GNet
{
	namespace AddressLocalImp
	{
		static constexpr std::size_t minsize()
		{
			return sizeof( sockaddr_un::sun_family ) ;
		}
		static constexpr std::size_t psize()
		{
			return sizeof( sockaddr_un::sun_path ) ;
		}
		static std::size_t poffset() noexcept
		{
			return offsetof( sockaddr_un , sun_path ) ;
		}
		std::string unescape( std::string path )
		{
			if( path.size() > 1U && path[0U] == '\\' && path[1U] == '0' )
			{
				path = path.substr( 1U ) ;
				path[0] = '\0' ;
			}
			return path ;
		}
	}
}

unsigned short GNet::AddressLocal::af() noexcept
{
	return AF_UNIX ;
}

int GNet::AddressLocal::domain() noexcept
{
	return PF_UNIX ;
}

GNet::AddressLocal::AddressLocal( std::nullptr_t ) :
	m_local{} ,
	m_size(AddressLocalImp::minsize())
{
	m_local.sun_family = af() ;
	std::memset( m_local.sun_path , 0 , AddressLocalImp::psize() ) ;
}

GNet::AddressLocal::AddressLocal( unsigned int /*port*/ ) :
	AddressLocal(nullptr)
{
}

GNet::AddressLocal::AddressLocal( unsigned int /*port*/ , int /*loopback_overload*/ ) :
	AddressLocal(nullptr)
{
}

GNet::AddressLocal::AddressLocal( const sockaddr * addr , socklen_t len , bool /*ipv6_scope_id_fixup*/ ) :
	AddressLocal(nullptr)
{
	std::size_t size = static_cast<std::size_t>( len ) ;

	if( addr == nullptr )
		throw Address::Error() ;
	if( addr->sa_family != af() || size > sizeof(sockaddr_type) )
		throw Address::BadFamily() ;

	m_local = *(reinterpret_cast<const sockaddr_type*>(addr)) ;
	m_size = size ;
}

GNet::AddressLocal::AddressLocal( const std::string & host_part , unsigned int /*port*/ ) :
	AddressLocal(nullptr)
{
	if( host_part.size() >= AddressLocalImp::psize() )
		throw Address::BadString( "local-domain address too long" ) ;
	std::memcpy( m_local.sun_path , host_part.data() , host_part.size() ) ;
	m_size = AddressLocalImp::poffset() + host_part.size() + 1U ; // include terminator
}

GNet::AddressLocal::AddressLocal( const std::string & host_part , const std::string & /*port_part*/ ) :
	AddressLocal(host_part,0)
{
}

GNet::AddressLocal::AddressLocal( const std::string & display_string ) :
	AddressLocal(AddressLocalImp::unescape(display_string),0)
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
	namespace imp = AddressLocalImp ;
	G_ASSERT( m_size >= imp::minsize() ) ;
	if( m_size <= imp::poffset() )
		return {} ;
	else if( m_local.sun_path[0] == '\0' ) // if abstract
		return { m_local.sun_path , std::min(m_size-imp::poffset(),imp::psize()) } ;
	else
		return { m_local.sun_path , std::min(std::strlen(m_local.sun_path),imp::psize()) } ;

}

std::string GNet::AddressLocal::displayString( bool /*ipv6_with_scope*/ ) const
{
	std::string p = path() ;
	return p.empty() ? std::string(1U,'/') : G::Str::printable( p ) ;
}

std::string GNet::AddressLocal::hostPartString( bool raw ) const
{
	return raw ? path() : G::Str::printable(path()) ;
}

std::string GNet::AddressLocal::queryString() const
{
	return {} ;
}

bool GNet::AddressLocal::validData( const sockaddr * addr , socklen_t len )
{
	return addr != nullptr && addr->sa_family == af() && len >= AddressLocalImp::minsize() && len <= sizeof(sockaddr_type) ;
}

bool GNet::AddressLocal::validString( const std::string & path , std::string * reason_p )
{
	const char * reason = nullptr ;
	if( path.size() > AddressLocalImp::psize() )
		reason = "local-domain address too long" ;
	if( path.empty() )
		reason = "empty string" ;
	if( path[0] != '\0' && path[0] != '/' )
		reason = "not an absolute filesystem path" ;
	if( reason && reason_p )
		*reason_p = std::string( reason ) ;
	return reason == nullptr ;
}

bool GNet::AddressLocal::validStrings( const std::string & host_part , const std::string & /*port_part*/ ,
	std::string * reason_p )
{
	return validString( host_part , reason_p ) ;
}

bool GNet::AddressLocal::validPort( unsigned int /*port*/ )
{
	return true ;
}

bool GNet::AddressLocal::same( const AddressLocal & other , bool /*ipv6_compare_with_scope*/ ) const
{
	G_ASSERT( m_local.sun_family == af() ) ;
	return
		m_local.sun_family == other.m_local.sun_family &&
		m_size == other.m_size &&
		path() == other.path() ;
}

bool GNet::AddressLocal::sameHostPart( const AddressLocal & other ) const
{
	return same( other ) ;
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
	return reinterpret_cast<const sockaddr*>(&m_local) ;
}

sockaddr * GNet::AddressLocal::address()
{
	return reinterpret_cast<sockaddr*>(&m_local) ;
}

socklen_t GNet::AddressLocal::length() const noexcept
{
	return m_size ;
}

G::StringArray GNet::AddressLocal::wildcards() const
{
	return { displayString() } ;
}

bool GNet::AddressLocal::format( const std::string & )
{
	return true ;
}

bool GNet::AddressLocal::isLocal( std::string & ) const
{
	return true ;
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
	return true ;
}

bool GNet::AddressLocal::isMulticast() const
{
	return false ;
}

bool GNet::AddressLocal::isAny() const
{
	return path().empty() ;
}

unsigned int GNet::AddressLocal::bits() const
{
	return 0U ;
}

