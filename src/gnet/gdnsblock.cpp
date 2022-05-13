//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gdnsblock.cpp
///

#include "gdef.h"
#include "gdnsblock.h"
#include "gdnsmessage.h"
#include "gresolver.h"
#include "glocal.h"
#include "gstr.h"
#include "gtest.h"
#include "gassert.h"
#include "glog.h"
#include <sstream>
#include <algorithm>
#include <cstdlib>

namespace GNet
{
	namespace DnsBlockImp /// An implementation namespace for GNet::DnsBlock.
	{
		struct Responder /// A functor that tests a GNet::DnsBlockServerResult.
		{
			bool operator()( const GNet::DnsBlockServerResult & r ) const
			{
				return r.valid() ;
			}
		} ;
		struct Denier /// A functor that tests a GNet::DnsBlockServerResult.
		{
			bool operator()( const GNet::DnsBlockServerResult & r ) const
			{
				return r.valid() && !r.addresses().empty() ;
			}
		} ;
		struct Laggard /// A functor that tests a GNet::DnsBlockServerResult.
		{
			bool operator()( const GNet::DnsBlockServerResult & r ) const
			{
				return !r.valid() ;
			}
		} ;
		struct HostList /// A list of addresses used by GNet::DnsBlock.
		{
			HostList( const std::vector<GNet::Address> & list ) : m_list(list) {}
			const std::vector<GNet::Address> & m_list ;
		} ;
		std::ostream & operator<<( std::ostream & stream , const HostList & list )
		{
			const char * sep = "" ;
			for( auto p = list.m_list.begin() ; p != list.m_list.end() ; ++p , sep = " " )
			{
				stream << sep << (*p).hostPartString() ;
			}
			return stream ;
		}
		template <typename T, typename P>
		G::StringArray server_names_if( T p , T end , P pred )
		{
			G::StringArray result ;
			for( ; p != end ; ++p )
			{
				if( pred(*p) )
					result.push_back( (*p).server() ) ;
			}
			return result ;
		}
	}
}

GNet::DnsBlock::DnsBlock( DnsBlockCallback & callback , ExceptionSink es , const std::string & config ) :
	m_callback(callback) ,
	m_es(es) ,
	m_timer(*this,&DnsBlock::onTimeout,es) ,
	m_threshold(1U) ,
	m_allow_on_timeout(true) ,
	m_dns_server(Address::defaultAddress()) ,
	m_timeout(0) ,
	m_id_base(0U)
{
	if( !config.empty() )
		configure( config ) ;
}

void GNet::DnsBlock::checkConfig( const std::string & config )
{
	try
	{
		configureImp( config , nullptr ) ;
	}
	catch( std::exception & e )
	{
		throw ConfigError( e.what() ) ;
	}
}

void GNet::DnsBlock::configure( const std::string & config )
{
	configureImp( config , this ) ;
}

void GNet::DnsBlock::configureImp( const std::string & config , DnsBlock * p )
{
	G::StringArray list = G::Str::splitIntoFields( config , ',' ) ;
	if( list.size() < 4U )
		throw BadFieldCount() ;

	Address dns_server = Address::parse( list.at(0U) , Address::NotLocal() ) ;

	// normally allow on timeout, but deny on timeout if configured value is negative
	std::size_t threshold = G::Str::toUInt( list.at(2U) ) ;
	bool allow_on_timeout = G::Str::toInt(list.at(1U)) >= 0 || threshold == 0U ;
	unsigned int timeout_ms = static_cast<unsigned int>( std::abs(G::Str::toInt(list.at(1U))) ) ;

	list.erase( list.begin() , list.begin()+3U ) ;
	if( p )
		p->configure( dns_server , threshold , allow_on_timeout , G::TimeInterval(0U,timeout_ms*1000U) , list ) ;
}

void GNet::DnsBlock::configure( const Address & dns_server , std::size_t threshold ,
	bool allow_on_timeout , G::TimeInterval timeout , const G::StringArray & servers )
{
	m_servers = servers ;
	m_threshold = threshold ;
	m_allow_on_timeout = allow_on_timeout ;
	m_dns_server = dns_server ;
	m_timeout = timeout ;
}

