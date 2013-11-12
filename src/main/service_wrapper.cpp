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
//
// service_wrapper.cpp
//
// A service wrapper program. On service startup a pre-configured process 
// is forked; on sutdown the forked process is terminated.
//
// usage: service_wrapper [ { --remove [<service-name>] | --install [<service-name> [<service-display-name>]] } ]
//
// The service wrapper looks for a one-line batch file called 
// "<name>-start.bat" in the same directory as itself which it then 
// reads to get the full command-line for the forked process. It
// adds "--no-daemon" and "--hidden" for good measure.
//
// After the process is forked the service wrapper waits for a bit and 
// then checks that it is running. It does this twice. If the forked 
// process is not running on the second check then the service is 
// considered not to have started.
//

#include "glimits.h"
#include "gconvert.h"
#include "service_install.h"
#include "service_remove.h"
#include <windows.h>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <map>
#include <string>

#define G_DEBUG( expr ) do { std::ostringstream ss ; ss << expr ; log( ss.str() ) ; } while(0)

unsigned int cfg_timeout_ms()
{
	return 3000U ;
}

unsigned int cfg_overall_timeout_ms()
{
	return 8000U ;
}

static void log( std::string s )
{
	// add this for debugging
 #if 0
	static std::ofstream f( "c:\\temp\\temp.out" ) ;
	f << s << std::endl ;
 #endif
}

struct Error : public std::runtime_error
{
	static std::string decode( DWORD ) ;
	DWORD m_error ;
	Error( const std::string & fn , DWORD e ) : std::runtime_error(fn+": "+decode(e)) , m_error(e) {}
	DWORD error() const { return m_error ; }
} ;

namespace
{
	std::string lowercase( const std::string & s_ )
	{
		std::map<char,char> map ;
		char c_out = 'a' ;
		for( char c_in = 'A' ; c_in <= 'Z' ; ++c_in , ++c_out )
			map[c_in] = c_out ;

		std::string s( s_ ) ;
		for( std::string::iterator p = s.begin() ; p != s.end() ; ++p )
		{
			if( map.find(*p) != map.end() )
				*p = map[*p] ;
		}
		return s ;
	}
}

struct Child
{
	HANDLE m_hprocess ;
	Child() ;
	explicit Child( std::string quoted_command_line ) ;
	bool isRunning() const ;
	void kill() ;
	void close() ;
} ;

class Service 
{
private:
	typedef SERVICE_STATUS_HANDLE Handle ;
	enum { Magic = 345897 } ;
	volatile int m_magic ;
	Handle m_hservice ;
	Child m_child ;
	DWORD m_status ;
	HANDLE m_hthread ;
	DWORD m_thread_id ;
	HANDLE m_thread_exit ;
public:
	typedef std::basic_string<TCHAR> tstring ;
	static void install( const std::string & name , const std::string & display_name ) ;
	static void remove( const std::string & name ) ;
	static void start() ;
	Service() ;
	void init( std::string name ) ;
	~Service() ;
	static Service * instance() ;
	bool valid() const ;
	void onEvent( DWORD ) ;
	void runThread() ;
	static std::string exe() ;
	static std::string bat( const std::string & service_name ) ;
	static std::string commandline( std::string bat_path ) ;
private:
	Service( const Service & ) ;
	void operator=( const Service & ) ;
	void setStatus( DWORD ) ;
	Handle statusHandle( std::string ) ;
	void stopThread() ;
	static void trim( std::string & ) ;
	static std::string tolower( const std::string & ) ;
private:
	static Service * m_this ;
} ;

// ==

int main( int argc , char * argv [] )
{
	try
	{
		std::string arg1 = argc > 1 ? lowercase(std::string(argv[1])) : std::string() ;
		std::string arg2 = argc > 2 ? std::string(argv[2]) : std::string("emailrelay") ;
		std::string arg3 = argc > 3 ? std::string(argv[3]) : std::string("E-MailRelay") ;

		bool help = arg1 == "--help" || arg1 == "/?" || arg1 == "-?" || arg1 == "-h" ;
		bool install = arg1 == "--install" || arg1 == "-install" || arg1 == "/install" ;
		bool remove = 
			arg1 == "--remove" || arg1 == "-remove" || arg1 == "/remove" ||
			arg1 == "--uninstall" || arg1 == "-uninstall" || arg1 == "/uninstall" ;

		if( help )
			std::cout << "usage: " << argv[0] << " [--help|--install|--remove] [<name> [<display-name>]]" << std::endl ;
		else if( install )
			Service::install( arg2 , arg3 ) ;
		else if( remove )
			Service::remove( arg2 ) ;
		else
			Service::start() ;
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
		Service::tstring service_name ;
		if( argc > 0 )
			service_name = Service::tstring(argv[0]) ;

		std::string sname ;
		G::Convert::convert( sname , service_name , G::Convert::ThrowOnError("converting service name") ) ;

		Service * service = Service::instance() ;
		if( service != NULL )
			service->init( sname ) ;
	}
	catch( std::exception & e )
	{
		G_DEBUG( "ServiceMain: exception: " << e.what() ) ;
	}
	catch(...)
	{
		G_DEBUG( "ServiceMain: unknown exception" ) ;
	}
	G_DEBUG( "ServiceMain: done" ) ;
}

