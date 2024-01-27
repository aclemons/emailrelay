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
		constexpr std::size_t minsize()
		{
			#if GCONFIG_HAVE_UDS_LEN
			return offsetof( sockaddr_un , sun_family ) + sizeof( sockaddr_un::sun_family ) ;
			#else
			return sizeof( sockaddr_un::sun_family ) ;
			#endif
		}
		void setsize( sockaddr_un & a ) noexcept
		{
			#if GCONFIG_HAVE_UDS_LEN
			a.sun_len = SUN_LEN( &a ) ; // ie. poffset() + strlen(sun_path)
			#else
			GDEF_IGNORE_PARAM( a ) ;
			#endif
		}
		constexpr std::size_t psize()
		{
			return sizeof( sockaddr_un::sun_path ) ;
		}
		constexpr std::size_t poffset() noexcept
		{
			return offsetof( sockaddr_un , sun_path ) ;
		}
		std::size_t strnlen( const char * p , std::size_t limit ) noexcept
		{
			std::size_t n = 0U ;
			for( ; p && *p && n < limit ; ++p )
				n++ ;
			return n ;
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
	namespace imp = AddressLocalImp ;
	m_local.sun_family = af() ;
	std::memset( m_local.sun_path , 0 , AddressLocalImp::psize() ) ;
	imp::setsize( m_local ) ;
}

GNet::AddressLocal::AddressLocal( unsigned int /*port*/ ) :
	AddressLocal(nullptr)
{
}

GNet::AddressLocal::AddressLocal( unsigned int /*port*/ , int /*loopback_overload*/ ) :
	AddressLocal(nullptr)
{
}

GNet::AddressLocal::AddressLocal( const sockaddr * addr , socklen_t len ) :
	AddressLocal(nullptr)
{
	namespace imp = AddressLocalImp ;
	std::size_t size = static_cast<std::size_t>( len ) ;

	if( addr == nullptr || size < imp::minsize() || size > sizeof(sockaddr_type) )
		throw Address::Error( "invalid unix domain sockaddr" ) ;

	if( addr->sa_family != af() )
		throw Address::BadFamily() ;

	std::memcpy( &m_local , addr , size ) ;

	if( size <= imp::poffset() )
	{
		// unnamed/unbound address
		m_size = imp::minsize() ;
	}
	else if( G::is_linux() && m_local.sun_path[0] == '\0' )
	{
		// abstract address (linux)
		m_size = size ;
	}
	else
	{
		// pathname address

		// make sure that sun_path[] is terminated somewhere
		if( size == sizeof(sockaddr_type) )
		{
			const char * p = &m_local.sun_path[0] ;
			const char * end = p + imp::psize() ;
			if( std::find( p , end , 0 ) == end )
				throw Address::Error( "unix domain path too long" ) ;
		}

		// our additional constraints
		if( !G::Str::isPrintable( std::string(&m_local.sun_path[0]) ) )
			throw Address::BadString( "invalid unix domain socket path" ) ;

		// the structure passed in might be sized to beyond the first
		// NUL, so calculate our own size
		m_size = imp::poffset() + std::strlen( &m_local.sun_path[0] ) + 1U ;
		G_ASSERT( m_size <= size ) ;

		imp::setsize( m_local ) ;
	}
}

GNet::AddressLocal::AddressLocal( const std::string & host_part ) :
	AddressLocal(nullptr)
{
	namespace imp = AddressLocalImp ;

	if( host_part.empty() || host_part.at(0) != '/' )
		throw Address::BadString() ;

	if( host_part == "/" || !G::Str::isPrintable(host_part) )
		throw Address::BadString() ;

	if( host_part.size() >= imp::psize() )
		throw Address::BadString( "unix domain address too long" ) ;

	std::memcpy( &m_local.sun_path[0] , host_part.data() , host_part.size() ) ;
	imp::setsize( m_local ) ;
	m_size = imp::poffset() + host_part.size() + 1U ; // include terminator in m_size (see unix(7))
}

std::string GNet::AddressLocal::path() const
{
	namespace imp = AddressLocalImp ;
	if( m_size <= imp::poffset() )
	{
		return std::string( 1U , '/' ) ; // unbound address displayed as "/" // NOLINT not return {...}
	}
	else if( G::is_linux() && m_local.sun_path[0] == '\0' )
	{
		return { &m_local.sun_path[0] , m_size-imp::poffset() } ;
	}
	else
	{
		std::string p = std::string( &m_local.sun_path[0] , imp::strnlen( &m_local.sun_path[0] , std::min(m_size-imp::poffset(),imp::psize()) ) ) ;
		return p.empty() ? std::string(1U,'/') : p ;
	}
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

std::string GNet::AddressLocal::displayString( bool /*ipv6_with_scope*/ ) const
{
	return path() ;
}

std::string GNet::AddressLocal::hostPartString() const
{
	return path() ;
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
	if( path[0] != '/' )
		reason = "not an absolute filesystem path" ;
	if( !G::Str::isPrintable(path) )
		reason = "invalid characters" ;
	if( reason && reason_p )
		*reason_p = std::string( reason ) ;
	return reason == nullptr ;
}

bool GNet::AddressLocal::validStrings( const std::string & host_part , const std::string & /*port_part*/ ,
	std::string * reason_p )
{
	return validString( host_part , reason_p ) ;
}

#ifndef G_LIB_SMALL
bool GNet::AddressLocal::validPort( unsigned int /*port*/ )
{
	return true ;
}
#endif

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

#ifndef G_LIB_SMALL
const sockaddr * GNet::AddressLocal::address() const
{
	return reinterpret_cast<const sockaddr*>(&m_local) ;
}
#endif

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

#ifndef G_LIB_SMALL
bool GNet::AddressLocal::format( const std::string & )
{
	return true ;
}
#endif

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

