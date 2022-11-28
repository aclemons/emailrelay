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
/// \file testrun.cpp
///
// A drop-in replacement for run.cpp that allows for soak testing
// if the config file on the command-line contains "%x".
//
// The %x is replaced by 1,2,3 etc and as long as there is a
// config file with that name it is used to add additional
// active components surrounding a spool directory. If the
// spool directory does not exist it is created, and if it is
// empty it is populated with a dummy message.
//
// The optional "0" file can be used for confuring singletons,
// such as logging.
//
// Eg:
//    $ ( echo log ; echo log-file log ; echo no-daemon ) > config-0
//    $ ( echo spool-dir spool-1 ; echo port 10001 ; echo forward-to localhost:10002 ; echo poll 1 ) > config-1
//    $ ( echo spool-dir spool-2 ; echo port 10002 ; echo forward-to localhost:10001 ; echo poll 1 ) > config-2
//    $ ./emailrelay config-%x
//

#include "gdef.h"
#include "gssl.h"
#include "testrun.h"
#include "options.h"
#include "gsmtpserver.h"
#include "gsmtpclient.h"
#include "gnewfile.h"
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
	return "2.4.1" ;
}

Main::Run::Unit::Unit( std::size_t unit_id , Run * run , const Configuration & config , int resolver_family ) :
	m_unit_id(unit_id) ,
	m_run(run) ,
	m_config(config) ,
	m_resolver_family(resolver_family)
{
	G_LOG( "Main::Run::Unit::ctor: unit " << m_unit_id << ": port=" << config.port() ) ;
	G_LOG( "Main::Run::Unit::ctor: unit " << m_unit_id << ": spool-dir=" << config.spoolDir() ) ;
	G_LOG( "Main::Run::Unit::ctor: unit " << m_unit_id << ": server-tls=" << config.serverTls() ) ;
	G_LOG( "Main::Run::Unit::ctor: unit " << m_unit_id << ": server-tls-certificate=" << config.serverTlsCertificate() ) ;
	G_LOG( "Main::Run::Unit::ctor: unit " << m_unit_id << ": server-tls-private-key=" << config.serverTlsPrivateKey() ) ;
	m_client_ptr = std::make_unique<GNet::ClientPtr<GSmtp::Client>>() ;
	G_ASSERT( !(*m_client_ptr).busy() ) ;
}