void WINAPI Handler( DWORD control )
{
	try
	{
		G_DEBUG( "Handler: " << control ) ;
		Service * service = Service::instance() ;
		if( service == NULL ) throw Error("Handler",ERROR_INVALID_HANDLE) ;
		service->onEvent( control ) ;
	}
	catch( std::exception & e )
	{
		G_DEBUG( "Handler: exception: " << e.what() ) ;
	}
	catch(...)
	{
		G_DEBUG( "Handler: unknown exception" ) ;
	}
	G_DEBUG( "Handler: done" ) ;
}

DWORD WINAPI RunThread( LPVOID arg )
{
	try
	{
		G_DEBUG( "RunThread: start" ) ;
		Service * service = reinterpret_cast<Service*>(arg) ;
		bool valid = service != NULL ;
		try { valid = service->valid() ; } catch(...) { valid = false ; }
		service->runThread() ;
		G_DEBUG( "RunThread: done" ) ;
		return 0 ;
	}
	catch(...)
	{
		G_DEBUG( "RunThread: unknown exception" ) ;
		return 1 ;
	}
}

// ==

Service * Service::m_this = NULL ;

void Service::install( const std::string & service_name , const std::string & display_name )
{
	// prepare the service-wrapper commandline
	std::string this_exe = Service::exe() ;
	std::string command_line = this_exe.find(" ") == std::string::npos ? 
		this_exe : ( std::string("\"")+this_exe+"\"") ;
	std::cout << "installing service \"" << service_name << "\": [" << command_line << "]" << std::endl ;

	// check that we will be able to read the batch file at service run-time
	std::string batch_file = Service::bat( service_name ) ;
	std::string server_command_line = Service::commandline( batch_file ) ;

	// create the service
	std::string description = display_name + " service (reads " + batch_file + " at service start time)" ;
	std::string reason = service_install( command_line , service_name , display_name , description ) ;
	if( !reason.empty() )
		throw std::runtime_error( reason ) ;
}

void Service::remove( const std::string & service_name )
{
	std::string reason = service_remove( service_name ) ;
	if( !reason.empty() )
		throw std::runtime_error( reason ) ;
}

Service::Service() :
	m_magic(Magic) ,
	m_hservice(0) ,
	m_status(SERVICE_START_PENDING) ,
	m_hthread(0) ,
	m_thread_exit(0)
{
	G_DEBUG( "Service::ctor" ) ;
	m_this = this ;
}

void Service::init( std::string name )
{
	try
	{
		G_DEBUG( "Service::init: start" ) ;
		m_hservice = statusHandle( name ) ;
		setStatus( SERVICE_START_PENDING ) ;
		m_child = Child( commandline(bat(name)) ) ;
		m_thread_exit = CreateEvent( NULL , FALSE , FALSE , NULL ) ;
		m_hthread = CreateThread( NULL , 0 , RunThread , this , 0 , &m_thread_id ) ;
		G_DEBUG( "Service::init: done" ) ;
	}
	catch( std::exception & e )
	{
		G_DEBUG( "Service::init: exception: " << e.what() ) ;
		if( m_hservice ) setStatus( SERVICE_STOPPED ) ;
		stopThread() ;
		throw ;
	}
	catch(...)
	{
		G_DEBUG( "Service::init: exception" ) ;
		if( m_hservice ) setStatus( SERVICE_STOPPED ) ;
		stopThread() ;
		throw ;
	}
}

Service::~Service()
{
	G_DEBUG( "Service::dtor" ) ;
	try { m_child.kill() ; } catch(...) {}
	try { m_child.close() ; } catch(...) {}
	try { setStatus(SERVICE_STOPPED) ; } catch(...) {}
	stopThread() ;
	m_magic = 0 ;
	m_this = NULL ;
	G_DEBUG( "Service::dtor: done" ) ;
}

Service * Service::instance()
{
	Service * service = m_this ;
	bool valid = false ;
	try { valid = service && service->valid() ; } catch(...) {}
	return valid ? service : NULL ;
}

bool Service::valid() const
{
	return m_magic == Magic ;
}

void Service::stopThread()
{
	if( m_thread_exit ) 
		SetEvent( m_thread_exit ) ;
}

