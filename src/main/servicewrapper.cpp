//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// and this looks for a one-line batch file called "<name>-start.bat"
// or (new) a configuration file called "<name>.cfg", which it then reads
// to get the command-line for the server process. It adds "--hidden" and
// spins off the server with CreateProcess().
//
// The "<name>-start.bat" or "<name>.cfg" file must be in the same
// directory as this service wrapper executable by default, but if
// there is a file "<service-wrapper>.cfg" then its "dir-config"
// entry is used for the directory to look in.
//
// In the batch file case the path of the server executable is defined
// by the contents of the batch file, but if there is a configuration
// file and no batch file then the server executable is taken to be
// "<name>.exe" in the same directory as the service wrapper or in the
// directory given by a "dir-install" entry in "<service-wrapper>.cfg".
//
// The "<service-wrapper>.cfg" file can use the "@app" substitution
// variable to stand for the directory containing service wrapper
// executable.
//
// The ServiceMain() function also registers the ControlHandler()
// entry point to receive service stop requests.
//
// Once the server process is created a separate thread is used to
// check that it is still running. If it is not running then the
// service is reported as failed and the wrapper terminates.
//
// To enable low-level diagnostic logging set the path of a log file
// in the registry at HKLM/SOFTWARE/emailrelay-service/logfile
// (see serviceimp_win32.cpp).
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
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>
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
	void closeHandle( HANDLE ) noexcept ;
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
	static void setErrorStatus( ServiceHandle , DWORD state , DWORD generic_error , DWORD specific_error ) noexcept ;
	static void setStatus( ServiceHandle , DWORD ) noexcept ;
	ServiceHandle statusHandle( const std::string & ) ;
	void stopThread() noexcept ;
	static G::Path wrapperConfigFile() ;
	static G::MapFile wrapperConfig() ;
	static G::Path batchFile( const G::MapFile & , const std::string & ) ;
	static G::Path configFile( const G::MapFile , const std::string & ) ;
	static G::Path serverExe( const G::MapFile , const std::string & ) ;
	static G::Path wrapperConfigPath( const G::MapFile & , const std::string & , const std::string & ) ;
	struct ServiceInfo
	{
		G::Path batch_file ;
		bool batch_file_exists {false} ;
		G::Path config_file ;
		bool config_file_exists {false} ;
		G::Path server_exe ;
		std::string commandline ;
		std::string description ;
		std::string error ;
	} ;
	static ServiceInfo serviceInfo( const std::string & , std::string = {} ) ;
	static void runThread( HANDLE , ServiceHandle , HANDLE ) ;
	static std::string quoted( const std::string & ) ;

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
	std::string wrapper = G::nowide::exe().str() ;
	if( wrapper.find(' ') != std::string::npos )
		wrapper = "\"" + wrapper + "\"" ;

	std::cout << "installing service \"" << service_name << "\": [" << wrapper << "]" << std::endl ;

	ServiceInfo service_info = serviceInfo( service_name , display_name ) ;
	if( !service_info.error.empty() )
		throw ServiceError( service_info.error , ERROR_FILE_NOT_FOUND ) ;

	// create the service
	auto pair = ServiceImp::install( wrapper , service_name , display_name , service_info.description ) ;
	if( !pair.first.empty() )
		throw ServiceError( pair.first , pair.second ) ;
}

void Service::remove( const std::string & service_name )
{
	std::cout << "removing service \"" << service_name << "\"" << std::endl ;
	auto pair = ServiceImp::remove( service_name ) ;
	if( !pair.first.empty() )
		throw ServiceError( pair.first , pair.second ) ;
}

