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
/// \file gaddress.h
///

#ifndef G_NET_ADDRESS_H
#define G_NET_ADDRESS_H

#include "gdef.h"
#include "gstringarray.h"
#include "gbasicaddress.h"
#include "gexception.h"
#include <string>
#include <memory>

namespace GNet
{
	class Address ;
	class Address4 ;
	class Address6 ;
	class AddressLocal ;
	class AddressStorage ;
	class AddressStorageImp ;
}

//| \class GNet::Address
/// The GNet::Address class encapsulates a TCP/UDP transport address. The
/// address is exposed as a 'sockaddr' structure for low-level socket
/// operations.
///
/// A multi-pimple pattern is used for the implementation, with implementation
/// classes including GNet::Address4 and GNet::Address6. In an IPv4-only build
/// the GNet::Address6 can be forward-declared but not defined, with all methods
/// forwarded to the GNet::Address4 sub-object.
///
/// Unix domain addresses are supported by the GNet::AddressLocal implementation
/// class. Port numbers are not expected when parsing and port numbers are not
/// included in the display string. Unix domain addresses are only allowed
/// to be absolute filesystem paths starting with '/' and with no unprintable
/// characters, or the well-defined zero-length address (for unbound sockets)
/// which is given the display string of "/". Paths with funny characters or
/// linux-specific abstract addresses (see man unix(7)) will throw.
///
/// \see GNet::Resolver
///
class GNet::Address
{
public:
	enum class Family
	{
		ipv4 ,
		ipv6 ,
		local
	} ;
	struct Domain /// Overload discriminator for Address::supports()
		{} ;

	G_EXCEPTION( Error , tx("address error") ) ;
	G_EXCEPTION( BadString , tx("invalid address") ) ;
	G_EXCEPTION_CLASS( BadFamily , tx("unsupported address family") ) ;

	static bool supports( Family ) noexcept ;
		///< Returns true if the implementation supports the given
		///< address family.

	static bool supports( int af , int dummy ) noexcept ;
		///< Returns true if the implementation supports the given
		///< address family given as AF_INET etc.

	static bool supports( const Domain & , int domain ) noexcept ;
		///< Returns true if the implementation supports the given
		///< address domain given as PF_INET etc.

	Address( const Address & ) ;
		///< Copy constructor.

	explicit Address( const AddressStorage & ) ;
		///< Constructor taking a storage object.

	Address( const sockaddr * addr , socklen_t len ) ;
		///< Constructor using a given sockaddr. Throws an exception if an
		///< invalid structure. See also: validData()

	Address( const sockaddr * addr , socklen_t len , bool fixup ) ;
		///< An overload that conditionally applies the bsd ipv6 scope-id
		///< fixup.

	Address( Family , unsigned int port ) ;
		///< Constructor for a wildcard address like INADDR_ANY with the
		///< given port number. Throws an exception if an invalid port number.
		///< Postcondition: isAny()
		/// \see validPort()

	Address( Address && ) noexcept ;
		///< Move constructor.

	~Address() ;
		///< Destructor.

	Address & operator=( const Address & ) ;
		///< Assignment operator.

	Address & operator=( Address && ) noexcept ;
		///< Move assignment operator.

	operator G::BasicAddress() const ;
		///< Returns a G::BasicAddress.

	struct NotLocal /// Overload discriminator for Address::parse()
		{} ;

	static Address parse( const std::string & display_string ) ;
		///< Factory function for any address family. Throws if
		///< an invalid string. See also validString().

	static Address parse( const std::string & display_string , NotLocal ) ;
		///< Factory function for Family::ipv4 or Family::ipv6.
		///< Throws if an invalid string. See also validString().

	static Address parse( const std::string & host_part_string , unsigned int port ) ;
		///< Factory function for Family::ipv4 or Family::ipv6.
		///< Throws if an invalid string. See also validStrings().

	static Address parse( const std::string & host_part_string , const std::string & port ) ;
		///< Factory function for Family::ipv4 or Family::ipv6.
		///< Throws if an invalid string. See also validStrings().

