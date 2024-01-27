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
/// \file emailrelay_test_server.cpp
///
// A dummy SMTP server for testing purposes.
//
// usage: emailrelay_test_server [options]
//           --port=<port>           port number
//           --pid-file=<path>       pid file
//           --quiet                 less logging
//           --debug                 more logging
//           --ipv6                  use ipv6
//           --auth-cram             enable mechanism cram-md5
//           --auth-foo-bar          enable mechanisms foo and bar
//           --auth-login            enable mechanism login
//           --auth-ok               successful authentication
//           --auth-plain            enable mechanism plain
//           --drop                  drop the connection when content has DROP or when
//           --fail-at=<n>           fail from the n'th message
//           --idle-timeout=<s>      idle timeout
//           --pause                 slow final ok response
//           --terminate             terminate when failing
//           --tls                   enable tls
//           --huge                  send huge smtp responses
//

#include "gdef.h"
#include "gfile.h"
#include "geventloop.h"
#include "gtimerlist.h"
#include "gnetdone.h"
#include "gstr.h"
#include "garg.h"
#include "ggetopt.h"
#include "goptionsusage.h"
#include "gpath.h"
#include "gserver.h"
#include "gserverpeer.h"
#include "gprocess.h"
#include "glogoutput.h"
#include "gexception.h"
#include "gsleep.h"
#include <string>
#include <fstream>
#include <exception>
#include <stdexcept>
#include <iostream>

struct TestServerConfig
{
	bool m_ipv6 ;
	unsigned int m_port ;
	bool m_auth_foo_bar ;
	bool m_auth_cram ;
	bool m_auth_login ;
	bool m_auth_plain ;
	bool m_auth_ok ;
	bool m_pause ;
	int m_fail_at ;
	bool m_drop ;
	bool m_terminate ;
	bool m_tls ;
	bool m_quiet ;
	unsigned int m_idle_timeout ;
	std::size_t m_huge ;
} ;

class Peer : public GNet::ServerPeer
{
public:
	Peer( GNet::ExceptionSinkUnbound , GNet::ServerPeerInfo && , TestServerConfig ) ;
	void onDelete( const std::string & ) override ;
	void onSendComplete() override ;
	bool onReceive( const char * , std::size_t , std::size_t , std::size_t , char ) override ;
	void onSecure( const std::string & , const std::string & , const std::string & ) override ;
	void tx( std::string ) ;

private:
	void onPauseTimeout() ;

private:
	GNet::ExceptionSink m_es ;
	TestServerConfig m_config ;
	GNet::Timer<Peer> m_pause_timer ;
	bool m_in_data ;
	bool m_in_auth_1 ;
	bool m_in_auth_2 ;
	int m_message ;
} ;

class Server : public GNet::Server
{
public:
	Server( GNet::ExceptionSink , TestServerConfig ) ;
	~Server() override ;
	std::unique_ptr<GNet::ServerPeer> newPeer( GNet::ExceptionSinkUnbound , GNet::ServerPeerInfo && ) override ;
	TestServerConfig m_config ;
	static GNet::Address address( const TestServerConfig & config )
	{
		auto family = config.m_ipv6 ? GNet::Address::Family::ipv6 : GNet::Address::Family::ipv4 ;
		//return GNet::Address( family , config.m_port ) ;
		return GNet::Address::loopback( family , config.m_port ) ;
	}
} ;

Server::Server( GNet::ExceptionSink es , TestServerConfig config ) :
	GNet::Server(es,
		GNet::Address(address(config)) ,
		GNet::ServerPeer::Config()
			.set_socket_protocol_config( GNet::SocketProtocol::Config() )
			.set_idle_timeout(config.m_idle_timeout),
		GNet::Server::Config()) ,
	m_config(config)
{
}

Server::~Server()
{
	serverCleanup() ; // base class early cleanup
}

std::unique_ptr<GNet::ServerPeer> Server::newPeer( GNet::ExceptionSinkUnbound esu , GNet::ServerPeerInfo && peer_info )
{
	try
	{
		G_LOG_S( "Server::newPeer: new connection from " << peer_info.m_address.displayString() ) ;
		return std::unique_ptr<GNet::ServerPeer>( new Peer( esu , std::move(peer_info) , m_config ) ) ;
	}
	catch( std::exception & e )
	{
		G_WARNING( "Server::newPeer: new connection error: " << e.what() ) ;
		return std::unique_ptr<GNet::ServerPeer>() ;
	}
}

