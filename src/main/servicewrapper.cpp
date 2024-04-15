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
/// \file servicewrapper.cpp
///
// A service wrapper program. When called from the command-line with
// "--install" the wrapper registers itself with the Windows Service
// sub-system so that it gets re-executed by the service manager
// when the service is started.
//
// When re-executed the wrapper just registers its ServiceMain() entry
// point and blocks within the service dispatcher function.
//
// usage: servicewrapper [ { --remove [<service-name>] | --install [<service-name> [<service-display-name>]] } ]
//
// When the service is started the ServiceMain() entry point is called
// and this looks for a one-line batch file called "<name>-start.bat",
// which it then reads to get the full command-line for the server
// process. It adds "--no-daemon" and "--hidden" for good measure
// and then spins off the server with CreateProcess().
//
// The ServiceMain() function also registers the ControlHandler()
// entry point to receive service stop requests.
//
// Once the server process is created a separate thread is used to
// check that it is still running. If it is not running then the
// service is reported as failed and the wrapper terminates.
//
// By default the "<name>-start.bat" file must be in the same directory
// as this service wrapper, but if there is a file "<service-wrapper>.cfg"
// then its "dir-config" entry is used as the batch file directory.
// A "dir-config" value of "@app" can be used to mean the service
// wrapper's directory.
//

#include "gdef.h"
#include "gnowide.h"
#include "gconvert.h"
#include "gscope.h"
#include "gbatchfile.h"
#include "gmapfile.h"
#include "gscope.h"
#include "garg.h"
#include "gfile.h"
#include "serviceimp.h"
#include "servicecontrol.h"
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <new>

#define G_SERVICE_DEBUG( expr ) do { std::ostringstream ss ; ss << expr ; ServiceImp::log( ss.str() ) ; } while(0)
static constexpr unsigned int cfg_overall_timeout_ms = 8000U ;
using ServiceHandle = SERVICE_STATUS_HANDLE ;

struct ServiceArg
{
	ServiceArg( const G::Arg & arg ) :
		m_array(arg.array()) ,
		m_prefix(arg.prefix())
	{
	}
	bool help() const
	{
		if( m_array.size() < 2U ) return false ;
		return
			m_array[1] == "--help" ||
			m_array[1] == "/?" ||
			m_array[1] == "-?" ||
			m_array[1] == "-h" ;
	}
	bool install() const
	{
		if( m_array.size() < 2U ) return false ;
		return
			m_array[1] == "--install" ||
			m_array[1] == "-install" ||
			m_array[1] == "/install" ;
	}
	bool remove() const
	{
		if( m_array.size() < 2U ) return false ;
		return
			m_array[1] == "--remove" ||
			m_array[1] == "-remove" ||
			m_array[1] == "/remove" ;
	}
	std::string name() const
	{
		if( m_array.size() < 3U ) return "emailrelay" ;
		return {m_array[2]} ;
	}
	std::string displayName() const
	{
		if( m_array.size() < 4U ) return "E-MailRelay" ;
		return {m_array[3]} ;
	}
	std::string usage() const
	{
		return m_prefix + " [{--help|--install|--remove}] [<name> [<display-name>]]" ;
	}
	std::vector<std::string> m_array ;
	std::string m_prefix ;
} ;

struct ServiceError : public std::runtime_error
{
	ServiceError( const std::string & fn , DWORD e ) : std::runtime_error(fn+": "+decode(e)) , m_error(e) {}
	DWORD error() const noexcept { return m_error ; }
	static std::string decode( DWORD ) ;
	DWORD m_error ;
} ;

struct ServiceChild
{
	ServiceChild() ;
	explicit ServiceChild( std::string quoted_command_line ) ;
	bool isRunning() const noexcept ;
	static bool isRunning( HANDLE hprocess ) noexcept ;
	void close() noexcept ;
	void kill( std::nothrow_t ) noexcept ;
	void kill() ;
	HANDLE m_hprocess ;
} ;

