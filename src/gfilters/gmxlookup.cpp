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
/// \file gmxlookup.cpp
///

#include "gdef.h"
#include "gmxlookup.h"
#include "gdnsmessage.h"
#include "gnameservers.h"
#include "gstr.h"
#include "gstringtoken.h"
#include "goptional.h"
#include "gassert.h"
#include "glog.h"
#include <fstream>
#include <vector>
#include <utility>

namespace GFilters
{
	namespace MxLookupImp
	{
		enum class Result { error , fatal , mx , cname , ip } ;
		std::pair<Result,std::string> parse( const GNet::DnsMessage & , const GNet::Address & , unsigned int ) ;
	}
}

bool GFilters::MxLookup::enabled()
{
	return true ;
}

#ifndef G_LIB_SMALL
GFilters::MxLookup::MxLookup( GNet::ExceptionSink es , Config config ) :
	MxLookup(es,config,GNet::nameservers(53U))
{
}
#endif

GFilters::MxLookup::MxLookup( GNet::ExceptionSink es , Config config ,
	const std::vector<GNet::Address> & nameservers ) :
		m_es(es) ,
		m_config(config) ,
		m_message_id(GStore::MessageId::none()) ,
		m_ns_index(0U) ,
		m_ns_failures(0U) ,
		m_nameservers(nameservers) ,
		m_timer(*this,&MxLookup::onTimeout,es)
{
	if( m_nameservers.empty() )
	{
		m_nameservers.push_back( GNet::Address::loopback( GNet::Address::Family::ipv4 , 53U ) ) ;
		m_nameservers.push_back( GNet::Address::loopback( GNet::Address::Family::ipv6 , 53U ) ) ;
	}

	bool ipv4 = std::find_if( m_nameservers.begin() , m_nameservers.end() ,
		[](const GNet::Address &a_){return a_.is4();} ) != m_nameservers.end() ;
	if( ipv4 )
	{
		m_socket4 = std::make_unique<GNet::DatagramSocket>( GNet::Address::Family::ipv4 , 0 , GNet::DatagramSocket::Config() ) ;
		m_socket4->addReadHandler( *this , m_es ) ;
		G_DEBUG( "GFilters::MxLookup::ctor: ipv4 udp socket: " << (*m_socket4).getLocalAddress().displayString() ) ;
	}

	bool ipv6 = std::find_if( m_nameservers.begin() , m_nameservers.end() ,
		[](const GNet::Address &a_){return a_.is6();} ) != m_nameservers.end() ;
	if( ipv6 )
	{
		m_socket6 = std::make_unique<GNet::DatagramSocket>( GNet::Address::Family::ipv4 , 0 , GNet::DatagramSocket::Config() ) ;
		m_socket6->addReadHandler( *this , m_es ) ;
		G_DEBUG( "GFilters::MxLookup::ctor: ipv6 udp socket: " << (*m_socket6).getLocalAddress().displayString() ) ;
	}
}

void GFilters::MxLookup::start( const GStore::MessageId & message_id , const std::string & forward_to , unsigned int port )
{
	if( !m_socket4 && !m_socket6 )
	{
		fail( "no nameserver" ) ;
	}
	else if( forward_to.empty() )
	{
		fail( "invalid empty doman" ) ;
	}
	else
	{
		m_message_id = message_id ;
		m_port = port ? port : 25U ;
		m_ns_index = 0U ;
		m_ns_failures = 0U ;
		m_question = forward_to ;
		sendMxQuestion( m_ns_index , m_question ) ;
		startTimer() ;
	}
}

void GFilters::MxLookup::readEvent()
{
	G_DEBUG( "GFilters::MxLookup::readEvent" ) ;
	std::vector<char> buffer( 4096U ) ; // 512 in RFC-1035 4.2.1
	ssize_t nread = m_socket4->read( &buffer[0] , buffer.size() ) ;
	if( nread > 0 )
		process( &buffer[0] , static_cast<std::size_t>(nread) ) ;
	else if( (nread=m_socket6->read( &buffer[0] , buffer.size() )) > 0 )
		process( &buffer[0] , static_cast<std::size_t>(nread) ) ;
	else
		fail( "dns socket error" ) ;
}

