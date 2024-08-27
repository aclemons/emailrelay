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
/// \file emailrelay_test_verifier.cpp
///
// A dummy address verifier for testing purposes.
//
// usage: emailrelay_test_verifier [--ipv6] [--port <port>] [--log] [--log-file <file>] [--debug] [--pid-file <pidfile>]
//
// Works as a network verifier if any options are given, or an executable verifier if none.
//
// The verification action is dictated by special sub-strings in the recipient address:
// * OK -- verify as remote ("1|<rcpt-to>")
// * L -- verify as local ("0|<user>|<user>")
// * A -- verify as remote 'alice' (with OK) or local 'alice' (with L) ("1|alice@<domain>" or "0|alice|alice")
// * B -- verify as remote 'bob' (with OK) or local 'bob' (with L) ("1|bob@<domain>" or "0|bob|bob")
// * C -- make the response lower-case
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
#include "goptionsusage.h"
#include "garg.h"
#include "gsleep.h"
#include "glogoutput.h"
#include "glog.h"
#include <sstream>
#include <iostream>
#include <fstream>
#include <stdexcept>

struct Request
{
	explicit Request( const G::StringArray & arg )
	{
		if( arg.size() < 6U )
			throw std::runtime_error( "invalid verifier request" ) ;
		rcpt_to = arg.at(0U) ;
		mail_from = arg.at(1U) ;
		ip_address = arg.at(2U) ;
		this_domain = arg.at(3U) ;
		auth_mechanism = arg.at(4U) ;
		auth_name = arg.at(5U) ;
		if( rcpt_to.empty() || mail_from.empty() || ip_address.empty() || this_domain.empty() || auth_mechanism.empty() )
			throw std::runtime_error( "invalid verifier request" ) ;
		//
		user_part = G::Str::head( rcpt_to , "@" , false ) ;
		std::string domain = G::Str::tail( rcpt_to , "@" , true ) ;
		at_domain = domain.empty() ? std::string() : ("@"+domain) ;
		//
		valid_remote = rcpt_to.find("OK") != std::string::npos ;
		valid_local = rcpt_to.find("L") != std::string::npos ;
		alice = rcpt_to.find("A") != std::string::npos ;
		bob = rcpt_to.find("B") != std::string::npos ;
		lowercase = rcpt_to.find("C") != std::string::npos ;
		blackhole = rcpt_to.find("X") != std::string::npos ;
		disconnect = rcpt_to.find("x") != std::string::npos ;
		abort = rcpt_to.find("!") != std::string::npos ;
		//
		if( lowercase )
		{
			G::Str::toLower( at_domain ) ;
			G::Str::toLower( rcpt_to ) ;
		}
	}
	std::string rcpt_to ;
	std::string mail_from ;
	std::string ip_address ;
	std::string this_domain ;
	std::string auth_mechanism ;
	std::string auth_name ;
	//
	std::string user_part ;
	std::string at_domain ;
	//
	bool valid_remote {false} ;
	bool valid_local {false} ;
	bool alice {false} ;
	bool bob {false} ;
	bool lowercase {false} ;
	bool blackhole {false} ;
	bool disconnect {false} ;
	bool abort {false} ;
} ;

class VerifierPeer : public GNet::ServerPeer
{
public:
	VerifierPeer( GNet::EventStateUnbound , GNet::ServerPeerInfo && ) ;
private:
	void onDelete( const std::string & ) override ;
	bool onReceive( const char * , std::size_t , std::size_t , std::size_t , char ) override ;
	void onSecure( const std::string & , const std::string & , const std::string & ) override ;
	void onSendComplete() override {}
	bool processLine( std::string ) ;
	bool processRequest( const Request & ) ;
} ;

VerifierPeer::VerifierPeer( GNet::EventStateUnbound esu , GNet::ServerPeerInfo && peer_info ) :
	GNet::ServerPeer( esbind(esu,this) , std::move(peer_info) , GNet::LineBuffer::Config::autodetect() )
{
	G_LOG_S( "VerifierPeer::ctor: new connection from " << peerAddress().displayString() ) ;
}

void VerifierPeer::onDelete( const std::string & )
{
	G_LOG_S( "VerifierPeer::onDelete: disconnected" ) ;
}

