//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#ifndef G_NET_LOCATION__H
#define G_NET_LOCATION__H

#include "gdef.h"
#include "gaddress.h"
#include "gdatetime.h"
#include "gexception.h"

namespace GNet
{
	class Location ;
	class Resolver ;
}

/// \class GNet::Location
/// A class that holds a host/service name pair and the preferred address
/// family (if any), and also the results of a name-to-address lookup, ie. the
/// remote address and canonical host name. The actual name-to-address lookup
/// is done externally, and the results are deposited into the Location object
/// with update().
///
/// One of the constructors allows an extended host/service format for
/// transparent SOCKS support: before the "@" separator is the host/service
/// pair passed verbatim to the socks server for it to resolve (see
/// socksFarHost()); after the "@" is the host/service pair for the
/// socks server itself, which we resolve and connect to (see host()).
///
/// \see GNet::Client
///
class GNet::Location
{
public:
	G_EXCEPTION( InvalidFormat , "invalid host:service format" ) ;

	Location( const std::string & host , const std::string & service , int family = AF_UNSPEC ) ;
		///< Constructor taking a host/service string.

	Location( const std::string & socks_host , const std::string & socks_service ,
		const std::string & far_host , const std::string & far_service ,
		int family = AF_UNSPEC ) ;
			///< Constructor for socks. The first pair of parameters are for
			///< the socks server, and the second pair are for the target
			///< server.

	explicit Location( const std::string & location_string , int family = AF_UNSPEC ) ;
		///< Constructor taking a formatted host/service string.
		///< The format supports the extended format for socks as
		///< "far-host:far-service@socks-host:socks-service".
		///< Throws if incorrectly formatted.

	bool socks() const ;
		///< Returns true if a socks location.

	std::string host() const ;
		///< Returns the remote host name, as passed in to the constructor.

	std::string service() const ;
		///< Returns the remote service name, as passed in to the constructor.

	void resolveTrivially() ;
		///< If host() and service() are already in address format then do a trivial
		///< update() so that the location is immediately resolved(), albeit with an
		///< empty canonical name().

	void update( const Address & address , const std::string & canonical_name ) ;
		///< Updates the address and canonical name, typically after doing
		///< a name lookup on host() and service().

	int family() const ;
		///< Returns the preferred name resolution address family, or AF_UNSPEC.

	bool dgram() const ;
		///< Returns true if the name resolution should be specifically for datagram
		///< sockets.

	bool resolved() const ;
		///< Returns true after update() has been called.

	Address address() const ;
		///< Returns the remote address.

	std::string name() const ;
		///< Returns the remote canonical name. Returns the empty string if
		///< not available.

	std::string displayString() const ;
		///< Returns a string representation for logging and debug.

	G::EpochTime updateTime() const ;
		///< Returns the time of the last update() or zero if never update()d.

	unsigned int socksFarPort() const ;
		///< Returns the port number for the socks far server.
		///< Precondition: socks()

	std::string socksFarHost() const ;
		///< Returns the port for the socks far server.
		///< Precondition: socks()

private:
	static std::string head( const std::string & ) ;
	static std::string tail( const std::string & ) ;
	static bool socksified( const std::string & , std::string & , unsigned int & ) ;
	static std::string sockless( const std::string & ) ;

private:
	std::string m_host ;
	std::string m_service ;
	bool m_address_valid ;
	Address m_address ;
	int m_family ;
	bool m_dgram ;
	std::string m_canonical_name ;
	G::EpochTime m_update_time ;
	bool m_socks ;
	std::string m_socks_far_host ;
	unsigned int m_socks_far_port ;
} ;

namespace GNet
{
	inline std::ostream & operator<<( std::ostream & stream , const Location & location )
	{
		return stream << location.displayString() ;
	}
}

#endif
