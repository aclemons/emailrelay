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
/// \file gdnsblock.h
///

#ifndef GNET_DNS_BLOCK__H
#define GNET_DNS_BLOCK__H

#include "gdef.h"
#include "gaddress.h"
#include "gdatetime.h"
#include "geventhandler.h"
#include "gexceptionsink.h"
#include "gexception.h"
#include "gstrings.h"
#include "gtimer.h"
#include "gsocket.h"
#include <vector>

namespace GNet
{
	class DnsBlock ;
	class DnsBlockResult ;
	class DnsBlockServerResult ;
	class DnsBlockCallback ;
}

/// \class GNet::DnsBlockServerResult
/// A result structure for one DNSBL server.
///
class GNet::DnsBlockServerResult
{
public:
	explicit DnsBlockServerResult( const std::string & server ) ;
		///< Constructor.

	void set( std::vector<Address> ) ;
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

/// \class GNet::DnsBlockResult
/// A result structure for GNet::DnsBlock.
///
class GNet::DnsBlockResult
{
public:
	g__enum(Type)
	{
		Inactive , // no configured servers
		Local , // local address not checked
		TimeoutAllow , // not all responses in the timeout period
		TimeoutDeny , // not enough responses in the timeout period
		Allow , // below threshold of deny responses
		Deny // threshold of deny responses
	} ; g__enum_end(Type)

public:
	DnsBlockResult() ;
		///< Constructor.

	void reset( size_t threshold , const Address & ) ;
		///< Initialiser.

	void add( const DnsBlockServerResult &  ) ;
		///< Appends the server result.

	DnsBlockServerResult & at( size_t ) ;
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
	typedef std::vector<DnsBlockServerResult> ResultList ;
	Type m_type ;
	size_t m_threshold ;
	Address m_address ;
	ResultList m_list ;
} ;

/// \class GNet::DnsBlock
/// Implements DNS blocklisting, as per RFC-5782.
///
class GNet::DnsBlock : private EventHandler
{
public:
	G_EXCEPTION( Error , "dnsbl error" ) ;
	typedef std::vector<DnsBlockServerResult> ResultList ;

	DnsBlock( DnsBlockCallback & , ExceptionSink , const std::string & config = std::string() ) ;
		///< Constructor. Use configure() if necessary and then start(),
		///< one time only.

	void configure( const Address & dns_server , size_t threshold , bool allow_on_timeout ,
		G::TimeInterval timeout , const G::StringArray & servers ) ;
			///< Configures the object after construction.

	void configure( const std::string & ) ;
		///< Configuration overload taking a configuration string containing
		///< comma-separated fields of: dns-server-address, timeout-ms,
		///< threshold, dnsbl-server-list.

	static void checkConfig( const std::string & ) ;
		///< Checks the configure() string, throwing on error.

	void start( const Address & ) ;
		///< Starts an asychronous check on the given address.

	bool busy() const ;
		///< Returns true after start() and before the callback.

private: // overrides
	virtual void readEvent() override ; // Override from GNet::EventHandler.

private:
	DnsBlock( const DnsBlock & ) g__eq_delete ;
	void operator=( const DnsBlock & ) g__eq_delete ;
	static void configureImp( const std::string & , DnsBlock * ) ;
	void onTimeout() ;
	static std::string queryString( const Address & ) ;
	static size_t countResponders( const ResultList & ) ;
	static size_t countDeniers( const ResultList & ) ;

private:
	DnsBlockCallback & m_callback ;
	ExceptionSink m_es ;
	Timer<DnsBlock> m_timer ;
	G::StringArray m_servers ;
	size_t m_threshold ;
	bool m_allow_on_timeout ;
	Address m_dns_server ;
	G::TimeInterval m_timeout ;
	DnsBlockResult m_result ;
	unsigned int m_id_base ;
	unique_ptr<DatagramSocket> m_socket_ptr ;
} ;

/// \class GNet::DnsBlockCallback
/// A callback interface for GNet::DnsBlock.
///
class GNet::DnsBlockCallback
{
public:
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
void GNet::DnsBlockServerResult::set( std::vector<Address> addresses )
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
	m_type(Type::Inactive) ,
	m_threshold(0U) ,
	m_address(Address::defaultAddress())
{
}

inline
void GNet::DnsBlockResult::reset( size_t threshold , const Address & address )
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
GNet::DnsBlockServerResult & GNet::DnsBlockResult::at( size_t i )
{
	return m_list.at( i ) ;
}

#endif
