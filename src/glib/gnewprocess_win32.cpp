//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gnewprocess_win32.cpp
//

#include "gdef.h"
#include "gnewprocess.h"
#include "gexception.h"
#include "gstr.h"
#include "glog.h"
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <process.h>
#include <io.h>
#include <algorithm> // std::swap()
#include <utility> // std::swap()

namespace
{
	bool valid( HANDLE h ) g__noexcept
	{
		return h != NULL && h != INVALID_HANDLE_VALUE ;
	}
}

/// \class G::Pipe
/// A private implementation class used by G::NewProcess to manage a
/// windows pipe.
///
class G::Pipe
{
public:
	Pipe() ;
	~Pipe() ;
	HANDLE hread() const ;
	HANDLE hwrite() const ;
	static size_t read( const SignalSafe & , HANDLE read , char * buffer , size_t buffer_size ) ;
	void close() ;

private:
	Pipe( const Pipe & ) g__eq_delete ;
	void operator=( const Pipe & ) g__eq_delete ;
	static void create( HANDLE & read , HANDLE & write ) ;
	static void uninherited( HANDLE h ) ;

private:
	HANDLE m_read ;
	HANDLE m_write ;
} ;

/// \class G::NewProcessImp
/// A pimple-pattern implementation class used by G::NewProcess.
///
class G::NewProcessImp
{
public:
	NewProcessImp( const Path & exe , const StringArray & args , int capture_stdxxx ) ;
		// Constructor. Spawns the new process.

	~NewProcessImp() ;
		// Destructor. Kills the process if it is still running.

	NewProcessWaitFuture & wait() ;
		// Returns a reference to the WaitFuture sub-object to allow
		// the caller to wait for the process to finish.

	void kill() g__noexcept ;
		// Tries to kill the spawned process.

	int id() const g__noexcept ;
		// Returns the process id.

private:
	NewProcessImp( const NewProcessImp & ) g__eq_delete ;
	void operator=( const NewProcessImp & ) g__eq_delete ;
	static std::string commandLine( const std::string & exe , const StringArray & args ) ;
	static std::pair<HANDLE,DWORD> createProcess( const std::string & exe , const std::string & command_line ,
		HANDLE hstdout , int capture_stdxxx ) ;

private:
	HANDLE m_hprocess ;
	DWORD m_pid ;
	bool m_killed ;
	Pipe m_pipe ;
	NewProcessWaitFuture m_wait_future ;
} ;

// ===

G::NewProcess::NewProcess( const Path & exe , const StringArray & args ,
	int capture_stdxxx , bool clean , bool strict_path ,
	Identity run_as_id , bool strict_id ,
	int exec_error_exit , const std::string & exec_error_format ,
	std::string (*exec_error_format_fn)(std::string,int) ) :
		m_imp(new NewProcessImp(exe,args,capture_stdxxx) )
{
}

G::NewProcess::~NewProcess()
{
}

G::NewProcessWaitFuture & G::NewProcess::wait()
{
	return m_imp->wait() ;
}

int G::NewProcess::id() const g__noexcept
{
	return m_imp->id() ;
}

void G::NewProcess::kill( bool yield ) g__noexcept
{
	m_imp->kill() ;
	if( yield )
	{
		G::threading::yield() ;
		SleepEx( 0 , FALSE ) ;
	}
}

// ===

G::NewProcessImp::NewProcessImp( const Path & exe_path , const StringArray & args , int capture_stdxxx ) :
	m_hprocess(0) ,
	m_killed(false) ,
	m_wait_future(NULL,NULL,0)
{
	G_DEBUG( "G::NewProcess::spawn: running [" << exe_path << "]: [" << Str::join("],[",args) << "]" ) ;

	std::string command_line = commandLine( exe_path.str() , args ) ;
	std::pair<HANDLE,DWORD> pair = createProcess( exe_path.str() , command_line , m_pipe.hwrite() , capture_stdxxx ) ;
	m_hprocess = pair.first ;
	m_pid = pair.second ;

	m_pipe.close() ; // close write end, now used by child process
	m_wait_future.assign( m_hprocess , m_pipe.hread() , 0 ) ;
}

