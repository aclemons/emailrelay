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
/// \file gaddress.h
///

#ifndef G_ADDRESS_H
#define G_ADDRESS_H

#include "gdef.h"
#include "gnet.h"
#include "gstrings.h"
#include "gexception.h"
#include <string>

/// \namespace GNet
namespace GNet
{
	class Address ;
	class AddressStorage ;
	class AddressImp ;
	class AddressStorageImp ;
}

/// \class GNet::Address
///
/// The Address class encapsulates an IP
/// transport address. The address is stored internally 
/// as a 'sockaddr_in/sockaddr_in6' structure and exposed
/// as a 'sockaddr' structure.
///
/// \see GNet::Resolver
///
class GNet::Address 
{
public:
	G_EXCEPTION( Error , "address error" ) ;
	G_EXCEPTION( BadFamily , "unsupported address family" ) ;
	G_EXCEPTION( BadString , "invalid ip address string" ) ;
	/// An overload discriminator class for GNet::Address.
	class Localhost 
		{} ;
	/// An overload discriminator class for GNet::Address.
	class Broadcast 
		{} ;

	static int defaultDomain() ;
		///< Returns the default address 'domain', eg. PF_INET.

	Address( const Address & addr ) ;
		///< Copy constructor.

	explicit Address( const AddressStorage & ) ;
		///< Constructor taking a storage object.

	Address( const sockaddr * addr , socklen_t len ) ;
		///< Constructor using a given sockaddr.
		///<
		///< The given sockaddr address must be an Internet
		///< address (ie. 'addr->sa_family' must be AF_INET
		///< or AF_INET6, and 'len' must be the sizeof sockaddr_in
		///< or sockaddr_in6).
		///<
		///< Throws an exception if an invalid structure.

	Address( const hostent & h , unsigned int port ) ;
		///< Constructor taking the host part from the
		///< first address in the given hostent's list of
		///< alternatives. Throws an exception if
		///< an invalid port number. See also: validPort()

	Address( const hostent & h , const servent & s ) ;
		///< Constructor taking the host part from the
		///< first address in the given hostent's list of
		///< alternatives, and the port number from
		///< the given servent structure.

	explicit Address( const servent & s ) ;
		///< Constructor for a local INADDR_ANY address, taking 
		///< the port from the given 'servent' structure.

	explicit Address( unsigned int port ) ;
		///< Constructor for a local INADDR_ANY address with the
		///< given port number. Throws an exception if
		///< an invalid port number. See also: validPort()

	Address( unsigned int port , Localhost ) ;
		///< Constructor for a local INADDR_LOOPBACK address with
		///< the given port number. Throws an exception if
		///< an invalid port number. See also: validPort()

	Address( unsigned int port , Broadcast ) ;
		///< Constructor for a local INADDR_BROADCAST address with
		///< the given port number. Throws an exception if
		///< an invalid port number. See also: validPort()

	explicit Address( const std::string & display_string ) ;
		///< Constructor taking a string originally obtained
		///< from displayString(). 
		///<
		///< Throws an exception if an invalid string. 
		///<
		///< See also validString().

	Address( const std::string & ip , unsigned int port ) ;
		///< Constructor taking an ip-address and
		///< a port number.
		///<
		///< Throws an exception if an invalid string. 

	~Address() ;
		///< Destructor.

	static Address invalidAddress() ;
		///< Returns an invalid address. Should only be
		///< needed by socket and resolver classes.

	static Address broadcastAddress( unsigned int port ) ;
		///< Returns a broadcast address. Only useful
		///< for datagram sockets.

	static Address localhost( unsigned int port = 0U ) ;
		///< Returns a localhost ("loopback") address.
		///< This is a convenience function as an
		///< alternative to the Localhost constructor.

	void operator=( const Address &addr ) ;
		///< Assignment operator.

	const sockaddr * address() const ;
		///< Returns the sockaddr address. Typically used when making
		///< socket system calls. Never returns NULL.

	sockaddr * address() ;
		///< This is a non-const version of address() for compiling on
		///< systems which are not properly const-clean.

	socklen_t length() const;
		///< Returns the size of the sockaddr address.
		///< See address().

	std::string displayString( bool with_port = true , bool with_scope_id = false ) const ;
		///< Returns a string which represents the address for
		///< debugging and diagnostics purposes.

	std::string hostString() const ;
		///< Returns a string which represents the host part
		///< of the address for debugging and diagnostics purposes.

	unsigned int port() const;
		///< Returns port part of address.

	int domain() const ;
		///< Returns the address 'domain', eg. PF_INET.

	unsigned long scopeId( unsigned long default_ = 0UL ) const ;
		///< Returns the scope-id. Returns the default
		///< if scope-ids are not supported by the
		///< underlying address type.

	static bool validPort( unsigned int n ) ;
		///< Returns true if the port number is within the
		///< valid range. This can be used to avoid exceptions from
		///< the relevant constructors.

	static bool validString( const std::string & display_string , std::string * reason = NULL ) ;
		///< Returns true if the display string is valid.
		///< This can be used to avoid exceptions from
		///< the relevant constructor.

	bool sameHost( const Address &other ) const ;
		///< Returns true if the two addresses have the
		///< same host part (ie. ignoring the port).
		///< (But note that a host can have more than
		///< one host address.)

	bool isLocal( std::string & reason ) const ;
		///< Returns true if the address is definitely local.
		///< Returns false if remote or unknown, with a helpful
		///< reason string returned by reference.

	bool isLocal( std::string & reason , const Address & local_hint ) const ;
		///< An overload with a hint that should be the host's 
		///< canonical address.

	void setPort( unsigned int port ) ;
		///< Sets the port number. Throws an exception 
		///< if an invalid port number (ie. too big).

	G::Strings wildcards() const ;
		///< Returns an ordered list of wildcard strings
		///< that match this address. The fully-address-specific 
		///< string (eg. "192.168.0.1") comes first, and the 
		///< most general match-all wildcard (eg. "*.*.*.*") 
		///< comes last.

	bool operator==( const Address & ) const ;
		///< Comparison operator.

private:
	void setHost( const hostent & ) ;

private:
	AddressImp * m_imp ;
} ;

/// \class GNet::AddressStorage
/// A helper class for calling getsockname() and getpeername()
/// and hiding the definition of sockaddr_storage.
///
class GNet::AddressStorage 
{
public:
	AddressStorage() ;
		///< Default constructor.

	~AddressStorage() ;
		///< Destructor.

	sockaddr * p1() ;
		///< Returns the sockaddr pointer for getsockname()/getpeername()
		///< to write into.

	socklen_t * p2() ;
		///< Returns the length pointer for getsockname()/getpeername()
		///< to write into.

	const sockaddr * p() const ;
		///< Returns the pointer.

	socklen_t n() const ;
		///< Returns the length.

private:
	AddressStorage( const AddressStorage & ) ;
	void operator=( const AddressStorage & ) ;

private:
	AddressStorageImp * m_imp ;
} ;

#endif