std::string Service::exe()
{
	HINSTANCE hinstance = 0 ;
	char buffer[G::limits::path] = { 0 } ;
	size_t size = G::limits::path ;
	::GetModuleFileNameA( hinstance , buffer , size-1U ) ;
	buffer[size-1U] = '\0' ;
	return std::string(buffer) ;
}

std::string Service::bat( const std::string & prefix )
{
	std::string this_exe = exe() ;
	std::string::size_type pos = this_exe.find_last_of( "\\/" ) ;
	std::string this_dir = pos == std::string::npos ? ".\\" : this_exe.substr(0U,pos+1U) ;
	std::string bat = this_dir + prefix + "-start.bat" ;
	return bat ;
}

void Service::trim( std::string & line )
{
	if( !line.empty() )
	{
		std::string::size_type start = line.find_first_not_of(" \t\r\n") ;
		std::string::size_type end = line.find_last_not_of(" \t\r\n") ;
		if( end != std::string::npos ) ++end ;
		line = start == end ? std::string() : line.substr(start,end-start) ;
	}
}

std::string Service::tolower( const std::string & s_in )
{
	const std::string in = "ABCDEFGHIJKLMNOPQRSTUVWXYZ" ;
	const std::string out = "abcdefghijklmnopqrstuvwxyz" ;
	std::string s = s_in ;
	for( std::string::iterator p = s.begin() ; p != s.end() ; ++p )
	{
		if( in.find(*p) != std::string::npos )
			*p = out.at( in.find(*p) ) ;
	}
	return s ;
}

std::string Service::commandline( std::string bat_path )
{
	G_DEBUG( "commandline: reading batch file: " << bat_path ) ;
	std::ifstream stream( bat_path.c_str() ) ;
	if( ! stream.good() )
		throw std::runtime_error( std::string() + "cannot open \"" + bat_path + "\"" +
			" (the service wrapper reads the command-line for the server process from this file)" ) ;

	std::string line ;
	while( stream.good() )
	{
		line.clear() ;
		std::getline( stream , line ) ;
		trim( line ) ;
		if( line.empty() || tolower(line).find("@echo") == 0U || tolower(line).find("rem") == 0U )
			;
		else
			break ;
	}
	if( line.empty() )
		throw std::runtime_error( std::string() + "cannot read \"" + bat_path + "\"" ) ;

	std::string next_line ;
	trim( next_line ) ;
	if( ! next_line.empty() )
		throw std::runtime_error( std::string() + "too many lines in the batch file \"" + bat_path + "\"; expecting only one line after initial \"@echo\" and \"rem\" lines" ) ;

	// the "start" command gets confused if its first parameter is
	// the quoted executable path - so require the quoted window title 
	// to appear first - (could do better parsing)
	std::string start_prefix = "start \"emailrelay\" " ;
	if( line.find(start_prefix) == 0U )
	{
		line.replace( 0U , start_prefix.length() , "" ) ;
		trim( line ) ;
	}

	// batch files require percent characters to be doubled up
	{
		std::string::size_type pos = line.rfind( "%%" ) ;
		if( pos != std::string::npos )
			line.replace( pos , 2U , "%" ) ;
	}

	if( line.find("--hidden") == std::string::npos )
		line.append( " --hidden" ) ;

	if( line.find("--no-daemon") == std::string::npos )
		line.append( " --no-daemon" ) ;

	G_DEBUG( "commandline: [" << line << "]" ) ;
	return line ;
}

void Service::onEvent( DWORD event )
{
	if( event == SERVICE_CONTROL_STOP ) 
	{
		G_DEBUG( "Service::onEvent: stop" ) ;
		m_child.kill() ;
		setStatus( SERVICE_STOPPED ) ;
	}
	else if( event == SERVICE_CONTROL_INTERROGATE ) 
	{
		G_DEBUG( "Service::onEvent: interrogate" ) ;
	}
	else
	{
		G_DEBUG( "Service::onEvent: " << event << ": not implemented" ) ;
		throw Error( "onEvent" , ERROR_CALL_NOT_IMPLEMENTED ) ;
	}
}

void Service::runThread()
{
	if( m_magic != Magic || m_thread_exit == NULL )
	{
		G_DEBUG( "Service::runThread: internal error" ) ;
		return ;
	}

	// test twice and then give up -- exit immediately if the exit 'event' is signalled
	G_DEBUG( "Service::runThread: waiting (1)" ) ;
	if( WaitForSingleObject(m_thread_exit,cfg_timeout_ms()) == WAIT_TIMEOUT )
	{
		if( m_child.isRunning() )
		{
			G_DEBUG( "Service::runThread: is running" ) ;
			setStatus( SERVICE_RUNNING ) ;
		}
		else
		{
			G_DEBUG( "Service::runThread: waiting (2)" ) ;
			if( WaitForSingleObject(m_thread_exit,cfg_timeout_ms()) == WAIT_TIMEOUT )
			{
				bool ok = m_child.isRunning() ;
				G_DEBUG( "Service::runThread: " << (ok?"is":"not") << " running" ) ;
				setStatus( ok ? SERVICE_RUNNING : SERVICE_STOPPED ) ;
			}
			else
			{
				G_DEBUG( "Service::runThread: signalled to stop" ) ;
			}
		}
	}
	else
	{
		G_DEBUG( "Service::runThread: signalled to stop" ) ;
	}

	HANDLE h = m_thread_exit ;
	m_thread_exit = NULL ;
	CloseHandle( h ) ;
	G_DEBUG( "Service::runThread: done" ) ;
}

