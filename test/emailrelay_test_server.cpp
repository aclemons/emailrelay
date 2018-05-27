//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
//
// emailrelay_test_server.cpp
//
// A dummy smtp server for testing purposes.
//
// usage: emailrelay-test-server [--quiet] [--tls] [--auth-foo-bar] [--auth-login] [--auth-plain]
//        [--auth-ok] [--slow] [--fail-at <n>] [--drop] [--ipv6] [--port <port>]
//

#include "gdef.h"
#include "geventloop.h"
#include "gtimerlist.h"
#include "gstr.h"
#include "garg.h"
#include "ggetopt.h"
#include "gpath.h"
#include "gserver.h"
#include "gbufferedserverpeer.h"
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
		#if G_WIN32
			::Sleep( ms ) ;
		#else
			struct timeval t ;
			t.tv_sec = 0 ;
			t.tv_usec = ms * 1000 ;
			::select( 0 , nullptr , nullptr , nullptr , &t ) ;
		#endif
	}
}

struct Config
{
	bool m_ipv6 ;
	unsigned int m_port ;
	bool m_auth_foo_bar ;
	bool m_auth_login ;
	bool m_auth_plain ;
	bool m_auth_ok ;
	bool m_slow ;
	int m_fail_at ;
	bool m_drop ;
	bool m_tls ;
	bool m_quiet ;
	Config( bool ipv6 , unsigned int port , bool auth_foo_bar , bool auth_login , bool auth_plain ,
		bool auth_ok , bool slow , int fail_at , bool drop , bool tls , bool quiet ) :
			m_ipv6(ipv6) ,
			m_port(port) ,
			m_auth_foo_bar(auth_foo_bar) ,
			m_auth_login(auth_login) ,
			m_auth_plain(auth_plain) ,
			m_auth_ok(auth_ok) ,
			m_slow(slow) ,
			m_fail_at(fail_at) ,
			m_drop(drop) ,
			m_tls(tls) ,
			m_quiet(quiet)
	{
	}
} ;

class Peer : public GNet::BufferedServerPeer
{
public:
	Peer( GNet::Server::PeerInfo , Config ) ;
	virtual void onDelete( const std::string & ) override ;
	virtual void onSendComplete() override ;
	virtual bool onReceive( const char * , size_t , size_t ) override ;
	virtual void onSecure( const std::string & ) override ;
	void tx( const std::string & ) ;

private:
	void onSlowTimeout() ;

private:
	Config m_config ;
	GNet::Timer<Peer> m_slow_timer ;
	bool m_in_data ;
	bool m_in_auth_1 ;
	bool m_in_auth_2 ;
	int m_message ;
} ;

class Server : public GNet::Server
{
public:
	Server( GNet::ExceptionHandler & , Config c ) ;
	virtual GNet::ServerPeer * newPeer( GNet::Server::PeerInfo ) override ;
	Config m_config ;
} ;

Server::Server( GNet::ExceptionHandler & eh , Config config ) :
	GNet::Server(eh,GNet::Address(config.m_ipv6?GNet::Address::Family::ipv6():GNet::Address::Family::ipv4(),config.m_port)) ,
	m_config(config)
{
	if( m_config.m_slow )
		GNet::SocketProtocol::setReadBufferSize( 3U ) ;
}

GNet::ServerPeer * Server::newPeer( PeerInfo info )
{
	G_LOG_S( "Server::newPeer: new connection from " << info.m_address.displayString() ) ;
	return new Peer( info , m_config ) ;
}

//

