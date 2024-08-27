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
/// \file emailrelay_test_dnsserver.cpp
///
// A dummy DNS server for testing purposes.
//
// usage: emailrelay_test_dnsserver [--port <port>] [--address <ipv4-address>]
//
// Default mappings:
//    MX(*zero*) -> A(smtp.*zero*) -> 0.0.0.0
//    MX(*localhost*) -> A(smtp.*localhost*) -> 127.0.0.1
//    MX(*one*) -> A(smtp.*one*) -> 127.0.1.1
//    MX(*two*) -> A(smtp.*two*) -> 127.0.2.1
//    MX(*three*) -> A(smtp.*three*) -> 127.0.3.1
//    MX(*) -> A(smtp.*) -> 127.0.0.1
//
// Testing:
//    $ dig @127.0.0.1 -p 10053 +short -t MX -q foo.zero.net
//    $ nslookup -type=MX -port=10053 foo.zero.net 127.0.0.1
//

#include "gdef.h"
#include "gfile.h"
#include "gdnsmessage.h"
#include "geventloop.h"
#include "gtimerlist.h"
#include "gstr.h"
#include "garg.h"
#include "ggetopt.h"
#include "goptionsusage.h"
#include "gsocket.h"
#include "glogoutput.h"
#include "gexception.h"
#include "glog.h"
#include <array>
#include <exception>
#include <stdexcept>
#include <iostream>

namespace GNet
{
	class DnsMessageBuilder
	{
		public:
			static DnsMessage response( DnsMessage , const Address & , unsigned int ttl ) ; // TYPE "A"
			static DnsMessage response( DnsMessage , const std::string & domain_name ) ; // TYPE "MX"
			static void addAddress( DnsMessage & , const Address & , unsigned int ttl ) ;
			static void addMx( DnsMessage & , const std::string & domain_name ) ;
			static void addDomainName( DnsMessage & , const std::string & domain_name ) ;
			static unsigned int domainNameSize( const std::string & domain_name ) ;
	} ;
}

class Server : private GNet::EventHandler
{
public:
	struct Config
	{
		unsigned int port {53U} ;
		std::string answer_a ;
		std::string answer_mx ;
		GNet::Address::Family family {GNet::Address::Family::ipv4} ;
		GNet::DatagramSocket::Config socket_config ;
		Config & set_port( unsigned int n ) { port = n ; return *this ; }
		Config & set_answer_a( const std::string & s ) { answer_a = s ; return *this ; }
		Config & set_answer_mx( const std::string & s ) { answer_mx = s ; return *this ; }
	} ;

public:
	Server( GNet::EventState es , Config ) ;
	~Server() override ;

private: // overrides
	void readEvent() override ;

private:
	void sendResponse( const GNet::Address & , GNet::DnsMessage ) ;

private:
	GNet::EventState m_es ;
	Config m_config ;
	GNet::DatagramSocket m_socket ;
} ;

Server::Server( GNet::EventState es , Config config ) :
	m_es(es) ,
	m_config(config) ,
	m_socket(m_config.family,0,m_config.socket_config)
{
	m_socket.bind( GNet::Address::loopback(m_config.family,m_config.port) ) ;
	G_LOG_S( "Server::ctor: listening on " << m_socket.getLocalAddress().displayString() ) ;
	m_socket.addReadHandler( *this , m_es ) ;
}

Server::~Server()
= default ;

void Server::readEvent()
{
	GNet::Address address = GNet::Address::defaultAddress() ;
	std::array<unsigned char,1000> buffer {} ;
	char * buffer_p = reinterpret_cast<char*>(&buffer[0]) ;
	auto nread = m_socket.readfrom( buffer_p , buffer.size() , address ) ;
	G_LOG( "Server::readEvent: request: received " << nread << " bytes from " << address.displayString() ) ;
	if( nread > 0 )
	{
		std::size_t size = static_cast<std::size_t>(nread) ;
		GNet::DnsMessage m ( buffer_p , size ) ;
		G_LOG( "Server::readEvent: request: " << nread << " bytes: "
			<< m.QDCOUNT() << " question" << (m.QDCOUNT()==1?"":"s") << ": "
			<< "TYPE="  << GNet::DnsMessageRecordType::name(m.question(0).qtype()) << " "
			<< "QNAME=" << m.question(0).qname() ) ;
		sendResponse( address , GNet::DnsMessage(buffer_p,size) ) ;
	}
}

