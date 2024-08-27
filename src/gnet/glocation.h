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
/// \file glocation.h
///

#ifndef G_NET_LOCATION_H
#define G_NET_LOCATION_H

#include "gdef.h"
#include "gaddress.h"
#include "gdatetime.h"
#include "gexception.h"
#include <new>

namespace GNet
{
	class Location ;
	class Resolver ;
}

//| \class GNet::Location
/// A class that represents the remote target for out-going client connections.
/// It holds a host/service name pair and the preferred address family (if any)
/// and also the results of a DNS lookup for the remote address.
///
/// The actual DNS lookup of host() and service() should be done externally,
/// with the results deposited into the Location object with update().
///
/// An extended format is supported for transparent SOCKS connection: before
/// the "@" separator is the host/port pair passed verbatim to the socks
/// server for it to resolve; after the "@" is the host/service pair for
/// the socks server itself, which should be resolved as normal.
///
/// URL-style square brackets can be used for IPv6 address, eg( "[::1]:1").
///
/// Local-domain socket addresses are supported, but obviously DNS lookups
/// of host() and service() will never work, update() will reject them,
/// and the socks code will not allow them as the 'far' address.
///
/// Synopsis:
/// \code
/// Location location( remote_server ) ;
/// location.resolveTrivially() ;
/// if( !location.resolved() )
/// {
///   Resolver resolver( location.host() , location.service() , location.family() ) ;
///   if( resolver.resolve() )
///     location.update( resolver.address() ) ;
/// }
/// \endcode
///
/// \see GNet::Client, GNet::Resolver
///
class GNet::Location
{
public:
	G_EXCEPTION( InvalidFormat , tx("invalid host:service format") )
	G_EXCEPTION( InvalidFamily , tx("invalid address family") )

	explicit Location( const std::string & spec , int family = AF_UNSPEC ) ;
		///< Constructor taking a formatted "host:service" string.
		///< The location specification allows an extended format for
		///< socks, as "far-host:far-port@socks-host:socks-service".
		///< Throws if incorrectly formatted. The optional 'family'
		///< parameter is made available to the resolver via
		///< the family() method.

	static Location nosocks( const std::string & spec , int family = AF_UNSPEC ) ;
		///< Factory function for a remote location but not allowing
		///< the extended syntax for socks.

	static Location socks( const std::string & socks_server , const std::string & far_server ) ;
		///< Factory function for a remote location explicitly
		///< accessed via socks.

	std::string host() const ;
		///< Returns the remote host name derived from the constructor
		///< parameter.

	std::string service() const ;
		///< Returns the remote service name derived from the constructor
		///< parameter.

	int family() const ;
		///< Returns the preferred name resolution address family as
		///< passed to the constructor.

	bool socks() const ;
		///< Returns true if a socks location.

	bool resolveTrivially() ;
		///< If host() and service() are already in address format then do a trivial
		///< update() so that the location is immediately resolved(). Does
		///< nothing if already resolved(). Returns resolved().

	void update( const Address & address ) ;
		///< Updates the address, typically after doing a name lookup on
		///< host() and service(). Throws if an invalid address family.

	bool update( const Address & address , std::nothrow_t ) ;
		///< Updates the address, typically after doing a name lookup on
		///< host() and service(). Returns false if an invalid address
		///< family.

	bool resolved() const ;
		///< Returns true after update() has been called or resolveTrivially()
		///< succeeded.

	Address address() const ;
		///< Returns the remote address.

	std::string displayString() const ;
		///< Returns a string representation for logging and debug.

	G::SystemTime updateTime() const ;
		///< Returns the time of the last update() or zero if never update()d.

	unsigned int socksFarPort() const ;
		///< Returns the port number for the socks far server.
		///< Precondition: socks()

	std::string socksFarHost() const ;
		///< Returns the port for the socks far server.
		///< Precondition: socks()

private:
	Location( const std::string & socks , const std::string & far_ , int family ) ; // socks()
	Location( const std::string & spec , int family , int ) ; // nosocks()
	static std::string head( const std::string & ) ;
	static std::string tail( const std::string & ) ;
	static bool socksified( const std::string & , std::string & , std::string & ) ;
	static std::string sockless( const std::string & ) ;

private:
	std::string m_host ;
	std::string m_service ;
	bool m_address_valid ;
	Address m_address ;
	int m_family ;
	G::SystemTime m_update_time ;
	bool m_using_socks ;
	std::string m_socks_far_host ;
	std::string m_socks_far_port ;
} ;

namespace GNet
{
	inline std::ostream & operator<<( std::ostream & stream , const Location & location )
	{
		return stream << location.displayString() ;
	}
}

#endif
