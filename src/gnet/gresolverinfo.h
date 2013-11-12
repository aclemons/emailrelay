//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gresolverinfo.h
///

#ifndef G_RESOLVER_INFO_H 
#define G_RESOLVER_INFO_H 

#include "gdef.h"
#include "gnet.h"
#include "gaddress.h"
#include "gdatetime.h"
#include "gexception.h"

/// \namespace GNet
namespace GNet
{
	class ResolverInfo ;
	class Resolver ;
}

/// \class GNet::ResolverInfo
/// A class that holds a host/service name pair and
/// optionally the results of a name-to-address lookup, ie. the
/// remote address and canonical host name. The actual name
/// lookup is done externally, and the results are deposited
/// into the ResolverInfo object with update().
///
/// This class can be used to minimise the amount of name lookup
/// performed when the same host/service name is used repeatedly.
/// See GNet::ClientPtr for an example of this.
///
class GNet::ResolverInfo 
{
public:
	G_EXCEPTION( InvalidFormat , "invalid host:service format" ) ;

	ResolverInfo( const std::string & host , const std::string & service ) ;
		///< Constructor.

	ResolverInfo( const std::string & host_and_service ) ;
		///< Implicit constructor. Throws if an invalid format.

	std::string host() const ;
		///< Returns the remote host name, as passed in to the constructor.

	std::string service() const ;
		///< Returns the remote service name, as passed in to the constructor.

	void update( const Address & address , const std::string & canonical_name ) ;
		///< Updates the address and canonical name, typically after doing
		///< a name lookup on host() and service().

	bool hasAddress() const ;
		///< Returns true after update() has been called.

	Address address() const ;
		///< Returns the remote address.

	std::string name() const ;
		///< Returns the remote canonical name. Returns the empty string if
		///< not available.

	std::string str() const ;
		///< Returns a string representation of the host and service names
		///< that can be passed to the Resolver's resolveReq() method.

	std::string displayString( bool simple = false ) const ;
		///< Returns a string representation for logging and debug.

	G::DateTime::EpochTime updateTime() const ;
		///< Returns the time of the last update(). Used in cacheing.
		///< Returns zero if not update()d.

	bool socks() const ;
		///< Returns true if using socks.

	unsigned int socksFarPort() const ;
		///< Returns the port number for the socks far server.

	std::string socksFarHost() const ;
		///< Returns the port for the socks far server.

private:
	static std::string part( const std::string & , bool ) ;
	static bool socked( const std::string & , std::string & , unsigned int & ) ;
	static std::string sockless( const std::string & ) ;

private:
	std::string m_host ;
	std::string m_service ;
	bool m_address_valid ;
	Address m_address ;
	std::string m_canonical_name ;
	G::DateTime::EpochTime m_update_time ;
	bool m_socks ;
	std::string m_socks_far_host ;
	unsigned int m_socks_far_port ;
} ;

#endif
