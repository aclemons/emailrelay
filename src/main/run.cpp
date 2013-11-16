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
// run.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gssl.h"
#include "gsmtp.h"
#include "run.h"
#include "admin.h"
#include "gsmtpserver.h"
#include "gsmtpclient.h"
#include "gsecrets.h"
#include "geventloop.h"
#include "garg.h"
#include "gdaemon.h"
#include "gpidfile.h"
#include "gfilestore.h"
#include "gnewfile.h"
#include "gmultiserver.h"
#include "gadminserver.h"
#include "gpopserver.h"
#include "gprocessorfactory.h"
#include "gverifierfactory.h"
#include "gslot.h"
#include "gmonitor.h"
#include "glocal.h"
#include "gfile.h"
#include "gpath.h"
#include "groot.h"
#include "gexception.h"
#include "gprocess.h"
#include "gmemory.h"
#include "gtest.h"
#include "glogoutput.h"
#include "gdebug.h"
#include "legal.h"
#include <iostream>
#include <exception>
#include <utility>

#ifndef G_CAPABILITIES
	#define G_CAPABILITIES ""
#endif

std::string Main::Run::capabilities()
{
	return G_CAPABILITIES ;
}

std::string Main::Run::versionNumber()
{
	return "1.9" ;
}

Main::Run::Run( Main::Output & output , const G::Arg & arg , const std::string & switch_spec ) :
	m_output(output) ,
	m_switch_spec(switch_spec) ,
	m_arg(arg) ,
	m_client_resolver_info(std::string(),std::string()) ,
	m_prepare_error(false)
{
	m_client.doneSignal().connect( G::slot(*this,&Run::pollingClientDone) ) ;
	m_client.eventSignal().connect( G::slot(*this,&Run::clientEvent) ) ;
}

Main::Run::~Run()
{
	try
	{
		if( m_store.get() ) m_store->signal().disconnect() ;
		if( m_smtp_server.get() ) m_smtp_server->eventSignal().disconnect() ;
		m_client.doneSignal().disconnect() ;
		m_client.eventSignal().disconnect() ;

		// there are no EventLoop or TimerList instances when this 
		// destuctor runs so delete the client with ..ForExit()
		m_client.cleanupForExit() ;

		// call some more destructors early (in the correct order) 
		// within the scope of the catch
		m_forwarding_timer <<= 0 ;
		m_poll_timer <<= 0 ;
		m_smtp_server <<= 0 ;
		m_admin_server <<= 0 ;
		m_pop_secrets <<= 0 ;
		m_client_secrets <<= 0 ;
		m_store <<= 0 ;
		m_cfg <<= 0 ;
		m_cl <<= 0 ;
		m_log_output <<= 0 ;
	}
	catch(...)
	{
	}
}

bool Main::Run::prepareError() const
{
	return m_prepare_error ;
}

const Main::Configuration & Main::Run::config() const
{
	cl() ;
	return *m_cfg.get() ;
}

std::string Main::Run::smtpIdent() const
{
	return std::string("E-MailRelay V") + versionNumber() ;
}

void Main::Run::closeFiles()
{
	if( config().daemon() )
	{
		const bool keep_stderr = true ;
		G::Process::closeFiles( keep_stderr ) ;
	}
}

void Main::Run::closeMoreFiles()
{
	if( config().closeStderr() )
		G::Process::closeStderr() ;
}

bool Main::Run::hidden() const
{
	return cl().contains("hidden") ;
}

bool Main::Run::prepare()
{ 
 #ifndef USE_SMALL_CONFIG
	if( cl().contains("help") )
	{
		cl().showHelp( false ) ;
		m_prepare_error = false ;
		return false ;
	}
	else if( cl().hasUsageErrors() )
	{
		cl().showUsageErrors( true ) ;
		m_prepare_error = true ;
		return false ;
	}
	else if( cl().contains("version") )
	{
		cl().showVersion( false ) ;
		m_prepare_error = false ;
		return false ;
	}
	else if( cl().argc() > 1U )
	{
		cl().showArgcError( true ) ;
		m_prepare_error = true ;
		return false ;
	}
	else if( cl().hasSemanticError() )
	{
		cl().showSemanticError( true ) ;
		m_prepare_error = true ;
		return false ;
	}
 #endif

	// early singletons...
	//
	const Configuration & cfg = config() ;
	m_log_output <<= new G::LogOutput( m_arg.prefix() , 
		cfg.log() , // output
		cfg.log() , // with-logging
		cfg.verbose() , // with-verbose-logging
		cfg.debug() , // with-debug
		true , // with-level
		cfg.logTimestamp() , // with-timestamp
		!cfg.debug() , // strip-context
		cfg.useSyslog() , // use-syslog
		cfg.logFile() , // stderr-replacement
		G::LogOutput::Mail // facility 
	) ;

 #ifndef USE_SMALL_CONFIG
	// emit warnings for dodgy switch combinations
	cl().logSemanticWarnings() ;
 #endif

	return true ;
}

