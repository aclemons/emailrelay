//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gfilterfactory.h"
#include "gverifierfactory.h"
#include "gslot.h"
#include "gmonitor.h"
#include "glocal.h"
#include "gfile.h"
#include "gpath.h"
#include "groot.h"
#include "gexception.h"
#include "gprocess.h"
#include "gtest.h"
#include "glogoutput.h"
#include "gdebug.h"
#include "gmacros.h"
#include "legal.h"
#include <vector>
#include <iostream>
#include <exception>
#include <utility>

#ifndef GCONFIG_CONFIGURATION
#define GCONFIG_CONFIGURATION
#endif

std::string Main::Run::buildConfiguration()
{
	std::string result = G_STR(GCONFIG_CONFIGURATION) ;
	if( result == "windows" )
	{
		result.append( sizeof(void*) == 8 ? " 64-bit" : " 32-bit" ) ;
	}
	return result ;
}

std::string Main::Run::versionNumber()
{
	return "2.0.1" ;
}

Main::Run::Run( Main::Output & output , const G::Arg & arg , const std::string & option_spec ) :
	m_output(output) ,
	m_eh_throw(true) ,
	m_eh_nothrow(false) ,
	m_option_spec(option_spec) ,
	m_arg(Configuration::backwardsCompatibilityFixup(arg)) ,
	m_forwarding_pending(false) ,
	m_quit_when_sent(false)
{
	m_client.doneSignal().connect( G::Slot::slot(*this,&Run::onClientDone) ) ;
	m_client.eventSignal().connect( G::Slot::slot(*this,&Run::onClientEvent) ) ;
}

void Main::Run::configure()
{
	// lazy construction so that the constructor doesn't throw
	m_commandline.reset( new CommandLine( m_output ,
		m_arg , m_option_spec , versionNumber() , buildConfiguration() ) ) ;
	m_configuration.reset( new Configuration(m_commandline->options(),m_commandline->map(),appDir(),G::Process::cwd()) ) ;
}

Main::Run::~Run()
{
	try
	{
		// disconnect slots and signals
		if( m_smtp_server.get() ) m_smtp_server->eventSignal().disconnect() ;
		if( m_store.get() ) m_store->messageStoreRescanSignal().disconnect() ;
		if( m_store.get() ) m_store->messageStoreUpdateSignal().disconnect() ;
		if( m_monitor.get() ) m_monitor->signal().disconnect() ;
		m_client.doneSignal().disconnect() ;
		m_client.eventSignal().disconnect() ;

		// call some destructors early, within the catch block
		m_client.resetForExit() ;
		m_admin_server.reset() ;
		m_pop_server.reset() ;
		m_smtp_server.reset() ;
		m_pop_secrets.reset() ;
		m_server_secrets.reset() ;
		m_client_secrets.reset() ;
		m_store.reset() ;
		m_monitor.reset() ;
		m_tls_library.reset() ;
		m_stop_timer.reset() ;
		m_poll_timer.reset() ;
		m_forwarding_timer.reset() ;
		//m_timer_list.reset() ;
		//m_event_loop.reset() ;
		//m_log_output.reset() ;
		m_configuration.reset() ;
		m_commandline.reset() ;
	}
	catch(...)
	{
	}
}

bool Main::Run::runnable()
{
	if( commandline().map().contains("help") )
	{
		commandline().showHelp( false ) ;
		return true ;
	}
	else if( commandline().hasUsageErrors() )
	{
		commandline().showUsageErrors( true ) ;
		return false ;
	}
	else if( commandline().map().contains("version") )
	{
		commandline().showVersion( false ) ;
		return true ;
	}
	else if( commandline().argc() > 1U )
	{
		commandline().showArgcError( true ) ;
		return false ;
	}

	if( !configuration().semanticError().empty() )
	{
		commandline().showSemanticError( configuration().semanticError() ) ;
		return false ;
	}

	if( m_output.simpleOutput() && !configuration().semanticWarnings().empty() )
	{
		commandline().showSemanticWarnings( configuration().semanticWarnings() ) ;
	}

	if( commandline().map().contains("test") )
	{
		G::Test::set( commandline().map().value("test") ) ;
	}

	return true ;
}