void Service::run()
{
	G_SERVICE_DEBUG( "Service::run: start" ) ;
	{
		Service service ;
		DWORD e = ServiceImp::dispatch( ServiceMain ) ;
		if( e )
			throw ServiceError( "dispatcher error" , e ) ;
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
		ServiceInfo service_info = serviceInfo( service_name ) ;
		if( !service_info.error.empty() )
			throw ServiceError( service_info.error , ERROR_FILE_NOT_FOUND ) ;
		m_child = ServiceChild( service_info.commandline ) ;
		m_thread_exit.create() ;
		m_hthread = CreateThread( nullptr , 0 , RunThread , this , 0 , &m_thread_id ) ;
		G_SERVICE_DEBUG( "Service::start: done" ) ;
	}
	catch( ServiceError & e )
	{
		G_SERVICE_DEBUG( "Service::start: exception: " << e.what() ) ;
		stopThread() ;
		setErrorStatus( m_hservice , SERVICE_STOPPED , e.m_error , 1 ) ;
		throw ;
	}
	catch( std::exception & e )
	{
		G_SERVICE_DEBUG( "Service::start: exception: " << e.what() ) ;
		stopThread() ;
		setErrorStatus( m_hservice , SERVICE_STOPPED , ERROR_SERVICE_SPECIFIC_ERROR , 1 ) ;
		throw ;
	}
	catch(...)
	{
		G_SERVICE_DEBUG( "Service::start: exception" ) ;
		stopThread() ;
		setErrorStatus( m_hservice , SERVICE_STOPPED , ERROR_SERVICE_SPECIFIC_ERROR , 1 ) ;
		throw ;
	}
}