#ifndef USE_SMALL_CONFIG
void Main::Run::checkPort( bool check , const std::string & interface_ , unsigned int port )
{
	if( check )
	{
		GNet::Address address = 
			interface_.length() ?
				GNet::Address(interface_,port) :
				GNet::Address(port) ;
		GNet::Server::canBind( address , true ) ;
	}
}
#endif

void Main::Run::checkPorts() const
{
 #ifndef USE_SMALL_CONFIG
	const Configuration & cfg = config() ;
	if( cfg.doServing() )
	{
		G::Strings smtp_interfaces = cfg.listeningInterfaces("smtp") ;
		checkPort( cfg.doSmtp() && smtp_interfaces.empty() , std::string() , cfg.port() ) ;
		for( G::Strings::iterator p1 = smtp_interfaces.begin() ; p1 != smtp_interfaces.end() ; ++p1 )
			checkPort( cfg.doSmtp() , *p1 , cfg.port() ) ;

		G::Strings pop_interfaces = cfg.listeningInterfaces("pop") ;
		checkPort( cfg.doPop() && pop_interfaces.empty() , std::string() , cfg.popPort() ) ;
		for( G::Strings::iterator p2 = pop_interfaces.begin() ; p2 != pop_interfaces.end() ; ++p2 )
				checkPort( cfg.doPop() , *p2 , cfg.popPort() ) ;

		G::Strings admin_interfaces = cfg.listeningInterfaces("admin") ;
		checkPort( cfg.doAdmin() && admin_interfaces.empty() , std::string() , cfg.adminPort() ) ;
		for( G::Strings::iterator p3 = admin_interfaces.begin() ; p3 != admin_interfaces.end() ; ++p3 )
			checkPort( cfg.doAdmin() , *p3 , cfg.adminPort() ) ;
	}
 #endif
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
		G_ERROR( "Main::Run::run: " << e.what() ) ;
		throw ;
	}
	catch(...)
	{
		G_ERROR( "Main::Run::run: unknown exception" ) ;
		throw ;
	}
}

