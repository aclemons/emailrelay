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
/// \file testrun.h
///

#ifndef G_MAIN_TESTRUN_H
#define G_MAIN_TESTRUN_H

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
class Main::Run : private GNet::EventHandler
{
public:
	Run( Output & output , const G::Arg & arg , bool is_windows = false , bool has_gui = false ) ;
		///< Constructor. Tries not to throw.

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
	Run & operator=( const Run & ) = delete ;
	Run & operator=( Run && ) = delete ;

public:
	// for Unit...
	bool onClientDone( const std::string & ) ;
	void emit( std::string , std::string , std::string = {} , std::string = {} ) ;

private:
	void doForwardingOnStartup( G::PidFile & ) ;
	void closeFiles() const ;
	void commit( G::PidFile & ) ;
	std::string smtpIdent() const ;
	void recordPid() ;
	const CommandLine & commandline() const ;
	void onServerEvent( const std::string & , const std::string & ) ; // Server::eventSignal()
	void onStoreUpdateEvent() ;
	void onNetworkEvent( const std::string & , const std::string & ) ;
	void onQueueTimeout() ;
	GNet::StreamSocket::Config netSocketConfig( bool server = true ) const ;
	GSmtp::Client::Config clientConfig( const Configuration & , const std::string & ) const ;
	GSmtp::ServerProtocol::Config serverProtocolConfig( const Configuration & ) const ;
	GSmtp::Server::Config smtpServerConfig( const Configuration & ) const ;
	GNet::Server::Config netServerConfig( const Configuration & ) const ;
	int resolverFamily( const Configuration & ) const ;
	static GNet::Address asAddress( const std::string & ) ;
	GPop::Server::Config popConfig( const Configuration & ) const ;
	void checkThreading() const ;
	std::string versionString() const ;
	static std::string buildConfiguration() ;
	G::Path appDir() const ;
	bool needTls( const Configuration & ) const ;
	bool preferTls( const Configuration & ) const ;

private:
	Output & m_output ;
	GNet::ExceptionSink m_es_rethrow ;
	GNet::ExceptionSink m_es_nothrow ;
	bool m_is_windows ;
	G::Arg m_arg ;
	G::Slot::Signal<std::string,std::string,std::string,std::string> m_signal ;
	std::unique_ptr<CommandLine> m_commandline ;
	std::unique_ptr<Configuration> m_configuration ;
	std::unique_ptr<G::LogOutput> m_log_output ;
	std::unique_ptr<GNet::EventLoop> m_event_loop ;
	std::unique_ptr<GNet::TimerList> m_timer_list ;
	std::unique_ptr<GNet::Monitor> m_monitor ;
	std::unique_ptr<GSsl::Library> m_tls_library ;
	std::deque<QueueItem> m_queue ;
	bool m_quit_when_sent ;
	bool m_has_gui ;
	struct Unit : private GNet::ExceptionSource
	{
		Unit( std::size_t , Run * , const Configuration & , int resolver_family ) ;
		void create( GSsl::Library * , GNet::Server::Config net_server_config ,
			GSmtp::Server::Config smtp_server_config , GSmtp::Client::Config smtp_client_config ,
			GPop::Server::Config pop_server_config ) ;
		void start() ;
		//
		std::unique_ptr<GSmtp::AdminServer> newAdminServer( GNet::ExceptionSink ,
			const Configuration & , GSmtp::MessageStore & , GSmtp::FilterFactory & ,
			G::Slot::Signal<const std::string&> & ,
			const GNet::ServerPeer::Config & ,
			const GNet::Server::Config & , const GSmtp::Client::Config & ,
			const GAuth::Secrets & , const std::string & ) ;
		void onForwardRequest( const std::string & ) ;
		void onClientDone( const std::string & ) ;
		void onClientEvent( const std::string & , const std::string & , const std::string & ) ;
		void onRequestForwardingTimeout() ;
		void onPollTimeout() ;
		void onStoreRescanEvent() ;
		void onServerEvent( const std::string & s1 , const std::string & ) ;
		void requestForwarding( const std::string & = std::string() ) ;
		std::string startForwarding() ;
		bool logForwarding() const ;
		void emit( const std::string & s0 , const std::string & s1 , const std::string & s2 = {} , const std::string & s3 = {} ) ;
		//
		std::string exceptionSourceId() const override ;
		//
		std::size_t m_unit_id ;
		Run * m_run ;
		Configuration m_config ;
		int m_resolver_family ;
		std::unique_ptr<GSmtp::Client::Config> m_smtp_client_config ;
		G::Slot::Signal<const std::string&> m_forward_request_signal ;
		std::unique_ptr<GNet::Timer<Unit>> m_forwarding_timer ;
		std::unique_ptr<GNet::Timer<Unit>> m_poll_timer ;
		std::unique_ptr<GSmtp::FileStore> m_store ;
		std::unique_ptr<GSmtp::FilterFactory> m_filter_factory ;
		std::unique_ptr<GAuth::Secrets> m_client_secrets ;
		std::unique_ptr<GAuth::Secrets> m_server_secrets ;
		std::unique_ptr<GAuth::Secrets> m_pop_secrets ;
		std::unique_ptr<GSmtp::Server> m_smtp_server ;
		std::unique_ptr<GPop::Store> m_pop_store ;
		std::unique_ptr<GPop::Server> m_pop_server ;
		std::unique_ptr<GSmtp::AdminServer> m_admin_server ;
		std::unique_ptr<GNet::ClientPtr<GSmtp::Client>> m_client_ptr ;
		std::string m_forwarding_reason ;
		bool m_forwarding_pending {false} ;
	} ;
	std::vector<Unit> m_unit ;
} ;

inline
const Main::Configuration & Main::Run::configuration() const
{
	return *m_configuration ;
}

#endif
