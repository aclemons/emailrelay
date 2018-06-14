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
///
/// \file run.h
///

#ifndef G_MAIN_RUN_H
#define G_MAIN_RUN_H

#include "gdef.h"
#include "gsmtp.h"
#include "gssl.h"
#include "configuration.h"
#include "commandline.h"
#include "output.h"
#include "geventloop.h"
#include "gtimer.h"
#include "gclientptr.h"
#include "glogoutput.h"
#include "gmonitor.h"
#include "gdaemon.h"
#include "gpidfile.h"
#include "gslot.h"
#include "garg.h"
#include "gsecrets.h"
#include "gmessagestore.h"
#include "gfilestore.h"
#include "gsmtpclient.h"
#include "gsmtpserver.h"
#include "gadminserver.h"
#include "gpopserver.h"
#include "gpopstore.h"
#include "gpopsecrets.h"
#include <iostream>
#include <exception>
#include <memory>

namespace Main
{
	class Run ;
}

/// \class Main::Run
/// A top-level class for the process.
/// Usage:
/// \code
/// int main( int argc , char ** argv )
/// {
///   G::Arg arg( argc , argv ) ;
///   Output output ;
///   Main::Run run( output , arg , Main::Options::spec() ) ;
///   run.configure() ;
///   if( run.runnable() )
///      run.run() ;
///   return 0 ;
/// }
/// \endcode
///
class Main::Run : private GNet::EventHandler
{
public:
	Run( Output & output , const G::Arg & arg , const std::string & option_spec ) ;
		///< Constructor. Tries not to throw.

	virtual ~Run() ;
		///< Destructor.

	void configure() ;
		///< Prepares to run() typically by parsing the commandline.

	bool hidden() const ;
		///< Returns true if the program should run in hidden mode.
		///< Precondition: configure()d

	bool runnable() ;
		///< Returns true if run() should be called.
		///< Precondition: configure()d

	void run() ;
		///< Runs the application. Error messages are sent to the Output
		///< interface.
		///< Precondition: runnable()

	const Configuration & configuration() const ;
		///< Returns a configuration object.

	static std::string versionNumber() ;
		///< Returns the application version number string.

	G::Slot::Signal3<std::string,std::string,std::string> & signal() ;
		///< Provides a signal which is activated when something changes.

private:
	Run( const Run & ) ; // not implemented
	void operator=( const Run & ) ; // not implemented
	void doForwardingOnStartup( G::PidFile & ) ;
	void doServing( const GAuth::Secrets & , GSmtp::MessageStore & , const GAuth::Secrets & ,
		GPop::Store & , const GPop::Secrets & , G::PidFile & ) ;
	void closeFiles() ;
	void closeMoreFiles() ;
	void commit( G::PidFile & ) ;
	std::string smtpIdent() const ;
	void recordPid() ;
	const CommandLine & commandline() const ;
	void onClientDone( std::string ) ; // Client::doneSignal()
	void onClientEvent( std::string , std::string ) ; // Client::eventSignal()
	void onServerEvent( std::string , std::string ) ; // Server::eventSignal()
	void onStoreUpdateEvent() ;
	void onStoreRescanEvent() ;
	void onNetworkEvent( std::string , std::string ) ;
	void emit( const std::string & , const std::string & , const std::string & ) ;
	void onPollTimeout() ;
	void onStopTimeout() ;
	void requestForwarding( const std::string & = std::string() ) ;
	void onRequestForwardingTimeout() ;
	std::string startForwarding() ;
	bool logForwarding() const ;
	void checkPorts() const ;
	static void checkPort( bool , const std::string & , unsigned int ) ;
	GSmtp::Client::Config clientConfig() const ;
	GSmtp::ServerProtocol::Config serverProtocolConfig() const ;
	GSmtp::Server::Config serverConfig() const ;
	int resolverFamily() const ;
	static GNet::Address asAddress( const std::string & ) ;
	GPop::Server::Config popConfig() const ;
	void checkScripts() const ;
	void checkVerifierScript( const std::string & ) const ;
	void checkFilterScript( const std::string & ) const ;
	std::string versionString() const ;
	static std::string buildConfiguration() ;
	G::Path appDir() const ;

private:
	struct ExceptionHandler : public GNet::ExceptionHandler
	{
		explicit ExceptionHandler( bool do_throw ) ;
		virtual void onException( std::exception & ) override ;
		bool m_do_throw ;
	} ;

private:
	Output & m_output ;
	ExceptionHandler m_eh_throw ;
	ExceptionHandler m_eh_nothrow ;
	std::string m_option_spec ;
	G::Arg m_arg ;
	G::Slot::Signal3<std::string,std::string,std::string> m_signal ;
	unique_ptr<CommandLine> m_commandline ;
	unique_ptr<Configuration> m_configuration ;
	unique_ptr<G::LogOutput> m_log_output ;
	unique_ptr<GNet::EventLoop> m_event_loop ;
	unique_ptr<GNet::TimerList> m_timer_list ;
	unique_ptr<GNet::Timer<Run> > m_forwarding_timer ;
	unique_ptr<GNet::Timer<Run> > m_poll_timer ;
	unique_ptr<GNet::Timer<Run> > m_stop_timer ;
	unique_ptr<GSsl::Library> m_tls_library ;
	unique_ptr<GNet::Monitor> m_monitor ;
	unique_ptr<GSmtp::FileStore> m_store ;
	unique_ptr<GAuth::Secrets> m_client_secrets ;
	unique_ptr<GAuth::Secrets> m_server_secrets ;
	unique_ptr<GPop::Secrets> m_pop_secrets ;
	unique_ptr<GSmtp::Server> m_smtp_server ;
	unique_ptr<GPop::Server> m_pop_server ;
	unique_ptr<GSmtp::AdminServer> m_admin_server ;
	GNet::ClientPtr<GSmtp::Client> m_client ;
	std::string m_forwarding_reason ;
	bool m_forwarding_pending ;
	bool m_quit_when_sent ;
	std::string m_stop_reason ;
} ;

inline
const Main::Configuration & Main::Run::configuration() const
{
	return *m_configuration.get() ;
}

#endif