void Main::Run::runCore()
{
	const Configuration & cfg = config() ;

	// fqdn override option
	//
	std::string nul( 1U , '\0' ) ;
	if( cfg.fqdn(nul) != nul )
		GNet::Local::fqdn( cfg.fqdn() ) ;

	// tighten the umask
	//
	G::Process::Umask::set( G::Process::Umask::Tightest ) ;

	// close inherited file descriptors to avoid locking file
	// systems when running as a daemon -- this has to be done 
	// early, before opening any sockets or message-store streams
	//
	if( cfg.daemon() ) 
	{
		closeFiles() ; 
	}

	// release root privileges and extra group memberships
	//
	G::Root::init( cfg.nobody() ) ;

	// event loop singletons
	//
	GNet::TimerList timer_list ;
	std::auto_ptr<GNet::EventLoop> event_loop(GNet::EventLoop::create()) ;
	if( ! event_loop->init() )
		throw G::Exception( "cannot initialise network layer" ) ;

	// early check on socket bindability
	//
	checkPorts() ;

 #ifndef USE_NO_EXEC
	// early check on script executablity
	//
	checkScripts() ;
 #endif

	// ssl library singleton
	//
	bool ssl_active = cfg.clientTls() || cfg.clientOverTls() || !cfg.serverTlsFile().empty() ;
	GSsl::Library ssl( ssl_active , cfg.serverTlsFile() , cfg.tlsConfig() ) ;
	if( ssl_active && !ssl.enabled() )
		throw G::Exception( "cannot do tls/ssl: openssl not built in: remove tls options from the command-line or rebuild the emailrelay executable with openssl" ) ;

	// network monitor singleton
	//
	GNet::Monitor monitor ;
	monitor.signal().connect( G::slot(*this,&Run::raiseNetworkEvent) ) ;

	// message store singletons
	//
	m_store <<= new GSmtp::FileStore( cfg.spoolDir() , false , cfg.maxSize() ) ;
	m_store->signal().connect( G::slot(*this,&Run::raiseStoreEvent) ) ;
	GPop::Store pop_store( cfg.spoolDir() , cfg.popByName() , ! cfg.popNoDelete() ) ;

	// authentication secrets
	//
	m_client_secrets <<= new GAuth::Secrets( cfg.clientSecretsFile() , "client" ) ;
	GAuth::Secrets server_secrets( cfg.serverSecretsFile() , "server" ) ;
	if( cfg.doPop() )
		m_pop_secrets <<= new GPop::Secrets( cfg.popSecretsFile() ) ;

	// daemonise
	//
	G::PidFile pid_file ;
	if( cfg.usePidFile() ) pid_file.init( G::Path(cfg.pidFile()) ) ;
	if( cfg.daemon() ) G::Daemon::detach( pid_file ) ;

	// run as forwarding agent
	//
	if( cfg.doForwardingOnStartup() )
	{
		if( m_store->empty() )
			cl().showNoop( true ) ;
		else
			doForwardingOnStartup( *m_store.get() , *m_client_secrets.get() , *event_loop.get() ) ;
	}

	// run as storage daemon
	//
	if( cfg.doServing() )
	{
		doServing( *m_client_secrets.get() , *m_store.get() , server_secrets , 
			pop_store , *m_pop_secrets.get() , pid_file , *event_loop.get() ) ;
	}
}

void Main::Run::doServing( const GAuth::Secrets & client_secrets , 
	GSmtp::MessageStore & store , const GAuth::Secrets & server_secrets , 
	GPop::Store & pop_store , const GPop::Secrets & pop_secrets ,
	G::PidFile & pid_file , GNet::EventLoop & event_loop )
{
	const Configuration & cfg = config() ;
	if( cfg.doSmtp() )
	{
		if( cfg.immediate() )
			G_WARNING( "Run::doServing: using --immediate can result in client timeout errors: try --poll=0 instead" ) ;

		m_smtp_server <<= new GSmtp::Server( 
			store , 
			client_secrets ,
			server_secrets , 
			serverConfig() ,
			cfg.immediate() ? cfg.serverAddress() : std::string() ,
			cfg.connectionTimeout() ,
			clientConfig() ) ;

		m_smtp_server->eventSignal().connect( G::slot(*this,&Run::serverEvent) ) ;
	}

 #ifndef USE_NO_POP
	std::auto_ptr<GPop::Server> pop_server ;
	if( cfg.doPop() )
	{
		pop_server <<= new GPop::Server( pop_store , pop_secrets , popConfig() ) ;
	}
 #endif

 #ifndef USE_NO_ADMIN
	if( cfg.doAdmin() )
	{
		std::auto_ptr<GSmtp::AdminServer> admin_server = Admin::newServer( cfg , store , clientConfig() , 
			client_secrets , versionNumber() ) ;
		m_admin_server <<= admin_server.release() ;
	}
 #endif

	if( cfg.doPolling() )
	{
		m_poll_timer <<= new GNet::Timer<Run>(*this,&Run::onPollTimeout,*this) ;
		m_poll_timer->startTimer( cfg.pollingTimeout() ) ;
	}

	if( cfg.forwardingOnStore() || cfg.forwardingOnDisconnect() )
	{
		m_forwarding_timer <<= new GNet::Timer<Run>(*this,&Run::onForwardingTimeout,*this) ;
	}

	{
		// dont change the effective group id here -- create the pid file with the 
		// unprivileged group ownership so that it can be deleted more easily
		G::Root claim_root(false) ; 
		pid_file.commit() ;
	}

	closeMoreFiles() ;
	if( m_smtp_server.get() ) m_smtp_server->report() ;
	if( m_admin_server.get() ) Admin::report( *m_admin_server.get() ) ;
 #ifndef USE_NO_POP
	if( pop_server.get() ) pop_server->report() ;
 #endif

	event_loop.run() ;

	if( m_smtp_server.get() ) m_smtp_server->eventSignal().disconnect() ;
	m_smtp_server <<= 0 ;
	m_admin_server <<= 0 ;
}

