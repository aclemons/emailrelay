//
// Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// run.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "run.h"
#include "gsmtpserver.h"
#include "gsmtpclient.h"
#include "gsasl.h"
#include "gsecrets.h"
#include "geventloop.h"
#include "garg.h"
#include "gdaemon.h"
#include "gpidfile.h"
#include "gfilestore.h"
#include "gnewfile.h"
#include "gadminserver.h"
#include "gslot.h"
#include "gmonitor.h"
#include "glocal.h"
#include "groot.h"
#include "gexception.h"
#include "gprocess.h"
#include "gmemory.h"
#include "glogoutput.h"
#include "gdebug.h"
#include <iostream>
#include <exception>

//static
std::string Main::Run::versionNumber()
{
	return "1.1.1" ;
}

Main::Run::Run( Main::Output & output , const G::Arg & arg , const std::string & switch_spec ) :
	m_output(output) ,
	m_switch_spec(switch_spec) ,
	m_arg(arg)
{
}

Main::Run::~Run()
{
}

Main::Configuration Main::Run::cfg() const
{
	return cl().cfg() ;
}

std::string Main::Run::smtpIdent() const
{
	return std::string("E-MailRelay V") + versionNumber() ;
}

void Main::Run::closeFiles()
{
	if( cfg().daemon() )
	{
		const bool keep_stderr = true ;
		G::Process::closeFiles( keep_stderr ) ;
	}
}

void Main::Run::closeMoreFiles()
{
	if( cfg().closeStderr() ) // was "daemon && close-stderr", but too confusing
		G::Process::closeStderr() ;
}

bool Main::Run::prepare()
{ 
	bool do_run = false ;
	if( cl().contains("help") )
	{
		cl().showHelp( false ) ;
	}
	else if( cl().hasUsageErrors() )
	{
		cl().showUsageErrors( true ) ;
	}
	else if( cl().contains("version") )
	{
		cl().showVersion( false ) ;
	}
	else if( cl().argc() > 1U )
	{
		cl().showArgcError( true ) ;
	}
	else if( cl().hasSemanticError() )
	{
		cl().showSemanticError( true ) ;
	}
	else
	{
		do_run = true ;
	}

	// early singletons...
	//
	// (prefix,output,log,verbose-log,debug,level,timestamp,strip-context,syslog)
	m_log_output <<= new G::LogOutput( m_arg.prefix() , cfg().log() , cfg().log() , 
		cfg().verbose() , cfg().debug() , true , 
		cfg().logTimestamp() , !cfg().debug() ,
		cfg().syslog() , G::LogOutput::Mail ) ;

	return do_run ;
}

void Main::Run::run()
{
	try
	{
		runCore() ;
		G_LOG( "Main::Run::run: done" ) ;
	}
	catch( std::exception & e )
	{
		G_LOG( "Main::Run::run: exception: " << e.what() ) ;
		throw ;
	}
	catch(...)
	{
		G_LOG( "Main::Run::run: unknown exception" ) ;
		throw ;
	}
}

void Main::Run::runCore()
{
	// fqdn override option
	//
	GNet::Local::fqdn( cfg().fqdn() ) ;

	// daemonising
	//
	G::PidFile pid_file ;
	G::Process::Umask::set( G::Process::Umask::Tightest ) ;
	if( cfg().daemon() ) closeFiles() ; // before opening any sockets or message-store streams
	if( cfg().usePidFile() ) pid_file.init( G::Path(cfg().pidFile()) ) ;
	if( cfg().daemon() ) G::Daemon::detach( pid_file ) ;

	// release root privileges
	//
	G::Root::init( cfg().nobody() ) ;

	// event loop singletons
	//
	GNet::TimerList timer_list ;
	std::auto_ptr<GNet::EventLoop> event_loop(GNet::EventLoop::create()) ;
	if( ! event_loop->init() )
		throw G::Exception( "cannot initialise network layer" ) ;

	// network monitor singleton
	//
	GNet::Monitor monitor ;
	monitor.signal().connect( G::slot(*this,&Run::raiseNetworkEvent) ) ;

	// message store singleton
	//
	m_store <<= new GSmtp::FileStore( cfg().spoolDir() ) ;
	m_store->signal().connect( G::slot(*this,&Run::raiseStoreEvent) ) ;
	if( cfg().useFilter() )
		GSmtp::NewFile::setPreprocessor( G::Path(cfg().filter()) ) ;

	// authentication secrets
	//
	m_client_secrets <<= new GSmtp::Secrets( cfg().clientSecretsFile() , "client" ) ;
	GSmtp::Secrets server_secrets( cfg().serverSecretsFile() , "server" ) ;

	// run as forwarding agent
	//
	if( cfg().doForwarding() )
	{
		if( m_store->empty() )
			cl().showNoop( true ) ;
		else
			doForwarding( *m_store.get() , *m_client_secrets.get() , *event_loop.get() ) ;
	}

	// run as storage daemon
	//
	if( cfg().doServing() )
	{
		doServing( *m_store.get() , server_secrets , *m_client_secrets.get() , pid_file , *event_loop.get() ) ;
	}
}

