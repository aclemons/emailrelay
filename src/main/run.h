//
// Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// run.h
//

#ifndef G_MAIN_RUN_H
#define G_MAIN_RUN_H

#include "gdef.h"
#include "gsmtp.h"
#include "configuration.h"
#include "commandline.h"
#include "output.h"
#include "geventloop.h"
#include "gtimer.h"
#include "glogoutput.h"
#include "gdaemon.h"
#include "gpidfile.h"
#include "gslot.h"
#include "garg.h"
#include "gsecrets.h"
#include "gmessagestore.h"
#include "gfilestore.h"
#include "gsmtpclient.h"
#include "gadminserver.h"
#include <iostream>
#include <exception>
#include <memory>

namespace Main
{
	class Run ;
}

// Class: Main::Run
// Description: A top-level class for the process.
// Usage:
/// int main( int argc , char ** argv )
/// {
///   G::Arg arg( argc , argv ) ;
///   Output output ;
///   Main::Run run( output , arg , CommandLine::switchSpec() ) ;
///   if( run.prepare() )
///      run.run() ;
///   return 0 ;
/// }
//
class Main::Run : private GNet::TimeoutHandler 
{
public:
	Run( Output & output , const G::Arg & arg , const std::string & switch_spec ) ;
		// Constructor.

	virtual ~Run() ;
		// Destructor.

	bool prepare() ;
		// Prepares to run(). Returns
		// false on error.

	void run() ;
		// Runs the application.
		// Precondition: prepare() returned true

	Configuration cfg() const ;
		// Returns a configuration object.

	static std::string versionNumber() ;
		// Returns the application version number string.

	G::Signal3<std::string,std::string,std::string> & signal() ;
		// Provides a signal which is activated when something changes.

private:
	Run( const Run & ) ; // not implemented
	void operator=( const Run & ) ; // not implemented
	void runCore() ;
	void doForwarding( GSmtp::MessageStore & , const GSmtp::Secrets & , GNet::EventLoop & ) ;
	void doServing( GSmtp::MessageStore & , const GSmtp::Secrets & , const GSmtp::Secrets & ,
		G::PidFile & , GNet::EventLoop & ) ;
	void closeFiles() ;
	void closeMoreFiles() ;
	std::string smtpIdent() const ;
	void recordPid() ;
	const CommandLine & cl() const ;
	virtual void clientDone( std::string ) ; // Client::doneSignal()
	virtual void clientEvent( std::string , std::string ) ; // Client::eventSignal()
	virtual void onTimeout( GNet::Timer & ) ; // from TimeoutHandler
	void raiseStoreEvent( bool ) ;
	void raiseNetworkEvent( std::string , std::string ) ;
	void emit( const std::string & , const std::string & , const std::string & ) ;
	std::string doPoll() ;

private:
	Output & m_output ;
	std::string m_switch_spec ;
	std::auto_ptr<CommandLine> m_cl ;
	std::auto_ptr<G::LogOutput> m_log_output ;
	std::auto_ptr<GSmtp::Client> m_client ;
	G::Arg m_arg ;
	G::Signal3<std::string,std::string,std::string> m_signal ;
	std::auto_ptr<GSmtp::FileStore> m_store ;
	std::auto_ptr<GSmtp::Secrets> m_client_secrets ;
	std::auto_ptr<GSmtp::AdminServer> m_admin_server ;
	std::auto_ptr<GNet::Timer> m_poll_timer ;
} ;

#endif
