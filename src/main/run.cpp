//
// Copyright (C) 2001-2020 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "run.h"
#include "gsmtpserver.h"
#include "gsmtpclient.h"
#include "gsecrets.h"
#include "geventloop.h"
#include "garg.h"
#include "gdaemon.h"
#include "gpidfile.h"
#include "gfilestore.h"
#include "gmultiserver.h"
#include "gadminserver.h"
#include "gpopserver.h"
#include "gsocket.h"
#include "gfilterfactory.h"
#include "gverifierfactory.h"
#include "gdnsblock.h"
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
#include "glog.h"
#include "gassert.h"
#include "legal.h"
#include <vector>
#include <iostream>
#include <exception>
#include <memory>
#include <utility>

std::string Main::Run::versionNumber()
{
	return "2.2rc3" ;
}

Main::Run::Run( Main::Output & output , const G::Arg & arg , const std::string & option_spec , bool has_gui ) :
	m_output(output) ,
	m_es_rethrow(GNet::ExceptionSink::Type::Rethrow,nullptr) ,
	m_es_nothrow(GNet::ExceptionSink::Type::Log,nullptr) ,
	m_option_spec(option_spec) ,
	m_arg(arg) ,
	m_forwarding_pending(false) ,
	m_quit_when_sent(false) ,
	m_has_gui(has_gui)
{
	m_client_ptr.deletedSignal().connect( G::Slot::slot(*this,&Run::onClientDone) ) ;
	m_client_ptr.eventSignal().connect( G::Slot::slot(*this,&Run::onClientEvent) ) ;
}

void Main::Run::configure()
{
	// lazy construction so that the constructor doesn't throw
	m_commandline = std::make_unique<CommandLine>( m_output , m_arg , m_option_spec , versionNumber() ) ;
	m_configuration = std::make_unique<Configuration>( m_commandline->options(),m_commandline->map(),appDir(),G::Process::cwd() ) ;
}