void Main::Run::doServing( GSmtp::MessageStore & store , const GSmtp::Secrets & server_secrets , 
	const GSmtp::Secrets & client_secrets , G::PidFile & pid_file , GNet::EventLoop & event_loop )
{
	std::auto_ptr<GSmtp::Server> smtp_server ;
	if( cfg().doSmtp() )
	{
		GSmtp::Server::AddressList interfaces ;
		if( cfg().interface_().length() )
			interfaces.push_back( GNet::Address(cfg().interface_(),cfg().port()) ) ;

		smtp_server <<= new GSmtp::Server( 
			store , 
			server_secrets , 
			GSmtp::Verifier(cfg().verifier(),cfg().deliverToPostmaster(),cfg().rejectLocalMailboxes()) ,
			smtpIdent() , 
			cfg().allowRemoteClients() , 
			cfg().port() , 
			interfaces ,
			cfg().immediate() ? cfg().serverAddress() : std::string() ,
			cfg().responseTimeout() , 
			cfg().connectionTimeout() ,
			client_secrets ) ;
	}

	if( cfg().doAdmin() )
	{
		GNet::Address address = 
			cfg().interface_().length() ? 
				GNet::Address(cfg().interface_(),cfg().adminPort()) :
				GNet::Address(cfg().adminPort()) ;

		m_admin_server <<= new GSmtp::AdminServer( 
			store , 
			client_secrets , 
			address ,
			cfg().allowRemoteClients() , 
			cfg().serverAddress() ,
			cfg().responseTimeout() , 
			cfg().connectionTimeout() ,
			cfg().withTerminate() ) ;
	}

	if( cfg().doPolling() )
	{
		m_poll_timer <<= new GNet::Timer( *this ) ;
		m_poll_timer->startTimer( cfg().pollingTimeout() ) ;
	}

	{
		G::Root claim_root ;
		pid_file.commit() ;
	}

	closeMoreFiles() ;
	if( smtp_server.get() ) smtp_server->report() ;
	if( m_admin_server.get() ) m_admin_server->report() ;
	event_loop.run() ;
	m_admin_server <<= 0 ;
}

void Main::Run::doForwarding( GSmtp::MessageStore & store , const GSmtp::Secrets & secrets , 
	GNet::EventLoop & event_loop )
{
	const bool quit_on_disconnect = true ;
	GSmtp::Client client( store , secrets , quit_on_disconnect , 
		cfg().responseTimeout() ) ;

	client.doneSignal().connect( G::slot(*this,&Run::clientDone) ) ;
	client.eventSignal().connect( G::slot(*this,&Run::clientEvent) ) ;
	std::string error = client.startSending( cfg().serverAddress() , cfg().connectionTimeout() ) ;
	if( error.length() )
		throw G::Exception( error + ": " + cfg().serverAddress() ) ;

	closeMoreFiles() ;
	event_loop.run() ;
}

void Main::Run::onTimeout( GNet::Timer & timer )
{
	G_ASSERT( &timer == m_poll_timer.get() ) ;
	G_DEBUG( "Main::Run::onTimeout" ) ;

	timer.startTimer( cfg().pollingTimeout() ) ;

	if( m_client.get() && m_client->busy() )
	{
		G_LOG( "Main::Run::onTimeout: polling: still busy from last time" ) ;
		emit( "poll" , "busy" , "" ) ;
	}
	else
	{
		emit( "poll" , "start" , "" ) ;
		std::string error = doPoll() ;
		emit( "poll" , "end" , error ) ;
	}
}

std::string Main::Run::doPoll()
{
	try
	{
		G_LOG( "Main::Run::doPoll: polling" ) ;

		const bool quit_on_disconnect = false ;
		m_client <<= new GSmtp::Client( *m_store.get() , *m_client_secrets.get() , quit_on_disconnect , 
			cfg().responseTimeout() ) ;

		m_client->doneSignal().connect( G::slot(*this,&Run::clientDone) ) ;
		m_client->eventSignal().connect( G::slot(*this,&Run::clientEvent) ) ;
		std::string error = m_client->startSending( cfg().serverAddress() , cfg().connectionTimeout() ) ;
		if( error.length() && ! GSmtp::Client::nothingToSend(error) )
			G_WARNING( "Main::Run::doPoll: polling: " + error ) ;
		return error ;
	}
	catch( G::Exception & e )
	{
		G_ERROR( "Main::Run::doPoll: polling: exception: " << e.what() ) ;
		return e.what() ;
	}
}

const Main::CommandLine & Main::Run::cl() const
{
	// lazy evaluation so that the constructor doesnt throw
	if( m_cl.get() == NULL )
	{
		const_cast<Run*>(this)->m_cl <<= new CommandLine( m_output , m_arg , m_switch_spec , versionNumber() ) ;
	}
	return *m_cl.get() ;
}

void Main::Run::clientDone( std::string reason )
{
	G_DEBUG( "Main::Run::clientDone: \"" << reason << "\"" ) ;
	if( ! reason.empty() && m_client.get() == NULL )
		throw G::Exception( reason ) ;
}

void Main::Run::clientEvent( std::string s1 , std::string s2 )
{
	emit( "client" , s1 , s2 ) ;
}

void Main::Run::raiseStoreEvent( bool repoll )
{
	emit( "store" , "update" , repoll ? std::string("poll") : std::string() ) ;
	if( repoll && cfg().doPolling() && m_poll_timer.get() != NULL )
	{
		G_LOG( "Main::Run::raiseStoreEvent: polling timeout forced" ) ;
		m_poll_timer->cancelTimer() ;
		m_poll_timer->startTimer( 1U ) ; // (could use zero)
	}
}

void Main::Run::raiseNetworkEvent( std::string s1 , std::string s2 )
{
	emit( "network" , s1 , s2 ) ;
}

void Main::Run::emit( const std::string & s0 , const std::string & s1 , const std::string & s2 )
{
	try
	{
		m_signal.emit( s0 , s1 , s2 ) ;
		if( m_admin_server.get() != NULL )
		{
			m_admin_server->notify( s0 , s1 , s2 ) ;
		}
	}
	catch( std::exception & e )
	{
		G_WARNING( "Main::Run::emit: exception: " << e.what() ) ;
	}
}

G::Signal3<std::string,std::string,std::string> & Main::Run::signal()
{
	return m_signal ;
}

