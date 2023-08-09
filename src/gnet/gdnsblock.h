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
/// \file gdnsblock.h
///

#ifndef G_NET_DNS_BLOCK_H
#define G_NET_DNS_BLOCK_H

#include "gdef.h"
#include "gaddress.h"
#include "gdatetime.h"
#include "geventhandler.h"
#include "gexceptionsink.h"
#include "gexception.h"
#include "gstringarray.h"
#include "gtimer.h"
#include "gsocket.h"
#include "gstringview.h"
#include <memory>
#include <vector>

namespace GNet
{
	class DnsBlock ;
	class DnsBlockResult ;
	class DnsBlockServerResult ;
	class DnsBlockCallback ;
}

//| \class GNet::DnsBlockServerResult
/// A result structure for one DNSBL server.
///
class GNet::DnsBlockServerResult
{
public:
	explicit DnsBlockServerResult( const std::string & server ) ;
		///< Constructor.

	void set( const std::vector<Address> & ) ;
		///< Sets the result list().

	bool valid() const ;
		///< Returns true if the list() is valid.

	std::string server() const ;
		///< Returns the server.

	const std::vector<Address> & addresses() const ;
		///< Returns the result list, which is empty if
		///< there is no block or not valid().

private:
	std::string m_server ;
	bool m_valid ;
	std::vector<Address> m_addresses ;
} ;

//| \class GNet::DnsBlockResult
/// A result structure for GNet::DnsBlock, as delivered by the
/// DnsBlockCallback interface. The principal attribute is the
/// type(), which indicates whether the connection should be
/// allowed or denied.
///
class GNet::DnsBlockResult
{
public:
	enum class Type
	{
		Inactive , // no configured servers
		Local , // local address not checked
		TimeoutAllow , // not all responses in the timeout period
		TimeoutDeny , // not enough responses in the timeout period
		Allow , // below threshold of deny responses
		Deny // threshold of deny responses
	} ;

public:
	DnsBlockResult() ;
		///< Constructor.

	void reset( std::size_t threshold , const Address & ) ;
		///< Initialiser.

	void add( const DnsBlockServerResult &  ) ;
		///< Appends the server result.

	DnsBlockServerResult & at( std::size_t ) ;
		///< Returns a reference to the given per-server result.

	Type & type() ;
		///< Returns a settable reference to the overall result type.

	void log() const ;
		///< Logs the results.

	void warn() const ;
		///< Emits warnings.

	bool allow() const ;
		///< Returns true if the type is Inactive, Local, TimeoutAllow or Allow.

	bool deny() const ;
		///< Returns true if the type is TimeoutDeny or Deny.

	const std::vector<DnsBlockServerResult> & list() const ;
		///< Returns a reference to the per-server results.

	G::StringArray deniers() const ;
		///< Returns the list of denying servers.

	G::StringArray laggards() const ;
		///< Returns the list of slow or unresponsive servers.

private:
	using ResultList = std::vector<DnsBlockServerResult> ;
	Type m_type {Type::Inactive} ;
	std::size_t m_threshold{0U} ;
	Address m_address ;
	ResultList m_list ;
} ;

//| \class GNet::DnsBlock
/// Implements DNS blocklisting, as per RFC-5782. The implementation
/// sends DNS requests for each configured block-list server
/// incorporating the IP address to be tested, for example
/// "1.0.168.192.nospam.com". All requests go to the same DNS
/// server and are cached or routed in the normal way, so the
/// block-list servers are not contacted directly.
///
class GNet::DnsBlock : private EventHandler
{
public:
	G_EXCEPTION( Error , tx("dnsbl error") ) ;
	G_EXCEPTION( ConfigError , tx("invalid dnsbl configuration") ) ;
	G_EXCEPTION( BadFieldCount , tx("not enough comma-sparated fields") ) ;
	G_EXCEPTION( SendError , tx("socket send failed") ) ;
	G_EXCEPTION( BadDnsResponse , tx("invalid dns response") ) ;
	using ResultList = std::vector<DnsBlockServerResult> ;

