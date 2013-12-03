//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
//        [--auth-ok] [--slow] [--fail-at <n>] [--port <port>]
//

#include "gdef.h"
#include "gnet.h"
#include "geventloop.h"
#include "gtimerlist.h"
#include "gstr.h"
#include "garg.h"
#include "gpath.h"
#include "gserver.h"
#include "gbufferedserverpeer.h"
#include "gprocess.h"
#include "glogoutput.h"
#include <string>
#include <fstream>
#include <exception>
#include <stdexcept>
#include <iostream>

struct Config
{
	unsigned int m_port ;
	bool m_auth_foo_bar ;
	bool m_auth_login ;
	bool m_auth_plain ;
	bool m_auth_ok ;
	int m_fail_at ;
	bool m_tls ;
	bool m_quiet ;
	Config( unsigned int port , bool auth_foo_bar , bool auth_login , bool auth_plain , bool auth_ok , int fail_at , bool tls , bool quiet ) : 
		m_port(port) ,
		m_auth_foo_bar(auth_foo_bar) ,
		m_auth_login(auth_login) ,
		m_auth_plain(auth_plain) ,
		m_auth_ok(auth_ok) ,
		m_fail_at(fail_at) ,
		m_tls(tls) ,
		m_quiet(quiet)
	{
	}
} ;

class Peer : public GNet::BufferedServerPeer 
{
public:
	Peer( GNet::Server::PeerInfo , Config ) ;
	virtual void onDelete( const std::string & ) ;
	virtual void onSendComplete() ;
	virtual bool onReceive( const std::string & ) ;
	virtual void onSecure( const std::string & ) ;
	void tx( const std::string & ) ;
	Config m_config ;
	bool m_in_data ;
	bool m_in_auth_1 ;
	bool m_in_auth_2 ;
	int m_message ;
} ;

class Server : public GNet::Server 
{
public:
	explicit Server( Config c ) ;
	virtual GNet::ServerPeer * newPeer( GNet::Server::PeerInfo ) ;
	Config m_config ;
} ;

Server::Server( Config config ) : 
	GNet::Server(config.m_port) ,
	m_config(config)
{
}

GNet::ServerPeer * Server::newPeer( PeerInfo info )
{
	G_LOG_S( "Server::newPeer: new connection from " << info.m_address.displayString() ) ;
	return new Peer( info , m_config ) ;
}

//

Peer::Peer( GNet::Server::PeerInfo info , Config config ) :
	GNet::BufferedServerPeer(info,"\r\n") ,
	m_config(config) ,
	m_in_data(false) ,
	m_in_auth_1(false) ,
	m_in_auth_2(false) ,
	m_message(0)
{
	send( "220 test server\r\n" ) ;
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

bool Peer::onReceive( const std::string & line )
{
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
		tx( fail ? "452 failed\r\n" : "250 OK\r\n" ) ;
	}
	else if( G::Str::upper(line) == "STARTTLS" )
	{
		; // no response (tbc?)
	}
	else if( G::Str::upper(line) == "QUIT" )
	{
		doDelete() ;
	}
	else if( G::Str::trimmed(G::Str::upper(line),G::Str::ws()) == "AUTH PLAIN" ) 
	{
		// got auth command on its own, without credentials
		m_in_auth_2 = true ;
		tx( "334\r\n" ) ;
	}
	else if( G::Str::upper(line).find("AUTH") == 0U && G::Str::upper(line).find("PLAIN") != std::string::npos )
	{
		// got auth command with credentials
		tx( m_config.m_auth_ok ? "235 authentication ok\r\n" : "535 authentication failed\r\n" ) ;
	}
	else if( G::Str::upper(line).find("AUTH") == 0U && G::Str::upper(line).find("LOGIN") != std::string::npos )
	{
		m_in_auth_1 = true ;
		tx( "334 VXNlcm5hbWU6\r\n" ) ;
	}
	else if( m_in_auth_1 )
	{
		m_in_auth_1 = false ;
		if( m_config.m_auth_ok )
		{
			tx( "334 UGFzc3dvcmQ6\r\n" ) ;
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
		bool auth_foo_bar = arg.remove( "--auth-foo-bar" ) ;
		bool auth_login = arg.remove( "--auth-login" ) ;
		bool auth_plain = arg.remove( "--auth-plain" ) ;
		bool auth_ok = arg.remove( "--auth-ok" ) ;
		bool slow = arg.remove( "--slow" ) ;
		bool tls = arg.remove( "--tls" ) ;
		bool quiet = arg.remove( "--quiet" ) ;
		int fail_at = arg.contains("--fail-at",1U) ? G::Str::toInt(arg.v(arg.index("--fail-at",1U)+1U)) : -1 ;
		unsigned int port = arg.contains("--port",1U) ? G::Str::toUInt(arg.v(arg.index("--port",1U)+1U)) : 10025U ;

		G::Path argv0 = G::Path(arg.v(0)).basename() ; argv0.removeExtension() ;
		std::string pid_file_name = std::string(".") + argv0.str() + ".pid" ;
		{
			std::ofstream pid_file( pid_file_name.c_str() ) ;
			pid_file << G::Process::Id().str() << std::endl ;
		}

		G::LogOutput log( "" , !quiet , !quiet , false , false , true , false , true , false ) ;
		GNet::EventLoop * loop = GNet::EventLoop::create() ;
		loop->init();
		GNet::TimerList timer_list ;
		Server server( Config(port,auth_foo_bar,auth_login,auth_plain,auth_ok,fail_at,tls,quiet) ) ;

		if( slow )
		{
			while( true )
			{
				loop->quit("x") ;
				if( loop->run() != "x" )
					break ;
				struct timeval t ;
				t.tv_sec = 0 ;
				t.tv_usec = 100000 ;
				::select( 0 , NULL , NULL , NULL , &t ) ;
			}
		}
		else
		{
			loop->run() ;
		}

		return 0 ;
	}
	catch( std::exception & e )
	{
		std::cerr << "exception: " << e.what() << std::endl ;
	}
	return 1 ;
}

/// \file emailrelay_test_server.cpp