void GNet::DnsBlock::start( const Address & address )
{
	G_DEBUG( "GNet::DnsBlock::start: dns-server=" << m_dns_server.displayString() << " "
		<< "threshold=" << m_threshold << " timeout=" << m_timeout << " "
		<< "address=" << address.hostPartString() << " "
		<< " servers=[" << G::Str::join(",",m_servers) << "]" ) ;

	m_result.reset( m_threshold , address ) ;

	// dont block connections from local addresses
	bool is_local = address.isLoopback() || address.isUniqueLocal() || address.isLinkLocal() ;
	if( G::Test::enabled("dns-block-allow-local") ) is_local = false ;
	if( m_servers.empty() || is_local )
	{
		m_timer.startTimer( 0 ) ;
		return ;
	}

	// re-base the sequence number if necessary
	static unsigned int id_generator = 10 ;
	if( (id_generator+m_servers.size()) > 65535U )
		id_generator = 10 ;

	// create a socket to receive responses
	m_socket_ptr = std::make_unique<DatagramSocket>( m_dns_server.family() ) ;
	m_socket_ptr->addReadHandler( *this , m_es ) ;

	// send a DNS query to each configured server
	std::string prefix = queryString( address ) ; // eg. "1.0.0.127"
	unsigned int id = m_id_base = id_generator ;
	for( G::StringArray::const_iterator server_p = m_servers.begin() ; server_p != m_servers.end() ;
		++server_p , id++ , id_generator++ )
	{
		std::string server = G::Str::trimmed( *server_p , G::Str::ws() ) ;

		m_result.add( DnsBlockServerResult(server) ) ;

		const char * type = address.family() == Address::Family::ipv4 ? "A" : "AAAA" ;
		DnsMessage message = DnsMessage::request( type , std::string(prefix).append(1U,'.').append(server) , id ) ;
		G_DEBUG( "GNet::DnsBlock::start: sending [" << prefix << "."
			<< server << "] to [" << m_dns_server.displayString() << "]: id " << id ) ;

		ssize_t rc = m_socket_ptr->writeto( message.p() , message.n() , m_dns_server ) ;
		if( rc < 0 || static_cast<std::size_t>(rc) != message.n() )
			throw SendError( m_socket_ptr->reason() ) ;
	}
	m_timer.startTimer( m_timeout ) ;
}

bool GNet::DnsBlock::busy() const
{
	return m_timer.active() ;
}

void GNet::DnsBlock::readEvent( Descriptor )
{
	static std::vector<char> buffer;
	buffer.resize( 4096U ) ; // 512 in RFC-1035 4.2.1
	ssize_t rc = m_socket_ptr->read( &buffer[0] , buffer.size() ) ;
	if( rc <= 0 || static_cast<std::size_t>(rc) >= buffer.size() )
		throw BadDnsResponse() ;
	buffer.resize( static_cast<std::size_t>(rc) ) ;

	DnsMessage message( buffer ) ;
	if( !message.QR() || message.ID() < m_id_base ||
		message.ID() >= (m_id_base+m_servers.size()) || message.RCODE() > 5 )
	{
		G_WARNING( "GNet::DnsBlock::readEvent: invalid dns response: qr=" << message.QR()
			<< " rcode=" << message.RCODE() << " id=" << message.ID() ) ;
		return ;
	}

	m_result.at(message.ID()-m_id_base).set( message.addresses() ) ;

	std::size_t server_count = m_result.list().size() ;
	std::size_t responder_count = countResponders( m_result.list() ) ;
	std::size_t laggard_count = server_count - responder_count ;
	std::size_t deny_count = countDeniers( m_result.list() ) ;

	G_ASSERT( laggard_count < server_count ) ;

	G_DEBUG( "GNet::DnsBlock::readEvent: id=" << message.ID() << " rcode=" << message.RCODE()
		<< (message.ANCOUNT()?" deny ":" allow ")
		<< "got=" << responder_count << "/" << server_count << " deny-count=" << deny_count << "/" << m_threshold ) ;

	bool finished = m_timer.active() && (
		responder_count == server_count ||
		( m_threshold && deny_count >= m_threshold ) ||
		( m_threshold && (deny_count+laggard_count) < m_threshold ) ) ;

	if( finished )
	{
		m_socket_ptr.reset() ;
		m_timer.cancelTimer() ;
		m_result.type() = ( m_threshold && deny_count >= m_threshold ) ?
			DnsBlockResult::Type::Deny :
			DnsBlockResult::Type::Allow ;
		m_callback.onDnsBlockResult( m_result ) ;
	}
}