	DnsBlock( DnsBlockCallback & , ExceptionSink , G::string_view config = {} ) ;
		///< Constructor. Use configure() if necessary and then start(),
		///< one time only.

	void configure( const Address & dns_server , unsigned int threshold ,
		bool allow_on_timeout , G::TimeInterval timeout , const G::StringArray & servers ) ;
			///< Configures the object after construction.

	void configure( G::string_view ) ;
		///< Configuration overload taking a configuration string containing
		///< comma-separated fields of: dns-server-address, timeout-ms,
		///< threshold, dnsbl-server-list.

	static void checkConfig( const std::string & ) ;
		///< Checks the configure() string, throwing on error.

	void start( const Address & ) ;
		///< Starts an asychronous check on the given address. The result
		///< is delivered via the callback interface passed to the ctor.

	bool busy() const ;
		///< Returns true after start() and before the completion callback.

public:
	~DnsBlock() override = default ;
	DnsBlock( const DnsBlock & ) = delete ;
	DnsBlock( DnsBlock && ) = delete ;
	DnsBlock & operator=( const DnsBlock & ) = delete ;
	DnsBlock & operator=( DnsBlock && ) = delete ;

private: // overrides
	void readEvent() override ; // Override from GNet::EventHandler.

private:
	static void configureImp( G::string_view , DnsBlock * ) ;
	void onTimeout() ;
	static std::string queryString( const Address & ) ;
	static std::size_t countResponders( const ResultList & ) ;
	static std::size_t countDeniers( const ResultList & ) ;
	static Address nameServerAddress() ;
	static Address nameServerAddress( const std::string & ) ;
	static bool isDomain( G::string_view ) noexcept ;
	static bool isPositive( G::string_view ) noexcept ;
	static unsigned int ms( G::string_view ) ;

private:
	DnsBlockCallback & m_callback ;
	ExceptionSink m_es ;
	Timer<DnsBlock> m_timer ;
	G::StringArray m_servers ;
	std::size_t m_threshold ;
	bool m_allow_on_timeout ;
	Address m_dns_server ;
	G::TimeInterval m_timeout ;
	DnsBlockResult m_result ;
	unsigned int m_id_base ;
	std::unique_ptr<DatagramSocket> m_socket_ptr ;
} ;

//| \class GNet::DnsBlockCallback
/// A callback interface for GNet::DnsBlock.
///
class GNet::DnsBlockCallback
{
public:
	virtual ~DnsBlockCallback() = default ;
		///< Destructor.

	virtual void onDnsBlockResult( const DnsBlockResult & ) = 0 ;
		///< Called with the results from DnsBlock::start().
} ;

// ==

inline
GNet::DnsBlockServerResult::DnsBlockServerResult( const std::string & server ) :
	m_server(server) ,
	m_valid(false)
{
}

inline
void GNet::DnsBlockServerResult::set( const std::vector<Address> & addresses )
{
	m_valid = true ;
	m_addresses = addresses ;
}

inline
bool GNet::DnsBlockServerResult::valid() const
{
	return m_valid ;
}

inline
std::string GNet::DnsBlockServerResult::server() const
{
	return m_server ;
}

inline
const std::vector<GNet::Address> & GNet::DnsBlockServerResult::addresses() const
{
	return m_addresses ;
}

inline
GNet::DnsBlockResult::DnsBlockResult() :
	m_address(Address::defaultAddress())
{
}

inline
void GNet::DnsBlockResult::reset( std::size_t threshold , const Address & address )
{
	m_threshold = threshold ;
	m_address = address ;
}

inline
GNet::DnsBlockResult::Type & GNet::DnsBlockResult::type()
{
	return m_type ;
}

inline
const std::vector<GNet::DnsBlockServerResult> & GNet::DnsBlockResult::list() const
{
	return m_list ;
}

inline
void GNet::DnsBlockResult::add( const DnsBlockServerResult & server_result )
{
	m_list.push_back( server_result ) ;
}

inline
GNet::DnsBlockServerResult & GNet::DnsBlockResult::at( std::size_t i )
{
	return m_list.at( i ) ;
}

#endif