struct ServiceEvent
{
	ServiceEvent() :
		m_h(0)
	{
		create() ;
	}
	explicit ServiceEvent( HANDLE h ) noexcept :
		m_h(h)
	{
	}
	ServiceEvent( std::nullptr_t ) noexcept :
		m_h(0)
	{
	}
	~ServiceEvent()
	{
		close() ;
	}
	void create()
	{
		m_h = CreateEvent( nullptr , FALSE , FALSE , nullptr ) ;
		if( m_h == 0 )
		{
			DWORD e = GetLastError() ;
			throw ServiceError( "CreateEvent" , e ) ;
		}
	}
	void close() noexcept
	{
		if( m_h )
			CloseHandle( m_h ) ;
		m_h = 0 ;
	}
	void set() noexcept
	{
		if( m_h )
			SetEvent( m_h ) ;
	}
	bool wait( unsigned int timeout_ms ) noexcept
	{
		return WaitForSingleObject(m_h,timeout_ms) == WAIT_TIMEOUT ;
	}
	HANDLE dup() const
	{
		HANDLE h = 0 ;
		BOOL ok = DuplicateHandle( GetCurrentProcess() , m_h , GetCurrentProcess() , &h ,
			0 , FALSE , DUPLICATE_SAME_ACCESS ) ;
		if( !ok )
		{
			DWORD e = GetLastError() ;
			throw ServiceError( "DuplicateHandle" , e ) ;
		}
		return h ;
	}
	HANDLE h() const noexcept
	{
		return m_h ;
	}
	HANDLE m_h ;
} ;

class Service
{
public:
	static void install( const std::string & name , const std::string & display_name ) ;
	static void remove( const std::string & name ) ;
	static void run() ;
	static Service * instance() ;

public:
	Service() ;
	~Service() ;
	void start( const std::string & name ) ;
	void onControlEvent( DWORD ) ;
	void runThread() ;
	bool valid() const noexcept ;

public:
	Service( const Service & ) = delete ;
	Service( Service && ) = delete ;
	Service & operator=( const Service & ) = delete ;
	Service & operator=( Service && ) = delete ;

private:
	void setStatus( DWORD ) ;
	void setStatus( DWORD , std::nothrow_t ) noexcept ;
	static void setStatus( ServiceHandle , DWORD ) noexcept ;
	ServiceHandle statusHandle( const std::string & ) ;
	void stopThread() noexcept ;
	static G::Path thisExe() ;
	static G::Path configFile( const G::Path & ) ;
	static G::Path bat( const std::string & service_name ) ;
	static std::string commandline( const G::Path & bat_path ) ;
	static void runThread( HANDLE , ServiceHandle , HANDLE ) ;

private:
	static Service * m_this ;
	static constexpr int Magic = 345897 ;
	volatile int m_magic ;
	ServiceHandle m_hservice ;
	ServiceChild m_child ;
	DWORD m_status ;
	HANDLE m_hthread ;
	DWORD m_thread_id ;
	ServiceEvent m_thread_exit ;
} ;

// ==

int main( int , char * [] )
{
	try
	{
		ServiceArg arg( G::Arg::windows() ) ;
		if( arg.help() )
			std::cout << "usage: " << arg.usage() << std::endl ;
		else if( arg.install() )
			Service::install( arg.name() , arg.displayName() ) ;
		else if( arg.remove() )
			Service::remove( arg.name() ) ;
		else
			Service::run() ;
		return 0 ;
	}
	catch( std::exception & e )
	{
		std::cerr << "exception: " << e.what() << std::endl ;
	}
	catch(...)
	{
		std::cerr << "unknown exception" << std::endl ;
	}
	return 1 ;
}

void ServiceMain( G::StringArray args )
{
	try
	{
		G_SERVICE_DEBUG( "ServiceMain: start: argc=" << args.size() ) ;
		std::string service_name ;
		if( !args.empty() )
			service_name = args[0] ;

		Service * service = Service::instance() ;
		if( service != nullptr )
			service->start( service_name ) ;
	}
	catch( std::exception & e )
	{
		G_SERVICE_DEBUG( "ServiceMain: exception: " << e.what() ) ;
	}
	catch(...)
	{
		G_SERVICE_DEBUG( "ServiceMain: exception" ) ;
	}
	G_SERVICE_DEBUG( "ServiceMain: done" ) ;
}