void VerifierPeer::onSecure( const std::string & , const std::string & , const std::string & )
{
}

bool VerifierPeer::onReceive( const char * line_data , std::size_t line_size , std::size_t , std::size_t , char )
{
	std::string line( line_data , line_size ) ;
	G_DEBUG( "VerifierPeer::onReceive: [" << G::Str::printable(line) << "]" ) ;
	processLine( line ) ;
	return true ;
}

bool VerifierPeer::processLine( std::string line )
{
	G_LOG_S( "VerifierPeer::processLine: line: \"" << line << "\"" ) ;
	G::StringArray parts ;
	G::Str::splitIntoFields( line , parts , '|' ) ;
	return processRequest( Request(parts) ) ;
}

bool VerifierPeer::processRequest( const Request & request )
{
	if( request.abort )
	{
		G_LOG_S( "VerifierPeer::processRequest: got '!': sending 100" ) ;
		send( std::string("100\n") ) ; // GNet::ServerPeer::send()
	}
	else if( request.blackhole )
	{
		G_LOG_S( "VerifierPeer::processRequest: got 'X': sending nothing" ) ;
	}
	else if( request.disconnect )
	{
		G_LOG_S( "VerifierPeer::processRequest: got 'x': disconnecting" ) ;
		throw std::runtime_error( "disconnection" ) ;
	}
	else if( request.valid_local && request.alice )
	{
		G_LOG_S( "VerifierPeer::processRequest: got 'A' and 'L': sending valid local [alice]" ) ;
		send( std::string("0|alice|alice\n") ) ; // GNet::ServerPeer::send()
	}
	else if( request.valid_local && request.bob )
	{
		G_LOG_S( "VerifierPeer::processRequest: got 'B' and 'L': sending valid local [bob]" ) ;
		send( std::string("0|bob|bob\n") ) ; // GNet::ServerPeer::send()
	}
	else if( request.valid_local )
	{
		G_LOG_S( "VerifierPeer::processRequest: got 'L': sending valid local [" << request.user_part << "]" ) ;
		send( "0|"+request.user_part+"|"+request.user_part+"\n" ) ; // GNet::ServerPeer::send()
	}
	else if( request.valid_remote && request.alice )
	{
		G_LOG_S( "VerifierPeer::processRequest: got 'A' and 'OK': sending valid remote [alice" << request.at_domain << "]" ) ;
		send( "1|alice"+request.at_domain+"\n" ) ; // GNet::ServerPeer::send()
	}
	else if( request.valid_remote && request.bob )
	{
		G_LOG_S( "VerifierPeer::processRequest: got 'B' and 'OK': sending valid remote [bob" << request.at_domain << "]" ) ;
		send( "1|bob"+request.at_domain+"\n" ) ; // GNet::ServerPeer::send()
	}
	else if( request.valid_remote )
	{
		G_LOG_S( "VerifierPeer::processRequest: got 'OK': sending valid remote [" << request.rcpt_to << "]" ) ;
		send( "1|"+request.rcpt_to+"\n" ) ; // GNet::ServerPeer::send()
	}
	else
	{
		G_LOG_S( "VerifierPeer::processRequest: no special directives in [" << request.rcpt_to << "]: sending error response code 2" ) ;
		send( std::string("2|VerifierError\n") ) ; // GNet::ServerPeer::send()
	}
	return true ;
}

int processRequest( const Request & request )
{
	if( request.valid_local && request.alice )
	{
		std::cout
			<< "alice" << "\n"
			<< "alice" << std::endl ;
		return 0 ;
	}
	else if( request.valid_local && request.bob )
	{
		std::cout
			<< "bob" << "\n"
			<< "bob" << std::endl ;
		return 0 ;
	}
	else if( request.valid_local )
	{
		std::cout
			<< request.user_part << "\n"
			<< request.user_part << std::endl ;
		return 0 ;
	}
	else if( request.valid_remote && request.alice )
	{
		std::cout
			<< "" << "\n"
			<< ("alice"+request.at_domain) << std::endl ;
		return 1 ;
	}
	else if( request.valid_remote && request.bob )
	{
		std::cout
			<< "" << "\n"
			<< ("bob"+request.at_domain) << std::endl ;
		return 1 ;
	}
	else if( request.valid_remote )
	{
		std::cout
			<< "" << "\n"
			<< request.rcpt_to << std::endl ;
		return 1 ;
	}
	else
	{
		std::cout << "VerifierError" << std::endl ;
		return 2 ;
	}
}