//

Peer::Peer( GNet::ExceptionSinkUnbound esu , GNet::ServerPeerInfo && peer_info , TestServerConfig config ) :
	GNet::ServerPeer(esu.bind(this),std::move(peer_info),GNet::LineBuffer::Config::smtp()) ,
	m_es(esu.bind(this)) ,
	m_config(config) ,
	m_pause_timer(*this,&Peer::onPauseTimeout,m_es) ,
	m_in_data(false) ,
	m_in_auth_1(false) ,
	m_in_auth_2(false) ,
	m_message(0)
{
	send( "220 test server\r\n"_sv ) ;
}

void Peer::onDelete( const std::string & )
{
	G_LOG_S( "Server::newPeer: connection dropped" ) ;
}

void Peer::onSendComplete()
{
}

void Peer::onSecure( const std::string & , const std::string & , const std::string & )
{
}

bool Peer::onReceive( const char * line_data , std::size_t line_size , std::size_t , std::size_t , char )
{
	std::string line( line_data , line_size ) ;
	G_LOG_IF( !m_config.m_quiet , "Peer::onReceive: " << "rx<<: [" << line << "]" ) ;

	G::StringArray uwords = G::Str::splitIntoTokens( G::Str::upper(line) , " \t\r" ) ;
	uwords.push_back( "" ) ;
	uwords.push_back( "" ) ;
	uwords.push_back( "" ) ;

	if( uwords[0] == "EHLO" )
	{
		bool auth = m_config.m_auth_foo_bar || m_config.m_auth_cram || m_config.m_auth_login || m_config.m_auth_plain ;

		std::ostringstream ss ;
		ss << "250-HELLO\r\n" ;
		if( m_config.m_huge )
			ss << "250-" << std::string(m_config.m_huge,'x') << "\r\n" ;
		ss << "250-VRFY\r\n" ;
		if( auth )
			ss << "250-AUTH" ;
		if( m_config.m_auth_foo_bar )
			ss << " FOO BAR" ;
		if( m_config.m_auth_cram )
			ss << " CRAM-MD5" ;
		if( m_config.m_auth_login )
			ss << " LOGIN" ;
		if( m_config.m_auth_plain )
			ss << " PLAIN" ;
		if( auth )
			ss << "\r\n" ;
		if( m_config.m_tls )
			ss << "250-STARTTLS\r\n" ;
		ss << "250 8BITMIME\r\n" ;
		tx( ss.str() ) ;
	}
	else if( uwords[0] == "DATA" )
	{
		m_in_data = true ;
		tx( "354 start mail input\r\n" ) ;
	}
	else if( line == "." )
	{
		m_in_data = false ;
		bool fail = m_config.m_fail_at >= 0 && m_message >= m_config.m_fail_at ;
		m_message++ ;
		if( fail )
		{
			if( m_config.m_drop )
				throw G::Exception( "connection dropped" ) ;
			else if( m_config.m_terminate )
				GNet::EventLoop::instance().quit( "fail-at with terminate" ) ;
			tx( "499-failed\r\n499 for testing\r\n" ) ;
		}
		else if( m_config.m_pause )
		{
			m_pause_timer.startTimer( 2U ) ;
		}
		else
		{
			tx( "250 OK\r\n" ) ;
		}
	}
	else if( uwords[0] == "STARTTLS" )
	{
		; // no starttls response -- could do better
	}
	else if( uwords[0] == "QUIT" )
	{
		throw GNet::Done() ;
	}
	else if( uwords[0] == "AUTH" && uwords[1] == "LOGIN" && uwords[2].empty() )
	{
		// got "auth login"
		m_in_auth_1 = true ;
		tx( "334 VXNlcm5hbWU6\r\n" ) ; // "Username:"
	}
	else if( uwords[0] == "AUTH" && uwords[1] == "LOGIN" )
	{
		// got "auth login <username>"
		m_in_auth_2 = true ;
		tx( "334 UGFzc3dvcmQ6\r\n" ) ; // "Password:"
	}
	else if( uwords[0] == "AUTH" && uwords[2].empty() ) // any mechanism except LOGIN
	{
		// got "auth whatever"
		m_in_auth_2 = true ;
		tx( "334 \r\n" ) ;
	}
	else if( uwords[0] == "AUTH" && uwords[1] == "PLAIN" )
	{
		// got "auth plain <initial-response>"
		tx( m_config.m_auth_ok ? "235 authentication ok\r\n" : "535 authentication failed\r\n" ) ;
	}
	else if( m_in_auth_1 )
	{
		m_in_auth_1 = false ;
		if( m_config.m_auth_ok )
		{
			tx( "334 UGFzc3dvcmQ6\r\n" ) ; // "Password:"
			m_in_auth_2 = true ;
		}
		else
		{
			tx( "535 authentication failed\r\n" ) ;
		}
	}
	else if( m_in_auth_2 )
	{
		m_in_auth_2 = false ;
		tx( m_config.m_auth_ok ? "235 authentication ok\r\n" : "535 authentication failed\r\n" ) ;
	}
	else if( uwords[0] == "RCPT" && uwords[1].find("TO:<REJECTME") == 0U )
	{
		tx( "551 invalid recipient\r\n" ) ;
	}
	else if( uwords[0] == "MAIL" && uwords[1].find("FROM:<REJECTME") == 0U )
	{
		tx( "552 invalid originator\r\n" ) ;
	}
	else if( m_in_data && line.find("SHUTDOWN") != std::string::npos )
	{
		for( int i = 0 ; i < 100 ; i++ )
			tx( "SHUTDOWN " + G::Str::fromInt(i) + "!\r\n" ) ;
		socket().shutdown() ;
	}
	else if( m_in_data && line.find("DROP") != std::string::npos )
	{
		throw G::Exception("connection dropped") ;
	}
	else if( !m_in_data )
	{
		tx( "250 OK\r\n" ) ;
	}
	return true ;
}

