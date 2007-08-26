//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gprocess_win32.cpp
//

#include "gdef.h"
#include "gprocess.h"
#include "gexception.h"
#include "gstr.h"
#include "glog.h"
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <process.h>
#include <io.h>
#include <fcntl.h>

namespace
{
	const int g_stderr_fileno = 2 ;
	const int g_sc_open_max = 256 ; // 32 in limits.h !?
	const HANDLE HNULL = INVALID_HANDLE_VALUE ;
}

namespace G
{
	class Pipe ;
	class ProcessImp ;
}

class G::Pipe 
{
public:
	G_EXCEPTION( Error , "pipe error" ) ;
	explicit Pipe( bool active , bool do_throw = true ) ;
	~Pipe() ;
	HANDLE h() const ;
	std::string read( bool do_throw = true ) ;
private:
	std::string readSome( BOOL & ok , DWORD & error ) ;
	static HANDLE create( HANDLE & h_write , bool do_throw ) ;
	static HANDLE duplicate( HANDLE h , bool do_throw ) ;
private:
	bool m_active ;
	bool m_first ;
	HANDLE m_read ;
	HANDLE m_write ;
} ;

class G::Process::IdImp 
{
public: 
	unsigned int m_pid ;
} ;

class G::ProcessImp 
{
public:
	static std::string commandLine( std::string exe , Strings args ) ;
	static HANDLE createProcess( const std::string & exe , const std::string & command_line , HANDLE hstdout ) ;
	static DWORD waitFor( HANDLE hprocess , DWORD default_exit_code ) ;
} ;

class G::Process::ChildProcessImp 
{
public:
	ChildProcessImp() ;
	unsigned long m_ref_count ;
	Pipe m_pipe ;
	HANDLE m_hprocess ;
private:
	void operator=( const ChildProcessImp & ) ;
	ChildProcessImp( const ChildProcessImp & ) ;
} ;

// ===

G::Process::ChildProcessImp::ChildProcessImp() :
	m_ref_count(0UL) ,
	m_pipe(true,true) ,
	m_hprocess(0)
{
}

// ===

G::Process::ChildProcess::ChildProcess( ChildProcessImp * imp ) :
	m_imp(imp)
{
	m_imp->m_ref_count = 1 ;
}

G::Process::ChildProcess::~ChildProcess()
{
	m_imp->m_ref_count-- ;
	if( m_imp->m_ref_count == 0 )
		delete m_imp ;
}

G::Process::ChildProcess::ChildProcess( const ChildProcess & other ) :
	m_imp(other.m_imp)
{
	m_imp->m_ref_count++ ;
}

void G::Process::ChildProcess::operator=( const ChildProcess & rhs )
{
	ChildProcess temp( rhs ) ;
	std::swap( m_imp , temp.m_imp ) ;
}

int G::Process::ChildProcess::wait()
{
	return G::ProcessImp::waitFor( m_imp->m_hprocess , 127 ) ;
}

std::string G::Process::ChildProcess::read()
{
	return m_imp->m_pipe.read(false) ;
}

// ===

G::Process::Id::Id() 
{
	m_pid = static_cast<unsigned int>(::_getpid()) ; // or ::GetCurrentProcessId()
}

G::Process::Id::Id( SignalSafe , const char * path ) :
	m_pid(0)
{
	std::ifstream file( path ? path : "" ) ;
	file >> m_pid ;
	if( !file.good() )
		m_pid = 0 ;
}

G::Process::Id::Id( std::istream & stream )
{
	stream >> m_pid ;
	if( !stream.good() )
		throw Process::InvalidId() ;
}

std::string G::Process::Id::str() const
{
	std::ostringstream ss ;
	ss << m_pid ;
	return ss.str() ;
}

bool G::Process::Id::operator==( const Id & rhs ) const
{
	return m_pid == rhs.m_pid ;
}

// not implemented...
//G::Process::Id::Id( const char * pid_file_path ) {} 

// ===

void G::Process::closeFiles( bool keep_stderr )
{
	const int n = g_sc_open_max ;
	for( int fd = 0 ; fd < n ; fd++ )
	{
		if( !keep_stderr || fd != g_stderr_fileno )
			::_close( fd ) ;
	}
}

void G::Process::closeStderr()
{
	int fd = g_stderr_fileno ;
	::_close( fd ) ;
}

void G::Process::cd( const Path & dir )
{
	if( !cd(dir,NoThrow()) )
		throw CannotChangeDirectory( dir.str() ) ;
}

bool G::Process::cd( const Path & dir , NoThrow )
{
	return 0 == ::_chdir( dir.str().c_str() ) ;
}

int G::Process::errno_()
{
	return errno ;
}