// ===

class Verifier : public GNet::Server
{
public:
	Verifier( GNet::EventState , bool ipv6 , unsigned int port , unsigned int idle_timeout ) ;
	~Verifier() override ;
	std::unique_ptr<GNet::ServerPeer> newPeer( GNet::EventStateUnbound , GNet::ServerPeerInfo && ) override ;
} ;

Verifier::Verifier( GNet::EventState es , bool ipv6 , unsigned int port , unsigned int idle_timeout ) :
	GNet::Server(es,
		GNet::Address(ipv6?GNet::Address::Family::ipv6:GNet::Address::Family::ipv4,port),
		GNet::ServerPeer::Config()
			.set_all_timeouts(idle_timeout),
		GNet::Server::Config())
{
}

Verifier::~Verifier()
{
	serverCleanup() ; // base class early cleanup
}

std::unique_ptr<GNet::ServerPeer> Verifier::newPeer( GNet::EventStateUnbound esu , GNet::ServerPeerInfo && peer_info )
{
	try
	{
		return std::make_unique<VerifierPeer>( esu , std::move(peer_info) ) ;
	}
	catch( std::exception & e )
	{
		G_WARNING( "Verifier::newPeer: new connection error: " << e.what() ) ;
		return std::unique_ptr<GNet::ServerPeer>() ;
	}
}

// ===

static int runAsExe( const G::StringArray & args )
{
	if( args.size() != 6U )
		throw std::runtime_error( "usage error" ) ;
	return ::processRequest( Request(args) ) ;
}

static int runAsServer( bool ipv6 , unsigned int port , unsigned int idle_timeout )
{
	auto loop = GNet::EventLoop::create() ;
	auto es = GNet::EventState::create() ;
	GNet::TimerList timer_list ;
	Verifier verifier( es , ipv6 , port , idle_timeout ) ;
	loop->run() ;
	return 0 ;
}

int main( int argc , char * argv [] )
{
	try
	{
		G::Arg arg( argc , argv ) ;
		G::Options options ;
		using M = G::Option::Multiplicity ;
		G::Options::add( options , 'h' , "help" , "show help" , "" , M::zero , "" , 1 , 0 ) ;
		G::Options::add( options , 'l' , "log" , "logging" , "" , M::zero , "" , 1 , 0 ) ;
		G::Options::add( options , 'f' , "log-file" , "log file" , "" , M::one , "file" , 1 , 0 ) ;
		G::Options::add( options , 'd' , "debug" , "more logging" , "" , M::zero , "" , 1 , 0 ) ;
		G::Options::add( options , '6' , "ipv6" , "use ipv6" , "" , M::zero , "" , 1 , 0 ) ;
		G::Options::add( options , 'P' , "port" , "port number" , "" , M::one , "port" , 1 , 0 ) ;
		G::Options::add( options , 'f' , "pid-file" , "pid file" , "" , M::one , "path" , 1 , 0 ) ;
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

		G::LogOutput log_output( arg.prefix() ,
			G::LogOutput::Config()
				.set_output_enabled(log)
				.set_summary_info(log)
				.set_verbose_info(debug)
				.set_debug(debug) ,
			log_file ) ;

		bool no_options = opt.args().c() == std::size_t(argc) ;
		if( no_options )
		{
			int rc = runAsExe( arg.array(1U) ) ;
			return rc ;
		}
		else
		{
			int rc = runAsServer( ipv6 , port , idle_timeout ) ;
			std::cout << "done" << std::endl ;
			return rc ;
		}
	}
	catch( std::exception & e )
	{
		std::cerr << G::Arg::prefix(argv) << ": error: " << e.what() << std::endl ;
	}
	catch(...)
	{
		std::cerr << G::Arg::prefix(argv) << ": error\n" ;
	}
	return 11 ;
}