void Service::start()
{
	G_DEBUG( "Service::start" ) ;
	{
		Service service ;
		static TCHAR empty[] = { 0 } ;
		static SERVICE_TABLE_ENTRY table [] = { { empty , ServiceMain } , { NULL , NULL } } ;
		bool ok = !! StartServiceCtrlDispatcher( table ) ; // this doesn't return until the service is stopped
		if( !ok )
		{
			DWORD e = GetLastError() ;
			throw Error( "StartServiceCtrlDispatcher" , e ) ;
		}
	}
	G_DEBUG( "Service::start: done" ) ;
}

Service::Handle Service::statusHandle( std::string service_name )
{
	Handle h = RegisterServiceCtrlHandlerA( service_name.c_str() , Handler ) ;
	if( h == 0 )
	{
		DWORD e = GetLastError() ;
		throw Error( "RegisterServiceCtrlHandlerEx" , e ) ;
	}
	return h ;
}

void Service::setStatus( DWORD new_state )
{
	G_DEBUG( "Service::setStatus: " << new_state ) ;

	static SERVICE_STATUS zero ;
	SERVICE_STATUS s = zero ;
	s.dwServiceType = SERVICE_WIN32_OWN_PROCESS ;
	s.dwCurrentState = new_state ;
	s.dwControlsAccepted = SERVICE_ACCEPT_STOP ;
	s.dwWin32ExitCode = NO_ERROR ;
	s.dwServiceSpecificExitCode = 0 ;
	s.dwCheckPoint = 0 ;
	s.dwWaitHint = cfg_overall_timeout_ms() ;

	bool ok = !! SetServiceStatus( m_hservice , &s ) ;
	if( ok )
	{
		m_status = new_state ;
	}
	else
	{
		DWORD e = GetLastError() ;
		throw Error( "SetServiceStatus" , e ) ;
	}
	G_DEBUG( "Service::setStatus: done" ) ;
}

// ==

Child::Child() :
	m_hprocess(0)
{
}

Child::Child( std::string command_line ) :
	m_hprocess(0)
{
	G_DEBUG( "Child::ctor: spawning [" << command_line << "]" ) ;

	static STARTUPINFOA zero_start ;
	STARTUPINFOA start(zero_start) ;
	start.cb = sizeof(start) ;

	BOOL inherit = FALSE ;
	DWORD flags = CREATE_NO_WINDOW ;
	LPVOID env = NULL ;
	LPCSTR cwd = NULL ;
	PROCESS_INFORMATION info ;
	SECURITY_ATTRIBUTES * process_attributes = NULL ;
	SECURITY_ATTRIBUTES * thread_attributes = NULL ;
	char * command_line_p = const_cast<char*>(command_line.c_str()) ;

	BOOL rc = ::CreateProcessA( NULL , command_line_p ,
		process_attributes , thread_attributes , inherit ,
		flags , env , cwd , &start , &info ) ;

	if( !rc )
		throw std::runtime_error( std::string() + "cannot create process: [" + command_line + "]" ) ;

	::CloseHandle( info.hThread ) ;
	m_hprocess = info.hProcess ;
	G_DEBUG( "Child::ctor: done" ) ;
}

void Child::close()
{
	if( m_hprocess )
	{
		HANDLE h = m_hprocess ;
		m_hprocess = 0 ;
		CloseHandle( h ) ;
	}
}

bool Child::isRunning() const
{
	if( m_hprocess )
		return WaitForSingleObject( m_hprocess , 0 ) == WAIT_TIMEOUT ;
	else
		return false ;
}

void Child::kill()
{
	if( m_hprocess )
	{
		G_DEBUG( "Child::kill: killing " << m_hprocess ) ;
		bool ok = !! TerminateProcess( m_hprocess , 50 ) ;
		if( ok )
		{
			close() ;
		}
		else
		{
			DWORD e = GetLastError() ;
			G_DEBUG( "Child::kill: failed: " << e ) ;
			throw Error( "TerminateProcess" , e ) ;
		}
	}
}

// ==

std::string Error::decode( DWORD e )
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

/// \file service_wrapper.cpp
