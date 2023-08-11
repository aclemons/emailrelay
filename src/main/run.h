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
/// \file run.h
///

#ifndef G_MAIN_RUN_H
#define G_MAIN_RUN_H

#include "gdef.h"
#include "unit.h"
#include "gssl.h"
#include "configuration.h"
#include "commandline.h"
#include "output.h"
#include "geventloop.h"
#include "gtimerlist.h"
#include "glogoutput.h"
#include "gmonitor.h"
#include "gdaemon.h"
#include "gpidfile.h"
#include "gslot.h"
#include "garg.h"
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
	Run( Output & output , const G::Arg & arg , bool has_gui = false ) ;
		///< Constructor. Tries not to throw.

	~Run() ;
		///< Destructor.

	void configure( const G::Options & ) ;
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

	std::size_t configurations() const ;
		///< Returns the number of configuration.

	const Configuration & configuration( std::size_t = 0U ) const ;
		///< Returns a configuration object.

	const Unit & unit( unsigned int unit_id = 0U ) const ;
		///< Returns a reference to a unit.

	static std::string versionNumber() ;
		///< Returns the application version number string.

	std::string defaultDomain() const ;
		///< Returns the local machine's fully qualified domain name,
		///< with some reasonable non-empty default if that cannot be
		///< determined. Used as a default for the configured domain
		///< name.

	G::Slot::Signal<std::string,std::string,std::string,std::string> & signal() noexcept ;
		///< Provides a signal which is activated when something changes.

private:
	struct QueueItem
	{
		int target ; // 1=gui, 2=admin
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

private:
	void closeFiles() const ;
	void commit( G::PidFile & ) ;
	const CommandLine & commandline() const ;
	void onUnitDone( unsigned int , std::string , bool ) ;
	void onUnitEvent( unsigned int , std::string , std::string , std::string ) ;
	void onNetworkEvent( const std::string & , const std::string & ) ;
	void addToSignalQueue( const std::string & , const std::string & , const std::string & = {} , const std::string & = {} ) ;
	void onQueueTimeout() ;
	void checkThreading() const ;
	G::Path appDir() const ;

private:
	Output & m_output ;
	G::Arg m_arg ;
	mutable std::string m_default_domain ;
	bool m_has_gui ;
	G::Slot::Signal<std::string,std::string,std::string,std::string> m_signal ;
	std::unique_ptr<CommandLine> m_commandline ;
	std::unique_ptr<G::LogOutput> m_log_output ;
	std::unique_ptr<GNet::EventLoop> m_event_loop ;
	std::unique_ptr<GNet::TimerList> m_timer_list ;
	std::unique_ptr<GNet::Monitor> m_monitor ;
	std::unique_ptr<GNet::Timer<Run>> m_queue_timer ;
	std::unique_ptr<GSsl::Library> m_tls_library ;
	std::deque<QueueItem> m_queue ;
	std::vector<Configuration> m_configurations ;
	std::vector<std::unique_ptr<Unit>> m_units ;
} ;

#endif