void Main::Run::run()
{
	if( commandline().map().contains("help") || commandline().map().contains("version") )
		return ;

	// override for local host's canonical network name
	//
	bool network_name_defined = configuration().networkName(std::string(1U,'\0')) != std::string(1U,'\0') ;
	if( network_name_defined )
		GNet::Local::canonicalName( configuration().networkName() ) ; // set the override

	// tighten the umask
	//
	G::Process::Umask::set( G::Process::Umask::Tightest ) ;

	// close inherited file descriptors to avoid locking file systems
	// when running as a daemon -- this has to be done early, before
	// opening any sockets or message-store streams
	//
	if( configuration().daemon() )
	{
		closeFiles() ;
	}

	// open log file and/or syslog after closeFiles()
	//
	m_log_output.reset( new G::LogOutput( m_arg.prefix() ,
		configuration().log() , // output
		configuration().log() , // summary-logging
		configuration().verbose() , // verbose-logging
		configuration().debug() , // debug
		true , // with-level
		configuration().logTimestamp() , // with-timestamp
		!configuration().debug() , // strip-context
		configuration().useSyslog() , // use-syslog
		configuration().logFile().str() , // stderr-replacement
		G::LogOutput::Mail // facility
	) ) ;
	if( commandline().map().contains("test") )
		G::LogOutput::groups( commandline().map().value("test") ) ;
	if( configuration().useSyslog() && configuration().daemon() &&
		configuration().closeStderr() && configuration().logFile() == G::Path() )
			m_log_output->quiet() ;

	// log command-line warnings
	//
	if( !m_output.simpleOutput() )
	{
		commandline().logSemanticWarnings( configuration().semanticWarnings() ) ;
	}

	// release root privileges and extra group memberships
	//
	G::Root::init( configuration().nobody() ) ;

	// create event loop singletons
	//
	m_event_loop.reset( GNet::EventLoop::create() ) ;
	m_timer_list.reset( new GNet::TimerList ) ;

	// hook up the timer callbacks now we have a timer list
	//
	m_forwarding_timer.reset( new GNet::Timer<Run>(*this,&Run::onRequestForwardingTimeout,m_eh_nothrow) ) ;
	m_poll_timer.reset( new GNet::Timer<Run>(*this,&Run::onPollTimeout,m_eh_nothrow) ) ;
	m_stop_timer.reset( new GNet::Timer<Run>(*this,&Run::onStopTimeout,m_eh_nothrow) ) ;

	// early check on socket bindability
	//
	checkPorts() ;

	// early check on script executablity
	//
	checkScripts() ;

	// tls library setup
	//
	bool need_tls =
		configuration().clientTls() ||
		configuration().clientOverTls() ||
		configuration().serverTls() ;

	bool prefer_tls = // secrets might need hash functions from tls library
		configuration().clientSecretsFile() != G::Path() ||
		configuration().serverSecretsFile() != G::Path() ||
		configuration().popSecretsFile() != G::Path() ;

	m_tls_library.reset( new GSsl::Library( need_tls || prefer_tls ,
		configuration().tlsConfig() , GSsl::Library::log , configuration().debug() ) ) ;

	if( configuration().serverTls() )
		m_tls_library->addProfile( "server" , true ,
			configuration().serverTlsCertificate().str() , // key
			configuration().serverTlsCertificate().str() , // cert
			configuration().serverTlsCaList().str() ) ;

	if( configuration().clientTls() || configuration().clientOverTls() )
		m_tls_library->addProfile( "client" , false ,
			configuration().clientTlsCertificate().str() , // key
			configuration().clientTlsCertificate().str() , // cert
			configuration().clientTlsCaList().str() ,
			configuration().clientTlsPeerCertificateName() ,
			configuration().clientTlsPeerHostName() ) ;

	if( need_tls && !m_tls_library->enabled() )
		throw G::Exception( "cannot do tls/ssl: tls library not built in: "
			"remove tls options from the command-line or rebuild "
			"the emailrelay executable with a supported tls library" ) ;

	// network monitor singleton
	//
	m_monitor.reset( new GNet::Monitor ) ;
	m_monitor->signal().connect( G::Slot::slot(*this,&Run::onNetworkEvent) ) ;

	// early check of the forward-to address
	//
	if( configuration().log() && !configuration().serverAddress().empty() && !configuration().forwardOnStartup() )
	{
		GNet::Location location( configuration().serverAddress() , resolverFamily() ) ;
		std::string error = GNet::Resolver::resolve( location ) ;
		if( !error.empty() )
			G_WARNING( "Main::Run::run: dns lookup of forward-to address failed: " << error ) ;
		else
			G_LOG( "Main::Run::run: forwarding address " << location.displayString() ) ;
	}

	// message store singletons
	//
	m_store.reset( new GSmtp::FileStore( configuration().spoolDir() , false , configuration().maxSize() ) ) ;
	m_store->messageStoreUpdateSignal().connect( G::Slot::slot(*this,&Run::onStoreUpdateEvent) ) ;
	m_store->messageStoreRescanSignal().connect( G::Slot::slot(*this,&Run::onStoreRescanEvent) ) ;
	GPop::Store pop_store( configuration().spoolDir() , configuration().popByName() , ! configuration().popNoDelete() ) ;

	// authentication secrets
	//
	m_client_secrets.reset( new GAuth::Secrets( configuration().clientSecretsFile().str() , "client" ) ) ;
	m_server_secrets.reset( new GAuth::Secrets( configuration().serverSecretsFile().str() , "server" ) ) ;
	if( configuration().doPop() )
		m_pop_secrets.reset( new GPop::Secrets( configuration().popSecretsFile().str() ) ) ;

	// daemonise
	//
	G::PidFile pid_file ;
	if( configuration().usePidFile() ) pid_file.init( G::Path(configuration().pidFile()) ) ;
	if( configuration().daemon() ) G::Daemon::detach( pid_file ) ;

	// figure out what we're doing
	//
	bool do_smtp = configuration().doServing() && configuration().doSmtp() ;
	bool do_pop = configuration().doServing() && configuration().doPop() ;
	bool do_admin = configuration().doServing() && configuration().doAdmin() ;
	bool serving = do_smtp || do_pop || do_admin ;
	bool admin_forwarding = do_admin && !configuration().serverAddress().empty() ;
	bool forwarding = configuration().forwardOnStartup() || configuration().doPolling() || admin_forwarding ;
	bool report_finished = false ; // configuration().forwardOnStartup() && !serving && m_output.simpleOutput() ;
	m_quit_when_sent =
		!serving &&
		configuration().forwardOnStartup() &&
		!configuration().doPolling() &&
		!admin_forwarding ;

	// create the smtp server
	//
	if( do_smtp )
	{
		if( configuration().immediate() )
			G_WARNING( "Run::doServing: using --immediate can result in client timeout errors: "
				"try --forward-on-disconnect instead" ) ;

		m_smtp_server.reset( new GSmtp::Server(
			m_eh_throw ,
			*m_store.get() ,
			*m_client_secrets.get() ,
			*m_server_secrets.get() ,
			serverConfig() ,
			configuration().immediate() ? configuration().serverAddress() : std::string() ,
			clientConfig() ) ) ;

		m_smtp_server->eventSignal().connect( G::Slot::slot(*this,&Run::onServerEvent) ) ;
	}

	// create the pop server
	//
	if( do_pop )
	{
		m_pop_server.reset( new GPop::Server( m_eh_throw , pop_store , *m_pop_secrets.get() , popConfig() ) ) ;
	}

	// create the admin server
	//
	if( do_admin )
	{
		unique_ptr<GSmtp::AdminServer> admin_server = Main::Admin::newServer( m_eh_throw ,
			configuration() , *m_store.get() , clientConfig() , *m_client_secrets.get() ,
			versionNumber() ) ;
		m_admin_server.reset( admin_server.release() ) ;
	}

	// do serving and/or forwarding
	//
	const bool empty = m_store->empty() ;
	if( !serving && !forwarding )
	{
		commandline().showNothingToDo( true ) ;
	}
	else if( m_quit_when_sent && empty )
	{
		commandline().showNothingToSend( true ) ;
	}
	else
	{
		// kick off some forwarding
		//
		if( configuration().forwardOnStartup() )
			requestForwarding( "startup" ) ;

		// kick off the polling cycle
		//
		if( configuration().doPolling() )
			m_poll_timer->startTimer( configuration().pollingTimeout() ) ;

		// report stuff
		//
		if( m_smtp_server.get() ) m_smtp_server->report() ;
		if( m_admin_server.get() ) Admin::report( *m_admin_server.get() ) ;
		if( m_pop_server.get() ) m_pop_server->report() ;

		// run the event loop
		//
		commit( pid_file ) ;
		closeMoreFiles() ;
		std::string quit_reason = m_event_loop->run() ;
		if( !quit_reason.empty() )
			throw std::runtime_error( quit_reason ) ;

		if( report_finished )
			commandline().showFinished() ;
	}
}

