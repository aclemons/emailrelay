//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "run.h"
#include "options.h"
#include "gssl.h"
#include "gpop.h"
#include "geventloop.h"
#include "garg.h"
#include "gdaemon.h"
#include "gpidfile.h"
#include "gslot.h"
#include "gmonitor.h"
#include "glocal.h"
#include "groot.h"
#include "gexception.h"
#include "gprocess.h"
#include "gformat.h"
#include "ggettext.h"
#include "gstrmacros.h"
#include "gscope.h"
#include "gtest.h"
#include "glogoutput.h"
#include "glog.h"
#include "gassert.h"
#include "legal.h"
#include <iostream>
#include <exception>
#include <memory>
#include <algorithm>
#include <utility>

#ifdef G_LOCALEDIR
namespace Main { std::string localedir() { return G_STR(G_LOCALEDIR) ; } }
#else
namespace Main { std::string localedir() { return std::string() ; } }
#endif

std::string Main::Run::versionNumber()
{
	return "2.5.1a" ;
}

Main::Run::Run( Main::Output & output , const G::Arg & arg , bool has_gui ) :
	m_output(output) ,
	m_arg(arg) ,
	m_has_gui(has_gui)
{
	// initialise gettext() early, before G::GetOpt
	std::string ldir = localedir() ;
	std::size_t pos = m_arg.index( "--localedir" , 1U ) ;
	std::size_t mpos = m_arg.match( "--localedir=" ) ;
	if( pos )
		ldir = m_arg.removeAt( pos , 1U ) ;
	else if( mpos )
		ldir = G::Str::tail( m_arg.removeAt(mpos) , "=" ) ;
	if( !ldir.empty() )
		G::gettext_init( ldir , "emailrelay" ) ;
}

Main::Run::~Run()
{
	if( m_monitor )
		m_monitor->signal().disconnect() ;
	for( auto & unit_ptr : m_units )
	{
		G_ASSERT( unit_ptr.get() != nullptr ) ;
		unit_ptr->clientDoneSignal().disconnect() ;
		unit_ptr->eventSignal().disconnect() ;
	}
}

void Main::Run::configure( const G::Options & options_spec )
{
	// lazy construction so that the constructor doesn't throw
	m_commandline = std::make_unique<CommandLine>( m_output ,
		m_arg , options_spec , versionNumber() ) ;

	G::Path cwd = G::Process::cwd() ; // before G::Daemon::detach()
	G_ASSERT( cwd.isAbsolute() ) ;

	// the CommandLine normally parses out one set of options
	// and we use it to create a single Configuration object --
	// but also allow multiple configurations, resulting in
	// several active 'units' running in parallel
	//
	for( std::size_t i = 0U ; i < m_commandline->configurations() ; i++ )
	{
		m_configurations.emplace_back( m_commandline->configurationOptionMap(i) ,
			m_commandline->configurationName(i) , appDir() , cwd ) ;
	}
}

const Main::Configuration & Main::Run::configuration( std::size_t i ) const
{
	return m_configurations.at( i ) ;
}

const Main::Unit & Main::Run::unit( unsigned int unit_id ) const
{
	G_ASSERT( m_units.at(unit_id).get() != nullptr ) ;
	return *m_units.at( unit_id ) ;
}

std::size_t Main::Run::configurations() const
{
	return m_configurations.size() ;
}

bool Main::Run::runnable()
{
	if( commandline()[0].contains("help") )
	{
		commandline().showHelp( false ) ;
		return true ;
	}
	else if( commandline().hasUsageErrors() )
	{
		commandline().showUsageErrors( true ) ;
		return false ;
	}
	else if( commandline()[0].contains("version") )
	{
		commandline().showVersion( false ) ;
		return true ;
	}
	else if( commandline().argcError() )
	{
		commandline().showArgcError( true ) ;
		return false ;
	}

	for( std::size_t i = 0U ; i < configurations() ; i++ )
	{
		if( !configuration(i).semanticError().empty() )
		{
			commandline().showSemanticError( configuration(i).semanticError() ) ;
			return false ;
		}
	}

	if( m_output.outputSimple() )
	{
		for( std::size_t i = 0U ; i < configurations() ; i++ )
		{
			if( !configuration(i).semanticWarnings().empty() )
			{
				commandline().showSemanticWarnings( configuration(i).semanticWarnings() ) ;
			}
		}
	}

	if( commandline()[0].contains("test") )
	{
		G::Test::set( commandline()[0].value("test") ) ;
	}

	return true ;
}