Main::Run::~Run()
{
	if( m_smtp_server ) m_smtp_server->eventSignal().disconnect() ;
	if( m_store ) m_store->messageStoreRescanSignal().disconnect() ;
	if( m_monitor ) m_monitor->signal().disconnect() ;
	m_client_ptr.deletedSignal().disconnect() ;
	m_client_ptr.eventSignal().disconnect() ;
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
	G::Process::Umask::set( G::Process::Umask::Mode::Tightest ) ;

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
	m_log_output = std::make_unique<G::LogOutput>(
		m_arg.prefix() ,
		G::LogOutput::Config()
			.set_output_enabled(configuration().log())
			.set_summary_info(configuration().log())
			.set_verbose_info(configuration().verbose())
			.set_debug(configuration().debug())
			.set_with_level(true)
			.set_with_timestamp(configuration().logTimestamp())
			.set_with_context(configuration().logAddress())
			.set_strip(!configuration().debug())
			.set_use_syslog(configuration().useSyslog())
			.set_allow_bad_syslog(!(m_has_gui&&configuration().logFile()==G::Path()))
			.set_facility(configuration().syslogFacility()) ,
		configuration().logFile().str() // stderr-replacement
	) ;

	// if we are going to close stderr soon then make stderr logging
	// less verbose so that startup scripts are cleaner, but without
	// affecting syslog output
	if( configuration().useSyslog() && configuration().daemon() &&
		configuration().closeStderr() && configuration().logFile() == G::Path() )
			m_log_output->configure( m_log_output->config().set_quiet_stderr() ) ;

	// log command-line warnings
	//
	if( !m_output.simpleOutput() )
	{
		commandline().logSemanticWarnings( configuration().semanticWarnings() ) ;
	}

	// release root privileges and extra group memberships
	//
	if( configuration().user() != "root" )
		G::Root::init( configuration().user() ) ;

	// create event loop singletons
	//
	m_event_loop.reset( GNet::EventLoop::create() ) ;
	m_timer_list = std::make_unique<GNet::TimerList>() ;

	// hook up the timer callbacks now we have a timer list
	//
	m_forwarding_timer = std::make_unique<GNet::Timer<Run>>( *this , &Run::onRequestForwardingTimeout , m_es_nothrow ) ;
	m_poll_timer = std::make_unique<GNet::Timer<Run>>( *this , &Run::onPollTimeout , m_es_nothrow ) ;
	m_queue_timer = std::make_unique<GNet::Timer<Run>>( *this , &Run::onQueueTimeout , m_es_nothrow ) ;

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
		configuration().serverTls() ||
		configuration().serverTlsConnection() ;

	bool prefer_tls = // secrets might need hash functions from tls library
		configuration().clientSecretsFile() != G::Path() ||
		configuration().serverSecretsFile() != G::Path() ||
		configuration().popSecretsFile() != G::Path() ;

	m_tls_library = std::make_unique<GSsl::Library>( need_tls || prefer_tls ,
		configuration().tlsConfig() , GSsl::Library::log , configuration().debug() ) ;

	if( configuration().serverTls() || configuration().serverTlsConnection() )
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
	m_monitor = std::make_unique<GNet::Monitor>() ;
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

	// early check on the DNSBL configuration string
	//
	if( !configuration().dnsbl().empty() )
		GNet::DnsBlock::checkConfig( configuration().dnsbl() ) ;

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

	// create message store singletons
	//
	m_store = std::make_unique<GSmtp::FileStore>( configuration().spoolDir() , false , configuration().maxSize() , configuration().eightBitTest() ) ;
	m_store->messageStoreRescanSignal().connect( G::Slot::slot(*this,&Run::onStoreRescanEvent) ) ;
	if( do_pop )
	{
		m_pop_store = std::make_unique<GPop::Store>( configuration().spoolDir() , configuration().popByName() , ! configuration().popNoDelete() ) ;
	}

	// authentication secrets
	//
	GAuth::Secrets::check( configuration().clientSecretsFile().str() , configuration().serverSecretsFile().str() ,
		configuration().doPop() ? configuration().popSecretsFile().str() : std::string() ) ;
	m_client_secrets = std::make_unique<GAuth::Secrets>( configuration().clientSecretsFile().str() , "client" ) ;
	m_server_secrets = std::make_unique<GAuth::Secrets>( configuration().serverSecretsFile().str() , "server" ) ;
	if( configuration().doPop() )
		m_pop_secrets = std::make_unique<GAuth::Secrets>( configuration().popSecretsFile().str() , "pop-server" ) ;

	// daemonise
	//
	G::PidFile pid_file ;
	if( configuration().usePidFile() ) pid_file.init( G::Path(configuration().pidFile()) ) ;
	if( configuration().daemon() ) G::Daemon::detach( pid_file ) ;

	// create the smtp server
	//
	if( do_smtp )
	{
		if( configuration().immediate() )
			G_WARNING( "Run::doServing: using --immediate can result in client timeout errors: "
				"try --forward-on-disconnect instead" ) ;

		G_ASSERT( m_store != nullptr ) ;
		G_ASSERT( m_client_secrets != nullptr ) ;
		G_ASSERT( m_server_secrets != nullptr ) ;
		m_smtp_server = std::make_unique<GSmtp::Server>(
			m_es_rethrow ,
			*m_store ,
			*m_client_secrets ,
			*m_server_secrets ,
			serverConfig() ,
			configuration().immediate() ? configuration().serverAddress() : std::string() ,
			clientConfig() ) ;

		m_smtp_server->eventSignal().connect( G::Slot::slot(*this,&Run::onServerEvent) ) ;
	}

	// create the pop server
	//
	if( do_pop )
	{
		G_ASSERT( m_pop_store != nullptr ) ;
		G_ASSERT( m_pop_secrets != nullptr ) ;
		m_pop_server = std::make_unique<GPop::Server>(
			m_es_rethrow ,
			*m_pop_store ,
			*m_pop_secrets ,
			popConfig() ) ;
	}

	// create the admin server
	//
	if( do_admin )
	{
		G_ASSERT( m_store != nullptr ) ;
		G_ASSERT( m_client_secrets != nullptr ) ;
		m_admin_server = newAdminServer(
			m_es_rethrow ,
			configuration() ,
			*m_store ,
			GNet::ServerPeerConfig(0U) ,
			clientConfig() ,
			*m_client_secrets ,
			versionNumber() ) ;
	}

	// do serving and/or forwarding
	//
	if( !serving && !forwarding )
	{
		commandline().showNothingToDo( true ) ;
	}
	else if( m_quit_when_sent && m_store && m_store->empty() )
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
		if( m_smtp_server ) m_smtp_server->report() ;
		if( m_admin_server ) m_admin_server->report() ;
		if( m_pop_server ) m_pop_server->report() ;

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
	G_ASSERT( m_commandline != nullptr ) ;
	return *m_commandline ;
}

