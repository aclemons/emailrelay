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
/// \file run.h
///

#ifndef G_MAIN_RUN_H
#define G_MAIN_RUN_H

#include "gdef.h"
#include "gssl.h"
#include "configuration.h"
#include "commandline.h"
#include "output.h"
#include "geventloop.h"
#include "gtimerlist.h"
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
#include <iostream>
#include <exception>
#include <memory>
#include <deque>

namespace Main
{
	class Run ;
}

//| \class Main::Run
/// A top-level class for the process.
/// Usage:
/// \code
/// int main( int argc , char ** argv )
/// {
///   G::Arg arg( argc , argv ) ;
///   Output output ;
///   Main::Run run( output , arg ) ;
///   run.configure() ;
///   if( run.runnable() )
///      run.run() ;
///   return 0 ;
/// }
/// \endcode
///
class Main::Run
{
public:
	Run( Output & output , const G::Arg & arg , bool is_windows = false , bool has_gui = false ) ;
		///< Constructor. Tries not to throw.

	~Run() ;
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

	G::Slot::Signal<std::string,std::string,std::string,std::string> & signal() ;
		///< Provides a signal which is activated when something changes.

private:
	struct QueueItem
	{
		int target ;
		std::string s0 ;
		std::string s1 ;
		std::string s2 ;
		std::string s3 ;
		QueueItem( int target_ , const std::string & s0_ , const std::string & s1_ , const std::string & s2_ , const std::string & s3_ ) :
			target(target_) ,
			s0(s0_) ,
			s1(s1_) ,
			s2(s2_) ,
			s3(s3_)
		{
		}
	} ;

public:
	Run( const Run & ) = delete ;
	Run( Run && ) = delete ;
	void operator=( const Run & ) = delete ;
	void operator=( Run && ) = delete ;

private:
	void doForwardingOnStartup( G::PidFile & ) ;
	void closeFiles() const ;
	void closeMoreFiles() const ;
	void commit( G::PidFile & ) ;
	std::string smtpIdent() const ;
	void recordPid() ;
	const CommandLine & commandline() const ;
	void onForwardRequest( const std::string & ) ; // m_forward_request_signal
	void onClientDone( const std::string & ) ; // Client::doneSignal()
	void onClientEvent( const std::string & , const std::string & , const std::string & ) ; // Client::eventSignal()
	void onServerEvent( const std::string & , const std::string & ) ; // Server::eventSignal()
	void onStoreUpdateEvent() ;
	void onStoreRescanEvent() ;
	void onNetworkEvent( const std::string & , const std::string & ) ;
	void emit( const std::string & , const std::string & , const std::string & = {} , const std::string & = {} ) ;
	void onPollTimeout() ;
	void requestForwarding( const std::string & = {} ) ;
	void onRequestForwardingTimeout() ;
	void onQueueTimeout() ;
	std::string startForwarding() ;
	bool logForwarding() const ;
	void checkPorts() const ;
	static void checkPort( bool , const std::string & , unsigned int ) ;
	int resolverFamily() const ;
	static GNet::Address asAddress( const std::string & ) ;
	void checkScripts() const ;
	void checkVerifier( const std::string & ) const ;
	void checkFilter( const std::string & , bool ) const ;
	void checkThreading() const ;
	std::string versionString() const ;
	static std::string buildConfiguration() ;
	G::Path appDir() const ;
	GSmtp::MessageStore & store() ;
	GSmtp::Client::Config smtpClientConfig() const ;
	GSmtp::ServerProtocol::Config smtpServerProtocolConfig() const ;
	GNet::Server::Config netServerConfig() const ;
	GSmtp::Server::Config smtpServerConfig() const ;
	GPop::Server::Config popServerConfig() const ;

private:
	Output & m_output ;
	bool m_is_windows ;
	G::Arg m_arg ;
	G::Slot::Signal<const std::string&> m_forward_request_signal ;
	G::Slot::Signal<std::string,std::string,std::string,std::string> m_signal ;
	std::unique_ptr<CommandLine> m_commandline ;
	std::unique_ptr<Configuration> m_configuration ;
	std::unique_ptr<G::LogOutput> m_log_output ;
	std::unique_ptr<GNet::EventLoop> m_event_loop ;
	std::unique_ptr<GNet::TimerList> m_timer_list ;
	std::unique_ptr<GNet::Timer<Run>> m_forwarding_timer ;
	std::unique_ptr<GNet::Timer<Run>> m_poll_timer ;
	std::unique_ptr<GNet::Timer<Run>> m_queue_timer ;
	std::unique_ptr<GSsl::Library> m_tls_library ;
	std::unique_ptr<GNet::Monitor> m_monitor ;
	std::unique_ptr<GSmtp::FileStore> m_file_store ;
	std::unique_ptr<GSmtp::FilterFactory> m_filter_factory ;
	std::unique_ptr<GAuth::Secrets> m_client_secrets ;
	std::unique_ptr<GAuth::Secrets> m_server_secrets ;
	std::unique_ptr<GAuth::Secrets> m_pop_secrets ;
	std::unique_ptr<GSmtp::Server> m_smtp_server ;
	std::unique_ptr<GPop::Store> m_pop_store ;
	std::unique_ptr<GPop::Server> m_pop_server ;
	std::unique_ptr<GSmtp::AdminServer> m_admin_server ;
	GNet::ClientPtr<GSmtp::Client> m_client_ptr ;
	std::deque<QueueItem> m_queue ;
	std::string m_forwarding_reason ;
	bool m_forwarding_pending ;
	bool m_quit_when_sent ;
	bool m_has_gui ;
} ;

inline
const Main::Configuration & Main::Run::configuration() const
{
	return *m_configuration ;
}

#endif
