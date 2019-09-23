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
/// \file gaddress.h
///

#ifndef G_NET_ADDRESS__H
#define G_NET_ADDRESS__H

#include "gdef.h"
#include "gstrings.h"
#include "gexception.h"
#include <string>

namespace GNet
{
	class Address ;
	class Address4 ;
	class Address6 ;
	class AddressStorage ;
	class AddressStorageImp ;
	int inet_pton_imp( int f , const char * p , void * result ) ;
	const char * inet_ntop_imp( int f , void * ap , char * buffer , size_t n ) ;
}

/// \class GNet::Address
/// The GNet::Address class encapsulates a TCP/UDP transport address. The
/// address is exposed as a 'sockaddr' structure for low-level socket
/// operations.
///
/// A double pimple pattern is used for the implementation; this class
/// instantiates either a GNet::Address4 implementation sub-object or a
/// GNet::Address6 sub-object depending on the address family, and forwards
/// its method calls directly to the one instantiated sub-object. In an
/// IPv4-only build the GNet::Address6 is forward-declared but not defined
/// and all methods are forwarded to the GNet::Address4 sub-object.
///
/// \see GNet::Resolver
///
class GNet::Address
{
public:
	g__enum(Family)
	{
		ipv4 ,
		ipv6
	} ; g__enum_end(Family) ;

	G_EXCEPTION( Error , "address error" ) ;
	G_EXCEPTION( BadString , "invalid address" ) ;
	G_EXCEPTION_CLASS( BadFamily , "unsupported address family" ) ;

	static bool supports( Family ) ;
		///< Returns true if the implementation supports the given address family,
		///< either ipv4 or ipv6.

	Address( const Address & ) ;
		///< Copy constructor.

	explicit Address( const AddressStorage & ) ;
		///< Constructor taking a storage object.

	Address( const sockaddr * addr , socklen_t len ) ;
		///< Constructor using a given sockaddr. Throws an exception if an
		///< invalid structure. See also: validData()

	Address( Family , unsigned int port ) ;
		///< Constructor for a wildcard INADDR_ANY address with the given port
		///< number. Throws an exception if an invalid port number.
		/// \see validPort()

	explicit Address( const std::string & display_string ) ;
		///< Constructor taking a string originally obtained from displayString().
		///< Throws an exception if an invalid string. See also validString().

	Address( const std::string & ip , unsigned int port ) ;
		///< Constructor taking an ip-address and a port number. Throws an
		///< exception if an invalid string.

	#if GCONFIG_HAVE_CXX_MOVE
	Address( Address && ) g__noexcept ;
		// Move constructor.
	#endif

	~Address() ;
		///< Destructor.

	static Address defaultAddress() ;
		///< Returns a default address, being the IPv4 wildcard address
		///< with a zero port number.

	static Address loopback( Family , unsigned int port = 0U ) ;
		///< Returns a loopback address.

	const sockaddr * address() const ;
		///< Returns the sockaddr address. Typically used when making socket
		///< system calls. Never returns nullptr.

	sockaddr * address() ;
		///< This is a non-const version of address() for compiling on systems
		///< which are not properly const-clean.

	socklen_t length() const;
		///< Returns the size of the sockaddr address. See address().

	std::string displayString() const ;
		///< Returns a string which represents the transport address for
		///< debugging and diagnostics purposes.

	std::string hostPartString() const ;
		///< Returns a string which represents the network address for
		///< debugging and diagnostics purposes.

	std::string queryString() const ;
		///< Returns a string that can be used as a prefix for rDNS or
		///< DNSBL queries.

	unsigned int port() const;
		///< Returns port part of the address.

	int domain() const ;
		///< Returns the address 'domain', eg. PF_INET.

	Family family() const ;
		///< Returns the address family enumeration.

	int af() const ;
		///< Returns the address family number, AF_INET or AFINET6.

	unsigned long scopeId( unsigned long default_ = 0UL ) const ;
		///< Returns the scope-id. Returns the default if scope-ids are not
		///< supported by the underlying address type.

	static bool validPort( unsigned int n ) ;
		///< Returns true if the port number is within the valid range. This
		///< can be used to avoid exceptions from the relevant constructors.

	static bool validString( const std::string & display_string , std::string * reason = nullptr ) ;
		///< Returns true if the transport-address display string is valid.
		///< This can be used to avoid exceptions from the relevant constructor.

	static bool validStrings( const std::string & ip , const std::string & port_string , std::string * reason = nullptr ) ;
		///< Returns true if the combined network-address string and port string
		///< is valid. This can be used to avoid exceptions from the relevant
		///< constructor.

	static bool validData( const sockaddr * , socklen_t len ) ;
		///< Returns true if the sockaddr data is valid. This can be used
		///< to avoid exceptions from the relevant constructor.

	bool sameHostPart( const Address & other ) const ;
		///< Returns true if the two addresses have the same host part
		///< (ie. the network address, ignoring the port number).

	void setPort( unsigned int port ) ;
		///< Sets the port number. Throws an exception if an invalid
		///< port number (ie. too big).

	G::StringArray wildcards() const ;
		///< Returns an ordered list of wildcard strings that match this
		///< address. The fully-address-specific string (eg. "192.168.0.1")
		///< comes first, and the most general match-all wildcard like
		///< "*.*.*.*" or "128.0.0.0/1" comes last.

	unsigned int bits() const ;
		///< Returns the number of leading bits set, relevant only
		///< to netmask addresses.

	bool isLoopback() const ;
		///< Returns true if this is a loopback address.

	bool isLocal( std::string & reason ) const ;
		///< Returns true if this seems to be a 'local' address, ie. an
		///< address that is inherently more trusted. Returns an
		///< explanation by reference otherwise.

	bool isPrivate() const ;
		///< Returns true if this seems to be a 'private' address,
		///< as in RFC-1918 for IPv4.

	bool operator==( const Address & ) const ;
		///< Comparison operator.

	bool operator!=( const Address & ) const ;
		///< Comparison operator.

	void swap( Address & other ) g__noexcept ;
		///< Swaps this with other.

	Address & operator=( const Address & addr ) ;
		///< Assignment operator.

	#if GCONFIG_HAVE_CXX_MOVE
	Address & operator=( Address && ) g__noexcept ;
		// Move assigmnemt operator.
	#endif

private:
	Address( Family , unsigned int , int ) ; // loopback()

private:
	unique_ptr<Address4> m_4imp ;
	unique_ptr<Address6> m_6imp ;
} ;

namespace GNet
{
	inline void swap( Address & a , Address & b ) g__noexcept
	{
		a.swap(b) ;
	}
}

/// \class GNet::AddressStorage
/// A helper class for calling accept(), getsockname() and getpeername()
/// and hiding the definition of sockaddr_storage.
///
class GNet::AddressStorage
{
public:
	AddressStorage() ;
		///< Default constructor, with n() reflecting the size of the
		///< largest supported address type.

	~AddressStorage() ;
		///< Destructor.

	sockaddr * p1() ;
		///< Returns the sockaddr pointer for accept()/getsockname()/getpeername()
		///< to write into.

	socklen_t * p2() ;
		///< Returns the length pointer for accept()/getsockname()/getpeername()
		///< to write into.

	const sockaddr * p() const ;
		///< Returns the pointer, typically set via p1().

	socklen_t n() const ;
		///< Returns the length, typically modified via p2().

private:
	AddressStorage( const AddressStorage & ) g__eq_delete ;
	void operator=( const AddressStorage & ) g__eq_delete ;

private:
	unique_ptr<AddressStorageImp> m_imp ;
} ;

#endif