std::string G::Process::strerror( int errno_ )
{
	std::ostringstream ss ;
	ss << errno_ ; // could do better
	return ss.str() ;
}

G::Process::ChildProcess G::Process::spawn( const Path & exe , const Strings & args )
{
	ChildProcess child( new ChildProcessImp ) ;
	std::string command_line = ProcessImp::commandLine( exe.str() , args ) ;
	child.m_imp->m_hprocess = ProcessImp::createProcess( exe.str() , command_line , child.m_imp->m_pipe.h() ) ;
	return child ;
}

int G::Process::spawn( Identity , const Path & exe_path , const Strings & args , 
	std::string * pipe_result_p , int error_return , std::string (*fn)(int) )
{
	G_DEBUG( "G::Process::spawn: [" << exe_path << "]: [" << Str::join(args,"],[") << "]" ) ;

	// create a pipe
	Pipe pipe( pipe_result_p != NULL ) ;

	// create the process
	std::string command_line = ProcessImp::commandLine( exe_path.str() , args ) ;
	HANDLE hprocess = ProcessImp::createProcess( exe_path.str() , command_line , pipe.h() ) ;
	if( hprocess == HNULL )
	{
		DWORD e = ::GetLastError() ;
		G_ERROR( "G::Process::spawn: create-process error " << e << ": " << command_line ) ;
		if( fn != 0 && pipe_result_p != NULL )
			*pipe_result_p = (*fn)(static_cast<int>(e)) ;
		return error_return ;
	}

	// wait for the child process to exit
	DWORD exit_code = ProcessImp::waitFor( hprocess , error_return ) ;
	G_LOG( "G::Process::spawn: exit " << exit_code << " from \"" << command_line << "\": " << exit_code ) ;

	// return the contents of the pipe
	if( pipe_result_p != NULL )
		*pipe_result_p = pipe.read(false) ;

	return exit_code ;
}

G::Identity G::Process::beOrdinary( Identity identity , bool )
{
	// not implemented -- see also ImpersonateLoggedOnUser()
	return identity ;
}

G::Identity G::Process::beOrdinary( SignalSafe , Identity identity , bool )
{
	// not implemented -- see also ImpersonateLoggedOnUser()
	return identity ;
}

G::Identity G::Process::beSpecial( Identity identity , bool )
{
	// not implemented -- see also RevertToSelf()
	return identity ;
}

G::Identity G::Process::beSpecial( SignalSafe , Identity identity , bool )
{
	// not implemented -- see also RevertToSelf()
	return identity ;
}

void G::Process::revokeExtraGroups()
{
	// not implemented
}

// not implemented...
// Who G::Process::fork() {}
// Who G::Process::fork( Id & child ) {}
// void G::Process::exec( const Path & exe , const std::string & arg ) {}
// int G::Process::wait( const Id & child ) {}
// int G::Process::wait( const Id & child , int error_return ) {}

// ===

G::Process::Umask::Umask( G::Process::Umask::Mode ) :
	m_imp(0)
{
}

G::Process::Umask::~Umask()
{
}

void G::Process::Umask::set( G::Process::Umask::Mode )
{
	// not implemented
}

// ===

G::Pipe::Pipe( bool active , bool do_throw ) :
	m_active(active) ,
	m_first(true) ,
	m_read(HNULL) ,
	m_write(HNULL)
{
	if( m_active )
	{
		const bool do_throw = true ;
		HANDLE h = create( m_write , do_throw ) ;
		if( h != HNULL )
		{
			// dont let child processes inherit the read end
			m_read = duplicate( h , do_throw ) ;
		}
	}
}

G::Pipe::~Pipe()
{
	if( m_active )
	{
		if( m_read != HNULL ) ::CloseHandle( m_read ) ;
		if( m_write != HNULL ) ::CloseHandle( m_write ) ;
	}
}

HANDLE G::Pipe::create( HANDLE & h_write , bool do_throw )
{
	static SECURITY_ATTRIBUTES zero_attributes ;
	SECURITY_ATTRIBUTES attributes( zero_attributes ) ;
	attributes.nLength = sizeof(attributes) ;
	attributes.lpSecurityDescriptor = NULL ;
	attributes.bInheritHandle = TRUE ;

	HANDLE h_read = HNULL ;
	h_write = HNULL ;
	BOOL rc = ::CreatePipe( &h_read , &h_write , &attributes , 0 ) ;
	if( rc == 0 )
	{
		DWORD error = ::GetLastError() ;
		G_ERROR( "G::Pipe::create: pipe error: create: " << error ) ;
		if( do_throw ) throw Error( "create" ) ;
		h_read = h_write = HNULL ;
	}
	return h_read ;
}

