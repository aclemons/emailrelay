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
/// \file emailrelay_test_scanner.cpp
///
// A dummy network processor for testing "emailrelay --filter net:<host>:<port>".
//
// usage: emailrelay_test_scanner [--port <port-or-address>] [--log] [--log-file <file>] [--debug] [--pid-file <pidfile>]
//
// Listens on port 10020 by default. Each request is a 'content' filename
// and the file should contain a mini script with commands of:
//  * send [<string>]
//  * sleep [<time>]
//  * delete-content
//  * delete-envelope
//  * shutdown
//  * disconnect
//  * terminate
//

#include "gdef.h"
#include "gserver.h"
#include "gnetdone.h"
#include "glinebuffer.h"
#include "gevent.h"
#include "gtimerlist.h"
#include "gfile.h"
#include "gprocess.h"
#include "gscope.h"
#include "gstr.h"
#include "garg.h"
#include "gfile.h"
#include "gsleep.h"
#include "glogoutput.h"
#include "glog.h"
#include <sstream>
#include <iostream>
#include <fstream>

namespace Main
{
	class ScannerPeer ;
	class Scanner ;
}

class Main::ScannerPeer : public GNet::ServerPeer
{
public:
	ScannerPeer( GNet::EventStateUnbound esu , GNet::ServerPeerInfo && ) ;
private:
	void onDelete( const std::string & ) override ;
	bool onReceive( const char * , std::size_t , std::size_t , std::size_t , char ) override ;
	void onSecure( const std::string & , const std::string & , const std::string & ) override ;
	void onSendComplete() override ;
	bool processFile( std::string , std::string ) ;
} ;

Main::ScannerPeer::ScannerPeer( GNet::EventStateUnbound esu , GNet::ServerPeerInfo && peer_info ) :
	ServerPeer(esbind(esu,this),std::move(peer_info),GNet::LineBuffer::Config::autodetect())
{
	G_LOG_S( "ScannerPeer::ctor: new connection from " << peerAddress().displayString() ) ;
}

void Main::ScannerPeer::onDelete( const std::string & )
{
	G_LOG_S( "ScannerPeer::onDelete: disconnected" ) ;
}

void Main::ScannerPeer::onSecure( const std::string & , const std::string & , const std::string & )
{
}

bool Main::ScannerPeer::onReceive( const char * p , std::size_t n , std::size_t , std::size_t , char )
{
	G_DEBUG( "ScannerPeer::onReceive: " << G::Str::printable(std::string(p,n)) ) ;
	std::string path( p , n ) ;
	G::Str::trim( path , " \r\n\t" ) ;
	if( !processFile( path , lineBuffer().eol() ) )
	{
		G_LOG_S( "ScannerPeer::process: disconnecting" ) ;
		throw GNet::Done() ;
	}
	return true ;
}

void Main::ScannerPeer::onSendComplete()
{
}

bool Main::ScannerPeer::processFile( std::string path , std::string eol )
{
	G_LOG_S( "ScannerPeer::processFile: file: \"" << path << "\"" ) ;

	bool sent = false ;
	std::ifstream file( path.c_str() ) ;
	if( !file.good() )
	{
		G_ERROR( "ScannerPeer::processFile: cannot read \"" << path << "\"" ) ;
		return false ;
	}
	bool do_delete = false ;
	while( file.good() )
	{
		std::string line = G::Str::readLineFrom( file ) ;
		G_LOG( "ScannerPeer::processFile: line: \"" << G::Str::printable(line) << "\"" ) ;
		if( line.find("send") == 0U )
		{
			line.append( eol ) ;
			line = line.substr(4U) ;
			G::Str::trimLeft( line , " \t" ) ;
			G_LOG_S( "ScannerPeer::processFile: response: \"" << G::Str::printable(line) << "\"" ) ;
			socket().write( line.data() , line.length() ) ;
			sent = true ;
		}
		if( line.find("delete-content") == 0U )
		{
			do_delete = true ;
		}
		if( line.find("delete-envelope") == 0U )
		{
			std::string envelope_path( path ) ;
			G::Str::replace( envelope_path , ".content" , ".envelope.new" ) ;
			G_LOG_S( "ScannerPeer::processFile: deleting envelope file [" << envelope_path << "]" ) ;
			G::File::remove( envelope_path ) ;
		}
		if( line.find("sleep") == 0U )
		{
			unsigned int sleep_time = 4U ;
			try
			{
				sleep_time = G::Str::toUInt(G::Str::trimmed(line,"slep \t\r\n")) ;
			}
			catch( std::exception & )
			{
			}
			G_LOG_S( "ScannerPeer::processFile: sleeping: " << sleep_time ) ;
			::sleep( sleep_time ) ;
		}
		if( line.find("shutdown") == 0U )
		{
			socket().shutdown() ; // no more sends (FIN)
		}
		if( line.find("disconnect") == 0U )
		{
			return false ;
		}
		if( line.find("terminate") == 0U )
		{
			GNet::EventLoop::instance().quit("terminate") ;
			return false ;
		}
	}
	file.close() ;
	if( do_delete )
	{
		G_LOG_S( "ScannerPeer::processFile: deleting content file [" << path << "]" ) ;
		G::File::remove( path ) ;
	}
	if( !sent )
	{
		std::string response = "ok" + eol ;
		G_LOG_S( "ScannerPeer::processFile: response: \"" << G::Str::printable(response) << "\"" ) ;
		socket().write( response.data() , response.length() ) ;
	}
	return true ;
}

