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
/// \file gaddress6.h
///

#ifndef G_NET_ADDRESS6_H
#define G_NET_ADDRESS6_H

#include "gdef.h"
#include "gaddress.h"
#include <string>

namespace GNet
{
	class Address6 ;
}

//| \class GNet::Address6
/// A 'sockaddr' wrapper class for IPv6 addresses.
///
class GNet::Address6
{
public:
	using sockaddr_type = sockaddr_in6 ;

	explicit Address6( unsigned int ) ;
	explicit Address6( const std::string & ) ;
	Address6( const std::string & , const std::string & ) ;
	Address6( unsigned int port , int /*for overload resolution*/ ) ; // canonical loopback address
	Address6( const sockaddr * addr , socklen_t len , bool ipv6_scope_id_fixup = false ) ;

	static int domain() noexcept ;
	static unsigned short af() noexcept ;
	const sockaddr * address() const ;
	sockaddr * address() ;
	static socklen_t length() noexcept ;
	unsigned long scopeId( unsigned long default_ = 0UL ) const ;
	unsigned int port() const ;
	void setPort( unsigned int port ) ;
	bool setZone( const std::string & ipv6_zone_name_or_scope_id ) ;
	void setScopeId( unsigned long ipv6_scope_id ) ;
	static bool validString( const std::string & , std::string * = nullptr ) ;
	static bool validStrings( const std::string & , const std::string & , std::string * = nullptr ) ;
	static bool validPort( unsigned int port ) ;
	static bool validData( const sockaddr * addr , socklen_t len ) ;

	bool same( const Address6 & other , bool ipv6_compare_with_scope = false ) const ;
	bool sameHostPart( const Address6 & other , bool ipv6_compare_with_scope = false ) const ;
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

private:
	explicit Address6( std::nullptr_t ) ;
	static const char * setAddress( sockaddr_type & , const std::string & ) ;
	static const char * setHostAddress( sockaddr_type & , const std::string & ) ;
	static const char * setPort( sockaddr_type & , unsigned int ) ;
	static const char * setPort( sockaddr_type & , const std::string & ) ;
	static bool sameAddr( const ::in6_addr & a , const ::in6_addr & b ) ;
	static bool setZone( sockaddr_type & , const std::string & ) ;

private:
	sockaddr_type m_inet ;
} ;

#endif
