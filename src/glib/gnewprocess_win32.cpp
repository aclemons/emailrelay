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
// gnewprocess_win32.cpp
//

#include "gdef.h"
#include "glimits.h"
#include "gnewprocess.h"
#include "gexception.h"
#include "gstr.h"
#include "glog.h"
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <process.h>
#include <io.h>
#include <fcntl.h>
#include <algorithm> // std::swap
#include <utility> // std::swap

namespace
{
	const HANDLE HNULL = INVALID_HANDLE_VALUE ;
}

namespace G
{
	class Pipe ;
	class NewProcessImp ;
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
	static void makeUninherited( HANDLE h , bool do_throw ) ;
private:
	bool m_active ;
	bool m_first ;
	HANDLE m_read ;
	HANDLE m_write ;
} ;

class G::NewProcessImp 
{
public:
	static std::string commandLine( std::string exe , Strings args ) ;
	static HANDLE createProcess( const std::string & exe , const std::string & command_line , HANDLE hstdout ) ;
	static DWORD waitFor( HANDLE hprocess , DWORD default_exit_code ) ;
} ;

class G::NewProcess::ChildProcessImp 
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

G::NewProcess::ChildProcessImp::ChildProcessImp() :
	m_ref_count(0UL) ,
	m_pipe(true,true) ,
	m_hprocess(0)
{
}

// ===

G::NewProcess::ChildProcess::ChildProcess( ChildProcessImp * imp ) :
	m_imp(imp)
{
	m_imp->m_ref_count = 1 ;
}

G::NewProcess::ChildProcess::~ChildProcess()
{
	m_imp->m_ref_count-- ;
	if( m_imp->m_ref_count == 0 )
		delete m_imp ;
}

G::NewProcess::ChildProcess::ChildProcess( const ChildProcess & other ) :
	m_imp(other.m_imp)
{
	m_imp->m_ref_count++ ;
}

void G::NewProcess::ChildProcess::operator=( const ChildProcess & rhs )
{
	ChildProcess temp( rhs ) ;
	std::swap( m_imp , temp.m_imp ) ;
}

int G::NewProcess::ChildProcess::wait()
{
	return G::NewProcessImp::waitFor( m_imp->m_hprocess , 127 ) ;
}

std::string G::NewProcess::ChildProcess::read()
{
	return m_imp->m_pipe.read(false) ;
}

// ===

G::NewProcess::ChildProcess G::NewProcess::spawn( const Path & exe , const Strings & args )
{
	ChildProcess child( new ChildProcessImp ) ;
	std::string command_line = NewProcessImp::commandLine( exe.str() , args ) ;
	child.m_imp->m_hprocess = NewProcessImp::createProcess( exe.str() , command_line , child.m_imp->m_pipe.h() ) ;
	return child ;
}

int G::NewProcess::spawn( Identity , const Path & exe_path , const Strings & args , 
	std::string * pipe_result_p , int error_return , std::string (*fn)(int) )
{
	G_DEBUG( "G::NewProcess::spawn: [" << exe_path << "]: [" << Str::join(args,"],[") << "]" ) ;

	// create a pipe
	Pipe pipe( pipe_result_p != NULL ) ;

	// create the process
	std::string command_line = NewProcessImp::commandLine( exe_path.str() , args ) ;
	HANDLE hprocess = NewProcessImp::createProcess( exe_path.str() , command_line , pipe.h() ) ;
	if( hprocess == HNULL )
	{
		DWORD e = ::GetLastError() ;
		G_ERROR( "G::Process::spawn: create-process error " << e << ": " << command_line ) ;
		if( fn != 0 && pipe_result_p != NULL )
			*pipe_result_p = (*fn)(static_cast<int>(e)) ;
		return error_return ;
	}

	// wait for the child process to exit
	DWORD exit_code = NewProcessImp::waitFor( hprocess , error_return ) ;
	G_LOG( "G::NewProcess::spawn: exit " << exit_code << " from \"" << command_line << "\": " << exit_code ) ;

	// return the contents of the pipe
	if( pipe_result_p != NULL )
		*pipe_result_p = pipe.read(false) ;

	return exit_code ;
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
		m_read = create( m_write , do_throw ) ;
		makeUninherited( m_read , do_throw ) ;
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
	DWORD buffer_size_hint = 0 ;
	BOOL rc = ::CreatePipe( &h_read , &h_write , &attributes , buffer_size_hint ) ;
	if( rc == 0 )
	{
		DWORD error = ::GetLastError() ;
		G_ERROR( "G::Pipe::create: pipe error: create: " << error ) ;
		if( do_throw ) throw Error( "create" ) ;
		h_read = h_write = HNULL ;
	}
	return h_read ;
}

void G::Pipe::makeUninherited( HANDLE h , bool do_throw )
{
	if( ! SetHandleInformation( h , HANDLE_FLAG_INHERIT , 0 ) && do_throw )
	{
		DWORD error = ::GetLastError() ;
		::CloseHandle( h ) ;
		G_ERROR( "G::Pipe::makeUninherited: uninherited error " << error ) ;
		if( do_throw ) throw Error( "uninherited" ) ;
	}
}

HANDLE G::Pipe::h() const
{
	return m_write ;
}

std::string G::Pipe::readSome( BOOL & ok , DWORD & error )
{
	char buffer[limits::pipe_buffer] ;
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

HANDLE G::NewProcessImp::createProcess( const std::string & exe , const std::string & command_line , HANDLE hstdout )
{
	static STARTUPINFOA zero_start ;
	STARTUPINFOA start(zero_start) ;
	start.cb = sizeof(start) ;
	start.dwFlags = STARTF_USESTDHANDLES ;
	start.hStdInput = HNULL ;
	start.hStdOutput = hstdout ;
	start.hStdError = HNULL ;

	BOOL inherit = TRUE ;
	DWORD flags = CREATE_NO_WINDOW ;
	LPVOID env = NULL ;
	LPCSTR cwd = NULL ;
	PROCESS_INFORMATION info ;
	SECURITY_ATTRIBUTES * process_attributes = NULL ;
	SECURITY_ATTRIBUTES * thread_attributes = NULL ;
	char * command_line_p = const_cast<char*>(command_line.c_str()) ;

	BOOL rc = ::CreateProcessA( exe.c_str() , command_line_p ,
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

std::string G::NewProcessImp::commandLine( std::string exe , Strings args )
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

DWORD G::NewProcessImp::waitFor( HANDLE hprocess , DWORD default_exit_code )
{
	// waits for the process to end and closes the handle
	DWORD timeout_ms = 30000UL ; // not critical -- only used for the warning message
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

/// \file gnewprocess_win32.cpp