// ===

class Main::Scanner : public GNet::Server
{
public:
	Scanner( GNet::EventState , const GNet::Address & , unsigned int idle_timeout ) ;
	~Scanner() override ;
	std::unique_ptr<GNet::ServerPeer> newPeer( GNet::EventStateUnbound ebu , GNet::ServerPeerInfo && ) override ;
} ;

Main::Scanner::Scanner( GNet::EventState es , const GNet::Address & address , unsigned int idle_timeout ) :
	GNet::Server(es,address,
		GNet::ServerPeer::Config().set_idle_timeout(idle_timeout),
		GNet::Server::Config().set_uds_open_permissions())
{
	G_LOG_S( "Scanner::ctor: listening on " << address.displayString() ) ;
}

Main::Scanner::~Scanner()
{
	serverCleanup() ; // base class early cleanup
}

std::unique_ptr<GNet::ServerPeer> Main::Scanner::newPeer( GNet::EventStateUnbound esu , GNet::ServerPeerInfo && peer_info )
{
	try
	{
		return std::unique_ptr<GNet::ServerPeer>( new ScannerPeer( esu , std::move(peer_info) ) ) ;
	}
	catch( std::exception & e )
	{
		G_WARNING( "Scanner::newPeer: new connection error: " << e.what() ) ;
		return std::unique_ptr<GNet::ServerPeer>() ;
	}
}

// ===

static int run( const GNet::Address & address , unsigned int idle_timeout )
{
	auto event_loop = GNet::EventLoop::create() ;
	auto es = GNet::EventState::create() ;
	GNet::TimerList timer_list ;
	Main::Scanner scanner( es , address , idle_timeout ) ;
	event_loop->run() ;
	return 0 ;
}

int main( int argc , char * argv [] )
{
	std::string pid_file ;
	try
	{
		G::Arg arg( argc , argv ) ;
		bool log = arg.remove("--log") ;
		bool debug = arg.remove("--debug") ;
		std::string log_file = arg.index("--log-file",1U) ? arg.v(arg.index("--log-file",1U)+1U) : std::string() ;
		std::string port_str = arg.index("--port",1U) ? arg.v(arg.index("--port",1U)+1U) : std::string("10020") ;
		pid_file = arg.index("--pid-file",1U) ? arg.v(arg.index("--pid-file",1U)+1U) : std::string() ;
		unsigned int idle_timeout = 10U ;

		GNet::Address address = G::Str::isNumeric(port_str) ?
			GNet::Address( GNet::Address::Family::ipv4 , G::Str::toUInt(port_str) ) :
			GNet::Address::parse( port_str ) ;

		if( !pid_file.empty() )
		{
			std::ofstream file ;
			G::File::open( file , pid_file , G::File::Text() ) ;
			file << G::Process::Id().str() << std::endl ;
		}

		G::LogOutput log_output( arg.prefix() ,
			G::LogOutput::Config()
				.set_output_enabled()
				.set_summary_info(log||debug)
				.set_verbose_info(log||debug)
				.set_debug(debug)
				.set_with_level(true) ,
			log_file ) ;

		int rc = run( address , idle_timeout ) ;
		std::cout << "done" << std::endl ;
		std::remove( pid_file.c_str() ) ;
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
	std::remove( pid_file.c_str() ) ;
	return 1 ;
}