void GFilters::MxLookup::process( const char * p , std::size_t n )
{
	G_DEBUG( "GFilters::MxLookup::process: dns message size " << n ) ;
	using namespace MxLookupImp ;
	GNet::DnsMessage response( p , n ) ;
	if( response.valid() && response.QR() && response.ID() && response.ID() < (m_nameservers.size()+1U) )
	{
		std::size_t ns_index = static_cast<std::size_t>(response.ID()) - 1U ;
		auto pair = parse( response , m_nameservers.at(ns_index) , m_port ) ;
		if( pair.first == Result::error && (m_ns_failures+1U) < m_nameservers.size() )
			disable( ns_index , pair.second ) ;
		else if( pair.first == Result::error || pair.first == Result::fatal )
			fail( pair.second ) ;
		else if( pair.first == Result::mx )
			sendHostQuestion( ns_index , pair.second ) ;
		else if( pair.first == Result::cname )
			sendMxQuestion( ns_index , pair.second ) ;
		else if( pair.first == Result::ip )
			succeed( pair.second ) ;
	}
}

void GFilters::MxLookup::disable( std::size_t ns_index , const std::string & reason )
{
	G_LOG_MORE( "GFilters::MxLookup::disable: mx: nameserver "
		<< "[" << m_nameservers.at(ns_index).displayString() << "] disabled (" << reason << ")" ) ;
	m_nameservers.at(ns_index) = GNet::Address::defaultAddress() ;
	m_ns_failures++ ;
}

std::pair<GFilters::MxLookupImp::Result,std::string> GFilters::MxLookupImp::parse( const GNet::DnsMessage & response ,
	const GNet::Address & ns_address , unsigned int port )
{
	G_ASSERT( port != 0U ) ;
	std::string from = " from " + ns_address.hostPartString() ;
	if( response.RCODE() == 3 && response.AA() )
	{
		return { Result::fatal , "rcode nxdomain" + from } ;
	}
	if( response.RCODE() != 0 )
	{
		return { Result::error , "rcode " + G::Str::fromUInt(response.RCODE()) + from } ;
	}
	else if( response.ANCOUNT() == 0U )
	{
		return { Result::error , "no answer section" + from } ;
	}
	else
	{
		GNet::Address address = GNet::Address::defaultAddress() ;
		std::string cname_result ;
		std::string mx_result ;
		unsigned int mx_pr = 0U ;
		G::optional<unsigned int> mx_prmin ;

		unsigned int offset = response.QDCOUNT() ;
		for( unsigned int i = 0 ; i < response.ANCOUNT() ; i++ )
		{
			auto rr = response.rr( i + offset ) ;
			if( rr.isa("MX") )
			{
				unsigned int pr = rr.rdata().word( 0U ) ;
				std::string name = rr.rdata().dname( 2U ) ;
				G_LOG_MORE( "GFilters::MxLookupImp::parse: mx: answer: "
					<< "mx [" << name << "](priority " << pr << ")" << from ) ;
				if( !name.empty() && ( mx_result.empty() || pr < mx_pr ) )
				{
					mx_pr = pr ;
					mx_result = name ;
				}
			}
			else if( rr.isa("CNAME") ) // RFC-974 p4
			{
				std::string cname = rr.rdata().dname( 0U ) ;
				G_LOG_MORE( "GFilters::MxLookupImp::parse: mx: answer: "
					<< "cname [" << cname << "]" << from ) ;
				cname_result = cname ;
			}
			else
			{
				GNet::Address a = rr.address( port , std::nothrow ) ;
				G_LOG_MORE_IF( a.port() , "GFilters::MxLookupImp::parse: mx: answer: "
					<< "host-ip [" << a.hostPartString() << "]" << from ) ;
				if( address.port() == 0U && a.port() != 0U )
        			address = a ;
			}
		}

		if( !cname_result.empty() )
			return { Result::cname , cname_result } ;
		else if( address.port() != 0U )
			return { Result::ip , address.displayString() } ;
		else if( !mx_result.empty() )
			return { Result::mx , mx_result } ;
		else
			return { Result::error , "invalid response" + from } ;
	}
}