const Main::CommandLine & Main::Run::commandline() const
{
	return *m_commandline.get() ;
}

std::string Main::Run::smtpIdent() const
{
	return std::string("E-MailRelay V") + versionNumber() ;
}

void Main::Run::closeFiles()
{
	if( configuration().daemon() )
	{
		const bool keep_stderr = true ;
		G::Process::closeFiles( keep_stderr ) ;
	}
}

void Main::Run::closeMoreFiles()
{
	if( configuration().closeStderr() )
		G::Process::closeStderr() ;
}

bool Main::Run::hidden() const
{
	return commandline().map().contains("hidden") ;
}

void Main::Run::checkPort( bool check , const std::string & ip , unsigned int port )
{
	if( check )
	{
		const bool do_throw = true ;
		if( ip.empty() )
		{
			if( GNet::Address::supports( GNet::Address::Family::ipv6() ) )
			{
				GNet::Address address( GNet::Address::Family::ipv6() , port ) ;
				GNet::Server::canBind( address , do_throw ) ;
			}
			if( GNet::Address::supports( GNet::Address::Family::ipv4() ) )
			{
				GNet::Address address( GNet::Address::Family::ipv4() , port ) ;
				GNet::Server::canBind( address , do_throw ) ;
			}
		}
		else
		{
			GNet::Address address( ip , port ) ;
			GNet::Server::canBind( address , do_throw ) ;
		}
	}
}