void Main::Run::doForwardingOnStartup( GSmtp::MessageStore & store , const GAuth::Secrets & secrets , 
	GNet::EventLoop & event_loop )
{
	const Configuration & cfg = config() ;

	GNet::ClientPtr<GSmtp::Client> client_ptr( new GSmtp::Client( 
		GNet::ResolverInfo(cfg.serverAddress()) , secrets , clientConfig() ) ) ;

	client_ptr->sendMessagesFrom( store ) ; // once connected

	// quit() the event loop when all done so that run() returns
	client_ptr.doneSignal().connect( G::slot(*this,&Run::forwardingClientDone) ) ;

	// emit progress events
	client_ptr.eventSignal().connect( G::slot(*this,&Run::clientEvent) ) ;

	closeMoreFiles() ;
	std::string quit_reason = event_loop.run() ;
	if( !quit_reason.empty() )
		cl().showError( quit_reason ) ;
}

GSmtp::Server::Config Main::Run::serverConfig() const
{
	const Configuration & cfg = config() ;
	return
		GSmtp::Server::Config(
			cfg.allowRemoteClients() , 
			cfg.port() , 
			GNet::MultiServer::addressList( cfg.listeningInterfaces("smtp") , cfg.port() ) ,
			smtpIdent() , 
			cfg.anonymous() ,
			cfg.filter() ,
			cfg.filterTimeout() ,
			cfg.verifier() ,
			cfg.filterTimeout() , // verifier timeout - re-use filter timeout value
			cfg.peerLookup() ) ; // use connection table to get username of local peers
}

#ifndef USE_NO_POP
GPop::Server::Config Main::Run::popConfig() const
{
	const Configuration & cfg = config() ;
	return GPop::Server::Config( cfg.allowRemoteClients() , cfg.popPort() , cfg.listeningInterfaces("pop") ) ;
}
#endif

GSmtp::Client::Config Main::Run::clientConfig() const
{
	const Configuration & cfg = config() ;

	return
		GSmtp::Client::Config(
			cfg.clientFilter() ,
			cfg.filterTimeout() ,
			clientBindAddress(cfg.clientInterface()) ,
			GSmtp::ClientProtocol::Config(
				GNet::Local::fqdn() ,
				cfg.responseTimeout() , 
				cfg.promptTimeout() , // waiting for "service ready"
				cfg.filterTimeout() ,
				true , // (must-authenticate)
				true , // (must-accept-all-recipients)
				false ) , // (eight-bit-strict)
			cfg.connectionTimeout() ,
			cfg.secureConnectionTimeout() ,
			cfg.clientOverTls() ) ;
}

GNet::Address Main::Run::clientBindAddress( const std::string & s )
{
	return s.empty() ?
			GNet::Address(0U) : (
				GNet::Address::validString( s ) ?
					GNet::Address( s ) :
					GNet::Address( s , 0U ) ) ;
}

void Main::Run::onException( std::exception & e )
{
	// gets here if onTimeout() throws
	G_ERROR( "Main::Run::onException: exception while forwarding: " << e.what() ) ;
}

void Main::Run::onPollTimeout()
{
	G_DEBUG( "Main::Run::onPollTimeout" ) ;
	m_poll_timer->startTimer( config().pollingTimeout() ) ;
	doForwarding( "poll" ) ;
}

void Main::Run::onForwardingTimeout()
{
	G_DEBUG( "Main::Run::onForwardingTimeout" ) ;
	doForwarding( "forward" ) ;
}

void Main::Run::doForwarding( const std::string & event_key )
{
	if( m_client.busy() )
	{
		G_LOG( "Main::Run::doForwarding: " << event_key << ": still busy from last time" ) ;
		emit( event_key , "busy" , "" ) ;
	}
	else
	{
		emit( event_key , "start" , "" ) ;
		std::string error = doForwardingCore() ;
		emit( event_key , "end" , error ) ;
	}
}