Service::~Service()
{
	G_SERVICE_DEBUG( "Service::dtor: start" ) ;
	m_child.kill( std::nothrow ) ;
	stopThread() ;
	setStatus( m_hservice , SERVICE_STOPPED ) ;
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

G::Path Service::wrapperConfigFile()
{
	G::Path exe = G::nowide::exe() ;
	return exe.dirname() / (exe.withoutExtension().basename()+".cfg") ;
}

G::MapFile Service::wrapperConfig()
{
	G::Path path = wrapperConfigFile() ;
	if( G::File::exists( path , std::nothrow ) )
		return G::MapFile( path , "service config" ) ;
	else
		return {} ;
}

G::Path Service::batchFile( const G::MapFile & wrapper_config , const std::string & name )
{
	return wrapperConfigPath( wrapper_config , "dir-config" , name+"-start.bat" ) ;
}

G::Path Service::configFile( const G::MapFile wrapper_config , const std::string & name )
{
	return wrapperConfigPath( wrapper_config , "dir-config" , name+".cfg" ) ;
}

G::Path Service::serverExe( const G::MapFile wrapper_config , const std::string & name )
{
	return wrapperConfigPath( wrapper_config , "dir-install" , name+".exe" ) ;
}

G::Path Service::wrapperConfigPath( const G::MapFile & wrapper_config , const std::string & key , const std::string & filename )
{
	G::Path this_exe = G::nowide::exe() ;
	G::Path dir_config = wrapper_config.pathValue( key , {} ) ;
	dir_config.replace( "@app" , this_exe.dirname().str() ) ;
	G::Path dir = dir_config.empty() ? this_exe.dirname() : dir_config ;
	if( dir.isRelative() )
		dir = G::Path::join( this_exe.dirname() , dir ) ;
	return dir/filename ;
}

Service::ServiceInfo Service::serviceInfo( const std::string & service_name , std::string display_name )
{
	if( display_name.empty() )
		display_name = service_name ;

	ServiceInfo info ;

	G::MapFile wrapper_config = wrapperConfig() ;
	info.batch_file = batchFile( wrapper_config , service_name ) ;
	info.batch_file_exists = G::File::exists( info.batch_file , std::nothrow ) ;
	info.config_file = configFile( wrapper_config , service_name ) ;
	info.config_file_exists = G::File::exists( info.config_file , std::nothrow ) ;
	info.server_exe = serverExe( wrapper_config , service_name ) ;

	G_SERVICE_DEBUG( "serviceInfo: batch file [" << info.batch_file << "]" << (info.batch_file_exists?" (exists)":"(missing)" ) ) ;
	G_SERVICE_DEBUG( "serviceInfo: config file [" << info.config_file << "]" << (info.config_file_exists?" (exists)":"(missing)") ) ;
	G_SERVICE_DEBUG( "serviceInfo: server exe [" << info.server_exe << "]" ) ;

	if( !info.batch_file_exists && !info.config_file_exists )
	{
		std::ostringstream ss ;
		ss << "cannot read \"" << info.batch_file << "\" or \"" << info.config_file << "\"" ;
		info.error = ss.str() ;
	}
	else
	{
		if( info.batch_file_exists )
		{
			G::BatchFile bf( info.batch_file ) ;
			G_SERVICE_DEBUG( "serviceInfo: batch file command [" << bf.line() << "] (" << bf.lineArgsPos() << "/" << bf.line().size() << ")" ) ;
			info.commandline = bf.line() ;
			info.commandline.insert( bf.lineArgsPos() , " --hidden" ) ;
			std::ostringstream ss ;
			ss << display_name << " service (reads " << info.batch_file << " at service start time)" ; // see also Gui::Boot::install()
			info.description = ss.str() ;
		}
		else if( info.config_file_exists )
		{
			info.commandline = quoted(info.server_exe.str()) + " --hidden " + quoted(info.config_file.str()) ;
			std::ostringstream ss ;
			ss << display_name << " service (reads " << info.config_file << " at service start time)" ;
			info.description = ss.str() ;
		}
		G_SERVICE_DEBUG( "serviceInfo: [" << info.commandline << "]" ) ;
	}
	return info ;
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
			DWORD exit_code = 99 ;
			GetExitCodeProcess( hprocess , &exit_code ) ;
			G_SERVICE_DEBUG( "Service::runThread: monitoring thread: server process has terminated: exit-code " << exit_code ) ;
			if( exit_code == 0U )
				exit_code = 3U ; // shouldn't get here
			setErrorStatus( hservice , SERVICE_STOPPED , ERROR_SERVICE_SPECIFIC_ERROR , exit_code ) ;
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

void Service::setErrorStatus( ServiceHandle hservice , DWORD new_state , DWORD generic_error , DWORD specific_error ) noexcept
{
	if( hservice )
		ServiceImp::setStatus( hservice , new_state , cfg_overall_timeout_ms , generic_error , specific_error ) ;
}

void Service::setStatus( ServiceHandle hservice , DWORD new_state ) noexcept
{
	if( hservice )
		ServiceImp::setStatus( hservice , new_state , cfg_overall_timeout_ms ) ;
}

std::string Service::quoted( const std::string & s )
{
	if( s.find(' ') != std::string::npos )
		return std::string(1U,'\"').append(s).append(1U,'\"') ;
	else
		return s ;
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

	G::nowide::STARTUPINFO_REAL_type startup_info {} ;
	DWORD startup_info_flags = G::nowide::STARTUPINFO_flags | CREATE_NO_WINDOW ;
	auto * startup_info_ptr = reinterpret_cast<G::nowide::STARTUPINFO_BASE_type*>(& startup_info) ;
	startup_info_ptr->cb = sizeof( startup_info ) ;

	PROCESS_INFORMATION process_info {} ;

	bool rc = G::nowide::createProcess( {} , command_line ,
		nullptr , nullptr , nullptr ,
		startup_info_flags , startup_info_ptr ,
		&process_info , /*inherit=*/false ) ;

	if( !rc )
	{
		DWORD e = GetLastError() ;
		throw ServiceError( "cannot create process: [" + command_line + "]" , e ) ;
	}

	closeHandle( process_info.hThread ) ;
	m_hprocess = process_info.hProcess ;
	G_SERVICE_DEBUG( "ServiceChild::ctor: done" ) ;
}

void ServiceChild::close() noexcept
{
	if( m_hprocess )
	{
		HANDLE h = m_hprocess ;
		m_hprocess = 0 ;
		closeHandle( h ) ;
	}
}

void ServiceChild::closeHandle( HANDLE h ) noexcept
{
	if( h )
		CloseHandle( h ) ;
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

