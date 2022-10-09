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
/// \file run.cpp
///

#include "gdef.h"
#include "gssl.h"
#include "run.h"
#include "options.h"
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
#include "gfactoryparser.h"
#include "gverifierfactory.h"
#include "gdnsbl.h"
#include "gslot.h"
#include "gmonitor.h"
#include "glocal.h"
#include "gfile.h"
#include "gpath.h"
#include "groot.h"
#include "gexception.h"
#include "gprocess.h"
#include "ggettext.h"
#include "gstrmacros.h"
#include "gformat.h"
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

#ifdef G_LOCALEDIR
namespace { std::string localedir() { return G_STR(G_LOCALEDIR) ; } }
#else
namespace { std::string localedir() { return std::string() ; } }
#endif

std::string Main::Run::versionNumber()
{
	return "2.4" ;
}

Main::Run::Run( Main::Output & output , const G::Arg & arg , bool is_windows , bool has_gui ) :
	m_output(output) ,
	m_is_windows(is_windows) ,
	m_arg(arg) ,
	m_forwarding_pending(false) ,
	m_quit_when_sent(false) ,
	m_has_gui(has_gui)
{
	m_client_ptr.deletedSignal().connect( G::Slot::slot(*this,&Run::onClientDone) ) ;
	m_client_ptr.eventSignal().connect( G::Slot::slot(*this,&Run::onClientEvent) ) ;
	m_forward_request_signal.connect( G::Slot::slot(*this,&Run::onForwardRequest) ) ;

	// initialise gettext() early iff "--localedir" is used
	std::string ldir = localedir() ;
	std::size_t pos = m_arg.index( "--localedir" , 1U ) ;
	std::size_t mpos = m_arg.match( "--localedir=" ) ;
	if( pos )
		ldir = m_arg.removeAt( pos , 1U ) ;
	else if( mpos )
		ldir = G::Str::tail( m_arg.removeAt(mpos) , "=" ) ;
	if( pos || mpos ) // moot, but avoid surprising regressions
		G::gettext_init( ldir , "emailrelay" ) ;
}

void Main::Run::configure()
{
	// lazy construction so that the constructor doesn't throw
	m_commandline = std::make_unique<CommandLine>( m_output , m_arg ,
		Main::Options::spec(m_is_windows) , versionNumber() ) ;

	m_configuration = std::make_unique<Configuration>( m_commandline->options() ,
		m_commandline->map() , appDir() , G::Process::cwd() ) ;
}