std::string Main::Run::smtpIdent() const
{
	return std::string("E-MailRelay V") + versionNumber() ;
}

void Main::Run::closeFiles() const
{
	if( configuration().daemon() )
	{
		const bool keep_stderr = true ;
		G::Process::closeFiles( keep_stderr ) ;
	}
}

void Main::Run::closeMoreFiles() const
{
	if( configuration().closeStderr() )
		G::Process::closeStderr() ;
}

bool Main::Run::hidden() const
{
	return configuration().hidden() || configuration().show("hidden") ;
}

void Main::Run::checkPort( bool check , const std::string & ip , unsigned int port )
{
	if( check )
	{
		const bool do_throw = true ;
		if( ip.empty() )
		{
			if( GNet::Address::supports( GNet::Address::Family::ipv6 ) &&
				GNet::StreamSocket::supports( GNet::Address::Family::ipv6 ) )
			{
				GNet::Address address( GNet::Address::Family::ipv6 , port ) ;
				GNet::Server::canBind( address , do_throw ) ;
			}
			if( GNet::Address::supports( GNet::Address::Family::ipv4 ) )
			{
				GNet::Address address( GNet::Address::Family::ipv4 , port ) ;
				GNet::Server::canBind( address , do_throw ) ;
			}
		}
		else if( GNet::Address::validStrings(ip,"0") )
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
		for( const auto & a : smtp_addresses )
			checkPort( configuration().doSmtp() , a , configuration().port() ) ;

		G::StringArray pop_addresses = configuration().listeningAddresses("pop") ;
		checkPort( configuration().doPop() && pop_addresses.empty() , std::string() , configuration().popPort() ) ;
		for( const auto & a : pop_addresses )
			checkPort( configuration().doPop() , a , configuration().popPort() ) ;

		G::StringArray admin_addresses = configuration().listeningAddresses("admin") ;
		checkPort( configuration().doAdmin() && admin_addresses.empty() , std::string() , configuration().adminPort() ) ;
		for( const auto & a : admin_addresses )
			checkPort( configuration().doAdmin() , a , configuration().adminPort() ) ;
	}
}

void Main::Run::commit( G::PidFile & pid_file )
{
	if( !pid_file.committed() )
	{
		// change user id so that we can write to /var/run or wherever -- but dont
		// change the effective group id -- as a result we create the pid file with
		// unprivileged group ownership, which is easier to clean up -- also use
		// a world-readable umask so that different users can play nice together
		G::Root claim_root( false ) ;
		G::Process::Umask umask( G::Process::Umask::Mode::Readable ) ;
		pid_file.commit() ;
	}
}

GSmtp::ServerProtocol::Config Main::Run::serverProtocolConfig() const
{
	return
		GSmtp::ServerProtocol::Config()
			.set_with_vrfy( !configuration().anonymous() )
			.set_filter_timeout( configuration().filterTimeout() )
			.set_max_size( configuration().maxSize() )
			.set_authentication_requires_encryption( configuration().serverTlsRequired() )
			.set_mail_requires_encryption( configuration().serverTlsRequired() )
			.set_tls_starttls( configuration().serverTls() )
			.set_tls_connection( configuration().serverTlsConnection() )
			.set_allow_pipelining( configuration().smtpPipelining() ) ;
}

GSmtp::Server::Config Main::Run::serverConfig() const
{
	return
		GSmtp::Server::Config()
			.set_allow_remote( configuration().allowRemoteClients() )
			.set_interfaces( configuration().listeningAddresses("smtp") )
			.set_port( configuration().port() )
			.set_ident( smtpIdent() )
			.set_anonymous( configuration().anonymous() )
			.set_filter_address( configuration().filter().str() )
			.set_filter_timeout( configuration().filterTimeout() )
			.set_verifier_address( configuration().verifier().str() )
			.set_verifier_timeout( configuration().filterTimeout() )
			.set_server_peer_config( GNet::ServerPeerConfig(configuration().idleTimeout()) )
			.set_protocol_config( serverProtocolConfig() )
			.set_sasl_server_config( configuration().smtpSaslServerConfig() )
			.set_dnsbl_config( configuration().dnsbl() ) ;
}

