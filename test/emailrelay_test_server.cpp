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
/// \file emailrelay_test_server.cpp
///
// A dummy smtp server for testing purposes.
//
// usage: emailrelay-test-server [--quiet] [--tls] [--auth-foo-bar] [--auth-cram] [--auth-login] [--auth-plain]
//        [--auth-ok] [--slow] [--fail-at <n>] [--drop] [--ipv6] [--port <port>]
//

#include "gdef.h"
#include "gfile.h"
#include "geventloop.h"
#include "gtimerlist.h"
#include "gnetdone.h"
#include "gstr.h"
#include "garg.h"
#include "ggetopt.h"
#include "goptionsoutput.h"
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

namespace
{
	void sleep_ms( int ms )
	{
		#if G_WINDOWS
			::Sleep( ms ) ;
		#else
			struct timeval t ;
			t.tv_sec = 0 ;
			t.tv_usec = ms * 1000 ;
			::select( 0 , nullptr , nullptr , nullptr , &t ) ;
		#endif
	}
}

struct TestServerConfig
{
	bool m_ipv6 ;
	unsigned int m_port ;
	bool m_auth_foo_bar ;
	bool m_auth_cram ;
	bool m_auth_login ;
	bool m_auth_plain ;
	bool m_auth_ok ;
	bool m_slow ;
	int m_fail_at ;
	bool m_drop ;
	bool m_tls ;
	bool m_quiet ;
	unsigned int m_idle_timeout ;
	TestServerConfig( bool ipv6 , unsigned int port , bool auth_foo_bar , bool auth_cram , bool auth_login , bool auth_plain ,
		bool auth_ok , bool slow , int fail_at , bool drop , bool tls , bool quiet , unsigned int idle_timeout ) :
			m_ipv6(ipv6) ,
			m_port(port) ,
			m_auth_foo_bar(auth_foo_bar) ,
			m_auth_cram(auth_cram) ,
			m_auth_login(auth_login) ,
			m_auth_plain(auth_plain) ,
			m_auth_ok(auth_ok) ,
			m_slow(slow) ,
			m_fail_at(fail_at) ,
			m_drop(drop) ,
			m_tls(tls) ,
			m_quiet(quiet) ,
			m_idle_timeout(idle_timeout)
	{
	}
} ;

class Peer : public GNet::ServerPeer
{
public:
	Peer( GNet::ExceptionSinkUnbound , GNet::ServerPeerInfo && , TestServerConfig ) ;
	void onDelete( const std::string & ) override ;
	void onSendComplete() override ;
	bool onReceive( const char * , std::size_t , std::size_t , std::size_t , char ) override ;
	void onSecure( const std::string & , const std::string & , const std::string & ) override ;
	void tx( const std::string & ) ;

private:
	void onSlowTimeout() ;

private:
	GNet::ExceptionSink m_es ;
	TestServerConfig m_config ;
	GNet::Timer<Peer> m_slow_timer ;
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
} ;

