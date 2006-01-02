//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// scanner.cpp
//
// A dummy scanner process for testing "emailrelay --scanner" 
// (eg. "emailrelay --as-proxy localhost:10025 --scanner localhost:10010")
//
// usage: scanner [<sleep-time>]
//
// Listens on port 10010. Reports messages as infected if the content
// included the string "cough". Sleeps for <sleep-time> (default 30s)
// if the message contains the string "sleep".
//

#include "gdef.h"
#include "gnet.h"
#include "gserver.h"
#include "glinebuffer.h"
#include "gstr.h"
#include "gevent.h"
#include "gmemory.h"
#include "garg.h"
#include "gsleep.h"
#include "gdebug.h"
#include <sstream>
#include <iostream>

static int sleep_time = 30 ;

class ScannerPeer : public GNet::ServerPeer 
{
public:
	explicit ScannerPeer( GNet::Server::PeerInfo info ) ;
private:	
	virtual void onDelete() ;
	virtual void onData( const char * , size_t ) ;
	void process() ;
	void processFile( std::string ) ;
private:
	GNet::LineBuffer m_buffer ;
} ;

ScannerPeer::ScannerPeer( GNet::Server::PeerInfo info ) :
	ServerPeer( info )
{
}

void ScannerPeer::onDelete() 
{
	process() ;
}

void ScannerPeer::onData( const char * p , size_t n )
{
	std::string s( p , n ) ;
	m_buffer.add( s ) ;
	process() ;
}

void ScannerPeer::process()
{
	if( m_buffer.more() )
	{
		std::string s = m_buffer.line() ;
		if( !s.empty() )
		{
			std::string path( s ) ;
			G::Str::trim( path , " \r\n\t" ) ;
			processFile( path ) ;
		}
	}
}

void ScannerPeer::processFile( std::string path )
{
	G_LOG( "ScannerPeer::processFile: file: \"" << path << "\"" ) ;
	bool infected = false ;
	bool do_sleep = false ;
	{
		std::ifstream file( path.c_str() ) ;
		while( file.good() )
		{
			std::string line = G::Str::readLineFrom( file ) ;
			G_LOG( "ScannerPeer::processFile: line: \"" << G::Str::toPrintableAscii(line) << "\"" ) ;
			if( line.find("cough") != std::string::npos )
				infected = true ;
			if( line.find("sleep") != std::string::npos )
				do_sleep = true ;
		}
	}
	G_LOG( "ScannerPeer::processFile: infected=" << infected ) ;

	if( do_sleep && ::sleep_time )
	{
		G_LOG( "ScannerPeer::processFile: sleeping..." ) ;
		::sleep( ::sleep_time ) ;
		G_LOG( "ScannerPeer::processFile: done sleeping" ) ;
	}

	std::ostringstream ss ;
	if( infected )
	{
		ss << "the message \"" << path << "\" is infected by flu\n" ;
	}
	else
	{
		ss << "ok\n" ;
	}
	std::string s = ss.str() ;
	socket().write( s.data() , s.length() ) ;
	doDelete() ;
}

// ===

class Scanner : public GNet::Server 
{
public:
	Scanner( unsigned int port ) ;
	virtual GNet::ServerPeer * newPeer( GNet::Server::PeerInfo ) ;
} ;

Scanner::Scanner( unsigned int port ) :
	GNet::Server( port )
{
}

GNet::ServerPeer * Scanner::newPeer( GNet::Server::PeerInfo info )
{
	return new ScannerPeer( info ) ;
}

// ===

static int run()
{
	unsigned int port = 10010 ;
	std::auto_ptr<GNet::EventLoop> loop( GNet::EventLoop::create() ) ;
	Scanner scanner( port ) ;
	loop->run() ;
	return 0 ;
}

int main( int argc , char * argv [] )
{
	try
	{
		G::Arg arg( argc , argv ) ;
		if( arg.c() > 1U )
			::sleep_time = G::Str::toInt( arg.v(1U) ) ;
		bool debug = true ;
		G::LogOutput log_output( debug , debug ) ;
		return run() ;
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

