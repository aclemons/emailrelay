//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// usage: emailrelay-test-server [--auth-foo-bar] [--auth-login] [--slow] [--fail-at <n>] [--port <port>]
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
	int m_fail_at ;
	Config( unsigned int port , bool auth_foo_bar , bool auth_login , int fail_at ) : 
		m_port(port) ,
		m_auth_foo_bar(auth_foo_bar) ,
		m_auth_login(auth_login) ,
		m_fail_at(fail_at)
	{
	}
} ;

class Peer : public GNet::BufferedServerPeer 
{
public:
	Peer( GNet::Server::PeerInfo , Config ) ;
	virtual void onDelete() ;
	virtual void onSendComplete() ;
	virtual bool onReceive( const std::string & ) ;
	void tx( const std::string & ) ;
	Config m_config ;
	bool m_in_data ;
	bool m_in_auth ;
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

Peer::Peer( GNet::Server::PeerInfo info , Config config ) :
	GNet::BufferedServerPeer(info,"\r\n",true) ,
	m_config(config) ,
	m_in_data(false) ,
	m_in_auth(false) ,
	m_message(0)
{
	send( "220 test server\r\n" ) ;
}

void Peer::onDelete()
{
	G_LOG_S( "Server::newPeer: connection dropped" ) ;
}

void Peer::onSendComplete()
{
}

bool Peer::onReceive( const std::string & line )
{
	std::cout << "rx<<: [" << line << "]" << std::endl ;

	if( G::Str::upper(line).find("EHLO") == 0UL )
	{
		std::string s1( "250-hello\r\n" "250-VRFY\r\n" ) ;
		std::string s2a( "250-AUTH FOO BAR\r\n" ) ;
		std::string s2b( "250-AUTH LOGIN\r\n" ) ;
		std::string s3( "250 8BITMIME\r\n" ) ;
		bool auth = m_config.m_auth_foo_bar || m_config.m_auth_login ;
		std::string s2 = m_config.m_auth_foo_bar ? s2a : s2b ;
		tx( auth ? (s1+s2+s3) : (s1+s3) ) ;
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
	else if( G::Str::upper(line).find("AUTH") == 0U )
	{
		m_in_auth = true ;
		tx( "334 VXNlcm5hbWU6\r\n" ) ; // assume login mechanism
	}
	else if( G::Str::upper(line) == "QUIT" )
	{
		doDelete() ;
	}
	else if( m_in_auth )
	{
		m_in_auth = false ; // assume one challenge
		tx( "535 authentication failed\r\n" ) ;
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
	std::cout << "tx>>: [" << ss << "]" << std::endl ;
	send( s ) ;
}

int main( int argc , char * argv [] )
{
	try
	{
		G::Arg arg( argc , argv ) ;
		bool auth_foo_bar = arg.remove( "--auth-foo-bar" ) ;
		bool auth_login = arg.remove( "--auth-login" ) ;
		bool slow = arg.remove( "--slow" ) ;
		int fail_at = arg.contains("--fail-at",1U) ? G::Str::toInt(arg.v(arg.index("--fail-at",1U)+1U)) : -1 ;
		int port = arg.contains("--port",1U) ? G::Str::toInt(arg.v(arg.index("--port",1U)+1U)) : 10025 ;

		std::string pid_file_name = std::string(".") + G::Path(arg.v(0)).basename() + ".pid" ;
		{
			std::ofstream pid_file( pid_file_name.c_str() ) ;
			pid_file << G::Process::Id().str() << std::endl ;
		}

		G::LogOutput log( "" , true , true , false , false , true , false , true , false ) ;
		GNet::EventLoop * loop = GNet::EventLoop::create() ;
		GNet::TimerList timer_list ;
		Server server( Config(port,auth_foo_bar,auth_login,fail_at) ) ;

		if( slow )
		{
			while( true )
			{
				if( loop->quit() )
					break ;
				loop->run() ;
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