void G::NewProcessImp::kill() g__noexcept
{
	if( !m_killed && valid(m_hprocess) )
		::TerminateProcess( m_hprocess , 127 ) ;
	m_killed = true ;
}

G::NewProcessImp::~NewProcessImp()
{
	if( m_hprocess != 0 )
		::CloseHandle( m_hprocess ) ;
}

G::NewProcessWaitFuture & G::NewProcessImp::wait()
{
	return m_wait_future ;
}

int G::NewProcessImp::id() const g__noexcept
{
	return static_cast<int>(m_pid) ;
}

std::pair<HANDLE,DWORD> G::NewProcessImp::createProcess( const std::string & exe , const std::string & command_line ,
	HANDLE hout , int capture_stdxxx )
{
	// redirect stdout or stderr onto the read end of our pipe
	static STARTUPINFOA zero_start ;
	STARTUPINFOA start(zero_start) ;
	start.cb = sizeof(start) ;
	start.dwFlags = STARTF_USESTDHANDLES ;
	start.hStdInput = INVALID_HANDLE_VALUE ;
	start.hStdOutput = capture_stdxxx == 1 ? hout : INVALID_HANDLE_VALUE ;
	start.hStdError = capture_stdxxx == 2 ? hout : INVALID_HANDLE_VALUE ;

	BOOL inherit = TRUE ;
	DWORD flags = CREATE_NO_WINDOW ;
	LPVOID env = NULL ;
	LPCSTR cwd = NULL ;
	SECURITY_ATTRIBUTES * process_attributes = NULL ;
	SECURITY_ATTRIBUTES * thread_attributes = NULL ;

	static PROCESS_INFORMATION zero_info ;
	PROCESS_INFORMATION info( zero_info ) ;

	BOOL rc = ::CreateProcessA( exe.c_str() ,
		const_cast<char*>(command_line.c_str()) ,
		process_attributes , thread_attributes , inherit ,
		flags , env , cwd , &start , &info ) ;

	if( rc == 0 || !valid(info.hProcess) )
	{
		DWORD e = ::GetLastError() ;
		std::ostringstream ss ;
		ss << "error " << e << ": [" << command_line << "]" ;
		throw NewProcess::CreateProcessError( ss.str() ) ;
	}

	G_DEBUG( "G::NewProcessImp::createProcess: hprocess=" << info.hProcess ) ;
	G_DEBUG( "G::NewProcessImp::createProcess: process-id=" << info.dwProcessId ) ;
	G_DEBUG( "G::NewProcessImp::createProcess: hthread=" << info.hThread ) ;
	G_DEBUG( "G::NewProcessImp::createProcess: thread-id=" << info.dwThreadId ) ;

	::CloseHandle( info.hThread ) ;
	return std::make_pair( info.hProcess , info.dwProcessId ) ;
}

std::string G::NewProcessImp::commandLine( const std::string & exe , const StringArray & args )
{
	// returns quoted exe followed by args -- args are quoted iff they have a
	// space and no quotes

	char q = '\"' ;
	const std::string quote = std::string(1U,q) ;
	const std::string space = std::string(" ") ;

	bool exe_is_quoted = exe.length() > 1U && exe.at(0U) == q && exe.at(exe.length()-1U) == q ;

	std::string command_line = exe_is_quoted ? exe : ( quote + exe + quote ) ;
	for( StringArray::const_iterator arg_p = args.begin() ; arg_p != args.end() ; ++arg_p )
	{
		std::string arg = *arg_p ;
		if( arg.find(" ") != std::string::npos && arg.find("\"") != 0U )
			arg = quote + arg + quote ;
		command_line += ( space + arg ) ;
	}

	return command_line ;
}

// ===

G::Pipe::Pipe() :
	m_read(NULL) ,
	m_write(NULL)
{
	create( m_read , m_write ) ;
	uninherited( m_read ) ;
}

G::Pipe::~Pipe()
{
	if( m_read != NULL ) ::CloseHandle( m_read ) ;
	if( m_write != NULL ) ::CloseHandle( m_write ) ;
}