	static bool isFamilyLocal( const std::string & display_string ) noexcept ;
		///< Returns true if the given address display string looks
		///< will parse to Family::local and Family::local is
		///< supported. The address may still fail to parse if
		///< it is invalid.

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

	std::string displayString( bool with_scope_id = false ) const ;
		///< Returns a printable string that represents the transport
		///< address.

	std::string hostPartString() const ;
		///< Returns a printable string that represents the network
		///< address.

	std::string queryString() const ;
		///< Returns a string that can be used as a prefix for rDNS or
		///< DNSBL queries.

	unsigned int port() const;
		///< Returns port part of the address.

	static int domain( Family ) noexcept ;
		///< Returns the address 'domain' for the given family, eg. PF_INET
		///< for Family::ipv4.

	Family family() const noexcept ;
		///< Returns the address family enumeration.

	int af() const noexcept ;
		///< Returns the address family number such as AF_INET or AFINET6.

	static bool validPort( unsigned int n ) ;
		///< Returns true if the port number is within the valid range. This
		///< can be used to avoid exceptions from the relevant constructors.

	static bool validString( const std::string & display_string , std::string * reason = nullptr ) ;
		///< Returns true if the transport-address display string is valid.
		///< This can be used to avoid exceptions from the relevant constructor.

	static bool validString( const std::string & display_string , NotLocal , std::string * reason = nullptr ) ;
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

	Address & setPort( unsigned int port ) ;
		///< Sets the port number. Throws an exception if an invalid
		///< port number (ie. too big).

	bool setZone( const std::string & ) ;
		///< Sets the zone. The parameter is normally a decimal string
		///< representation of the zone-id, aka scope-id (eg. "1"), but
		///< if not numeric then it is treated as an interface name
		///< which is mapped to a zone-id by if_nametoindex(3). Returns
		///< false on error. Returns true if zones are not used by the
		///< address family.

	Address & setScopeId( unsigned long ) ;
		///< Sets the scope-id.

	unsigned long scopeId( unsigned long default_ = 0UL ) const ;
		///< Returns the scope-id. Returns the default if scope-ids are not
		///< supported by the underlying address type.

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

	bool isLinkLocal() const ;
		///< Returns true if this is a link-local address.

	bool isUniqueLocal() const ;
		///< Returns true if this is a locally administered address.

	bool isAny() const ;
		///< Returns true if this is the address family's 'any' address.

	bool isLocal( std::string & reason ) const ;
		///< Returns true if this seems to be a 'local' address, ie. an
		///< address that is likely to be more trusted. Returns an
		///< explanation by reference otherwise.

	bool isMulticast() const ;
		///< Returns true if this is a multicast address.

	bool is4() const noexcept ;
		///< Returns true if family() is ipv4.

	bool is6() const noexcept ;
		///< Returns true if family() is ipv6.

	bool same( const Address & , bool ipv6_compare_with_scope ) const ;
		///< Comparison function.

	bool operator==( const Address & ) const ;
		///< Comparison operator.

	bool operator!=( const Address & ) const ;
		///< Comparison operator.

	void swap( Address & other ) noexcept ;
		///< Swaps this with other.

private:
	Address( Family , unsigned int , int ) ; // loopback()
	Address( const std::string & display_string , bool with_local ) ; // parse(), parse(NotLocal)
	Address( const std::string & ip , const std::string & port ) ; // parse(ip,port)
	Address( const std::string & ip , unsigned int port ) ; // parse(ip,port)

private:
	std::unique_ptr<Address4> m_ipv4 ;
	std::unique_ptr<Address6> m_ipv6 ;
	std::unique_ptr<AddressLocal> m_local ;
} ;

namespace GNet
{
	inline void swap( Address & a , Address & b ) noexcept
	{
		a.swap(b) ;
	}
}

//| \class GNet::AddressStorage
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

public:
	AddressStorage( const AddressStorage & ) = delete ;
	AddressStorage( AddressStorage && ) = delete ;
	AddressStorage & operator=( const AddressStorage & ) = delete ;
	AddressStorage & operator=( AddressStorage && ) = delete ;

private:
	std::unique_ptr<AddressStorageImp> m_imp ;
} ;

#endif