void ControlHandler( DWORD control )
{
	try
	{
		G_SERVICE_DEBUG( "ControlHandler: start: control=" << control ) ;

		Service * service = Service::instance() ;
		if( service == nullptr )
			throw ServiceError( "ControlHandler" , ERROR_INVALID_HANDLE ) ;

		service->onControlEvent( control ) ;
	}
	catch( std::exception & e )
	{
		G_SERVICE_DEBUG( "ControlHandler: exception: " << e.what() ) ;
	}
	catch(...)
	{
		G_SERVICE_DEBUG( "ControlHandler: exception" ) ;
	}
	G_SERVICE_DEBUG( "ControlHandler: done" ) ;
}

DWORD WINAPI RunThread( LPVOID arg )
{
	try
	{
		G_SERVICE_DEBUG( "RunThread: start" ) ;

		Service * service = static_cast<Service*>( arg ) ;
		if( service && service->valid() )
			service->runThread() ;

		G_SERVICE_DEBUG( "RunThread: done" ) ;
		return 0 ;
	}
	catch(...)
	{
		G_SERVICE_DEBUG( "RunThread: exception" ) ;
		return 1 ;
	}
}

// ==

Service * Service::m_this = nullptr ;

void Service::install( const std::string & service_name , const std::string & display_name )
{
	// prepare the service-wrapper commandline
	G::Path this_exe = G::nowide::exe() ;
	std::string command_line = this_exe.str().find(' ') == std::string::npos ? this_exe.str() : (std::string(1U,'\"').append(this_exe.str()).append(1U,'\"')) ;
	std::cout << "installing service \"" << service_name << "\": [" << command_line << "]" << std::endl ;

	// check that we will be able to read the batch file at service run-time
	Service::commandline( bat(service_name) ) ;

	// create the service
	std::string description = display_name + " service (reads " + bat(service_name).str() + " at service start time)" ;
	std::string reason = ServiceImp::install( command_line , service_name , display_name , description ) ;
	if( !reason.empty() )
		throw std::runtime_error( reason ) ;
}

void Service::remove( const std::string & service_name )
{
	std::cout << "removing service \"" << service_name << "\"" << std::endl ;
	std::string reason = ServiceImp::remove( service_name ) ;
	if( !reason.empty() )
		throw std::runtime_error( reason ) ;
}

void Service::run()
{
	G_SERVICE_DEBUG( "Service::run: start" ) ;
	{
		Service service ;
		DWORD e = ServiceImp::dispatch( ServiceMain ) ;
		if( e )
			throw ServiceError( "StartServiceCtrlDispatcher" , e ) ;
	}
	G_SERVICE_DEBUG( "Service::run: done" ) ;
}

Service::Service() :
	m_magic(Magic) ,
	m_hservice(0) ,
	m_status(SERVICE_START_PENDING) ,
	m_hthread(0) ,
	m_thread_id(0) ,
	m_thread_exit(nullptr)
{
	m_this = this ;
}

void Service::start( const std::string & service_name )
{
	try
	{
		G_SERVICE_DEBUG( "Service::start: start" ) ;
		m_hservice = statusHandle( service_name ) ;
		setStatus( SERVICE_START_PENDING ) ;
		m_child = ServiceChild( commandline(bat(service_name)) ) ;
		m_thread_exit.create() ;
		m_hthread = CreateThread( nullptr , 0 , RunThread , this , 0 , &m_thread_id ) ;
		G_SERVICE_DEBUG( "Service::start: done" ) ;
	}
	catch( std::exception & e )
	{
		G_SERVICE_DEBUG( "Service::start: exception: " << e.what() ) ;
		stopThread() ;
		setStatus( SERVICE_STOPPED , std::nothrow ) ;
		throw ;
	}
	catch(...)
	{
		G_SERVICE_DEBUG( "Service::start: exception" ) ;
		stopThread() ;
		setStatus( SERVICE_STOPPED , std::nothrow ) ;
		throw ;
	}
}

Service::~Service()
{
	G_SERVICE_DEBUG( "Service::dtor: start" ) ;
	m_child.kill( std::nothrow ) ;
	stopThread() ;
	setStatus( SERVICE_STOPPED , std::nothrow ) ;
	m_magic = 0 ;
	m_this = nullptr ;
	G_SERVICE_DEBUG( "Service::dtor: done" ) ;
}