Peer::Peer( GNet::Server::PeerInfo info , Config config ) :
	GNet::BufferedServerPeer(info,GNet::LineBufferConfig::smtp()) ,
	m_config(config) ,
	m_slow_timer(*this,&Peer::onSlowTimeout,*this) ,
	m_in_data(false) ,
	m_in_auth_1(false) ,
	m_in_auth_2(false) ,
	m_message(0)
{
	send( "220 test server\r\n" ) ;
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

void Peer::onSecure( const std::string & )
{
}

bool Peer::onReceive( const char * line_data , size_t line_size , size_t )
{
	std::string line( line_data , line_size ) ;
	if( !m_config.m_quiet )
		std::cout << "rx<<: [" << line << "]" << std::endl ;

	if( G::Str::upper(line).find("EHLO") == 0UL )
	{
		bool auth = m_config.m_auth_foo_bar || m_config.m_auth_login || m_config.m_auth_plain ;

		std::ostringstream ss ;
		ss << "250-HELLO\r\n" ;
		ss << "250-VRFY\r\n" ;
		if( auth )
			ss << "250-AUTH" ;
		if( m_config.m_auth_foo_bar )
			ss << " FOO BAR" ;
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
	else if( G::Str::upper(line) == "DATA" )
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
	else if( G::Str::upper(line) == "STARTTLS" )
	{
		; // no starttls response -- could do better
	}
	else if( G::Str::upper(line) == "QUIT" )
	{
		doDelete() ;
	}
	else if( G::Str::trimmed(G::Str::upper(line),G::Str::ws()) == "AUTH PLAIN" )
	{
		// got "auth plain" command on its own, without credentials
		m_in_auth_2 = true ;
		tx( "334\r\n" ) ;
	}
	else if( G::Str::upper(line).find("AUTH") == 0U && G::Str::upper(line).find("PLAIN") != std::string::npos )
	{
		// got "auth plain" command with credentials
		tx( m_config.m_auth_ok ? "235 authentication ok\r\n" : "535 authentication failed\r\n" ) ;
	}
	else if( G::Str::upper(line).find("AUTH") == 0U && G::Str::upper(line).find("LOGIN") != std::string::npos )
	{
		// got "auth login"
		m_in_auth_1 = true ;
		tx( "334 VXNlcm5hbWU6\r\n" ) ; // "Username:"
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
	else if( G::Str::upper(line).find("RCPT TO:<REJECTME") == 0U )
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
	std::string ss( s ) ;
	G::Str::trimRight( ss , "\n\r" ) ;
	if( !m_config.m_quiet )
		std::cout << "tx>>: [" << ss << "]" << std::endl ;
	send( s ) ;
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
			"l!auth-login!enable mechanism login!!0!!1" "|"
			"p!auth-plain!enable mechanism plain!!0!!1" "|"
			"o!auth-ok!successful authentication!!0!!1" "|"
			"s!slow!slow responses!!0!!1" "|"
			"t!tls!enable tls!!0!!1" "|"
			"q!quiet!less logging!!0!!1" "|"
			"f!fail-at!fail the n'th message! (zero-based index)!1!n!1" "|"
			"d!drop!drop the connection when content has DROP or when failing!!0!!1" "|"
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
			opt.options().showUsage( std::cout , arg.prefix() ) ;
			return 0 ;
		}

		bool auth_foo_bar = opt.contains( "auth-foo-bar" ) ;
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

		G::Path argv0 = G::Path(arg.v(0)).withoutExtension().basename() ;
		std::string pid_file_name = opt.value( "pid-file" , "."+argv0.str()+".pid" ) ;

		G::LogOutput log( "" , !quiet , !quiet , false , false , true , false , true , false ) ;
		G_LOG_S( "pid=[" << G::Process::Id() << "]" ) ;
		G_LOG_S( "pidfile=[" << pid_file_name << "]" ) ;
		G_LOG_S( "port=[" << port << "]" ) ;
		G_LOG_S( "fail-at=[" << fail_at << "]" ) ;

		{
			std::ofstream pid_file( pid_file_name.c_str() ) ;
			pid_file << G::Process::Id().str() << std::endl ;
		}

		GNet::EventLoop * event_loop = GNet::EventLoop::create() ;
		GNet::TimerList timer_list ;
		Server server( *event_loop , Config(ipv6,port,auth_foo_bar,auth_login,auth_plain,auth_ok,slow,fail_at,drop,tls,quiet) ) ;

		event_loop->run() ;

		return 0 ;
	}
	catch( std::exception & e )
	{
		std::cerr << "exception: " << e.what() << std::endl ;
	}
	return 1 ;
}

/// \file emailrelay_test_server.cpp