std::string Main::Run::doForwardingCore()
{
	try
	{
		const Configuration & cfg = config() ;

		G_DEBUG( "Main::Run::doForwarding: polling" ) ;
		if( cfg.pollingLog() )
			G_LOG( "Main::Run::doForwarding: polling" ) ;

		if( ! m_store->empty() )
		{
			m_client.reset( new GSmtp::Client( GNet::ResolverInfo(cfg.serverAddress()) ,
				*m_client_secrets.get() , clientConfig() ) ) ;

			m_client->sendMessagesFrom( *m_store.get() ) ; // once connected
		}
		return std::string() ;
	}
	catch( std::exception & e )
	{
		G_ERROR( "Main::Run::doForwarding: polling: " << e.what() ) ;
		return e.what() ;
	}
}

const Main::CommandLine & Main::Run::cl() const
{
	// lazy evaluation so that the constructor doesnt throw
	if( m_cl.get() == NULL )
	{
		const_cast<Run*>(this)->m_cl <<= new CommandLine( m_output , m_arg , m_switch_spec , versionNumber() , capabilities() ) ;
		const_cast<Run*>(this)->m_cfg <<= new Configuration( cl().cfg() ) ;
	}
	return *m_cl.get() ;
}

void Main::Run::forwardingClientDone( std::string reason , bool )
{
	G_DEBUG( "Main::Run::forwardingClientDone: \"" << reason << "\"" ) ;
	GNet::EventLoop::instance().quit( reason ) ;
}

void Main::Run::pollingClientDone( std::string reason , bool )
{
	G_DEBUG( "Main::Run::pollingClientDone: \"" << reason << "\"" ) ;
	if( ! reason.empty() )
	{
		G_ERROR( "Main::Run::pollingClientDone: polling: " << reason ) ;
	}
}

void Main::Run::clientEvent( std::string s1 , std::string s2 )
{
	emit( "client" , s1 , s2 ) ;
}

void Main::Run::serverEvent( std::string s1 , std::string )
{
	if( s1 == "done" && config().forwardingOnDisconnect() )
	{
		G_ASSERT( m_forwarding_timer.get() != NULL ) ;
		m_forwarding_timer->cancelTimer() ;
		m_forwarding_timer->startTimer( 0U ) ;
	}
}

void Main::Run::raiseStoreEvent( bool repoll )
{
	emit( "store" , "update" , repoll ? std::string("poll") : std::string() ) ;

	const bool expiry_forced_by_filter = repoll ;
	if( config().doPolling() && expiry_forced_by_filter )
	{
		G_ASSERT( m_poll_timer.get() != NULL ) ;
		m_poll_timer->cancelTimer() ;
		m_poll_timer->startTimer( 0U ) ;
	}

	if( config().forwardingOnStore() )
	{
		G_ASSERT( m_forwarding_timer.get() != NULL ) ;
		m_forwarding_timer->cancelTimer() ;
		m_forwarding_timer->startTimer( 0U ) ;
	}
}

void Main::Run::raiseNetworkEvent( std::string s1 , std::string s2 )
{
	emit( "network" , s1 , s2 ) ;
}

void Main::Run::emit( const std::string & s0 , const std::string & s1 , const std::string & s2 )
{
 #ifndef USE_NO_ADMIN
	try
	{
		m_signal.emit( s0 , s1 , s2 ) ;
		if( m_admin_server.get() != NULL )
		{
			Admin::notify( *m_admin_server.get() , s0 , s1 , s2 ) ;
		}
	}
	catch( std::exception & e )
	{
		G_WARNING( "Main::Run::emit: " << e.what() ) ;
	}
 #endif
}

G::Signal3<std::string,std::string,std::string> & Main::Run::signal()
{
	return m_signal ;
}

#ifndef USE_NO_EXEC
void Main::Run::checkScripts() const
{
	const Configuration & cfg = config() ;
	checkProcessorScript( cfg.filter() ) ;
	checkProcessorScript( cfg.clientFilter() ) ;
	checkVerifierScript( cfg.verifier() ) ;
}
void Main::Run::checkVerifierScript( const std::string & s ) const
{
	std::string reason = GSmtp::VerifierFactory::check( s ) ;
	if( !reason.empty() )
	{
		G_WARNING( "Main::Run::checkScript: invalid verifier \"" << s << "\": " << reason ) ;
	}
}
void Main::Run::checkProcessorScript( const std::string & s ) const
{
	std::string reason = GSmtp::ProcessorFactory::check( s ) ;
	if( !reason.empty() )
	{
		G_WARNING( "Main::Run::checkScript: invalid preprocessor \"" << s << "\": " << reason ) ;
	}
}
#endif

/// \file run.cpp