void Main::Run::Unit::create( GSsl::Library * tls_library ,
	GNet::Server::Config net_server_config ,
	GSmtp::Server::Config smtp_server_config , GSmtp::Client::Config smtp_client_config ,
	GPop::Server::Config pop_server_config )
{
	(*m_client_ptr).deletedSignal().connect( G::Slot::slot(*this,&Unit::onClientDone) ) ;
	(*m_client_ptr).eventSignal().connect( G::Slot::slot(*this,&Unit::onClientEvent) ) ;
	m_forward_request_signal.connect( G::Slot::slot(*this,&Unit::onForwardRequest) ) ;

	auto es_log_only = GNet::ExceptionSink::logOnly() ;
	m_forwarding_timer = std::make_unique<GNet::Timer<Unit>>( *this , &Unit::onRequestForwardingTimeout , es_log_only ) ;
	m_poll_timer = std::make_unique<GNet::Timer<Unit>>( *this , &Unit::onPollTimeout , es_log_only ) ;

	bool do_smtp = m_config.doServing() && m_config.doSmtp() ;
	bool do_pop = m_config.doServing() && m_config.doPop() ;
	bool do_admin = m_config.doServing() && m_config.doAdmin() ;

	std::string server_tls_profile = std::string("server-").append(std::to_string(m_unit_id)) ;
	std::string client_tls_profile = std::string("client-").append(std::to_string(m_unit_id)) ;

	smtp_server_config.server_peer_config.socket_protocol_config.set_server_tls_profile( server_tls_profile ) ;
	pop_server_config.server_peer_config.socket_protocol_config.set_server_tls_profile( server_tls_profile ) ;
	smtp_client_config.set_client_tls_profile( client_tls_profile ) ;

	if( m_config.serverTls() || m_config.serverTlsConnection() )
	{
		G_ASSERT( tls_library != nullptr ) ;
		G_LOG( "Main::Run::Unit::create: unit " << m_unit_id << ": server tls profile: " << server_tls_profile ) ;
		tls_library->addProfile( server_tls_profile , true ,
			m_config.serverTlsPrivateKey().str() ,
			m_config.serverTlsCertificate().str() ,
			m_config.serverTlsCaList().str() ) ;
	}
	else
	{
		G_LOG( "Main::Run::Unit::create: unit " << m_unit_id << ": no server tls" ) ;
	}

	if( m_config.clientTls() || m_config.clientOverTls() )
	{
		G_ASSERT( tls_library != nullptr ) ;
		G_LOG( "Main::Run::Unit::create: unit " << m_unit_id << ": client tls profile: " << client_tls_profile ) ;
		tls_library->addProfile( client_tls_profile , false ,
			m_config.clientTlsPrivateKey().str() ,
			m_config.clientTlsCertificate().str() ,
			m_config.clientTlsCaList().str() ,
			m_config.clientTlsPeerCertificateName() ,
			m_config.clientTlsPeerHostName() ) ;
	}
	else
	{
		G_LOG( "Main::Run::Unit::create: unit " << m_unit_id << ": no client tls" ) ;
	}

	using G::format ;
	using G::txt ;
	auto es_rethrow = GNet::ExceptionSink() ;

	m_smtp_client_config = std::make_unique<GSmtp::Client::Config>( smtp_client_config ) ;

	G::File::mkdir( m_config.spoolDir() , std::nothrow ) ;

	m_store = std::make_unique<GSmtp::FileStore>( m_config.spoolDir() ,
		m_config.maxSize() , m_config.eightBitTest() ) ;

	if( m_unit_id && m_store->empty() )
	{
		std::string from = m_config.spoolDir().basename() ;
		GSmtp::NewFile new_file( *m_store , from , {} , {} , 0U , false ) ;
		GSmtp::NewMessage & new_message = new_file ;
		new_message.addTo( "you" , false ) ;
		new_message.addTextLine( "Subject: test" ) ;
		new_message.addTextLine( {} ) ;
		new_message.addTextLine( "created by testrun.cpp" ) ;
		new_message.prepare( {} , "127.0.0.1" , {} ) ;
		new_message.commit( true ) ;
	}

	m_filter_factory = std::make_unique<GSmtp::FilterFactoryFileStore>( *m_store ) ;
	m_store->messageStoreRescanSignal().connect( G::Slot::slot(*this,&Unit::onStoreRescanEvent) ) ;
	if( do_pop )
	{
		m_pop_store = std::make_unique<GPop::Store>( m_config.spoolDir() ,
			m_config.popByName() , ! m_config.popNoDelete() ) ;
	}

	// authentication secrets
	//
	GAuth::Secrets::check( m_config.clientSecretsFile().str() , m_config.serverSecretsFile().str() ,
		m_config.doPop() ? m_config.popSecretsFile().str() : std::string() ) ;
	m_client_secrets = std::make_unique<GAuth::Secrets>( m_config.clientSecretsFile().str() , "client" ) ;
	m_server_secrets = std::make_unique<GAuth::Secrets>( m_config.serverSecretsFile().str() , "server" ) ;
	if( m_config.doPop() )
		m_pop_secrets = std::make_unique<GAuth::Secrets>( m_config.popSecretsFile().str() , "pop-server" ) ;

	// create the smtp server
	//
	if( do_smtp )
	{
		if( m_config.immediate() )
			G_WARNING( "Run::doServing: " << txt("using --immediate can result in client timeout errors: "
				"try --forward-on-disconnect instead") ) ;

		G_ASSERT( m_store != nullptr ) ;
		G_ASSERT( m_client_secrets != nullptr ) ;
		G_ASSERT( m_server_secrets != nullptr ) ;
		m_smtp_server = std::make_unique<GSmtp::Server>(
			es_rethrow ,
			*m_store ,
			*m_filter_factory ,
			*m_client_secrets ,
			*m_server_secrets ,
			smtp_server_config ,
			m_config.immediate() ? m_config.serverAddress() : std::string() ,
			m_resolver_family ,
			*m_smtp_client_config ) ;

		m_smtp_server->eventSignal().connect( G::Slot::slot(*this,&Unit::onServerEvent) ) ;
	}

	// create the pop server
	//
	if( do_pop )
	{
		G_ASSERT( m_pop_store != nullptr ) ;
		G_ASSERT( m_pop_secrets != nullptr ) ;
		m_pop_server = std::make_unique<GPop::Server>(
			es_rethrow ,
			*m_pop_store ,
			*m_pop_secrets ,
			pop_server_config ) ;
	}

	// create the admin server
	//
	if( do_admin )
	{
		G_ASSERT( m_store != nullptr ) ;
		G_ASSERT( m_client_secrets != nullptr ) ;
		m_admin_server = newAdminServer(
			es_rethrow ,
			m_config ,
			*m_store ,
			*m_filter_factory ,
			m_forward_request_signal ,
			GNet::ServerPeer::Config().set_idle_timeout(0U) ,
			net_server_config ,
			smtp_client_config ,
			*m_client_secrets ,
			versionNumber() ) ;
	}
}