void Main::Run::checkPorts() const
{
	if( configuration().doServing() )
	{
		G::StringArray smtp_addresses = configuration().listeningAddresses("smtp") ;
		checkPort( configuration().doSmtp() && smtp_addresses.empty() , std::string() , configuration().port() ) ;
		for( G::StringArray::iterator p1 = smtp_addresses.begin() ; p1 != smtp_addresses.end() ; ++p1 )
			checkPort( configuration().doSmtp() , *p1 , configuration().port() ) ;

		G::StringArray pop_addresses = configuration().listeningAddresses("pop") ;
		checkPort( configuration().doPop() && pop_addresses.empty() , std::string() , configuration().popPort() ) ;
		for( G::StringArray::iterator p2 = pop_addresses.begin() ; p2 != pop_addresses.end() ; ++p2 )
				checkPort( configuration().doPop() , *p2 , configuration().popPort() ) ;

		G::StringArray admin_addresses = configuration().listeningAddresses("admin") ;
		checkPort( configuration().doAdmin() && admin_addresses.empty() , std::string() , configuration().adminPort() ) ;
		for( G::StringArray::iterator p3 = admin_addresses.begin() ; p3 != admin_addresses.end() ; ++p3 )
			checkPort( configuration().doAdmin() , *p3 , configuration().adminPort() ) ;
	}
}

void Main::Run::commit( G::PidFile & pid_file )
{
	if( !pid_file.committed() )
	{
		// change user id so that we can write to /var/run or wherever -- but dont
		// change the effective group id -- as a result we create the pid file with
		// unprivileged group ownership that is easier to clean up
		G::Root claim_root( false ) ;
		// use a world-readable umask so that different users can play nice together
		G::Process::Umask umask( G::Process::Umask::Readable ) ;
		pid_file.commit() ;
	}
}

GSmtp::ServerProtocol::Config Main::Run::serverProtocolConfig() const
{
	return
		GSmtp::ServerProtocol::Config(
			!configuration().anonymous() , // (with-vrfy)
			configuration().filterTimeout() , // (filter-timeout)
			configuration().maxSize() , // (max-size)
			configuration().serverTlsRequired() , // (authentication-requires-encryption)
			configuration().serverTlsRequired() , // (mail-requires-encryption)
			configuration().serverTls() ) ; // (advertise-tls-if-possible)
}