GPop::Server::Config Main::Run::popConfig() const
{
	return
		GPop::Server::Config()
			.set_allow_remote( configuration().allowRemoteClients() )
			.set_port( configuration().popPort() )
			.set_addresses( configuration().listeningAddresses("pop") )
			.set_server_peer_config(
				GNet::ServerPeerConfig()
					.set_idle_timeout( configuration().idleTimeout()) )
			.set_sasl_server_config( configuration().popSaslServerConfig() ) ;
}

GSmtp::Client::Config Main::Run::clientConfig() const
{
	return
		GSmtp::Client::Config()
			.set_filter_address( configuration().clientFilter().str() )
			.set_filter_timeout( configuration().filterTimeout() )
			.set_bind_local_address( !configuration().clientBindAddress().empty() )
			.set_local_address( asAddress(configuration().clientBindAddress()) )
			.set_client_protocol_config(
				GSmtp::ClientProtocol::Config()
					.set_thishost_name( GNet::Local::canonicalName() )
					.set_response_timeout( configuration().responseTimeout() )
					.set_ready_timeout( configuration().promptTimeout() )
					.set_filter_timeout( configuration().filterTimeout() )
					.set_use_starttls_if_possible( configuration().clientTls() && !configuration().clientOverTls() )
					.set_must_use_tls( configuration().clientTlsRequired() && !configuration().clientOverTls() )
					.set_must_authenticate( true )
					.set_anonymous( configuration().anonymous() )
					.set_must_accept_all_recipients( !configuration().forwardToSome() )
					.set_eight_bit_strict( false ) )
			.set_connection_timeout( configuration().connectionTimeout() )
			.set_secure_connection_timeout( configuration().secureConnectionTimeout() )
			.set_secure_tunnel( configuration().clientOverTls() )
			.set_sasl_client_config( configuration().smtpSaslClientConfig() ) ;
}

GNet::Address Main::Run::asAddress( const std::string & s )
{
	// (port number is optional)
	return s.empty() ?
		GNet::Address::defaultAddress() :
		( GNet::Address::validString(s) ? GNet::Address(s) : GNet::Address(s,0U) ) ;
}

void Main::Run::onPollTimeout()
{
	G_DEBUG( "Main::Run::onPollTimeout" ) ;
	m_poll_timer->startTimer( configuration().pollingTimeout() ) ;
	requestForwarding( "poll" ) ;
}

void Main::Run::requestForwarding( const std::string & reason )
{
	G_ASSERT( m_forwarding_timer != nullptr ) ;
	if( !reason.empty() )
		m_forwarding_reason = reason ;
	m_forwarding_timer->startTimer( 0U ) ;
}

void Main::Run::onRequestForwardingTimeout()
{
	if( m_client_ptr.busy() )
	{
		G_LOG( "Main::Run::onRequestForwardingTimeout: forwarding: [" << m_forwarding_reason << "]: still busy from last time" ) ;
		m_forwarding_pending = true ;
	}
	else
	{
		if( logForwarding() )
			G_LOG( "Main::Run::onRequestForwardingTimeout: forwarding: [" << m_forwarding_reason << "]" ) ;

		emit( "forward" , "start" ) ;
		std::string error = startForwarding() ;
		if( !error.empty() )
			emit( "forward" , "end" , error ) ;
	}
}

bool Main::Run::logForwarding() const
{
	return m_forwarding_reason != "poll" || configuration().pollingLog() ||
		( G::LogOutput::instance() && G::LogOutput::instance()->at( G::Log::Severity::s_Debug ) ) ;
}