void Main::Run::run()
{
	// optionally show help and quit
	//
	if( commandline()[0].contains("help") || commandline()[0].contains("version") )
		return ;

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

	// open log file and/or syslog after closeFiles() and before Root::init()
	//
	m_log_output =
		std::make_unique<G::LogOutput>( m_arg.prefix() ,
			configuration().logOutputConfig(m_has_gui) ,
			configuration().logFile() ) ;

	// if we are going to close stderr soon then make stderr logging
	// less verbose so that startup scripts are cleaner, but without
	// affecting syslog output
	//
	if( configuration().useSyslog() && configuration().daemon() &&
		configuration().closeStderr() && configuration().logFile().empty() )
	{
		m_log_output->configure( m_log_output->config().set_quiet_stderr() ) ;
	}

	// log command-line warnings
	//
	if( !m_output.outputSimple() )
	{
		for( std::size_t i = 0U ; i < configurations() ; i++ )
		{
			if( !configuration(i).semanticWarnings().empty() )
			{
				commandline().logSemanticWarnings( configuration(i).semanticWarnings() ) ;
			}
		}
	}

	// release root privileges and extra group memberships
	//
	if( configuration().user() != "root" )
	{
		G::Root::init( configuration().user() ) ;
	}

	// create event loop singletons
	//
	m_event_loop = GNet::EventLoop::create() ;
	m_timer_list = std::make_unique<GNet::TimerList>() ;

	// set up the output event queue
	//
	auto log_only = GNet::ExceptionSink::logOnly() ;
	m_queue_timer = std::make_unique<GNet::Timer<Run>>( *this , &Run::onQueueTimeout , log_only ) ;

	// early check on multi-threading behaviour
	//
	checkThreading() ;

	// tls library setup
	//
	bool need_tls = false ;
	bool prefer_tls = false ;
	for( std::size_t i = 0U ; i < configurations() ; i++ )
	{
		need_tls = need_tls || Unit::needsTls( configuration(i) ) ;
		prefer_tls = prefer_tls || Unit::prefersTls( configuration(i) ) ; // secrets might need hash functions from tls library
	}

	m_tls_library = std::make_unique<GSsl::Library>( need_tls || prefer_tls ,
		configuration().tlsConfig() , GSsl::Library::log , configuration().debug() ) ;

	if( need_tls && !m_tls_library->enabled() )
	{
		using G::txt ;
		throw G::Exception( txt("cannot do tls/ssl: tls library not built in: "
			"remove tls options from the command-line or "
			"rebuild the emailrelay executable with a supported tls library") ) ;
	}

	// network monitor singleton
	//
	m_monitor = std::make_unique<GNet::Monitor>() ;
	m_monitor->signal().connect( G::Slot::slot(*this,&Run::onNetworkEvent) ) ;

	// create the active units
	//
	for( std::size_t i = 0U ; i < configurations() ; i++ )
	{
		m_units.push_back( std::make_unique<Unit>( *this , static_cast<unsigned>(i) , versionNumber() ) ) ;
		m_units.back()->clientDoneSignal().connect( G::Slot::slot(*this,&Run::onUnitDone) ) ;
		m_units.back()->eventSignal().connect( G::Slot::slot(*this,&Run::onUnitEvent) ) ;
	}

	// do serving and/or forwarding
	//
	if( m_units.at(0U)->nothingToDo() )
	{
		commandline().showNothingToDo( true ) ;
	}
	else if( m_units[0]->quitWhenSent() && m_units[0]->nothingToSend() )
	{
		commandline().showNothingToSend( true ) ;
	}
	else
	{
		// prepare the pid file
		//
		G::Path pid_file_path = configuration().usePidFile() ? G::Path(configuration().pidFile()) : G::Path() ;
		G::PidFile pid_file( pid_file_path ) ;
		{
			G::Root claim_root ;
			G::Process::Umask _( G::Process::Umask::Mode::GroupOpen ) ;
			pid_file.mkdir() ;
		}

		// daemonise, create the pid file and close stderr
		//
		if( configuration().daemon() )
			G::Daemon::detach( pid_file.path() ) ;
		commit( pid_file ) ;
		if( configuration().closeStderr() )
			G::Process::closeStderr() ;

		// start the units
		//
		for( auto & unit : m_units )
		{
			unit->start() ;
		}

		// run the event loop
		//
		std::string quit_reason = m_event_loop->run() ;
		if( !quit_reason.empty() )
			throw std::runtime_error( quit_reason ) ;
	}
}