void Peer::tx( std::string s )
{
	if( m_config.m_huge && s.size() >= 4U && G::Str::isNumeric(s.substr(0U,3U)) )
	{
		s.insert( 3U , std::string(1U,'-').append(m_config.m_huge,'x').append("\r\n").append(s.substr(0U,3U)) ) ;
	}
	if( !m_config.m_quiet )
	{
		std::string log_text( s ) ;
		G::Str::trimRight( log_text , "\n\r" ) ;
		G_LOG_IF( !m_config.m_quiet , "Peer::tx: tx>>: [" << log_text << "]" ) ;
	}
	send( s ) ; // GNet::ServerPeer::send()
}

void Peer::onPauseTimeout()
{
	tx( "250 OK\r\n" ) ;
}

//

int main( int argc , char * argv [] )
{
	try
	{
		G::Arg arg( argc , argv ) ;
		G::Options options ;
		using M = G::Option::Multiplicity ;
		G::Options::add( options , 'L' , "log-file" , "log to file" , "" , M::one , "log-file" , 1 , 0 ) ;
		G::Options::add( options , 'h' , "help" , "show help" , "" , M::zero , "" , 1 , 0 ) ;
		G::Options::add( options , 'b' , "auth-foo-bar" , "enable mechanisms foo and bar" , "" , M::zero , "" , 1 , 0 ) ;
		G::Options::add( options , 'c' , "auth-cram" , "enable mechanism cram-md5" , "" , M::zero , "" , 1 , 0 ) ;
		G::Options::add( options , 'l' , "auth-login" , "enable mechanism login" , "" , M::zero , "" , 1 , 0 ) ;
		G::Options::add( options , 'p' , "auth-plain" , "enable mechanism plain" , "" , M::zero , "" , 1 , 0 ) ;
		G::Options::add( options , 'o' , "auth-ok" , "successful authentication" , "" , M::zero , "" , 1 , 0 ) ;
		G::Options::add( options , 'S' , "pause" , "slow final ok response" , "" , M::zero , "" , 1 , 0 ) ;
		G::Options::add( options , 't' , "tls" , "enable tls" , "" , M::zero , "" , 1 , 0 ) ;
		G::Options::add( options , 'q' , "quiet" , "less logging" , "" , M::zero , "" , 1 , 0 ) ;
		G::Options::add( options , '\0', "debug" , "debug logging" , "" , M::zero , "" , 1 , 0 ) ;
		G::Options::add( options , 'f' , "fail-at" , "fail from the n'th message" , "of the session (zero-based index)" , M::one , "n" , 1 , 0 ) ;
		G::Options::add( options , 'd' , "drop" , "drop the connection when content has DROP or when failing" , "" , M::zero , "" , 1 , 0 ) ;
		G::Options::add( options , 'T' , "terminate" , "terminate when failing" , "" , M::zero , "" , 1 , 0 ) ;
		G::Options::add( options , 'i' , "idle-timeout" , "idle timeout" , "" , M::one , "s" , 1 , 0 ) ;
		G::Options::add( options , 'P' , "port" , "port number" , "" , M::one , "port" , 1 , 0 ) ;
		G::Options::add( options , 'f' , "pid-file" , "pid file" , "" , M::one , "path" , 1 , 0 ) ;
		G::Options::add( options , '6' , "ipv6" , "use ipv6" , "" , M::zero , "" , 1 , 0 ) ;
		G::Options::add( options , 'H' , "huge" , "send huge ehlo response" , "" , M::zero , "" , 1 , 0 ) ;
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

		TestServerConfig test_config ;
		test_config.m_auth_foo_bar = opt.contains( "auth-foo-bar" ) ;
		test_config.m_auth_cram = opt.contains( "auth-cram" ) ;
		test_config.m_auth_login = opt.contains( "auth-login" ) ;
		test_config.m_auth_plain = opt.contains( "auth-plain" ) ;
		test_config.m_auth_ok = opt.contains( "auth-ok" ) ;
		test_config.m_pause = opt.contains( "pause" ) ;
		test_config.m_tls = opt.contains( "tls" ) ;
		test_config.m_quiet = opt.contains( "quiet" ) ;
		test_config.m_fail_at = opt.contains("fail-at") ? G::Str::toInt(opt.value("fail-at")) : -1 ;
		test_config.m_drop = opt.contains( "drop" ) ;
		test_config.m_terminate = opt.contains( "terminate" ) ;
		test_config.m_ipv6 = opt.contains( "ipv6" ) ;
		test_config.m_huge = opt.contains( "huge" ) ? 100000U : 0U ;
		test_config.m_port = opt.contains("port") ? G::Str::toUInt(opt.value("port")) : 10025U ;
		test_config.m_idle_timeout = opt.contains("idle-timeout") ? G::Str::toInt(opt.value("idle-timeout")) : 300U ;
		bool debug = opt.contains( "debug" ) ;

		G::Path argv0 = G::Path(arg.v(0)).withoutExtension().basename() ;
		std::string pid_file_name = opt.value( "pid-file" , "."+argv0.str()+".pid" ) ;
		std::string log_file = opt.value( "log-file" , std::string() ) ;

		try
		{
			G::LogOutput log( arg.prefix() ,
				G::LogOutput::Config()
					.set_output_enabled(!test_config.m_quiet)
					.set_summary_info(!test_config.m_quiet)
					.set_verbose_info(!test_config.m_quiet)
					.set_debug(debug)
					.set_with_level(true)
					.set_strip(true) ,
				log_file ) ;

			G_LOG_S( "pid=[" << G::Process::Id() << "]" ) ;
			G_LOG_S( "pidfile=[" << pid_file_name << "]" ) ;
			G_LOG_S( "port=[" << test_config.m_port << "]" ) ;
			G_LOG_S( "fail-at=[" << test_config.m_fail_at << "]" ) ;

			{
				std::ofstream pid_file ;
				G::File::open( pid_file , pid_file_name , G::File::Text() ) ;
				pid_file << G::Process::Id().str() << std::endl ;
			}

			auto event_loop = GNet::EventLoop::create() ;
			GNet::ExceptionSink es ;
			GNet::TimerList timer_list ;
			Server server( es , test_config ) ;

			event_loop->run() ;

			return 0 ;
		}
		catch( std::exception & e )
		{
			G_ERROR( "main: exception: " << e.what() ) ;
			throw ;
		}
	}
	catch( std::exception & e )
	{
		std::cerr << "exception: " << e.what() << std::endl ;
	}
	return 1 ;
}