GSmtp::Server::Config Main::Run::serverConfig() const
{
	return
		GSmtp::Server::Config(
			configuration().allowRemoteClients() ,
			configuration().port() ,
			GNet::MultiServer::addressList( configuration().listeningAddresses("smtp") , configuration().port() ) ,
			smtpIdent() ,
			configuration().anonymous() ,
			configuration().filter().str() ,
			configuration().filterTimeout() ,
			configuration().verifier().str() ,
			configuration().filterTimeout() , // (verifier-timeout)
			configuration().verifierCompatibility() ,
			serverProtocolConfig() ) ;
}

GPop::Server::Config Main::Run::popConfig() const
{
	return GPop::Server::Config( configuration().allowRemoteClients() ,
		configuration().popPort() , configuration().listeningAddresses("pop") ) ;
}

GSmtp::Client::Config Main::Run::clientConfig() const
{
	return
		GSmtp::Client::Config(
			configuration().clientFilter().str() ,
			configuration().filterTimeout() ,
			!configuration().clientBindAddress().empty() ,
			asAddress(configuration().clientBindAddress()) ,
			GSmtp::ClientProtocol::Config(
				GNet::Local::canonicalName() ,
				configuration().responseTimeout() ,
				configuration().promptTimeout() , // waiting for "service ready"
				configuration().filterTimeout() ,
				configuration().clientTls() && !configuration().clientOverTls() , // (use-starttls-if-possible)
				configuration().clientTlsRequired() && !configuration().clientOverTls() , // (must-use-starttls)
				true , // (must-authenticate)
				configuration().anonymous() , // (anonymous)
				true , // (must-accept-all-recipients)
				false ) , // (eight-bit-strict)
			configuration().connectionTimeout() ,
			configuration().secureConnectionTimeout() ,
			configuration().clientOverTls() ) ;
}

GNet::Address Main::Run::asAddress( const std::string & s )
{
	bool has_port_number = GNet::Address::validString( s ) ;
	return s.empty() ?
		GNet::Address::defaultAddress() :
		( has_port_number ? GNet::Address(s) : GNet::Address(s,0U) ) ;
}

Main::Run::ExceptionHandler::ExceptionHandler( bool do_throw ) :
	m_do_throw(do_throw)
{
}

void Main::Run::ExceptionHandler::onException( std::exception & e )
{
	if( m_do_throw )
		throw ;
	else
		G_ERROR( "Main::Run::ExceptionHandler::onException: exception: " << e.what() ) ;
}

void Main::Run::onPollTimeout()
{
	G_DEBUG( "Main::Run::onPollTimeout" ) ;
	m_poll_timer->startTimer( configuration().pollingTimeout() ) ;
	requestForwarding( "poll" ) ;
}

void Main::Run::onStopTimeout()
{
	G_ASSERT( m_event_loop.get() != nullptr ) ;
	if( m_event_loop.get() )
		m_event_loop->quit( m_stop_reason ) ;
}

void Main::Run::requestForwarding( const std::string & reason )
{
	G_ASSERT( m_forwarding_timer.get() != nullptr ) ;
	if( !reason.empty() )
		m_forwarding_reason = reason ;
	m_forwarding_timer->startTimer( 0U ) ;
}

void Main::Run::onRequestForwardingTimeout()
{
	if( m_client.busy() )
	{
		G_LOG( "Main::Run::onRequestForwardingTimeout: forwarding: [" << m_forwarding_reason << "]: still busy from last time" ) ;
		m_forwarding_pending = true ;
		emit( "forward" , "busy" , "" ) ;
	}
	else
	{
		if( logForwarding() )
			G_LOG( "Main::Run::onRequestForwardingTimeout: forwarding: [" << m_forwarding_reason << "]" ) ;

		emit( "forward" , "start" , "" ) ;
		std::string error = startForwarding() ;
		if( !error.empty() )
			emit( "forward" , "end" , error ) ;
	}
}

bool Main::Run::logForwarding() const
{
	return m_forwarding_reason != "poll" || configuration().pollingLog() ||
		( G::LogOutput::instance() != nullptr && G::LogOutput::instance()->at( G::Log::s_Debug ) ) ;
}

