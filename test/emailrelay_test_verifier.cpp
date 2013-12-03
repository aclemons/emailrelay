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
// emailrelay_test_verifier.cpp
//
// A dummy address verifier for testing "emailrelay --verifier net:<host>:<port>".
//
// usage: emailrelay-test-verifier [--port <port>] [--log] [--log-file <file>] [--debug] [--pid-file <pidfile>]
//
// Listens on port 10020 by default.
//

#include "gdef.h"
#include "gnet.h"
#include "gserver.h"
#include "glinebuffer.h"
#include "gstr.h"
#include "gevent.h"
#include "gprocess.h"
#include "gmemory.h"
#include "gbufferedserverpeer.h"
#include "garg.h"
#include "gsleep.h"
#include "gdebug.h"
#include <sstream>
#include <iostream>
#include <fstream>

namespace Main
{
	class VerifierPeer ;
	class Verifier ;
}

class Main::VerifierPeer : public GNet::BufferedServerPeer 
{
public:
	explicit VerifierPeer( GNet::Server::PeerInfo info ) ;
private:	
	virtual void onDelete( const std::string & ) ;
	virtual bool onReceive( const std::string & ) ;
	virtual void onSecure( const std::string & ) ;
	virtual void onSendComplete() {}
	bool processLine( std::string ) ;
private:
	GNet::LineBuffer m_buffer ;
} ;

Main::VerifierPeer::VerifierPeer( GNet::Server::PeerInfo info ) :
	BufferedServerPeer( info , "\n" )
{
	G_LOG_S( "VerifierPeer::ctor: new connection from " << info.m_address.displayString() ) ;
}

void Main::VerifierPeer::onDelete( const std::string & ) 
{
	G_LOG_S( "VerifierPeer::onDelete: disconnected" ) ;
}

void Main::VerifierPeer::onSecure( const std::string & )
{
}

bool Main::VerifierPeer::onReceive( const std::string & line )
{
	G_DEBUG( "VerifierPeer::onReceive: [" << G::Str::printable(line) << "]" ) ;
	processLine( line ) ;
	return true ;
}

bool Main::VerifierPeer::processLine( std::string line )
{
	G_LOG_S( "VerifierPeer::processLine: line: \"" << line << "\"" ) ;

	G::StringArray part ;
	G::Str::splitIntoFields( line , part , "|" ) ;
	std::string to = part.at(0U) ;

	bool local = to.find("L") != std::string::npos ;
	bool valid = to.find("OK") != std::string::npos ;
	bool blackhole = to.find("X") != std::string::npos ;
	bool abort = to.find("!") != std::string::npos ;

	if( abort )
	{
		G_LOG_S( "VerifierPeer::processLine: sending 100" ) ;
		send( "100\n" ) ;
	}
	else if( blackhole )
	{
		G_LOG_S( "VerifierPeer::processLine: sending nothing" ) ;
	}
	else if( valid && local )
	{
		G_LOG_S( "VerifierPeer::processLine: sending postmaster" ) ;
		send( "0|postmaster|Postmaster <postmaster@localhost>\n" ) ;
	}
	else if( valid )
	{
		G_LOG_S( "VerifierPeer::processLine: sending valid" ) ;
		send( "1|" + to + "\n" ) ;
	}
	else
	{
		G_LOG_S( "VerifierPeer::processLine: sending error" ) ;
		send( "2|Error\n" ) ;
	}
	return true ;
}

// ===

class Main::Verifier : public GNet::Server 
{
public:
	Verifier( unsigned int port ) ;
	virtual GNet::ServerPeer * newPeer( GNet::Server::PeerInfo ) ;
} ;

Main::Verifier::Verifier( unsigned int port ) :
	GNet::Server( port )
{
}

GNet::ServerPeer * Main::Verifier::newPeer( GNet::Server::PeerInfo info )
{
	return new VerifierPeer( info ) ;
}

// ===

static int run( unsigned int port )
{
	std::auto_ptr<GNet::EventLoop> loop( GNet::EventLoop::create() ) ;
	loop->init() ;
	GNet::TimerList timer_list ;
	Main::Verifier verifier( port ) ;
	loop->run() ;
	return 0 ;
}

int main( int argc , char * argv [] )
{
	try
	{
		G::Arg arg( argc , argv ) ;
		bool log = arg.remove("--log") ;
		bool debug = arg.remove("--debug") ;
		std::string log_file = arg.index("--log-file",1U) ? arg.v(arg.index("--log-file",1U)+1U) : std::string() ;
		unsigned int port = arg.index("--port",1U) ? G::Str::toUInt(arg.v(arg.index("--port",1U)+1U)) : 10020U ;
		std::string pid_file = arg.index("--pid-file",1U) ? arg.v(arg.index("--pid-file",1U)+1U) : std::string() ;

		if( !pid_file.empty() )
		{
			std::ofstream file( pid_file.c_str() ) ;
			file << G::Process::Id().str() << std::endl ;
		}

		G::LogOutput log_output( log , debug , log_file ) ;
		int rc = run( port ) ;
		std::cout << "done" << std::endl ;
		return rc ;
	}
	catch( std::exception & e )
	{
		std::cerr << "exception: " << e.what() << std::endl ;
	}
	catch(...)
	{
		std::cerr << "exception\n" ;
	}
	return 1 ;
}

/// \file emailrelay_test_verifier.cpp