std::string Main::Run::startForwarding()
{
	try
	{
		if( m_store->empty() )
		{
			if( logForwarding() )
				G_LOG( "Main::Run::startForwarding: forwarding: no messages to send" ) ;
			return "no messages" ;
		}
		else
		{
			G_ASSERT( m_client_secrets != nullptr ) ;
			m_client_ptr.reset( new GSmtp::Client(
				GNet::ExceptionSink(m_client_ptr,nullptr) ,
				GNet::Location(configuration().serverAddress(),resolverFamily()) ,
				*m_client_secrets ,
				clientConfig() ) ) ;

			G_ASSERT( m_store != nullptr ) ;
			m_client_ptr->sendMessagesFrom( *m_store ) ; // once connected
			return std::string() ;
		}
	}
	catch( std::exception & e )
	{
		G_ERROR( "Main::Run::startForwarding: forwarding failure: " << e.what() ) ;
		return e.what() ;
	}
}

void Main::Run::onClientDone( const std::string & reason )
{
	G_DEBUG( "Main::Run::onClientDone: reason=[" << reason << "]" ) ;
	if( m_quit_when_sent )
	{
		// quit the event loop
		if( m_event_loop )
			m_event_loop->quit( reason ) ;
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

void Main::Run::onClientEvent( const std::string & s1 , const std::string & s2 , const std::string & s3 )
{
	emit( "client" , s1 , s2 , s3 ) ;
}

void Main::Run::onServerEvent( const std::string & s1 , const std::string & )
{
	if( s1 == "done" && configuration().forwardOnDisconnect() )
		requestForwarding( "client disconnect" ) ;
}

void Main::Run::onStoreRescanEvent()
{
	requestForwarding( "rescan" ) ;
}

void Main::Run::onNetworkEvent( const std::string & s1 , const std::string & s2 )
{
	emit( "network" , s1 , s2 ) ;
}

void Main::Run::emit( const std::string & s0 , const std::string & s1 , const std::string & s2 , const std::string & s3 )
{
	// use an asynchronous queue to avoid side-effects from callbacks
	bool notifying = m_admin_server && m_admin_server->notifying() ;
	if( notifying || m_has_gui )
	{
		G_ASSERT( m_queue_timer != nullptr ) ;
		if( m_queue_timer )
			m_queue_timer->startTimer( 0U ) ;

		while( m_queue.size() > 100U )
		{
			G_WARNING_ONCE( "Main::Run::emit: too many notification events: discarding old ones" ) ;
			m_queue.pop_front() ;
		}

		if( m_has_gui )
			m_queue.emplace_back( 1 , s0 , s1 , s2 , s3 ) ;

		if( notifying )
			m_queue.emplace_back( 2 , s0 , s1 , s2 , s3 ) ;
	}
}

void Main::Run::onQueueTimeout()
{
	if( !m_queue.empty() )
	{
		m_queue_timer->startTimer( 0U ) ;
		QueueItem item = m_queue.front() ;
		m_queue.pop_front() ;
		if( item.target == 1 )
			m_signal.emit( item.s0 , item.s1 , item.s2 , item.s3 ) ;
		else if( item.target == 2 && m_admin_server )
			m_admin_server->notify( item.s0 , item.s1 , item.s2 , item.s3 ) ;
	}
}

G::Slot::Signal<std::string,std::string,std::string,std::string> & Main::Run::signal()
{
	return m_signal ;
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

std::unique_ptr<GSmtp::AdminServer> Main::Run::newAdminServer( GNet::ExceptionSink es ,
	const Configuration & cfg , GSmtp::MessageStore & store ,
	const GNet::ServerPeerConfig & server_peer_config ,
	const GSmtp::Client::Config & client_config , const GAuth::Secrets & client_secrets ,
	const std::string & version_number )
{
	G::StringMap info_map ;
	info_map["version"] = version_number ;
	info_map["warranty"] = Legal::warranty("","\n") ;
	info_map["credit"] = GSsl::Library::credit("","\n","") ;
	info_map["copyright"] = Legal::copyright() ;

	G::StringMap config_map ;
	//config_map["forward-to"] = cfg.serverAddress() ;
	//config_map["spool-dir"] = cfg.spoolDir().str() ;

	return std::make_unique<GSmtp::AdminServer>(
			es ,
			store ,
			server_peer_config ,
			client_config ,
			client_secrets ,
			cfg.listeningAddresses("admin") ,
			cfg.adminPort() ,
			cfg.allowRemoteClients() ,
			cfg.serverAddress() ,
			cfg.connectionTimeout() ,
			info_map ,
			config_map ,
			cfg.withTerminate() ) ;
}

/// \file run.cpp
