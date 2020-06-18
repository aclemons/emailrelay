//
// Copyright (C) 2001-2020 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gpath.h"
#include "glog.h"
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <process.h>
#include <io.h>
#include <algorithm> // std::swap()
#include <utility> // std::swap()

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
	static std::size_t read( const SignalSafe & , HANDLE read , char * buffer , std::size_t buffer_size ) ;
	void close() ;

public:
	Pipe( const Pipe & ) = delete ;
	Pipe( Pipe && ) = delete ;
	void operator=( const Pipe & ) = delete ;
	void operator=( Pipe && ) = delete ;

private:
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
	using Fd = NewProcess::Fd ;

	NewProcessImp( const Path & exe , const StringArray & args , const Environment & ,
		Fd fd_stdin , Fd fd_stdout , Fd fd_stderr , const G::Path & cd ) ;
			// Constructor. Spawns the new process.

	~NewProcessImp() ;
		// Destructor. Kills the process if it is still running.

	NewProcessWaitFuture & wait() ;
		// Returns a reference to the WaitFuture sub-object to allow
		// the caller to wait for the process to finish.

	void kill() noexcept ;
		// Tries to kill the spawned process.

	int id() const noexcept ;
		// Returns the process id.

	static bool valid( HANDLE h ) noexcept ;
		// Returns true if a valid handle.

public:
	NewProcessImp( const NewProcessImp & ) = delete ;
	NewProcessImp( NewProcessImp && ) = delete ;
	void operator=( const NewProcessImp & ) = delete ;
	void operator=( NewProcessImp && ) = delete ;

private:
	static std::string commandLine( const std::string & exe , const StringArray & args ) ;
	static std::pair<HANDLE,DWORD> createProcess( const std::string & exe , const std::string & command_line ,
		const Environment & , HANDLE hpipe , Fd fd_stdout , Fd fd_stderr , const char * cd ) ;

private:
	HANDLE m_hprocess ;
	DWORD m_pid ;
	bool m_killed ;
	Pipe m_pipe ;
	NewProcessWaitFuture m_wait_future ;
} ;

// ===

G::NewProcess::NewProcess( const Path & exe , const StringArray & args , const Environment & env ,
	Fd fd_stdin , Fd fd_stdout , Fd fd_stderr , const G::Path & cd ,
	bool strict_path , Identity run_as_id , bool strict_id ,
	int exec_error_exit , const std::string & exec_error_format ,
	std::string (*exec_error_format_fn)(std::string,int) ) :
		m_imp(new NewProcessImp(exe,args,env,fd_stdin,fd_stdout,fd_stderr,cd) )
{
}

G::NewProcess::~NewProcess()
{
}

G::NewProcessWaitFuture & G::NewProcess::wait()
{
	return m_imp->wait() ;
}

int G::NewProcess::id() const noexcept
{
	return m_imp->id() ;
}

bool G::NewProcessImp::valid( HANDLE h ) noexcept
{
	return h != HNULL && h != INVALID_HANDLE_VALUE ;
}

void G::NewProcess::kill( bool yield ) noexcept
{
	m_imp->kill() ;
	if( yield )
	{
		G::threading::yield() ;
		SleepEx( 0 , FALSE ) ;
	}
}

// ===

G::NewProcessImp::NewProcessImp( const Path & exe_path , const StringArray & args , const Environment & env ,
	Fd fd_stdin , Fd fd_stdout , Fd fd_stderr , const G::Path & cd ) :
		m_hprocess(0) ,
		m_killed(false) ,
		m_wait_future(HNULL,HNULL,0)
{
	G_DEBUG( "G::NewProcess::spawn: running [" << exe_path << "]: [" << Str::join("],[",args) << "]" ) ;

	// only support Fd::devnull() and Fd::pipe() here
	if( fd_stdin != Fd::devnull() ||
		( fd_stdout != Fd::devnull() && fd_stdout != Fd::pipe() ) ||
		( fd_stderr != Fd::devnull() && fd_stderr != Fd::pipe() ) ||
		( fd_stdout == Fd::pipe() && fd_stderr == Fd::pipe() ) )
	{
		throw NewProcess::Error( "invalid parameters" ) ;
	}

	std::string command_line = commandLine( exe_path.str() , args ) ;
	std::pair<HANDLE,DWORD> pair = createProcess( exe_path.str() , command_line , env ,
		m_pipe.hwrite() , fd_stdout , fd_stderr ,
		cd == G::Path() ? nullptr : cd.str().c_str() ) ;

	m_hprocess = pair.first ;
	m_pid = pair.second ;

	m_pipe.close() ; // close write end, now used by child process
	m_wait_future.assign( m_hprocess , m_pipe.hread() , 0 ) ;
}

void G::NewProcessImp::kill() noexcept
{
	if( !m_killed && valid(m_hprocess) )
		TerminateProcess( m_hprocess , 127 ) ;
	m_killed = true ;
}

G::NewProcessImp::~NewProcessImp()
{
	if( m_hprocess != HNULL )
		CloseHandle( m_hprocess ) ;
}

G::NewProcessWaitFuture & G::NewProcessImp::wait()
{
	return m_wait_future ;
}

int G::NewProcessImp::id() const noexcept
{
	return static_cast<int>(m_pid) ;
}