void Server::sendResponse( const GNet::Address & address , GNet::DnsMessage m )
{
	std::string log_message = "rejection RCODE=4" ;
	GNet::DnsMessage response = GNet::DnsMessage::rejection( m , 4 ) ; // RCODE
	if( m.valid() && m.QDCOUNT() == 1U )
	{
		if( m.question(0U).qtype() == 15U ) // QTYPE "MX"
		{
			// allow substitution variable '@' for qname
			std::string answer = m_config.answer_mx ;
			G::Str::replace( answer , "@" , m.question(0U).qname() ) ;
			log_message = "answer TYPE=MX EXCHANGE=" + answer ;
			response = GNet::DnsMessageBuilder::response( m , answer ) ;
		}
		else if( m.question(0U).qtype() == 1U ) // QTYPE "A"
		{
			std::string qname = m.question(0U).qname() ;
			std::string answer = m_config.answer_a + ":0" ;
			const char * replacement = "0" ;
			if( qname.find("one") != std::string::npos ) replacement = "1" ;
			if( qname.find("two") != std::string::npos ) replacement = "2" ;
			if( qname.find("three") != std::string::npos ) replacement = "3" ;
			if( qname.find("four") != std::string::npos ) replacement = "4" ;
			if( qname.find("five") != std::string::npos ) replacement = "5" ;
			G::Str::replace( answer , "@" , replacement ) ;
			if( qname.find("zero") != std::string::npos ) answer = "0.0.0.0:0" ;
			if( qname.find("localhost") != std::string::npos ) answer = "127.0.0.1:0" ;

			GNet::Address a = GNet::Address::parse( answer ) ;
			log_message = "answer TYPE=A NAME=" + a.displayString() ;
			response = GNet::DnsMessageBuilder::response( m , a , 10U ) ;
		}
	}
	G_LOG( "Server::readEvent: response: " << response.n() << " bytes: " << log_message ) ;
	auto nsent = m_socket.writeto( response.p() , response.n() , address ) ;
	G_LOG( "Server::readEvent: response: sent " << nsent << " bytes to " << address.displayString() ) ;
}

// ==

GNet::DnsMessage GNet::DnsMessageBuilder::response( DnsMessage m , const Address & address , unsigned int ttl )
{
	m.convertToResponse( 0U , true ) ;
	addAddress( m , address , ttl ) ;
	return m ;
}

GNet::DnsMessage GNet::DnsMessageBuilder::response( DnsMessage m , const std::string & domain_name )
{
	m.convertToResponse( 0U , true ) ;
	addMx( m , domain_name ) ;
	return m ;
}

void GNet::DnsMessageBuilder::addMx( DnsMessage & m , const std::string & domain_name )
{
	unsigned int ttl = 10 ;
	++(m.m_buffer.at(7U)) ; // ANCOUNT
	m.addWord( 0xC00CU ) ; // NAME -- pointer into first question
	m.addWord( DnsMessageRecordType::value("MX") ) ; // TYPE "MX"
	m.addWord( 0x01U ) ; // CLASS "IN"
	m.addWord( (ttl >> 16) & 0xFFFFU ) ; // TTL
	m.addWord( (ttl >> 0) & 0xFFFFU ) ; // TTL
	m.addWord( domainNameSize(domain_name) + 2U ) ; // RDLENGTH
	m.addWord( 1U ) ; // PREFERENCE
	addDomainName( m , domain_name ) ; // EXCHANGE
}

unsigned int GNet::DnsMessageBuilder::domainNameSize( const std::string & domain_name )
{
	unsigned int n = 0U ;
	G::StringArray parts = G::Str::splitIntoFields( domain_name , '.' ) ;
	for( const auto & label : parts )
		n += ( 1U + static_cast<unsigned>(label.size()) ) ;
	n += 1U ;
	return n ;
}

void GNet::DnsMessageBuilder::addDomainName( DnsMessage & m , const std::string & domain_name )
{
	G::StringArray parts = G::Str::splitIntoFields( domain_name , '.' ) ;
	for( const auto & label : parts )
	{
		m.addByte( label.size() & 0x3FU ) ;
		for( char c : label )
			m.addByte( static_cast<unsigned char>(c) ) ;
	}
	m.addByte( 0U ) ; // root
}