std::string Main::Run::defaultDomain() const
{
	if( m_default_domain.empty() )
	{
		m_default_domain = GNet::Local::canonicalName() ;
		G_ASSERT( !m_default_domain.empty() ) ;
	}
	return m_default_domain ;
}

const Main::CommandLine & Main::Run::commandline() const
{
	G_ASSERT( m_commandline != nullptr ) ;
	return *m_commandline ;
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

void Main::Run::onUnitDone( unsigned int unit_id , std::string reason , bool quit_when_sent )
{
	if( quit_when_sent && unit_id == 0U )
	{
		if( m_event_loop )
			m_event_loop->quit( reason ) ;
	}
	else if( !reason.empty() )
	{
		using G::txt ;
		using G::format ;
		G_ERROR( "Main::Run::onUnitDone: " << format(txt("forwarding: %1%")) % reason ) ;
	}
}

void Main::Run::onUnitEvent( unsigned int /*unit_id*/ , std::string s1 , std::string s2 , std::string s3 )
{
	G_ASSERT( s1 == "forward" || s1 == "resolving" || s1 == "connecting" || s1 == "connected" || s1 == "sending" || s1 == "sent" ) ;
	addToSignalQueue( "client" , s1 , s2 , s3 ) ; // eg. client,connecting,127.0.0.1:25
}

void Main::Run::onNetworkEvent( const std::string & s1 , const std::string & s2 )
{
	G_ASSERT( s1 == "in" || s1 == "out" || s1 == "listen" ) ;
	addToSignalQueue( "network" , s1 , s2 ) ; // eg. network,out,start
}

void Main::Run::addToSignalQueue( const std::string & s0 , const std::string & s1 ,
	const std::string & s2 , const std::string & s3 )
{
	// emit events via an asynchronous queue to avoid side-effects from callbacks

	bool to_gui = m_has_gui ;
	bool to_admin = !m_units.empty() && m_units.at(0U) && m_units[0]->adminNotification() ;

	if( to_gui )
		m_queue.emplace_back( 1 , s0 , s1 , s2 , s3 ) ;

	if( to_admin )
		m_queue.emplace_back( 2 , s0 , s1 , s2 , s3 ) ;

	if( to_gui || to_admin )
	{
		while( m_queue.size() > 100U )
		{
			using G::txt ;
			G_WARNING_ONCE( "Main::Run::emit: " << txt("too many notification events: discarding old ones") ) ;
			m_queue.pop_front() ;
		}

		G_ASSERT( m_queue_timer != nullptr ) ;
		m_queue_timer->startTimer( 0U ) ;
	}
}

void Main::Run::onQueueTimeout()
{
	// send queued items to gui (via the signal()) and to the first unit's admin interface
	if( !m_queue.empty() )
	{
		m_queue_timer->startTimer( 0U ) ;
		G::ScopeExit _( [&](){m_queue.pop_front();} ) ;
		const QueueItem & item = m_queue.front() ;
		if( item.target == 1 )
			m_signal.emit( item.s0 , item.s1 , item.s2 , item.s3 ) ; // see Main::WinForm::setStatus()
		else if( item.target == 2 && !m_units.empty() )
			m_units.at(0U)->adminNotify( item.s0 , item.s1 , item.s2 , item.s3 ) ; // see GSmtp::AdminServerPeer::notify()
	}
}

void Main::Run::checkThreading() const
{
	if( G::threading::using_std_thread )
	{
		// try to provoke an early weak-symbol linking failure
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

G::Slot::Signal<std::string,std::string,std::string,std::string> & Main::Run::signal() noexcept
{
	return m_signal ;
}