void GFilters::MxLookup::sendMxQuestion( std::size_t ns_index , const std::string & mx_question )
{
	if( m_nameservers[ns_index] != GNet::Address::defaultAddress() )
	{
		G_LOG_MORE( "GFilters::MxLookup::sendMxQuestion: mx: question: mx [" << mx_question << "] "
			<< "to " << m_nameservers[ns_index].hostPartString()
			<< (m_nameservers[ns_index].port()==53U?"":(" port "+G::Str::fromUInt(m_nameservers[ns_index].port()))) ) ;
		unsigned int id = static_cast<unsigned int>(ns_index) + 1U ;
		GNet::DnsMessageRequest request( "MX" , mx_question , id ) ;
		socket(ns_index).writeto( request.p() , request.n() , m_nameservers[ns_index] ) ;
	}
}

void GFilters::MxLookup::sendHostQuestion( std::size_t ns_index , const std::string & host_question )
{
	if( m_nameservers[ns_index] != GNet::Address::defaultAddress() )
	{
		G_LOG_MORE( "GFilters::MxLookup::sendHostQuestion: mx: question: host-ip [" << host_question << "] "
			<< "to " << m_nameservers[ns_index].hostPartString() ) ;
		unsigned int id = static_cast<unsigned int>(ns_index) + 1U ;
		GNet::DnsMessageRequest request( "A" , host_question , id ) ;
		socket(ns_index).writeto( request.p() , request.n() , m_nameservers[ns_index] ) ;
	}
}

void GFilters::MxLookup::cancel()
{
	dropReadHandlers() ;
	m_timer.cancelTimer() ;
}

void GFilters::MxLookup::dropReadHandlers()
{
	if( m_socket4 )
		m_socket4->dropReadHandler() ;
	if( m_socket6 )
		m_socket6->dropReadHandler() ;
}

void GFilters::MxLookup::fail( const std::string & error )
{
	m_error = "mx: " + error ;
	dropReadHandlers() ;
	m_timer.startTimer( 0U ) ;
}

void GFilters::MxLookup::onTimeout()
{
	if( !m_error.empty() )
	{
		cancel() ;
		m_done_signal.emit( m_message_id , "" , m_error ) ;
	}
	else
	{
		m_ns_index++ ;
		if( m_ns_index == m_nameservers.size() )
			m_ns_index = 0U ;

		sendMxQuestion( m_ns_index , m_question ) ;
		startTimer() ;
	}
}

void GFilters::MxLookup::startTimer()
{
	bool last = (m_ns_index+1U) == m_nameservers.size() ;
	G::TimeInterval timeout = last ? m_config.restart_timeout : m_config.ns_timeout ;
	m_timer.startTimer( timeout ) ;
}

void GFilters::MxLookup::succeed( const std::string & result )
{
	cancel() ;
	m_done_signal.emit( m_message_id , result , "" ) ;
}

GNet::DatagramSocket & GFilters::MxLookup::socket( std::size_t ns_index )
{
	return m_nameservers.at(ns_index).is4() ? *m_socket4 : *m_socket6 ;
}

G::Slot::Signal<GStore::MessageId,std::string,std::string> & GFilters::MxLookup::doneSignal() noexcept
{
	return m_done_signal ;
}

GFilters::MxLookup::Config::Config()
= default ;