void GNet::DnsMessageBuilder::addAddress( DnsMessage & m , const Address & address , unsigned int ttl )
{
	if( !address.is4() )
		throw G::Exception( "invalid address family" ) ;

	G::StringArray address_part = G::Str::splitIntoFields( address.hostPartString() , '.' ) ;
	if( address_part.size() != 4U )
		throw G::Exception( "invalid address" ) ;

	++(m.m_buffer.at(7U)) ; // ANCOUNT
	m.addWord( 0xC00CU ) ; // NAME -- pointer into first question
	m.addWord( DnsMessageRecordType::value("A") ) ; // TYPE "A"
	m.addWord( 0x01U ) ; // CLASS "IN"
	m.addWord( (ttl >> 16) & 0xFFFFU ) ; // TTL
	m.addWord( (ttl >> 0) & 0xFFFFU ) ; // TTL
	m.addWord( 0x04U ) ; // RDLENGTH
	m.addByte( G::Str::toUInt(address_part[0]) ) ;
	m.addByte( G::Str::toUInt(address_part[1]) ) ;
	m.addByte( G::Str::toUInt(address_part[2]) ) ;
	m.addByte( G::Str::toUInt(address_part[3]) ) ;
}

//

int main( int argc , char * argv [] )
{
	try
	{
		G::Arg arg( argc , argv ) ;
		G::Options options ;
		using M = G::Option::Multiplicity ;
		G::Options::add( options , 'h' , "help" , "show help" , "" , M::zero , "" , 1 , 0 ) ;
		G::Options::add( options , '\0', "debug" , "debug logging" , "" , M::zero , "" , 1 , 0 ) ;
		G::Options::add( options , 'P' , "port" , "port number" , "" , M::one , "port" , 1 , 0 ) ;
		G::Options::add( options , 'f' , "pid-file" , "pid file" , "" , M::one , "path" , 1 , 0 ) ;
		G::Options::add( options , 'l' , "log" , "enable logging" , "" , M::zero , "" , 1 , 0 ) ;
		G::Options::add( options , 'N' , "log-file" , "output log to file" , "" , M::one , "path" , 1 , 0 ) ;
		G::Options::add( options , '\0' , "address" , "address in response" , "" , M::one , "address" , 1 , 0 ) ;
		G::GetOpt opt( arg , options ) ;
		if( opt.hasErrors() )
		{
			opt.showErrors(std::cerr) ;
			return 2 ;
		}
		if( opt.contains("help") )
		{
			G::OptionsUsage(opt.options()).output( {} , std::cout , arg.prefix() ) ;
			return 0 ;
		}

		G::Path argv0 = G::Path(arg.v(0)).withoutExtension().basename() ;

		Server::Config config ;
		config.port = opt.contains("port") ? G::Str::toUInt(opt.value("port")) : 10053U ;
		config.answer_a = opt.value( "address" , "127.0.@.1" ) ; // @ -> "1" if query contains "one" etc.
		config.answer_mx = "smtp.@" ; // @ -> <qname>
		bool debug = opt.contains( "debug" ) ;
		std::string pid_file_name = opt.value( "pid-file" , "."+argv0.str()+".pid" ) ;
		std::string log_file = opt.value( "log-file" , "" ) ;

		G::LogOutput log( "" ,
			G::LogOutput::Config()
				.set_output_enabled()
				.set_summary_info()
				.set_verbose_info()
				.set_debug(debug)
				.set_with_level()
				.set_with_timestamp()
				.set_strip(false) ,
			log_file ) ;

		G_LOG_S( "pid=[" << G::Process::Id() << "]" ) ;
		G_LOG_S( "pidfile=[" << pid_file_name << "]" ) ;
		G_LOG_S( "port=[" << config.port << "]" ) ;

		{
			std::ofstream pid_file ;
			G::File::open( pid_file , pid_file_name , G::File::Text() ) ;
			pid_file << G::Process::Id().str() << std::endl ;
		}

		auto event_loop = GNet::EventLoop::create() ;
		auto es = GNet::EventState::create() ;
		GNet::TimerList timer_list ;
		Server server( es , config ) ;

		event_loop->run() ;

		return 0 ;
	}
	catch( std::exception & e )
	{
		std::cerr << "exception: " << e.what() << std::endl ;
	}
	return 1 ;
}