Service * Service::instance()
{
	Service * service = m_this ;
	return service && service->valid() ? service : nullptr ;
}

bool Service::valid() const noexcept
{
	try
	{
		return m_magic == Magic ;
	}
	catch(...)
	{
		return false ;
	}
}

void Service::stopThread() noexcept
{
	m_thread_exit.set() ;
}

G::Path Service::configFile( const G::Path & p )
{
	return p.dirname() / (p.withoutExtension().basename()+".cfg") ;
}

G::Path Service::bat( const std::string & prefix )
{
	std::string filename = prefix + "-start.bat" ;

	G::Path this_exe = G::nowide::exe() ;

	G::MapFile config_map ;
	G::Path config_file = configFile( this_exe ) ;
	if( G::File::exists(config_file) )
		config_map = G::MapFile( config_file , "service config" ) ;

	G::Path dir_config = config_map.pathValue( "dir-config" , {} ) ;
	dir_config.replace( "@app" , this_exe.dirname().str() ) ;

	G::Path dir = dir_config.empty() ? this_exe.dirname() : G::Path(dir_config) ;

	return dir / filename ;
}

std::string Service::commandline( const G::Path & bat_path )
{
	G_SERVICE_DEBUG( "commandline: reading batch file [" << bat_path << "]" ) ;
	std::string line ;
	try
	{
		G::BatchFile bat_file( bat_path ) ;
		line = bat_file.line() ;
		line.insert( bat_file.lineArgsPos() , " --hidden --no-daemon" ) ;
	}
	catch( G::Exception & )
	{
		throw std::runtime_error( "cannot open \"" + bat_path.str() + "\"" +
			" (the service wrapper reads the command-line for the server process from this file)" ) ;
	}
	G_SERVICE_DEBUG( "commandline: [" << line << "]" ) ;
	return line ;
}

void Service::onControlEvent( DWORD event )
{
	if( event == SERVICE_CONTROL_STOP )
	{
		G_SERVICE_DEBUG( "Service::onControlEvent: start: event=stop" ) ;
		stopThread() ; // probably already finished
		m_child.kill() ;
		setStatus( SERVICE_STOPPED ) ;
	}
	else if( event == SERVICE_CONTROL_INTERROGATE )
	{
		G_SERVICE_DEBUG( "Service::onControlEvent: interrogate" ) ;
		// sample code does nothing, documentation says use
		// SetStatus() only if changed, and interrogate never
		// gets used anyways
	}
	else
	{
		G_SERVICE_DEBUG( "Service::onControlEvent: event=" << event << ": not implemented" ) ;
		throw ServiceError( "onControlEvent" , ERROR_CALL_NOT_IMPLEMENTED ) ;
	}
	G_SERVICE_DEBUG( "Service::onControlEvent: done" ) ;
}

void Service::runThread()
{
	runThread( m_thread_exit.dup() , m_hservice , m_child.m_hprocess ) ;
}

void Service::runThread( HANDLE hthread_exit , ServiceHandle hservice , HANDLE hprocess )
{
	G_SERVICE_DEBUG( "Service::runThread: monitoring thread: start" ) ;
	setStatus( hservice , SERVICE_RUNNING ) ;
	for(;;)
	{
		HANDLE handles[2] = { hprocess , hthread_exit } ;
		DWORD timeout_ms = INFINITE ;
		DWORD rc =  WaitForMultipleObjects( 2U , handles , FALSE , timeout_ms ) ;
		if( rc == WAIT_OBJECT_0 ) // hprocess
		{
			G_SERVICE_DEBUG( "Service::runThread: monitoring thread: server process has terminated" ) ;
			setStatus( hservice , SERVICE_STOPPED ) ;
			break ;
		}
		else if( rc == (WAIT_OBJECT_0+1) ) // hthread_exit
		{
			G_SERVICE_DEBUG( "Service::runThread: monitoring thread: asked to stop" ) ;
			break ;
		}
		else if( rc == WAIT_TIMEOUT )
		{
			G_SERVICE_DEBUG( "Service::runThread: monitoring thread: timeout" ) ;
		}
		else
		{
			G_SERVICE_DEBUG( "Service::runThread: monitoring thread: wait error" ) ;
			break ;
		}
	}
	G_SERVICE_DEBUG( "Service::runThread: monitoring thread: done" ) ;
}