std::string Main::Run::startForwarding()
{
	try
	{
		if( m_store->empty() )
		{
			if( logForwarding() )
				G_LOG( "Main::Run::startForwarding: forwarding: no messages to send" ) ;
		}
		else
		{
			m_client.reset( new GSmtp::Client( GNet::Location(configuration().serverAddress(),resolverFamily()) ,
				*m_client_secrets.get() , clientConfig() ) ) ;

			m_client->sendMessagesFrom( *m_store.get() ) ; // once connected
		}
		return std::string() ;
	}
	catch( std::exception & e )
	{
		G_ERROR( "Main::Run::startForwarding: forwarding failure: " << e.what() ) ;
		return e.what() ;
	}
}

void Main::Run::onClientDone( std::string reason )
{
	G_DEBUG( "Main::Run::onClientDone: reason=[" << reason << "]" ) ;
	if( m_quit_when_sent )
	{
		// quit the event loop, but do it via a timer so that the
		// client's onDelete() cleanup has a chance to work
		m_stop_reason = reason ;
		m_stop_timer->startTimer( 0U ) ;
	}
	else
	{
		if( ! reason.empty() )
			G_ERROR( "Main::Run::onClientDone: forwarding: " << reason ) ;

		// go round again if necessary
		if( m_forwarding_pending )
		{
			m_forwarding_pending = false ;
			G_LOG( "Main::Run::onClientDone: forwarding: queued request [" << m_forwarding_reason << "]" ) ;
			requestForwarding() ;
		}
	}
	emit( "forward" , "end" , reason ) ;
}

void Main::Run::onClientEvent( std::string s1 , std::string s2 )
{
	emit( "client" , s1 , s2 ) ;
}

void Main::Run::onServerEvent( std::string s1 , std::string )
{
	if( s1 == "done" && configuration().forwardOnDisconnect() )
		requestForwarding( "client disconnect" ) ;
}

void Main::Run::onStoreUpdateEvent()
{
	// raise a "store/update" event for the gui
	emit( "store" , "update" , std::string() ) ;
}

void Main::Run::onStoreRescanEvent()
{
	requestForwarding( "rescan" ) ;
}

void Main::Run::onNetworkEvent( std::string s1 , std::string s2 )
{
	// raise a "network" event for the gui
	emit( "network" , s1 , s2 ) ;
}

void Main::Run::emit( const std::string & s0 , const std::string & s1 , const std::string & s2 )
{
	try
	{
		m_signal.emit( s0 , s1 , s2 ) ;
		if( m_admin_server.get() != nullptr )
		{
			Admin::notify( *m_admin_server.get() , s0 , s1 , s2 ) ;
		}
	}
	catch( std::exception & e )
	{
		G_WARNING( "Main::Run::emit: " << e.what() ) ;
	}
}

int Main::Run::resolverFamily() const
{
	// choose an address family for the DNS lookup based on the "--client-interface" address
	std::string client_bind_address = configuration().clientBindAddress() ;
	return
		client_bind_address.empty() ?
			AF_UNSPEC :
			asAddress(client_bind_address).domain() ;
}

G::Slot::Signal3<std::string,std::string,std::string> & Main::Run::signal()
{
	return m_signal ;
}

void Main::Run::checkScripts() const
{
	checkFilterScript( configuration().filter().str() ) ;
	checkFilterScript( configuration().clientFilter().str() ) ;
	checkVerifierScript( configuration().verifier().str() ) ;
}

void Main::Run::checkVerifierScript( const std::string & s ) const
{
	std::string reason = GSmtp::VerifierFactory::check( s ) ;
	if( !reason.empty() )
	{
		G_WARNING( "Main::Run::checkScript: invalid verifier \"" << s << "\": " << reason ) ;
	}
}

void Main::Run::checkFilterScript( const std::string & s ) const
{
	std::string reason = GSmtp::FilterFactory::check( s ) ;
	if( !reason.empty() )
	{
		G_WARNING( "Main::Run::checkScript: invalid filter \"" << s << "\": " << reason ) ;
	}
}

G::Path Main::Run::appDir() const
{
	G::Path this_exe = G::Arg::exe() ;
	if( this_exe == G::Path() ) // eg. linux with no procfs
		return G::Path(m_arg.v(0U)).dirname() ; // may be relative and/or bogus
	else if( this_exe.dirname().basename() == "MacOS" && this_exe.dirname().dirname().basename() == "Contents" )
		return this_exe.dirname().dirname().dirname() ; // .app
	else
		return this_exe.dirname() ;
}

/// \file run.cpp