std::pair<HANDLE,DWORD> G::NewProcessImp::createProcess( const std::string & exe , const std::string & command_line ,
	const Environment & env , HANDLE hpipe , Fd fd_stdout , Fd fd_stderr , const char * cd )
{
	// redirect stdout or stderr onto the read end of our pipe
	static STARTUPINFOA zero_start ;
	STARTUPINFOA start(zero_start) ;
	start.cb = sizeof(start) ;
	start.dwFlags = STARTF_USESTDHANDLES ;
	start.hStdInput = INVALID_HANDLE_VALUE ;
	start.hStdOutput = fd_stdout == Fd::pipe() ? hpipe : INVALID_HANDLE_VALUE ;
	start.hStdError = fd_stderr == Fd::pipe() ? hpipe : INVALID_HANDLE_VALUE ;

	BOOL inherit = TRUE ;
	DWORD flags = CREATE_NO_WINDOW ;
	LPVOID envp = env.empty() ? nullptr : static_cast<LPVOID>(const_cast<char*>(env.ptr())) ;
	LPCSTR cwd = cd ;
	SECURITY_ATTRIBUTES * process_attributes = nullptr ;
	SECURITY_ATTRIBUTES * thread_attributes = nullptr ;

	static PROCESS_INFORMATION zero_info ;
	PROCESS_INFORMATION info( zero_info ) ;

	BOOL rc = CreateProcessA( exe.c_str() ,
		const_cast<char*>(command_line.c_str()) ,
		process_attributes , thread_attributes , inherit ,
		flags , envp , cwd , &start , &info ) ;

	if( rc == 0 || !valid(info.hProcess) )
	{
		DWORD e = GetLastError() ;
		std::ostringstream ss ;
		ss << "error " << e << ": [" << command_line << "]" ;
		throw NewProcess::CreateProcessError( ss.str() ) ;
	}

	G_DEBUG( "G::NewProcessImp::createProcess: hprocess=" << info.hProcess ) ;
	G_DEBUG( "G::NewProcessImp::createProcess: process-id=" << info.dwProcessId ) ;
	G_DEBUG( "G::NewProcessImp::createProcess: hthread=" << info.hThread ) ;
	G_DEBUG( "G::NewProcessImp::createProcess: thread-id=" << info.dwThreadId ) ;

	CloseHandle( info.hThread ) ;
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
	m_read(HNULL) ,
	m_write(HNULL)
{
	create( m_read , m_write ) ;
	uninherited( m_read ) ;
}

G::Pipe::~Pipe()
{
	if( m_read != HNULL ) CloseHandle( m_read ) ;
	if( m_write != HNULL ) CloseHandle( m_write ) ;
}

void G::Pipe::create( HANDLE & h_read , HANDLE & h_write )
{
	static SECURITY_ATTRIBUTES zero_attributes ;
	SECURITY_ATTRIBUTES attributes( zero_attributes ) ;
	attributes.nLength = sizeof(attributes) ;
	attributes.lpSecurityDescriptor = nullptr ;
	attributes.bInheritHandle = TRUE ;

	h_read = HNULL ;
	h_write = HNULL ;
	DWORD buffer_size_hint = 0 ;
	BOOL rc = CreatePipe( &h_read , &h_write , &attributes , buffer_size_hint ) ;
	if( rc == 0 )
	{
		DWORD error = GetLastError() ;
		G_ERROR( "G::Pipe::create: pipe error: create: " << error ) ;
		throw NewProcess::PipeError( "create" ) ;
	}
}

void G::Pipe::uninherited( HANDLE h )
{
	if( ! SetHandleInformation( h , HANDLE_FLAG_INHERIT , 0 ) )
	{
		DWORD error = GetLastError() ;
		CloseHandle( h ) ;
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
	if( m_write != HNULL )
		CloseHandle( m_write ) ;
	m_write = HNULL ;
}

std::size_t G::Pipe::read( const SignalSafe & signal_safe , HANDLE hread , char * buffer , std::size_t buffer_size_in )
{
	// (worker thread - keep it simple)
	DWORD buffer_size = static_cast<DWORD>(buffer_size_in) ;
	DWORD nread = 0U ;
	BOOL ok = ReadFile( hread , buffer , buffer_size , &nread , nullptr ) ;
	//DWORD error = GetLastError() ;
	nread = ok ? std::min( nread , buffer_size ) : DWORD(0) ;
	return static_cast<std::size_t>(nread) ;
}

// ==

G::NewProcessWaitFuture::NewProcessWaitFuture() :
	m_hprocess(HNULL),
	m_hpipe(HNULL) ,
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
	if( NewProcessImp::valid(m_hprocess) )
	{
		DWORD exit_code = 1 ;
		bool ok = WaitForSingleObject( m_hprocess , INFINITE ) == WAIT_OBJECT_0 ; G_IGNORE_VARIABLE(bool,ok) ;
		GetExitCodeProcess( m_hprocess , &exit_code ) ; // returns zero on error, but nothing we can do here
		m_status = static_cast<int>(exit_code) ;
		m_hprocess = HNULL ;
	}
	if( m_hpipe != HNULL )
	{
		std::size_t nread = Pipe::read( SignalSafe() , m_hpipe , &m_buffer[0] , m_buffer.size() ) ;
		m_buffer.resize( nread ) ;
		m_hpipe = HNULL ;
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