HANDLE G::Pipe::duplicate( HANDLE h , bool do_throw )
{
	HANDLE result = HNULL ;
	BOOL rc = ::DuplicateHandle( GetCurrentProcess() , h , GetCurrentProcess() , 
		&result , 0 , FALSE , DUPLICATE_SAME_ACCESS ) ;
	if( rc == 0 || result == HNULL )
	{
		DWORD error = ::GetLastError() ;
		::CloseHandle( h ) ;
		G_ERROR( "G::Pipe::duplicate: pipe error: dup: " << error ) ;
		if( do_throw ) throw Error( "dup" ) ;
		result = HNULL ;
	}
	::CloseHandle( h ) ;
	return result ;
}

HANDLE G::Pipe::h() const
{
	return m_write ;
}

std::string G::Pipe::readSome( BOOL & ok , DWORD & error )
{
	char buffer[4096] ;
	buffer[0] = '\0' ;
	DWORD n = sizeof(buffer) ;
	DWORD m = 0UL ;
	ok = ::ReadFile( m_read , buffer , n , &m , NULL ) ;
	error = ::GetLastError() ;
	m = m > n ? n : m ;
	return m ? std::string(buffer,m) : std::string() ;
}

std::string G::Pipe::read( bool do_throw )
{
	if( ! m_active ) 
		return std::string() ;

	bool first = m_first ;
	m_first = false ;
	if( first )
	{
		::CloseHandle( m_write ) ; 
		m_write = HNULL ;
	}

	BOOL ok = FALSE ;
	DWORD error = 0 ;
	std::string s = readSome( ok , error ) ;
	if( !ok )
	{
		G_DEBUG( "G::Pipe::read: pipe read error: " << error ) ;
		if( error != ERROR_BROKEN_PIPE )
		{
			G_ERROR( "G::Pipe::read: pipe read error: " << error ) ;
			if( do_throw ) throw Error( "read" ) ;
		}
	}
	return s ;
}

// ===

HANDLE G::ProcessImp::createProcess( const std::string & exe , const std::string & command_line , HANDLE hstdout )
{
	static STARTUPINFO zero_start ;
	STARTUPINFO start(zero_start) ;
	start.cb = sizeof(start) ;
	start.dwFlags = STARTF_USESTDHANDLES ;
	start.hStdInput = HNULL ;
	start.hStdOutput = hstdout ;
	start.hStdError = HNULL ;

	BOOL inherit = TRUE ;
	DWORD flags = CREATE_NO_WINDOW ;
	LPVOID env = NULL ;
	LPCTSTR cwd = NULL ;
	PROCESS_INFORMATION info ;
	SECURITY_ATTRIBUTES * process_attributes = NULL ;
	SECURITY_ATTRIBUTES * thread_attributes = NULL ;
	char * command_line_p = const_cast<char*>(command_line.c_str()) ;

	BOOL rc = ::CreateProcess( exe.c_str() , command_line_p ,
		process_attributes , thread_attributes , inherit ,
		flags , env , cwd , &start , &info ) ;

	if( rc )
	{
		::CloseHandle( info.hThread ) ;
		return info.hProcess ;
	}
	else
	{
		return HNULL ;
	}
}

std::string G::ProcessImp::commandLine( std::string exe , Strings args )
{
	// returns quoted exe followed by args -- args are quoted iff they have a 
	// space and no quotes 

	char q = '\"' ;
	const std::string quote = std::string(1U,q) ;
	const std::string space = std::string(" ") ;

	bool exe_is_quoted = exe.length() > 1U && exe.at(0U) == q && exe.at(exe.length()-1U) == q ;

	std::string command_line = exe_is_quoted ? exe : ( quote + exe + quote ) ;
	for( Strings::const_iterator arg_p = args.begin() ; arg_p != args.end() ; ++arg_p )
	{
		std::string arg = *arg_p ;
		if( arg.find(" ") != std::string::npos && arg.find("\"") != 0U )
			arg = quote + arg + quote ;
		command_line += ( space + arg ) ;
	}

	return command_line ;
}

DWORD G::ProcessImp::waitFor( HANDLE hprocess , DWORD default_exit_code )
{
	// waits for the process to end and closes the handle
	DWORD timeout_ms = 30000UL ;
	if( WAIT_TIMEOUT == ::WaitForSingleObject( hprocess , timeout_ms ) )
	{
		G_WARNING( "G::Process::spawn: child process has not terminated: still waiting" ) ;
		::WaitForSingleObject( hprocess , INFINITE ) ;
	}
	DWORD exit_code = default_exit_code ;
	BOOL rc = ::GetExitCodeProcess( hprocess , &exit_code ) ;
	::CloseHandle( hprocess ) ;
	if( rc == 0 ) exit_code = default_exit_code ;
	return exit_code ;
}

/// \file gprocess_win32.cpp
