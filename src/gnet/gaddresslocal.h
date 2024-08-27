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
/// \file gaddresslocal.h
///

#ifndef G_NET_ADDRESSLOCAL_H
#define G_NET_ADDRESSLOCAL_H

#include "gdef.h"
#include "gaddress.h"
#include "gstringview.h"
#include "gstringarray.h"
#include <string>
#if GCONFIG_HAVE_UDS
#include <sys/types.h>
#include <sys/un.h>
#else
#ifdef G_DOXYGEN
struct sockaddr_un ;
#else
struct sockaddr_un {} ;
#endif
#endif

namespace GNet
{
	class AddressLocal ;
}

//| \class GNet::AddressLocal
/// A 'sockaddr' wrapper class for local-domain addresses.
///
/// Use "netcat -U" or "socat" to connect to local-domain
/// sockets, eg:
/// \code
/// $ nc -U -C /run/cmd.s  # nc.openbsd, not nc.traditional
/// $ socat -d tcp-listen:8080,fork unix:/run/cmd.s
/// \endcode
///
class GNet::AddressLocal
{
public:
	using sockaddr_type = sockaddr_un ;

	explicit AddressLocal( unsigned int ) ;
	explicit AddressLocal( std::string_view ) ;
	AddressLocal( unsigned int port , int /*for overload resolution*/ ) ;
	AddressLocal( const sockaddr * addr , socklen_t len ) ;

	static int domain() noexcept ;
	static unsigned short af() noexcept ;
	const sockaddr * address() const ;
	sockaddr * address() ;
	socklen_t length() const noexcept ;
	unsigned long scopeId( unsigned long default_ = 0UL ) const ;
	unsigned int port() const ;
	void setPort( unsigned int port ) ;
	bool setZone( std::string_view ipv6_zone_name_or_scope_id ) ;
	void setScopeId( unsigned long ipv6_scope_id ) ;
	static bool validString( std::string_view , std::string * = nullptr ) ;
	static bool validStrings( std::string_view , std::string_view , std::string * = nullptr ) ;
	static bool validPort( unsigned int port ) ;
	static bool validData( const sockaddr * addr , socklen_t len ) ;

	bool same( const AddressLocal & other , bool ipv6_compare_with_scope = false ) const ;
	bool sameHostPart( const AddressLocal & other ) const ;
	bool isLoopback() const ;
	bool isLocal( std::string & ) const ;
	bool isLinkLocal() const ;
	bool isUniqueLocal() const ;
	bool isMulticast() const ;
	bool isAny() const ;
	unsigned int bits() const ;
	std::string displayString( bool ipv6_with_scope = false ) const ;
	std::string hostPartString() const ;
	std::string queryString() const ;
	G::StringArray wildcards() const ;
	static bool format( const std::string & ) ;

private:
	explicit AddressLocal( std::nullptr_t ) ;
	std::string path() const ;

private:
	sockaddr_type m_local ;
	std::size_t m_size ;
} ;

#endif