Main::Run::~Run()
{
	if( m_smtp_server ) m_smtp_server->eventSignal().disconnect() ;
	if( m_store ) m_store->messageStoreRescanSignal().disconnect() ;
	if( m_monitor ) m_monitor->signal().disconnect() ;
	m_forward_request_signal.disconnect() ;
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

	if( m_output.outputSimple() && !configuration().semanticWarnings().empty() )
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
	using G::format ;
	using G::txt ;

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
	if( configuration().closeFiles() )
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
			.set_allow_bad_syslog(!(m_has_gui&&configuration().logFile().empty()))
			.set_umask( G::Process::Umask::Mode::Tighter )
			.set_facility(configuration().syslogFacility()) ,
		configuration().logFile().str() // stderr-replacement
	) ;

	// if we are going to close stderr soon then make stderr logging
	// less verbose so that startup scripts are cleaner, but without
	// affecting syslog output
	if( configuration().useSyslog() && configuration().daemon() &&
		configuration().closeStderr() && configuration().logFile().empty() )
			m_log_output->configure( m_log_output->config().set_quiet_stderr() ) ;

	// log command-line warnings
	//
	if( !m_output.outputSimple() )
	{
		commandline().logSemanticWarnings( configuration().semanticWarnings() ) ;
	}

	// release root privileges and extra group memberships
	//
	if( configuration().user() != "root" )
		G::Root::init( configuration().user() ) ;

	// create event loop singletons
	//
	m_event_loop = GNet::EventLoop::create() ;
	m_timer_list = std::make_unique<GNet::TimerList>() ;

	// hook up the timer callbacks now we have a timer list
	//
	using Timer = GNet::Timer<Run> ;
	GNet::ExceptionSink es_log_only = GNet::ExceptionSink::logOnly() ;
	m_forwarding_timer = std::make_unique<Timer>( *this , &Run::onRequestForwardingTimeout , es_log_only ) ;
	m_poll_timer = std::make_unique<Timer>( *this , &Run::onPollTimeout , es_log_only ) ;
	m_queue_timer = std::make_unique<Timer>( *this , &Run::onQueueTimeout , es_log_only ) ;

	// early check on multi-threading behaviour
	//
	checkThreading() ;

	// tls library setup
	//
	bool need_tls =
		configuration().clientTls() ||
		configuration().clientOverTls() ||
		configuration().serverTls() ||
		configuration().serverTlsConnection() ;

	bool prefer_tls = // secrets might need hash functions from tls library
		!configuration().clientSecretsFile().empty() ||
		!configuration().serverSecretsFile().empty() ||
		!configuration().popSecretsFile().empty() ;

	m_tls_library = std::make_unique<GSsl::Library>( need_tls || prefer_tls ,
		configuration().tlsConfig() , GSsl::Library::log , configuration().debug() ) ;

	if( configuration().serverTls() || configuration().serverTlsConnection() )
		m_tls_library->addProfile( "server" , true ,
			configuration().serverTlsPrivateKey().str() ,
			configuration().serverTlsCertificate().str() ,
			configuration().serverTlsCaList().str() ) ;

	if( configuration().clientTls() || configuration().clientOverTls() )
		m_tls_library->addProfile( "client" , false ,
			configuration().clientTlsPrivateKey().str() ,
			configuration().clientTlsCertificate().str() ,
			configuration().clientTlsCaList().str() ,
			configuration().clientTlsPeerCertificateName() ,
			configuration().clientTlsPeerHostName() ) ;

	if( need_tls && !m_tls_library->enabled() )
		throw G::Exception( txt("cannot do tls/ssl: tls library not built in: "
			"remove tls options from the command-line or "
			"rebuild the emailrelay executable with a supported tls library") ) ;

	// network monitor singleton
	//
	m_monitor = std::make_unique<GNet::Monitor>() ;
	m_monitor->signal().connect( G::Slot::slot(*this,&Run::onNetworkEvent) ) ;

	// early check that the forward-to address can be resolved
	//
	if( configuration().log() && !configuration().serverAddress().empty() && !configuration().forwardOnStartup() &&
		!GNet::Address::isFamilyLocal( configuration().serverAddress() ) )
	{
		GNet::Location location( configuration().serverAddress() , resolverFamily() ) ;
		std::string error = GNet::Resolver::resolve( location ) ;
		if( !error.empty() )
			G_WARNING( "Main::Run::run: " << format(txt("dns lookup of forward-to address failed: %1%")) % error ) ;
		else
			G_LOG( "Main::Run::run: " << format(txt("forwarding address %1%")) % location.displayString() ) ;
	}

	// early check on the DNSBL configuration string
	//
	if( !configuration().dnsbl().empty() )
		GNet::Dnsbl::checkConfig( configuration().dnsbl() ) ;

	// figure out what we're doing
	//
	bool do_smtp = configuration().doServing() && configuration().doSmtp() ;
	bool do_pop = configuration().doServing() && configuration().doPop() ;
	bool do_admin = configuration().doServing() && configuration().doAdmin() ;
	bool serving = do_smtp || do_pop || do_admin ;
	bool admin_forwarding = do_admin && !configuration().serverAddress().empty() ;
	bool forwarding = configuration().forwardOnStartup() || configuration().doPolling() || admin_forwarding ;
	bool report_finished = false ; // configuration().forwardOnStartup() && !serving && m_output.outputSimple() ;
	m_quit_when_sent =
		!serving &&
		configuration().forwardOnStartup() &&
		!configuration().doPolling() &&
		!admin_forwarding ;

	// create message store singletons
	//
	m_store = std::make_unique<GSmtp::FileStore>( configuration().spoolDir() ,
		configuration().maxSize() , configuration().eightBitTest() ) ;
	m_filter_factory = std::make_unique<GSmtp::FilterFactoryFileStore>( *m_store ) ;
	m_store->messageStoreRescanSignal().connect( G::Slot::slot(*this,&Run::onStoreRescanEvent) ) ;
	if( do_pop )
	{
		m_pop_store = std::make_unique<GPop::Store>( configuration().spoolDir() ,
			configuration().popByName() , ! configuration().popNoDelete() ) ;
	}

	// authentication secrets
	//
	GAuth::Secrets::check( configuration().clientSecretsFile().str() , configuration().serverSecretsFile().str() ,
		configuration().doPop() ? configuration().popSecretsFile().str() : std::string() ) ;
	m_client_secrets = std::make_unique<GAuth::Secrets>( configuration().clientSecretsFile().str() , "client" ) ;
	m_server_secrets = std::make_unique<GAuth::Secrets>( configuration().serverSecretsFile().str() , "server" ) ;
	if( configuration().doPop() )
		m_pop_secrets = std::make_unique<GAuth::Secrets>( configuration().popSecretsFile().str() , "pop-server" ) ;

	// prepare the pid file
	//
	G::Path pid_file_path = configuration().usePidFile() ? G::Path(configuration().pidFile()) : G::Path() ;
	G::PidFile pid_file( pid_file_path ) ;
	{
		G::Root claim_root ;
		G::Process::Umask _( G::Process::Umask::Mode::GroupOpen ) ;
		pid_file.mkdir() ;
	}

	// create the smtp server
	//
	if( do_smtp )
	{
		if( configuration().immediate() )
			G_WARNING( "Run::doServing: " << txt("using --immediate can result in client timeout errors: "
				"try --forward-on-disconnect instead") ) ;

		G_ASSERT( m_store != nullptr ) ;
		G_ASSERT( m_client_secrets != nullptr ) ;
		G_ASSERT( m_server_secrets != nullptr ) ;
		m_smtp_server = std::make_unique<GSmtp::Server>(
			m_es_rethrow ,
			*m_store ,
			*m_filter_factory ,
			*m_client_secrets ,
			*m_server_secrets ,
			smtpServerConfig() ,
			configuration().immediate() ? configuration().serverAddress() : std::string() ,
			resolverFamily() ,
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
			*m_filter_factory ,
			m_forward_request_signal ,
			GNet::ServerPeer::Config().set_idle_timeout(0U) ,
			netServerConfig() ,
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
		// daemonise etc
		if( configuration().daemon() )
			G::Daemon::detach( pid_file.path() ) ;
		commit( pid_file ) ;
		if( configuration().closeStderr() )
			G::Process::closeStderr() ;

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

bool Main::Run::hidden() const
{
	return configuration().hidden() || configuration().show("hidden") ;
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

GNet::StreamSocket::Config Main::Run::netSocketConfig( bool /*server*/ ) const
{
	std::pair<int,int> linger = { -1 , 0 } ;
	return
		GNet::StreamSocket::Config()
			.set_create_linger( linger )
			.set_accept_linger( linger )
			.set_bind_reuse( !G::is_windows() || G::is_wine() )
			.set_bind_exclusive( G::is_windows() && !G::is_wine() )
			.set_last<GNet::StreamSocket::Config>() ;
}

GSmtp::ServerProtocol::Config Main::Run::serverProtocolConfig() const
{
	return
		GSmtp::ServerProtocol::Config()
			.set_with_vrfy( !configuration().anonymousServerVrfy() )
			.set_filter_timeout( configuration().filterTimeout() )
			.set_max_size( configuration().maxSize() )
			.set_authentication_requires_encryption( configuration().serverTlsRequired() )
			.set_mail_requires_authentication( !configuration().serverSecretsFile().empty() )
			.set_mail_requires_encryption( configuration().serverTlsRequired() )
			.set_tls_starttls( configuration().serverTls() )
			.set_tls_connection( configuration().serverTlsConnection() )
			.set_allow_pipelining( configuration().smtpPipelining() ) ;
}

GNet::Server::Config Main::Run::netServerConfig() const
{
	bool open = configuration().user().empty() || configuration().user() == "root" ;
	return GNet::Server::Config()
		.set_stream_socket_config( netSocketConfig() )
		.set_uds_open_permissions( open ) ;
}

GSmtp::Server::Config Main::Run::smtpServerConfig() const
{
	return
		GSmtp::Server::Config()
			.set_allow_remote( configuration().allowRemoteClients() )
			.set_interfaces( configuration().listeningNames("smtp") )
			.set_port( configuration().port() )
			.set_ident( smtpIdent() )
			.set_anonymous_smtp( configuration().anonymousServerSmtp() )
			.set_anonymous_content( configuration().anonymousContent() )
			.set_filter_spec( configuration().filter() )
			.set_filter_timeout( configuration().filterTimeout() )
			.set_verifier_spec( configuration().verifier() )
			.set_verifier_timeout( configuration().filterTimeout() )
			.set_server_peer_config( GNet::ServerPeer::Config().set_idle_timeout(configuration().idleTimeout()) )
			.set_server_config( netServerConfig() )
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
			.set_addresses( configuration().listeningNames("pop") )
			.set_server_peer_config(
				GNet::ServerPeer::Config()
					.set_idle_timeout( configuration().idleTimeout()) )
			.set_server_config( netServerConfig() )
			.set_sasl_server_config( configuration().popSaslServerConfig() ) ;
}

GSmtp::Client::Config Main::Run::clientConfig() const
{
	return
		GSmtp::Client::Config()
			.set_stream_socket_config( netSocketConfig(false) )
			.set_client_protocol_config(
				GSmtp::ClientProtocol::Config()
					.set_thishost_name( GNet::Local::canonicalName() )
					.set_response_timeout( configuration().responseTimeout() )
					.set_ready_timeout( configuration().promptTimeout() )
					.set_filter_timeout( configuration().filterTimeout() )
					.set_use_starttls_if_possible( configuration().clientTls() && !configuration().clientOverTls() )
					.set_must_use_tls( configuration().clientTlsRequired() && !configuration().clientOverTls() )
					.set_must_authenticate( true )
					.set_anonymous( configuration().anonymousClientSmtp() )
					.set_must_accept_all_recipients( !configuration().forwardToSome() )
					.set_eight_bit_strict( false ) )
			.set_filter_spec( configuration().clientFilter() )
			.set_filter_timeout( configuration().filterTimeout() )
			.set_bind_local_address( !configuration().clientBindAddress().empty() )
			.set_local_address( asAddress(configuration().clientBindAddress()) )
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
		( GNet::Address::validString(s,GNet::Address::NotLocal()) ? GNet::Address::parse(s,GNet::Address::NotLocal()) : GNet::Address::parse(s,0U) ) ;
}

void Main::Run::onPollTimeout()
{
	G_DEBUG( "Main::Run::onPollTimeout" ) ;
	m_poll_timer->startTimer( configuration().pollingTimeout() ) ;
	requestForwarding( "poll" ) ;
}

void Main::Run::onForwardRequest( const std::string & reason )
{
	requestForwarding( reason ) ;
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
	using G::format ;
	using G::txt ;
	if( m_client_ptr.busy() )
	{
		G_LOG( "Main::Run::onRequestForwardingTimeout: "
			<< format(txt("forwarding: [%1%]: still busy from last time")) % m_forwarding_reason ) ;
		m_forwarding_pending = true ;
	}
	else
	{
		if( logForwarding() )
		{
			G_LOG( "Main::Run::onRequestForwardingTimeout: "
				<< format(txt("forwarding: [%1%]")) % m_forwarding_reason ) ;
		}

		emit( "forward" , "start" ) ;
		std::string error = startForwarding() ;
		if( !error.empty() )
			emit( "forward" , "end" , error ) ;
	}
}

bool Main::Run::logForwarding() const
{
	return m_forwarding_reason != "poll" || configuration().pollingLog() ||
		( G::LogOutput::instance() && G::LogOutput::instance()->at( G::Log::Severity::Debug ) ) ;
}

std::string Main::Run::startForwarding()
{
	using G::txt ;
	try
	{
		if( m_store->empty() )
		{
			if( logForwarding() )
				G_LOG( "Main::Run::startForwarding: " << txt("forwarding: no messages to send") ) ;
			return "no messages" ;
		}
		else
		{
			G_ASSERT( m_client_secrets != nullptr ) ;
			m_client_ptr.reset( std::make_unique<GSmtp::Client>(
				GNet::ExceptionSink(m_client_ptr,nullptr) ,
				*m_filter_factory ,
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
		G_ERROR( "Main::Run::startForwarding: " << txt("forwarding failure") << ": " << e.what() ) ;
		return e.what() ;
	}
}

void Main::Run::onClientDone( const std::string & reason )
{
	using G::format ;
	using G::txt ;
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
			G_ERROR( "Main::Run::onClientDone: " << format(txt("forwarding: %1%")) % reason ) ;

		// go round again if necessary
		if( m_forwarding_pending )
		{
			m_forwarding_pending = false ;
			G_LOG( "Main::Run::onClientDone: "
				<< format(txt("forwarding: queued request [%1%]")) % m_forwarding_reason ) ;
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

void Main::Run::emit( const std::string & s0 , const std::string & s1 ,
	const std::string & s2 , const std::string & s3 )
{
	// use an asynchronous queue to avoid side-effects from callbacks
	using G::txt ;
	bool notifying = m_admin_server && m_admin_server->notifying() ;
	if( notifying || m_has_gui )
	{
		G_ASSERT( m_queue_timer != nullptr ) ;
		if( m_queue_timer )
			m_queue_timer->startTimer( 0U ) ;

		while( m_queue.size() > 100U )
		{
			G_WARNING_ONCE( "Main::Run::emit: " << txt("too many notification events: discarding old ones") ) ;
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
	if( client_bind_address.empty() )
		return AF_UNSPEC ;

	GNet::Address address = asAddress( client_bind_address ) ;
	return ( address.af() == AF_INET || address.af() == AF_INET6 ) ? address.af() : AF_UNSPEC ;
}

void Main::Run::checkThreading() const
{
	if( G::threading::using_std_thread )
	{
		// ignore the result here -- we are just trying to provoke an early weak-symbol linking failure
		G::threading::works() ;
	}
}

G::Path Main::Run::appDir() const
{
	G::Path this_exe = G::Arg::exe() ;
	if( this_exe.empty() ) // eg. linux with no procfs
		return G::Path(m_arg.v(0U)).dirname() ; // may be relative and/or bogus
	else if( this_exe.dirname().basename() == "MacOS" && this_exe.dirname().dirname().basename() == "Contents" )
		return this_exe.dirname().dirname().dirname() ; // .app
	else
		return this_exe.dirname() ;
}

std::unique_ptr<GSmtp::AdminServer> Main::Run::newAdminServer( GNet::ExceptionSink es ,
	const Configuration & cfg , GSmtp::MessageStore & store , GSmtp::FilterFactory & ff ,
	G::Slot::Signal<const std::string&> & forward_request_signal ,
	const GNet::ServerPeer::Config & server_peer_config , const GNet::Server::Config & net_server_config ,
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
			ff ,
			forward_request_signal ,
			server_peer_config ,
			net_server_config ,
			client_config ,
			client_secrets ,
			cfg.listeningNames("admin") ,
			cfg.adminPort() ,
			cfg.allowRemoteClients() ,
			cfg.serverAddress() ,
			cfg.connectionTimeout() ,
			info_map ,
			config_map ,
			cfg.withTerminate() ) ;
}