void GNet::DnsBlock::onTimeout()
{
	m_socket_ptr.reset() ;
	m_result.type() = m_result.list().empty() ?
		( m_servers.empty() ? DnsBlockResult::Type::Inactive : DnsBlockResult::Type::Local ) :
		( m_allow_on_timeout ? DnsBlockResult::Type::TimeoutAllow : DnsBlockResult::Type::TimeoutDeny ) ;
	m_callback.onDnsBlockResult( m_result ) ;
}

std::string GNet::DnsBlock::queryString( const Address & address )
{
	return address.queryString() ;
}

void GNet::DnsBlockResult::log() const
{
	using namespace DnsBlockImp ;
	if( m_type == Type::Local )
	{
		G_LOG( "GNet::DnsBlockResult::log: dnsbl: not checking local address [" << m_address.hostPartString() << "]" ) ;
	}
	else if( m_type != Type::Inactive )
	{
		for( const auto & result : m_list )
		{
			std::ostringstream ss ;
			ss << "address [" << m_address.hostPartString() << "] " ;
			if( result.valid() && result.addresses().empty() )
				ss << "allowed by [" << result.server() << "]" ;
			else if( result.valid() )
				ss << "denied by [" << result.server() << "]: " << HostList(result.addresses()) ;
			else
				ss << "not checked by [" << result.server() << "]" ;
			G_LOG( "GNet::DnsBlockResult::log: dnsbl: " << ss.str() ) ;
		}
	}
}

void GNet::DnsBlockResult::warn() const
{
	if( m_type == Type::Deny || m_type == Type::TimeoutDeny || m_type == Type::TimeoutAllow )
	{
		std::ostringstream ss ;
		ss << "client address [" << m_address.hostPartString() << "]" ;
		if( m_type == Type::Deny || m_type == Type::TimeoutDeny )
			ss << " blocked" ;
		if( m_type == Type::TimeoutDeny || m_type == Type::TimeoutAllow )
			ss << ": timeout: no answer from [" << G::Str::join("] [",laggards()) << "]" ;
		else
			ss << " by [" << G::Str::join("] [",deniers()) << "]" ;
		G_WARNING( "GNet::DnsBlockResult::log: dnsbl: " << ss.str() ) ;
	}
}

bool GNet::DnsBlockResult::allow() const
{
	return m_type == Type::Inactive || m_type == Type::Local || m_type == Type::TimeoutAllow || m_type == Type::Allow ;
}

bool GNet::DnsBlockResult::deny() const
{
	return !allow() ;
}

std::size_t GNet::DnsBlock::countResponders( const ResultList & list )
{
	using namespace DnsBlockImp ;
	return static_cast<std::size_t>(std::count_if(list.begin(),list.end(),Responder())) ;
}

std::size_t GNet::DnsBlock::countDeniers( const ResultList & list )
{
	using namespace DnsBlockImp ;
	return static_cast<std::size_t>(std::count_if(list.begin(),list.end(),Denier())) ;
}

G::StringArray GNet::DnsBlockResult::deniers() const
{
	using namespace DnsBlockImp ;
	return server_names_if( m_list.begin() , m_list.end() , Denier() ) ;
}

G::StringArray GNet::DnsBlockResult::laggards() const
{
	using namespace DnsBlockImp ;
	return server_names_if( m_list.begin() , m_list.end() , Laggard() ) ;
}