void G::Pipe::create( HANDLE & h_read , HANDLE & h_write )
{
	static SECURITY_ATTRIBUTES zero_attributes ;
	SECURITY_ATTRIBUTES attributes( zero_attributes ) ;
	attributes.nLength = sizeof(attributes) ;
	attributes.lpSecurityDescriptor = NULL ;
	attributes.bInheritHandle = TRUE ;

	h_read = NULL ;
	h_write = NULL ;
	DWORD buffer_size_hint = 0 ;
	BOOL rc = ::CreatePipe( &h_read , &h_write , &attributes , buffer_size_hint ) ;
	if( rc == 0 )
	{
		DWORD error = ::GetLastError() ;
		G_ERROR( "G::Pipe::create: pipe error: create: " << error ) ;
		throw NewProcess::PipeError( "create" ) ;
	}
}

void G::Pipe::uninherited( HANDLE h )
{
	if( ! SetHandleInformation( h , HANDLE_FLAG_INHERIT , 0 ) )
	{
		DWORD error = ::GetLastError() ;
		::CloseHandle( h ) ;
		G_ERROR( "G::Pipe::uninherited: uninherited error " << error ) ;
		throw NewProcess::PipeError( "uninherited" ) ;
	}
}

HANDLE G::Pipe::hwrite() const
{
	return m_write ;
}

HANDLE G::Pipe::hread() const
{
	return m_read ;
}

void G::Pipe::close()
{
	if( m_write != NULL )
		::CloseHandle( m_write ) ;
	m_write = NULL ;
}

size_t G::Pipe::read( const SignalSafe & signal_safe , HANDLE hread , char * buffer , size_t buffer_size_in )
{
	// (worker thread - keep it simple)
	DWORD buffer_size = static_cast<DWORD>(buffer_size_in) ;
	DWORD nread = 0U ;
	BOOL ok = ::ReadFile( hread , buffer , buffer_size , &nread , NULL ) ;
	//DWORD error = ::GetLastError() ;
	nread = ok ? std::min( nread , buffer_size ) : DWORD(0) ;
	return static_cast<size_t>(nread) ;
}

// ==

G::NewProcessWaitFuture::NewProcessWaitFuture() :
	m_hprocess(NULL),
	m_hpipe(NULL) ,
	m_pid(0),
	m_rc(0),
	m_status(0),
	m_error(0)
{
}

G::NewProcessWaitFuture::NewProcessWaitFuture( HANDLE hprocess , HANDLE hpipe , int ) :
	m_buffer(1024U) ,
	m_hprocess(hprocess),
	m_hpipe(hpipe),
	m_pid(0),
	m_rc(0),
	m_status(0),
	m_error(0)
{
}

void G::NewProcessWaitFuture::assign( HANDLE hprocess , HANDLE hpipe , int )
{
	m_buffer.resize( 1024U ) ;
	m_hprocess = hprocess ;
	m_hpipe = hpipe ;
	m_pid = 0 ;
	m_rc = 0 ;
	m_status = 0 ;
	m_error = 0 ;
}

G::NewProcessWaitFuture & G::NewProcessWaitFuture::run()
{
	// (worker thread - keep it simple)
	if( valid(m_hprocess) )
	{
		DWORD exit_code = 1 ;
		bool ok = ::WaitForSingleObject( m_hprocess , INFINITE ) == WAIT_OBJECT_0 ; G_IGNORE_VARIABLE(bool,ok) ;
		::GetExitCodeProcess( m_hprocess , &exit_code ) ;
		m_status = static_cast<int>(exit_code) ;
		m_hprocess = NULL ;
	}
	if( m_hpipe != NULL )
	{
		size_t nread = Pipe::read( SignalSafe() , m_hpipe , &m_buffer[0] , m_buffer.size() ) ;
		m_buffer.resize( nread ) ;
		m_hpipe = NULL ;
	}
	return *this ;
}

int G::NewProcessWaitFuture::get()
{
	return m_status ;
}

std::string G::NewProcessWaitFuture::output()
{
	return m_buffer.size() ? std::string(&m_buffer[0],m_buffer.size()) : std::string() ;
}