void Main::Run::Unit::start()
{
	// kick off some forwarding
	//
	if( m_config.forwardOnStartup() )
		requestForwarding( "startup" ) ;

	// kick off the polling cycle
	//
	if( m_config.doPolling() )
		m_poll_timer->startTimer( m_config.pollingTimeout() ) ;

	// report stuff
	//
	if( m_smtp_server ) m_smtp_server->report() ;
	if( m_admin_server ) m_admin_server->report() ;
	if( m_pop_server ) m_pop_server->report() ;
}

std::string Main::Run::Unit::exceptionSourceId() const
{
	return m_config.spoolDir().basename() ;
}

bool Main::Run::needTls( const Configuration & config ) const
{
	return
		config.clientTls() ||
		config.clientOverTls() ||
		config.serverTls() ||
		config.serverTlsConnection() ;
}

bool Main::Run::preferTls( const Configuration & config ) const
{
	// secrets might need hash functions from tls library
	return
		!config.clientSecretsFile().empty() ||
		!config.serverSecretsFile().empty() ||
		!config.popSecretsFile().empty() ;
}

Main::Run::Run( Main::Output & output , const G::Arg & arg , bool is_windows , bool has_gui ) :
	m_output(output) ,
	m_is_windows(is_windows) ,
	m_arg(arg) ,
	m_quit_when_sent(false) ,
	m_has_gui(has_gui)
{
	// initialise gettext() early iff "--localedir" is used
	{
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

	// initialise the base configuration
	//
	bool multi = m_arg.c() > 1U && m_arg.v(m_arg.c()-1U).find("%x") != std::string::npos ;
	if( multi )
	{
		std::string config_file = m_arg.v(m_arg.c()-1U) ;
		G::Str::replace( config_file , "%x" , "0" ) ;
		G::StringArray parts = m_arg.array() ;
		parts.pop_back() ;
		if( G::File::exists(config_file) ) parts.push_back( config_file ) ; // "0" file is optional
		m_commandline = std::make_unique<CommandLine>( m_output , G::Arg(parts) , Main::Options::spec(m_is_windows) , versionNumber() ) ;
		m_configuration = std::make_unique<Configuration>( m_commandline->options() , m_commandline->map() , appDir() , G::Process::cwd() ) ;
	}
	else
	{
		m_commandline = std::make_unique<CommandLine>( m_output , m_arg , Main::Options::spec(m_is_windows) , versionNumber() ) ;
		m_configuration = std::make_unique<Configuration>( m_commandline->options() , m_commandline->map() , appDir() , G::Process::cwd() ) ;
	}
}

void Main::Run::configure()
{
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

	// tighten the umask
	//
	G::Process::Umask::set( G::Process::Umask::Mode::Tightest ) ;

	// log command-line warnings
	//
	if( !m_output.outputSimple() )
	{
		commandline().logSemanticWarnings( configuration().semanticWarnings() ) ;
	}

	// if we are going to close stderr soon then make stderr logging
	// less verbose so that startup scripts are cleaner, but without
	// affecting syslog output
	if( configuration().useSyslog() && configuration().daemon() &&
		configuration().closeStderr() && configuration().logFile().empty() )
			m_log_output->configure( m_log_output->config().set_quiet_stderr() ) ;

	// release root privileges and extra group memberships
	//
	if( configuration().user() != "root" )
		G::Root::init( configuration().user() ) ;

	// create event loop singletons
	//
	m_event_loop = GNet::EventLoop::create() ;
	m_timer_list = std::make_unique<GNet::TimerList>() ;

	// early check on multi-threading behaviour
	//
	checkThreading() ;

	// network monitor singleton
	//
	m_monitor = std::make_unique<GNet::Monitor>() ;
	m_monitor->signal().connect( G::Slot::slot(*this,&Run::onNetworkEvent) ) ;

	// early check that the forward-to address can be resolved
	//
	if( configuration().log() && !configuration().serverAddress().empty() && !configuration().forwardOnStartup() &&
		!GNet::Address::isFamilyLocal( configuration().serverAddress() ) )
	{
		GNet::Location location( configuration().serverAddress() , resolverFamily(configuration()) ) ;
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

	// prepare the pid file
	//
	G::Path pid_file_path = configuration().usePidFile() ? G::Path(configuration().pidFile()) : G::Path() ;
	G::PidFile pid_file( pid_file_path ) ;
	{
		G::Root claim_root ;
		G::Process::Umask _( G::Process::Umask::Mode::GroupOpen ) ;
		pid_file.mkdir() ;
	}

	// read the config files
	//
	std::vector<G::Arg> args_list ;
	bool need_tls = false ;
	bool prefer_tls = false ;
	std::string tls_config ;
	bool multi = m_arg.c() > 1U && m_arg.v(m_arg.c()-1U).find("%x") != std::string::npos ;
	if( multi )
	{
		for( int i = 1 ; i < 100 ; i++ )
		{
			std::string config_file = m_arg.v(m_arg.c()-1U) ;
			G::Str::replace( config_file , "%x" , G::Str::fromInt(i) ) ;
			if( !G::File::exists(config_file) ) break ;
			G_LOG( "Unit::ctor: reading config file: " << config_file ) ;
			G::StringArray arg_parts = m_arg.array() ;
			arg_parts.back() = config_file ;
			CommandLine cl( m_output , G::Arg(arg_parts) , Main::Options::spec(m_is_windows) , versionNumber() ) ;
			if( cl.hasUsageErrors() )
				throw G::Exception( "unit configuration file error" , config_file , G::Str::join("|",cl.usageErrors()) ) ;
			Configuration cfg( cl.options() , cl.map() , appDir() , G::Process::cwd() ) ;
			if( !cfg.semanticError().empty() )
				throw G::Exception( "unit configuration file error" , config_file , cfg.semanticError() ) ;
			if( !cfg.tlsConfig().empty() )
				throw G::Exception( "invalid tls configuration string in unit configuration" ) ;
			args_list.push_back( G::Arg(arg_parts) ) ;
			if( needTls(cfg) ) need_tls = true ;
			if( preferTls(cfg) ) prefer_tls = true ;
		}
	}
	else
	{
		need_tls = needTls( configuration() ) ;
		prefer_tls = preferTls( configuration() ) ;
		tls_config = configuration().tlsConfig() ;
	}

	// tls library setup
	//
	G_LOG( "Run::ctor: need tls: " << need_tls ) ;
	G_LOG( "Run::ctor: prefer tls: " << prefer_tls ) ;
	m_tls_library = std::make_unique<GSsl::Library>( need_tls || prefer_tls ,
		tls_config , GSsl::Library::log , configuration().debug() ) ;
	if( need_tls && !m_tls_library->enabled() )
		throw G::Exception( txt("cannot do tls/ssl: tls library not built in: "
			"remove tls options from the command-line or "
			"rebuild the emailrelay executable with a supported tls library") ) ;

	// create the units
	//
	if( multi )
	{
		for( std::size_t i = 0U ; i < args_list.size() ; i++ )
		{
			G::Arg args = args_list.at( i ) ;
			G_LOG( "Unit::ctor: args: " << G::Str::join("|",args.array(1U)) ) ;
			CommandLine cl( m_output , G::Arg(args) , Main::Options::spec(m_is_windows) , versionNumber() ) ;
			Configuration cfg( cl.options() , cl.map() , appDir() , G::Process::cwd() ) ;
			m_unit.emplace_back( i+1U , this , cfg , resolverFamily(cfg) ) ;
		}
	}
	else
	{
		m_unit.emplace_back( 0U , this , configuration() , resolverFamily(configuration()) ) ;
	}
	if( m_unit.empty() )
		throw G::Exception( "no units" ) ;

	// figure out what we're doing
	//
	bool do_smtp = configuration().doServing() && configuration().doSmtp() ;
	bool do_pop = configuration().doServing() && configuration().doPop() ;
	bool do_admin = configuration().doServing() && configuration().doAdmin() ;
	bool serving = do_smtp || do_pop || do_admin ;
	bool admin_forwarding = do_admin && !configuration().serverAddress().empty() ;
	bool forwarding = configuration().forwardOnStartup() || configuration().doPolling() || admin_forwarding ;
	m_quit_when_sent =
		!serving &&
		configuration().forwardOnStartup() &&
		!configuration().doPolling() &&
		!admin_forwarding ;

	// activate the units
	//
	for( auto & unit : m_unit )
	{
		std::string name = unit.m_config.spoolDir().basename() ;
		unit.create( m_tls_library.get() ,
			netServerConfig(unit.m_config) ,
			smtpServerConfig(unit.m_config) ,
			clientConfig(unit.m_config,name) ,
			popConfig(unit.m_config) ) ;
	}

	// do serving and/or forwarding
	//
	if( !serving && !forwarding )
	{
		commandline().showNothingToDo( true ) ;
	}
	else if( m_quit_when_sent &&
		std::all_of( m_unit.begin() , m_unit.end() ,
			[](Unit &unit){return unit.m_store && unit.m_store->empty();} ) )
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

		for( auto & unit : m_unit )
		{
			unit.start() ;
		}

		// run the event loop
		//
		std::string quit_reason = m_event_loop->run() ;
		if( !quit_reason.empty() )
			throw std::runtime_error( quit_reason ) ;
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

GSmtp::ServerProtocol::Config Main::Run::serverProtocolConfig( const Configuration & config ) const
{
	return
		GSmtp::ServerProtocol::Config()
			.set_with_vrfy( !config.anonymousServerVrfy() )
			.set_filter_timeout( config.filterTimeout() )
			.set_max_size( config.maxSize() )
			.set_authentication_requires_encryption( config.serverTlsRequired() )
			.set_mail_requires_authentication( !config.serverSecretsFile().empty() )
			.set_mail_requires_encryption( config.serverTlsRequired() )
			.set_tls_starttls( config.serverTls() )
			.set_tls_connection( config.serverTlsConnection() )
			.set_allow_pipelining( config.smtpPipelining() ) ;
}

GNet::Server::Config Main::Run::netServerConfig( const Configuration & config ) const
{
	bool open = config.user().empty() || config.user() == "root" ;
	return GNet::Server::Config()
		.set_stream_socket_config( netSocketConfig() )
		.set_uds_open_permissions( open ) ;
}

GSmtp::Server::Config Main::Run::smtpServerConfig( const Configuration & config ) const
{
	return
		GSmtp::Server::Config()
			.set_allow_remote( config.allowRemoteClients() )
			.set_interfaces( config.listeningNames("smtp") )
			.set_port( config.port() )
			.set_ident( smtpIdent() )
			.set_anonymous_smtp( config.anonymousServerSmtp() )
			.set_anonymous_content( config.anonymousContent() )
			.set_filter_spec( config.filter() )
			.set_filter_timeout( config.filterTimeout() )
			.set_verifier_spec( config.verifier() )
			.set_verifier_timeout( config.filterTimeout() )
			.set_server_peer_config( GNet::ServerPeer::Config().set_idle_timeout(config.idleTimeout()) )
			.set_server_config( netServerConfig(config) )
			.set_protocol_config( serverProtocolConfig(config) )
			.set_sasl_server_config( config.smtpSaslServerConfig() )
			.set_dnsbl_config( config.dnsbl() ) ;
}

GPop::Server::Config Main::Run::popConfig( const Configuration & config ) const
{
	return
		GPop::Server::Config()
			.set_allow_remote( config.allowRemoteClients() )
			.set_port( config.popPort() )
			.set_addresses( config.listeningNames("pop") )
			.set_server_peer_config(
				GNet::ServerPeer::Config()
					.set_idle_timeout( config.idleTimeout()) )
			.set_server_config( netServerConfig(config) )
			.set_sasl_server_config( config.popSaslServerConfig() ) ;
}

GSmtp::Client::Config Main::Run::clientConfig( const Configuration & config , const std::string & name ) const
{
	return
		GSmtp::Client::Config()
			.set_stream_socket_config( netSocketConfig(false) )
			.set_client_protocol_config(
				GSmtp::ClientProtocol::Config()
					.set_thishost_name( name )
					.set_response_timeout( config.responseTimeout() )
					.set_ready_timeout( config.promptTimeout() )
					.set_filter_timeout( config.filterTimeout() )
					.set_use_starttls_if_possible( config.clientTls() && !config.clientOverTls() )
					.set_must_use_tls( config.clientTlsRequired() && !config.clientOverTls() )
					.set_must_authenticate( true )
					.set_anonymous( config.anonymousClientSmtp() )
					.set_must_accept_all_recipients( !config.forwardToSome() )
					.set_eight_bit_strict( false ) )
			.set_filter_spec( config.clientFilter() )
			.set_filter_timeout( config.filterTimeout() )
			.set_bind_local_address( !config.clientBindAddress().empty() )
			.set_local_address( asAddress(config.clientBindAddress()) )
			.set_connection_timeout( config.connectionTimeout() )
			.set_secure_connection_timeout( config.secureConnectionTimeout() )
			.set_secure_tunnel( config.clientOverTls() )
			.set_sasl_client_config( config.smtpSaslClientConfig() ) ;
}

GNet::Address Main::Run::asAddress( const std::string & s )
{
	// (port number is optional)
	return s.empty() ?
		GNet::Address::defaultAddress() :
		( GNet::Address::validString(s,GNet::Address::NotLocal()) ? GNet::Address::parse(s,GNet::Address::NotLocal()) : GNet::Address::parse(s,0U) ) ;
}

void Main::Run::Unit::onPollTimeout()
{
	G_DEBUG( "Main::Run::onPollTimeout" ) ;
	m_poll_timer->startTimer( m_config.pollingTimeout() ) ;
	requestForwarding( "poll" ) ;
}

void Main::Run::Unit::onForwardRequest( const std::string & reason )
{
	requestForwarding( reason ) ;
}

void Main::Run::Unit::requestForwarding( const std::string & reason )
{
	G_ASSERT( m_forwarding_timer != nullptr ) ;
	G_LOG( "Main::Run::Unit::requestForwarding: " << m_config.spoolDir().basename() << ": forwarding request [" << reason << "]" ) ;
	if( !reason.empty() )
		m_forwarding_reason = reason ;
	m_forwarding_timer->startTimer( 0U ) ;
}

void Main::Run::Unit::onRequestForwardingTimeout()
{
	using G::format ;
	using G::txt ;
	if( (*m_client_ptr).busy() )
	{
		G_LOG( "Main::Run::onRequestForwardingTimeout: " << m_config.spoolDir().basename() << ": "
			<< format(txt("forwarding: [%1%]: still busy from last time")) % m_forwarding_reason ) ;
		m_forwarding_pending = true ;
	}
	else
	{
		if( logForwarding() )
		{
			G_LOG( "Main::Run::onRequestForwardingTimeout: " << m_config.spoolDir().basename() << ": "
				<< format(txt("forwarding: [%1%]")) % m_forwarding_reason ) ;
		}

		emit( "forward" , "start" ) ;
		std::string error = startForwarding() ;
		if( !error.empty() )
			emit( "forward" , "end" , error ) ;
	}
}

bool Main::Run::Unit::logForwarding() const
{
	return m_forwarding_reason != "poll" || m_config.pollingLog() ||
		( G::LogOutput::instance() && G::LogOutput::instance()->at( G::Log::Severity::Debug ) ) ;
}

std::string Main::Run::Unit::startForwarding()
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
			G_LOG( "Main::Run::startForwarding: " << m_config.spoolDir().basename() << ": "
				<< "now forwarding to " << m_config.serverAddress() << " "
				<< "(resolver family " << m_resolver_family << ")" ) ;
			G_ASSERT( m_client_secrets != nullptr ) ;
			(*m_client_ptr).reset( std::make_unique<GSmtp::Client>(
				GNet::ExceptionSink(*m_client_ptr,this) ,
				*m_filter_factory ,
				GNet::Location(m_config.serverAddress(),m_resolver_family) ,
				*m_client_secrets ,
				*m_smtp_client_config ) ) ;

			G_ASSERT( m_store != nullptr ) ;
			(*m_client_ptr)->sendMessagesFrom( *m_store ) ; // once connected
			return std::string() ;
		}
	}
	catch( std::exception & e )
	{
		G_ERROR( "Main::Run::startForwarding: " << txt("forwarding failure") << ": " << e.what() ) ;
		return e.what() ;
	}
}

