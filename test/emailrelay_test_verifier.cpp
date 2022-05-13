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
/// \file emailrelay_test_verifier.cpp
///
// A dummy network address verifier for testing "emailrelay --verifier net:<transport-address>".
//
// usage: emailrelay-test-verifier [--ipv6] [--port <port>] [--log] [--log-file <file>] [--debug] [--pid-file <pidfile>]
//
// The action of the verifier is dictated by special sub-strings in the
// recipient:
// * OK -- valid
// * L -- local
// * X -- no response (to test response timeouts)
// * x -- disconnect
// * ! -- abort
//
// Listens on port 10020 by default.
//

#include "gdef.h"
#include "gserver.h"
#include "glinebuffer.h"
#include "gstr.h"
#include "gevent.h"
#include "gtimerlist.h"
#include "gprocess.h"
#include "gserverpeer.h"
#include "gfile.h"
#include "ggetopt.h"
#include "goptionsoutput.h"
#include "garg.h"
#include "gsleep.h"
#include "glogoutput.h"
#include "glog.h"
#include <sstream>
#include <iostream>
#include <fstream>

namespace Main
{
	class VerifierPeer ;
	class Verifier ;
}

class Main::VerifierPeer : public GNet::ServerPeer
{
public:
	VerifierPeer( GNet::ExceptionSinkUnbound , GNet::ServerPeerInfo && ) ;
private:
	void onDelete( const std::string & ) override ;
	bool onReceive( const char * , std::size_t , std::size_t , std::size_t , char ) override ;
	void onSecure( const std::string & , const std::string & , const std::string & ) override ;
	void onSendComplete() override {}
	bool processLine( std::string ) ;
} ;

Main::VerifierPeer::VerifierPeer( GNet::ExceptionSinkUnbound ebu , GNet::ServerPeerInfo && peer_info ) :
	GNet::ServerPeer( ebu.bind(this) , std::move(peer_info) , GNet::LineBufferConfig::newline() )
{
	G_LOG_S( "VerifierPeer::ctor: new connection from " << peerAddress().displayString() ) ;
}

void Main::VerifierPeer::onDelete( const std::string & )
{
	G_LOG_S( "VerifierPeer::onDelete: disconnected" ) ;
}

void Main::VerifierPeer::onSecure( const std::string & , const std::string & , const std::string & )
{
}

bool Main::VerifierPeer::onReceive( const char * line_data , std::size_t line_size , std::size_t , std::size_t , char )
{
	std::string line( line_data , line_size ) ;
	G_DEBUG( "VerifierPeer::onReceive: [" << G::Str::printable(line) << "]" ) ;
	processLine( line ) ;
	return true ;
}

bool Main::VerifierPeer::processLine( std::string line )
{
	G_LOG_S( "VerifierPeer::processLine: line: \"" << line << "\"" ) ;

	G::StringArray part ;
	G::Str::splitIntoFields( line , part , '|' ) ;
	std::string to = part.at(0U) ;

	bool local = to.find("L") != std::string::npos ;
	bool valid = to.find("OK") != std::string::npos ;
	bool blackhole = to.find("X") != std::string::npos ;
	bool disconnect = to.find("x") != std::string::npos ;
	bool abort = to.find("!") != std::string::npos ;

	if( abort )
	{
		G_LOG_S( "VerifierPeer::processLine: sending 100" ) ;
		send( "100\n"_sv ) ; // GNet::ServerPeer::send()
	}
	else if( blackhole )
	{
		G_LOG_S( "VerifierPeer::processLine: sending nothing" ) ;
	}
	else if( disconnect )
	{
		throw std::runtime_error( "disconnection" ) ;
	}
	else if( valid && local )
	{
		G_LOG_S( "VerifierPeer::processLine: sending postmaster" ) ;
		send( "0|postmaster|Postmaster <postmaster@localhost>\n"_sv ) ; // GNet::ServerPeer::send()
	}
	else if( valid )
	{
		G_LOG_S( "VerifierPeer::processLine: sending valid" ) ;
		send( "1|" + to + "\n" ) ; // GNet::ServerPeer::send()
	}
	else
	{
		G_LOG_S( "VerifierPeer::processLine: sending error" ) ;
		send( "2|VerifierError\n"_sv ) ; // GNet::ServerPeer::send()
	}
	return true ;
}

// ===

class Main::Verifier : public GNet::Server
{
public:
	Verifier( GNet::ExceptionSink , bool ipv6 , unsigned int port , unsigned int idle_timeout ) ;
	~Verifier() override ;
	std::unique_ptr<GNet::ServerPeer> newPeer( GNet::ExceptionSinkUnbound , GNet::ServerPeerInfo && ) override ;
} ;

Main::Verifier::Verifier( GNet::ExceptionSink es , bool ipv6 , unsigned int port , unsigned int idle_timeout ) :
	GNet::Server(es,
		GNet::Address(ipv6?GNet::Address::Family::ipv6:GNet::Address::Family::ipv4,port),
		GNet::ServerPeer::Config()
			.set_all_timeouts(idle_timeout),
		GNet::Server::Config())
{
}

Main::Verifier::~Verifier()
{
	serverCleanup() ; // base class early cleanup
}

std::unique_ptr<GNet::ServerPeer> Main::Verifier::newPeer( GNet::ExceptionSinkUnbound ebu , GNet::ServerPeerInfo && peer_info )
{
	try
	{
		return std::unique_ptr<GNet::ServerPeer>( new VerifierPeer( ebu , std::move(peer_info) ) ) ;
	}
	catch( std::exception & e )
	{
		G_WARNING( "Verifier::newPeer: new connection error: " << e.what() ) ;
		return std::unique_ptr<GNet::ServerPeer>() ;
	}
}

// ===

static int run( bool ipv6 , unsigned int port , unsigned int idle_timeout )
{
	std::unique_ptr<GNet::EventLoop> loop = GNet::EventLoop::create() ;
	GNet::ExceptionSink es ;
	GNet::TimerList timer_list ;
	Main::Verifier verifier( es , ipv6 , port , idle_timeout ) ;
	loop->run() ;
	return 0 ;
}

int main( int argc , char * argv [] )
{
	try
	{
		G::Arg arg( argc , argv ) ;
		G::GetOpt opt( arg ,
			"h!help!show help!!0!!1" "|"
			"l!log!logging!!0!!1" "|"
			"f!log-file!log file!!1!file!1" "|"
			"d!debug!more logging!!0!!1" "|"
			"6!ipv6!use ipv6!!0!!1" "|"
			"P!port!port number!!1!port!1" "|"
			"f!pid-file!pid file!!1!path!1" "|"
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
		bool log = opt.contains("log") ;
		bool debug = opt.contains("debug") ;
		std::string log_file = opt.value("log-file","") ;
		bool ipv6 = opt.contains("ipv6") ;
		unsigned int port = G::Str::toUInt( opt.value("port","10020") ) ;
		std::string pid_file = opt.value("pid-file","") ;
		unsigned int idle_timeout = 30U ;

		if( !pid_file.empty() )
		{
			std::ofstream file ;
			G::File::open( file , pid_file , G::File::Text() ) ;
			file << G::Process::Id().str() << std::endl ;
		}

		G::LogOutput log_output( log , debug , log_file ) ;
		int rc = run( ipv6 , port , idle_timeout ) ;
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

