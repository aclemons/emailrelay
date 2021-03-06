//
// Copyright (C) 2001-2020 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// This file is formatted for side-by-side comparison with gaddress4.h.

#ifndef G_NET_ADDRESS6__H
#define G_NET_ADDRESS6__H

#include "gdef.h"
#include "gaddress.h"
#include <string>

namespace GNet
{
	class Address6 ;
}

/// \class GNet::Address6
/// A 'sockaddr' wrapper class for IPv6 addresses.
///
class GNet::Address6
{
public:
	using general_type = sockaddr ;
	using specific_type = sockaddr_in6 ;
	using storage_type = sockaddr_storage ;
	union union_type /// Used by GNet::Address6 to cast between sockaddr and sockaddr_in6.
		{ specific_type specific ; general_type general ; storage_type storage ; } ;

	explicit Address6( unsigned int ) ;
	explicit Address6( const std::string & ) ;
	Address6( const std::string & , const std::string & ) ;
	Address6( const std::string & , unsigned int ) ;
	Address6( unsigned int port , int /*for overload resolution*/ ) ; // canonical loopback address
	Address6( const sockaddr * addr , socklen_t len ) ;

	static int domain() ;
	static unsigned short family() ;
	const sockaddr * address() const ;
	sockaddr * address() ;
	static socklen_t length() ;
	unsigned long scopeId() const ;
	unsigned int port() const ;
	void setPort( unsigned int port ) ;
	bool setZone( const std::string & zone_name_or_scope_id ) ;
	void setScopeId( unsigned long ) ;
	static bool validString( const std::string & , std::string * = nullptr ) ;
	static bool validStrings( const std::string & , const std::string & , std::string * = nullptr ) ;
	static bool validPort( unsigned int port ) ;
	static bool validData( const sockaddr * addr , socklen_t len ) ;

	bool same( const Address6 & other , bool with_scope = false ) const ;
	bool sameHostPart( const Address6 & other , bool with_scope = false ) const ;
	bool isLoopback() const ;
	bool isLocal( std::string & ) const ;
	bool isLinkLocal() const ;
	bool isUniqueLocal() const ;
	bool isAny() const ;
	unsigned int bits() const ;
	std::string displayString() const ;
	std::string hostPartString() const ;
	std::string queryString() const ;
	G::StringArray wildcards() const ;


private:
	explicit Address6( std::nullptr_t ) ;
	static const char * setAddress( union_type & , const std::string & ) ;
	static const char * setHostAddress( union_type & , const std::string & ) ;
	static const char * setPort( union_type & , unsigned int ) ;
	static const char * setPort( union_type & , const std::string & ) ;
	static bool sameAddr( const ::in6_addr & a , const ::in6_addr & b ) ;
	static bool setZone( union_type & , const std::string & ) ;


private:
	union_type m_inet ;
} ;

#endif