Server::Server( GNet::ExceptionSink es , TestServerConfig config ) :
	GNet::Server(es,
		GNet::Address(config.m_ipv6?GNet::Address::Family::ipv6:GNet::Address::Family::ipv4,config.m_port),
		GNet::ServerPeer::Config()
			.set_read_buffer_size(config.m_slow?3U:0U)
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
	GNet::ServerPeer(esu.bind(this),std::move(peer_info),GNet::LineBufferConfig::smtp()) ,
	m_es(esu.bind(this)) ,
	m_config(config) ,
	m_slow_timer(*this,&Peer::onSlowTimeout,m_es) ,
	m_in_data(false) ,
	m_in_auth_1(false) ,
	m_in_auth_2(false) ,
	m_message(0)
{
	send( "220 test server\r\n"_sv ) ;
	if( m_config.m_slow )
		m_slow_timer.startTimer( 0U ) ;
}

void Peer::onSlowTimeout()
{
	sleep_ms( 800 ) ;
	m_slow_timer.startTimer( 0U , 10 ) ;
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
	if( !m_config.m_quiet )
		std::cout << "rx<<: [" << line << "]" << std::endl ;

	G::StringArray uwords = G::Str::splitIntoTokens( G::Str::upper(line) , " \t\r" ) ;
	uwords.push_back( "" ) ;
	uwords.push_back( "" ) ;
	uwords.push_back( "" ) ;

	if( uwords[0] == "EHLO" )
	{
		bool auth = m_config.m_auth_foo_bar || m_config.m_auth_cram || m_config.m_auth_login || m_config.m_auth_plain ;

		std::ostringstream ss ;
		ss << "250-HELLO\r\n" ;
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
		if( fail && m_config.m_drop ) throw G::Exception("connection dropped") ;
		tx( fail ? "452 failed\r\n" : "250 OK\r\n" ) ;
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
		tx( "550 invalid recipient\r\n" ) ;
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

void Peer::tx( const std::string & s )
{
	if( !m_config.m_quiet )
	{
		std::string ss( s ) ;
		G::Str::trimRight( ss , "\n\r" ) ;
		std::cout << "tx>>: [" << ss << "]" << std::endl ;
	}
	send( s ) ; // GNet::ServerPeer::send()
}

//

int main( int argc , char * argv [] )
{
	try
	{
		G::Arg arg( argc , argv ) ;
		G::GetOpt opt( arg ,
			"h!help!show help!!0!!1" "|"
			"b!auth-foo-bar!enable mechanisms foo and bar!!0!!1" "|"
			"c!auth-cram!enable mechanism cram-md5!!0!!1" "|"
			"l!auth-login!enable mechanism login!!0!!1" "|"
			"p!auth-plain!enable mechanism plain!!0!!1" "|"
			"o!auth-ok!successful authentication!!0!!1" "|"
			"s!slow!slow responses!!0!!1" "|"
			"t!tls!enable tls!!0!!1" "|"
			"q!quiet!less logging!!0!!1" "|"
			"f!fail-at!fail from the n'th message! of the session (zero-based index)!1!n!1" "|"
			"d!drop!drop the connection when content has DROP or when failing!!0!!1" "|"
			"i!idle-timeout!idle timeout!!1!seconds!1" "|"
			"P!port!port number!!1!port!1" "|"
			"f!pid-file!pid file!!1!path!1" "|"
			"6!ipv6!use ipv6!!0!!1" "|"
		) ;
		if( opt.hasErrors() )
		{
			opt.showErrors(std::cerr) ;
			return 2 ;
		}
		if( opt.contains("help") )
		{
			G::OptionsOutput(opt.options()).showUsage( {} , std::cout , arg.prefix() ) ;
			return 0 ;
		}

		bool auth_foo_bar = opt.contains( "auth-foo-bar" ) ;
		bool auth_cram = opt.contains( "auth-cram" ) ;
		bool auth_login = opt.contains( "auth-login" ) ;
		bool auth_plain = opt.contains( "auth-plain" ) ;
		bool auth_ok = opt.contains( "auth-ok" ) ;
		bool slow = opt.contains( "slow" ) ;
		bool tls = opt.contains( "tls" ) ;
		bool quiet = opt.contains( "quiet" ) ;
		int fail_at = opt.contains("fail-at") ? G::Str::toInt(opt.value("fail-at")) : -1 ;
		bool drop = opt.contains( "drop" ) ;
		bool ipv6 = opt.contains( "ipv6" ) ;
		unsigned int port = opt.contains("port") ? G::Str::toUInt(opt.value("port")) : 10025U ;
		unsigned int idle_timeout = opt.contains("idle-timeout") ? G::Str::toInt(opt.value("idle-timeout")) : 300U ;

		G::Path argv0 = G::Path(arg.v(0)).withoutExtension().basename() ;
		std::string pid_file_name = opt.value( "pid-file" , "."+argv0.str()+".pid" ) ;

		G::LogOutput log( "" ,
			G::LogOutput::Config()
				.set_output_enabled(!quiet)
				.set_summary_info(!quiet)
				.set_with_level(true)
				.set_strip(true) ) ;

		G_LOG_S( "pid=[" << G::Process::Id() << "]" ) ;
		G_LOG_S( "pidfile=[" << pid_file_name << "]" ) ;
		G_LOG_S( "port=[" << port << "]" ) ;
		G_LOG_S( "fail-at=[" << fail_at << "]" ) ;

		{
			std::ofstream pid_file ;
			G::File::open( pid_file , pid_file_name , G::File::Text() ) ;
			pid_file << G::Process::Id().str() << std::endl ;
		}

		std::unique_ptr<GNet::EventLoop> event_loop = GNet::EventLoop::create() ;
		GNet::ExceptionSink es ;
		GNet::TimerList timer_list ;
		Server server( es , TestServerConfig(ipv6,port,auth_foo_bar,auth_cram,auth_login,auth_plain,auth_ok,slow,fail_at,drop,tls,quiet,idle_timeout) ) ;

		event_loop->run() ;

		return 0 ;
	}
	catch( std::exception & e )
	{
		std::cerr << "exception: " << e.what() << std::endl ;
	}
	return 1 ;
}