ServiceHandle Service::statusHandle( const std::string & service_name )
{
	std::pair<ServiceHandle,DWORD> pair = ServiceImp::statusHandle( service_name , ControlHandler ) ;
	if( pair.second )
		throw ServiceError( "RegisterServiceCtrlHandlerEx" , pair.second ) ;
	return pair.first ;
}

void Service::setStatus( DWORD new_state )
{
	G_SERVICE_DEBUG( "Service::setStatus: begin: new-status=" << new_state ) ;
	DWORD e = ServiceImp::setStatus( m_hservice , new_state , cfg_overall_timeout_ms ) ;
	if( e )
		throw ServiceError( "SetServiceStatus" , e ) ;
	m_status = new_state ;
	G_SERVICE_DEBUG( "Service::setStatus: done" ) ;
}

void Service::setStatus( DWORD new_state , std::nothrow_t ) noexcept
{
	setStatus( m_hservice , new_state ) ;
}

void Service::setStatus( ServiceHandle hservice , DWORD new_state ) noexcept
{
	if( hservice )
		ServiceImp::setStatus( hservice , new_state , cfg_overall_timeout_ms ) ;
}

// ==

ServiceChild::ServiceChild() :
	m_hprocess(0)
{
}

ServiceChild::ServiceChild( std::string command_line ) :
	m_hprocess(0)
{
	G_SERVICE_DEBUG( "ServiceChild::ctor: spawning [" << command_line << "]" ) ;

	G::nowide::STARTUPINFO_BASE_type startup_info {} ;
	startup_info.cb = sizeof(startup_info) ;

	PROCESS_INFORMATION process_info {} ;

	bool rc = G::nowide::createProcess( {} , command_line ,
		nullptr , nullptr , CREATE_NO_WINDOW ,
		&startup_info , &process_info , FALSE ) ;

	if( !rc )
		throw std::runtime_error( std::string() + "cannot create process: [" + command_line + "]" ) ;

	CloseHandle( process_info.hThread ) ;
	m_hprocess = process_info.hProcess ;
	G_SERVICE_DEBUG( "ServiceChild::ctor: done" ) ;
}

void ServiceChild::close() noexcept
{
	if( m_hprocess )
	{
		HANDLE h = m_hprocess ;
		m_hprocess = 0 ;
		CloseHandle( h ) ;
	}
}

bool ServiceChild::isRunning() const noexcept
{
	return isRunning( m_hprocess ) ;
}

bool ServiceChild::isRunning( HANDLE hprocess ) noexcept
{
	if( hprocess )
		return WaitForSingleObject( hprocess , 0 ) == WAIT_TIMEOUT ;
	else
		return false ;
}

void ServiceChild::kill()
{
	if( m_hprocess )
	{
		G_SERVICE_DEBUG( "ServiceChild::kill: killing " << m_hprocess ) ;
		bool ok = !! TerminateProcess( m_hprocess , 50 ) ;
		if( ok )
		{
			close() ;
		}
		else
		{
			DWORD e = GetLastError() ;
			G_SERVICE_DEBUG( "ServiceChild::kill: failed: " << e ) ;
			throw ServiceError( "TerminateProcess" , e ) ;
		}
	}
}

void ServiceChild::kill( std::nothrow_t ) noexcept
{
	if( m_hprocess && !!TerminateProcess( m_hprocess , 50 ) )
		close() ;
}

// ==

std::string ServiceError::decode( DWORD e )
{
	switch( e )
	{
		case ERROR_INVALID_NAME: return "invalid name" ;
		case ERROR_SERVICE_DOES_NOT_EXIST: return "service does not exist" ;
		case ERROR_INVALID_DATA: return "invalid data" ;
		case ERROR_INVALID_HANDLE: return "invalid handle" ;
		case ERROR_FAILED_SERVICE_CONTROLLER_CONNECT: return "cannot connect" ;
		case ERROR_SERVICE_ALREADY_RUNNING: return "already running" ;
	}
	std::ostringstream ss ;
	ss << e ;
	return ss.str() ;
}

