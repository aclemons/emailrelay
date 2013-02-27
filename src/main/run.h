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
///
/// \file run.h
///

#ifndef G_MAIN_RUN_H
#define G_MAIN_RUN_H

#include "gdef.h"
#include "gsmtp.h"
#include "configuration.h"
#include "commandline.h"
#include "output.h"
#include "geventloop.h"
#include "gtimer.h"
#include "gclientptr.h"
#include "glogoutput.h"
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

/// \namespace Main
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
///   Main::Run run( output , arg , CommandLine::switchSpec() ) ;
///   if( run.prepare() )
///      run.run() ;
///   return 0 ;
/// }
/// \endcode
///
class Main::Run : private GNet::EventHandler 
{
public:
	Run( Output & output , const G::Arg & arg , const std::string & switch_spec ) ;
		///< Constructor. Tries not to throw.

	virtual ~Run() ;
		///< Destructor.

	bool hidden() const ;
		///< Returns true if the program should run in hidden mode.

	bool prepare() ;
		///< Prepares to run() typically by parsing the commandline. 
		///< Error messages are sent to the Output interface.
		///< Returns true if run() should be called.

	bool prepareError() const ;
		///< Returns true if prepare() failed.

	void run() ;
		///< Runs the application.
		///< Precondition: prepare() returned true

	const Configuration & config() const ;
		///< Returns a configuration object.

	static std::string versionNumber() ;
		///< Returns the application version number string.

	G::Signal3<std::string,std::string,std::string> & signal() ;
		///< Provides a signal which is activated when something changes.

	virtual void onException( std::exception & ) ; 
		///< Final override from GNet::EventHandler.

private:
	Run( const Run & ) ; // not implemented
	void operator=( const Run & ) ; // not implemented
	void runCore() ;
	void doForwardingOnStartup( GSmtp::MessageStore & , const GAuth::Secrets & , GNet::EventLoop & ) ;
	void doServing( const GAuth::Secrets & , GSmtp::MessageStore & , const GAuth::Secrets & , 
		GPop::Store & , const GPop::Secrets & , G::PidFile & , GNet::EventLoop & ) ;
	void closeFiles() ;
	void closeMoreFiles() ;
	std::string smtpIdent() const ;
	void recordPid() ;
	const CommandLine & cl() const ;
	void forwardingClientDone( std::string , bool ) ; // Client::doneSignal()
	void pollingClientDone( std::string , bool ) ; // Client::doneSignal()
	void clientEvent( std::string , std::string ) ; // Client::eventSignal()
	void serverEvent( std::string , std::string ) ; // Server::eventSignal()
	void raiseStoreEvent( bool ) ;
	void raiseNetworkEvent( std::string , std::string ) ;
	void emit( const std::string & , const std::string & , const std::string & ) ;
	void onPollTimeout() ;
	void onForwardingTimeout() ;
	void doForwarding( const std::string & ) ;
	std::string doForwardingCore() ;
	void checkPorts() const ;
	static void checkPort( bool , const std::string & , unsigned int ) ;
	GSmtp::Client::Config clientConfig() const ;
	GSmtp::Server::Config serverConfig() const ;
	static GNet::Address clientBindAddress( const std::string & ) ;
	GPop::Server::Config popConfig() const ;
	void checkScripts() const ;
	void checkVerifierScript( const std::string & ) const ;
	void checkProcessorScript( const std::string & ) const ;
	std::string versionString() const ;
	static std::string capabilities() ;

private:
	Output & m_output ;
	std::string m_switch_spec ;
	std::auto_ptr<G::LogOutput> m_log_output ;
	std::auto_ptr<CommandLine> m_cl ;
	std::auto_ptr<Configuration> m_cfg ;
	G::Arg m_arg ;
	G::Signal3<std::string,std::string,std::string> m_signal ;
	std::auto_ptr<GSmtp::FileStore> m_store ; // order dependency -- early
	std::auto_ptr<GAuth::Secrets> m_client_secrets ;
	std::auto_ptr<GPop::Secrets> m_pop_secrets ;
	std::auto_ptr<GSmtp::AdminServer> m_admin_server ;
	std::auto_ptr<GSmtp::Server> m_smtp_server ;
	std::auto_ptr<GNet::Timer<Run> > m_poll_timer ;
	std::auto_ptr<GNet::Timer<Run> > m_forwarding_timer ;
	GNet::ResolverInfo m_client_resolver_info ;
	GNet::ClientPtr<GSmtp::Client> m_client ; // order dependency -- late
	bool m_prepare_error ;
} ;

#endif
