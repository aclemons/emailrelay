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
#include "gconvert.h"
#include "gbatchfile.h"
#include "gmapfile.h"
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
	ServiceArg( int argc , char ** argv )
	{
		m_arg0 = argc > 0 ? std::string(argv[0]) : std::string() ;
		std::size_t pos = m_arg0.find_last_of( "/\\" ) ;
		if( pos != std::string::npos )
			m_arg0 = m_arg0.substr( pos+1U ) ;

		m_arg1 = argc > 1 ? lowercase(std::string(argv[1])) : std::string() ;
		m_arg2 = argc > 2 ? std::string(argv[2]) : std::string("emailrelay") ;
		m_arg3 = argc > 3 ? std::string(argv[3]) : std::string("E-MailRelay") ;
		m_help = m_arg1 == "--help" || m_arg1 == "/?" || m_arg1 == "-?" || m_arg1 == "-h" ;
		m_install = m_arg1 == "--install" || m_arg1 == "-install" || m_arg1 == "/install" ;
		m_remove =
			m_arg1 == "--remove" || m_arg1 == "-remove" || m_arg1 == "/remove" ||
			m_arg1 == "--uninstall" || m_arg1 == "-uninstall" || m_arg1 == "/uninstall" ;
	}
	std::string usage() const
	{
		return m_arg0 + " [{--help|--install|--remove}] [<name> [<display-name>]]" ;
	}
	static std::string lowercase( std::string s )
	{
		for( char & c : s )
			c = ( ( c >= 'A' && c <= 'Z' ) ? (c-'\x20') : c ) ;
		return s ;
	}
	std::string v2() const
	{
		return m_arg2 ;
	}
	std::string v3() const
	{
		return m_arg3 ;
	}
	bool m_help ;
	bool m_install ;
	bool m_remove ;
	std::string m_arg0 ;
	std::string m_arg1 ;
	std::string m_arg2 ;
	std::string m_arg3 ;
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
	using tstring = std::basic_string<TCHAR> ;

public:
	static void install( const std::string & name , const std::string & display_name ) ;
	static void remove( const std::string & name ) ;
	static void run() ;
	static Service * instance() ;

public:
	Service() ;
	~Service() ;
	void start( const tstring & name ) ;
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
	static std::string thisExe() ;
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

int main( int argc , char * argv [] )
{
	try
	{
		ServiceArg arg( argc , argv ) ;
		if( arg.m_help )
			std::cout << "usage: " << arg.usage() << std::endl ;
		else if( arg.m_install )
			Service::install( arg.v2() , arg.v3() ) ;
		else if( arg.m_remove )
			Service::remove( arg.v2() ) ;
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

void WINAPI ServiceMain( DWORD argc , LPTSTR * argv )
{
	try
	{
		G_SERVICE_DEBUG( "ServiceMain: start: argc=" << argc ) ;
		Service::tstring service_name ;
		if( argc > 0 )
			service_name = Service::tstring( argv[0] ) ;

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

void WINAPI ControlHandler( DWORD control )
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
	std::string this_exe = Service::thisExe() ;
	std::string command_line = this_exe.find(" ") == std::string::npos ?
		this_exe : ( std::string("\"")+this_exe+"\"") ;
	std::cout << "installing service \"" << service_name << "\": [" << command_line << "]" << std::endl ;

	// check that we will be able to read the batch file at service run-time
	G::Path batch_file = Service::bat( service_name ) ;
	Service::commandline( batch_file ) ;

	// create the service
	std::string description = display_name + " service (reads " + batch_file.str() + " at service start time)" ;
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

void Service::start( const tstring & name_in )
{
	try
	{
		G_SERVICE_DEBUG( "Service::start: start" ) ;
		std::string name ; // active-code-page
		G::Convert::convert( name , name_in , G::Convert::ThrowOnError("converting service name") ) ;
		if( name.empty() ) name = "emailrelay" ; // for testing purposes
		m_hservice = statusHandle( name ) ;
		setStatus( SERVICE_START_PENDING ) ;
		m_child = ServiceChild( commandline(bat(name)) ) ;
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

std::string Service::thisExe()
{
	G::Arg arg ;
	HINSTANCE hinstance = 0 ;
	arg.parse( hinstance , std::string() ) ;
	return arg.v(0U) ;
}

G::Path Service::configFile( const G::Path & p )
{
	return p.dirname() + (p.withoutExtension().basename()+".cfg") ;
}

G::Path Service::bat( const std::string & prefix )
{
	std::string filename = prefix + "-start.bat" ;
	G::Path this_exe = thisExe() ;

	G::MapFile config_map ;
	G::Path config_file = configFile( this_exe ) ;
	if( G::File::exists(config_file) )
		config_map = G::MapFile( config_file , "service config" ) ;

	std::string dir_config = config_map.value( "dir-config" ) ;
	if( dir_config.find("@app") == 0U )
		G::Str::replace( dir_config , "@app" , this_exe.dirname().str() ) ;

	G::Path dir = dir_config.empty() ? this_exe.dirname() : G::Path(dir_config) ;

	return dir + filename ;
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

	STARTUPINFOA start {} ;
	start.cb = sizeof(start) ;

	BOOL inherit = FALSE ;
	DWORD flags = CREATE_NO_WINDOW ;
	LPVOID env = nullptr ;
	LPCSTR cwd = nullptr ;
	PROCESS_INFORMATION info ;
	SECURITY_ATTRIBUTES * process_attributes = nullptr ;
	SECURITY_ATTRIBUTES * thread_attributes = nullptr ;
	char * command_line_p = const_cast<char*>(command_line.c_str()) ;

	BOOL rc = CreateProcessA( nullptr , command_line_p ,
		process_attributes , thread_attributes , inherit ,
		flags , env , cwd , &start , &info ) ;

	if( !rc )
		throw std::runtime_error( std::string() + "cannot create process: [" + command_line + "]" ) ;

	CloseHandle( info.hThread ) ;
	m_hprocess = info.hProcess ;
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