void Main::Run::Unit::onClientDone( const std::string & reason )
{
	using G::format ;
	using G::txt ;
	G_DEBUG( "Main::Run::onClientDone: reason=[" << reason << "]" ) ;
	bool quit = m_run->onClientDone( reason ) ;
	if( !quit )
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

bool Main::Run::onClientDone( const std::string & reason )
{
	if( m_quit_when_sent )
	{
		// quit the event loop
		if( m_event_loop )
			m_event_loop->quit( reason ) ;
		return true ;
	}
	else
	{
		return false ;
	}
}

void Main::Run::Unit::onClientEvent( const std::string & s1 , const std::string & s2 , const std::string & s3 )
{
	emit( "client" , s1 , s2 , s3 ) ;
}

void Main::Run::Unit::onServerEvent( const std::string & s1 , const std::string & )
{
	if( s1 == "done" && m_config.forwardOnDisconnect() )
		requestForwarding( "client disconnect" ) ;
}

void Main::Run::Unit::onStoreRescanEvent()
{
	requestForwarding( "rescan" ) ;
}

void Main::Run::onNetworkEvent( const std::string & s1 , const std::string & s2 )
{
	emit( "network" , s1 , s2 ) ;
}

void Main::Run::Unit::emit( const std::string & s0 , const std::string & s1 ,
	const std::string & s2 , const std::string & s3 )
{
	m_run->emit( s0 , s1 , s2 , s3 ) ;
}

void Main::Run::emit( std::string s0 , std::string s1 , std::string s2 , std::string s3 )
{
	// (now gutted)
}

G::Slot::Signal<std::string,std::string,std::string,std::string> & Main::Run::signal()
{
	return m_signal ;
}

int Main::Run::resolverFamily( const Configuration & config ) const
{
	// choose an address family for the DNS lookup based on the "--client-interface" address
	std::string client_bind_address = config.clientBindAddress() ;
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

std::unique_ptr<GSmtp::AdminServer> Main::Run::Unit::newAdminServer( GNet::ExceptionSink es ,
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

