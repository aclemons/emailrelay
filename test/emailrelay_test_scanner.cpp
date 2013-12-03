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
// emailrelay_test_scanner.cpp
//
// A dummy network processor for testing "emailrelay --filter net:<host>:<port>".
//
// usage: emailrelay-test-scanner [--log] [--log-file <file>] [--debug] [--pid-file <pidfile>]
//
// Listens on port 10020 by default. Each request is a filename and the message
// file is treated as a mini script with commands of:
//  * send [<string>]
//  * sleep [<time>]
//  * delete-content
//  * delete-envelope
//  * disconnect
//  * terminate
//

#include "gdef.h"
#include "gnet.h"
#include "gserver.h"
#include "glinebuffer.h"
#include "gstr.h"
#include "gevent.h"
#include "gprocess.h"
#include "gmemory.h"
#include "garg.h"
#include "gfile.h"
#include "gsleep.h"
#include "gdebug.h"
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
	explicit ScannerPeer( GNet::Server::PeerInfo info ) ;
private:	
	virtual void onDelete( const std::string & ) ;
	virtual void onData( const char * , GNet::ServerPeer::size_type ) ;
	virtual void onSecure( const std::string & ) ;
	virtual void onSendComplete() ;
	void process() ;
	bool processFile( std::string ) ;
private:
	GNet::LineBuffer m_buffer ;
} ;

Main::ScannerPeer::ScannerPeer( GNet::Server::PeerInfo info ) :
	ServerPeer( info )
{
	G_LOG_S( "ScannerPeer::ctor: new connection from " << info.m_address.displayString() ) ;
}

void Main::ScannerPeer::onDelete( const std::string & )
{
	G_LOG_S( "ScannerPeer::onDelete: disconnected" ) ;
}

void Main::ScannerPeer::onSecure( const std::string & )
{
}

void Main::ScannerPeer::onData( const char * p , GNet::ServerPeer::size_type n )
{
	G_DEBUG( "ScannerPeer::onData: " << G::Str::printable(std::string(p,n)) ) ;
	m_buffer.add( p , n ) ;
	process() ;
}

void Main::ScannerPeer::onSendComplete()
{
}

void Main::ScannerPeer::process()
{
	if( m_buffer.more() )
	{
		std::string s = m_buffer.line() ;
		if( !s.empty() )
		{
			std::string path( s ) ;
			G::Str::trim( path , " \r\n\t" ) ;
			if( !processFile( path ) )
			{
				G_LOG_S( "ScannerPeer::process: disconnecting" ) ;
				doDelete() ;
				return ;
			}
		}
	}
}

bool Main::ScannerPeer::processFile( std::string path )
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
		std::string line = G::Str::readLineFrom( file , "\n" ) ;
		G_LOG( "ScannerPeer::processFile: line: \"" << G::Str::printable(line) << "\"" ) ;
		if( line.find("send") == 0U )
		{
			line.append( 1U , '\n' ) ;
			line = line.substr(4U) ;
			G::Str::trimLeft( line , " \t" ) ;
			G_LOG_S( "ScannerPeer::processFile: response: \"" << G::Str::trimmed(line,"\n\r") << "\"" ) ;
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
		if( line.find("disconnect") == 0U )
		{
			if( sent )
				::sleep( 1U ) ; // allow the data to get down the pipe
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
		std::string response = "ok\n" ;
		G_LOG_S( "ScannerPeer::processFile: response: \"" << G::Str::trimmed(response,"\n\r") << "\"" ) ;
		socket().write( response.data() , response.length() ) ;
	}
	return true ;
}

// ===

/// \class Main::Scanner
/// A GNet::Server class used by the scanner utility.
/// 
class Main::Scanner : public GNet::Server 
{
public:
	Scanner( unsigned int port ) ;
	virtual GNet::ServerPeer * newPeer( GNet::Server::PeerInfo ) ;
} ;

Main::Scanner::Scanner( unsigned int port ) :
	GNet::Server( port )
{
}

GNet::ServerPeer * Main::Scanner::newPeer( GNet::Server::PeerInfo info )
{
	return new ScannerPeer( info ) ;
}

// ===

static int run( unsigned int port )
{
	std::auto_ptr<GNet::EventLoop> loop( GNet::EventLoop::create() ) ;
	loop->init() ;
	GNet::TimerList timer_list ;
	Main::Scanner scanner( port ) ;
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

