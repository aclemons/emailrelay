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
// gprocess_win32.cpp
//

#include "gdef.h"
#include "gprocess.h"
#include "gexception.h"
#include "gstr.h"
#include "glog.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <process.h>
#include <io.h>
#include <fcntl.h>

namespace G
{
	const int g_stderr_fileno = 2 ;
	const int g_sc_open_max = 256 ; // 32 in limits.h !?
	const HANDLE HNULL = INVALID_HANDLE_VALUE ;
	class Pipe ;
} ;

class G::Pipe 
{
public:
	G_EXCEPTION( Error , "pipe error" ) ;
	explicit Pipe( bool active , bool do_throw = true ) ;
	~Pipe() ;
	HANDLE h() const ;
	std::string read( bool do_throw = true ) ;
private:
	static HANDLE create( HANDLE & h_write , bool do_throw ) ;
	static HANDLE duplicate( HANDLE h , bool do_throw ) ;
private:
	bool m_active ;
	HANDLE m_read ;
	HANDLE m_write ;
} ;

class G::Process::IdImp 
{
public: 
	unsigned int m_pid ;
} ;

// ===

G::Pipe::Pipe( bool active , bool do_throw ) :
	m_active(active) ,
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

//static
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

//static
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

std::string G::Pipe::read( bool do_throw )
{
	if( ! m_active ) return std::string() ;
	::CloseHandle( m_write ) ; m_write = HNULL ;
	char buffer[4096] ;
	DWORD n = sizeof(buffer) ;
	DWORD m = 0UL ;
	BOOL rc = ::ReadFile( m_read , buffer , n , &m , NULL ) ;
	DWORD error = ::GetLastError() ;
	::CloseHandle( m_read ) ; m_read = HNULL ;
	if( !rc )
	{
		G_DEBUG( "G::Pipe::read: error: " << m << " byte(s): error=" << error ) ;
		if( error != ERROR_BROKEN_PIPE )
		{
			G_ERROR( "G::Pipe::read: pipe read error: " << error ) ;
			if( do_throw ) throw Error( "read" ) ;
		}
	}
	return std::string( buffer , m ) ;
}

// ===

G::Process::Id::Id() 
{
	m_pid = static_cast<unsigned int>(::_getpid()) ; // or ::GetCurrentProcessId()
}

G::Process::Id::Id( const char * path ) :
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

int G::Process::spawn( Identity , const Path & exe , const Strings & args , 
	std::string * pipe_result_p , int error_return )
{
	G_DEBUG( "G::Process::spawn: \"" << exe << "\": \"" << Str::join(args,"\",\"") << "\"" ) ;

	std::string command_line = std::string("\"") + exe.str() + "\"" ;
	for( Strings::const_iterator arg_p = args.begin() ; arg_p != args.end() ; ++arg_p )
	{
		std::string arg = *arg_p ;
		if( arg.find(" ") != std::string::npos && arg.find("\"") != 0U )
			arg = std::string("\"") + arg + "\"" ;
		command_line += ( std::string(" ") + arg ) ;
	}

	Pipe pipe( pipe_result_p != NULL ) ;

	SECURITY_ATTRIBUTES * process_attributes = NULL ;
	SECURITY_ATTRIBUTES * thread_attributes = NULL ;
	BOOL inherit = TRUE ;
	DWORD flags = CREATE_NO_WINDOW ;
	LPVOID env = NULL ;
	LPCTSTR cwd = NULL ;
	static STARTUPINFO zero_start ;
	STARTUPINFO start(zero_start) ;
	start.cb = sizeof(start) ;
	start.dwFlags = STARTF_USESTDHANDLES ;
	start.hStdInput = HNULL ;
	start.hStdOutput = pipe.h() ;
	start.hStdError = HNULL ;
	PROCESS_INFORMATION info ;
	char * command_line_p = const_cast<char*>(command_line.c_str()) ;
	BOOL rc = ::CreateProcess( exe.str().c_str() , command_line_p ,
		process_attributes , thread_attributes , inherit ,
		flags , env , cwd , &start , &info ) ;

	bool ok = !!rc ;
	DWORD exit_code = error_return ;
	if( !ok )
	{
		DWORD e = ::GetLastError() ;
		G_ERROR( "G::Process::spawn: create-process error " << e << ": " << command_line ) ;
	}
	else
	{
		DWORD timeout_ms = 30000UL ;
		if( WAIT_TIMEOUT == ::WaitForSingleObject( info.hProcess , timeout_ms ) )
		{
			G_ERROR( "G::Process::spawn: child process has not terminated: still waiting" ) ;
			::WaitForSingleObject( info.hProcess , INFINITE ) ;
		}
		::GetExitCodeProcess( info.hProcess , &exit_code ) ;
		G_LOG( "G::Process::spawn: exit " << exit_code << " from \"" << exe << "\": " << exit_code ) ;

		if( pipe_result_p != NULL )
			*pipe_result_p = pipe.read(false) ;
	}

	return exit_code ;
}

G::Identity G::Process::beOrdinary( Identity identity , bool )
{
	// not implemented
	return identity ;
}

void G::Process::beSpecial( Identity , bool )
{
	// not implemented
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
