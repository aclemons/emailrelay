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
// gnewprocess_unix_exec_enabled.cpp
//

#include "gdef.h"
#include "glimits.h"
#include "gnewprocess.h"
#include "gprocess.h"
#include "gidentity.h"
#include "gassert.h"
#include "gfs.h"
#include "glog.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h> // open()
#include <unistd.h> // setuid() etc
#include <algorithm> // std::swap()
#include <utility> // std::swap()
#include <iostream>

namespace
{
	void noCloseOnExec( int fd )
	{
		::fcntl( fd , F_SETFD , 0 ) ;
	}
}

namespace G
{
	class Pipe ;
}

/// \class G::Pipe
/// A private implementation class used by G::NewProcess.
/// 
class G::Pipe 
{
public:
	explicit Pipe( bool active ) ;
	~Pipe() ;
	void inChild() ; // writer
	void inParent() ; // reader
	int fd() const ;
	void dup() ; // onto stdout
	std::string read() ; // size-limited
	void write( const std::string & ) ;
private:
	G_EXCEPTION( Error , "pipe error" ) ;
	int m_fds[2] ;
	int m_fd ;
} ;

/// \class G::NewProcess::ChildProcessImp
/// A private implementation class used by G::NewProcess.
/// 
class G::NewProcess::ChildProcessImp 
{
public:
	ChildProcessImp() ;
	unsigned long m_ref_count ;
	Process::Id m_id ;
	Pipe m_pipe ;
private:
	void operator=( const ChildProcessImp & ) ;
	ChildProcessImp( const ChildProcessImp & ) ;
} ;

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
	return G::NewProcess::wait( m_imp->m_id , 127 ) ;
}

std::string G::NewProcess::ChildProcess::read()
{
	return m_imp->m_pipe.read() ;
}

// ===

G::NewProcess::ChildProcessImp::ChildProcessImp() :
	m_ref_count(0UL) ,
	m_pipe(true)
{
}

// ===

G::NewProcess::Who G::NewProcess::fork()
{
	Process::Id id ;
	return fork( id ) ;
}

G::NewProcess::Who G::NewProcess::fork( Process::Id & child_pid )
{
	std::cout << std::flush ;
	std::cerr << std::flush ;
	pid_t rc = ::fork() ;
	const bool ok = rc != -1 ;
	if( ok )
	{
		if( rc != 0 )
			child_pid.m_pid = rc ;
	}
	else
	{
		throw CannotFork() ;
	}
	return rc == 0 ? Child : Parent ;
}

int G::NewProcess::wait( const Process::Id & child_pid )
{
	int status ;
	for(;;)
	{
		G_DEBUG( "G::NewProcess::wait: waiting" ) ;
		int rc = ::waitpid( child_pid.m_pid , &status , 0 ) ;
		if( rc == -1 && Process::errno_() == EINTR )
		{
			; // signal in parent -- keep waiting
		}
		else if( rc == -1 )
		{
			int error = Process::errno_() ;
			std::ostringstream ss ;
			ss << "errno=" << error ;
			throw WaitError( ss.str() ) ;
		}
		else
		{
			break ;
		}
	}
	G_DEBUG( "G::NewProcess::wait: done" ) ;

	if( ! WIFEXITED(status) )
	{
		// uncaught signal or stopped
		std::ostringstream ss ;
		ss << "status=" << status ;
		throw ChildError( ss.str() ) ;
	}

	const int exit_status = WEXITSTATUS(status) ;
	return exit_status ;
}

int G::NewProcess::wait( const Process::Id & child_pid , int error_return )
{
	try
	{
		return wait( child_pid ) ;
	}
	catch(...)
	{
	}
	return error_return ;
}

G::NewProcess::ChildProcess G::NewProcess::spawn( const Path & exe , const Strings & args )
{
	ChildProcess child( new ChildProcessImp ) ;
	if( fork(child.m_imp->m_id) == Child )
	{
		try
		{
			child.m_imp->m_pipe.inChild() ;
			Process::closeFiles( child.m_imp->m_pipe.fd() ) ;
			child.m_imp->m_pipe.dup() ;
			execCore( exe , args ) ;
		}
		catch(...)
		{
		}
		::_exit( 127 ) ;
		return ChildProcess(0) ; // pacify the compiler
	}
	else
	{
		child.m_imp->m_pipe.inParent() ;
		return child ;
	}
}

int G::NewProcess::spawn( Identity nobody , const Path & exe , const Strings & args , 
	std::string * pipe_result_p , int error_return , std::string (*fn)(int) )
{
	if( exe.isRelative() )
		throw InvalidPath( exe.str() ) ;

	if( Identity::effective().isRoot() || nobody.isRoot() )
		throw Insecure() ;

	Pipe pipe( pipe_result_p != NULL ) ;
	Process::Id child_pid ;
	if( fork(child_pid) == Child )
	{
		try
		{
			Process::beNobody( nobody ) ;
			G_ASSERT( ::getuid() != 0U && ::geteuid() != 0U ) ;
			pipe.inChild() ;
			Process::closeFiles( pipe.fd() ) ;
			pipe.dup() ; // dup() onto stdout
			int error = execCore( exe , args ) ;
			if( fn != 0 )
			{
				std::string s = (*fn)(error) ;
				ssize_t rc = ::write( STDOUT_FILENO , s.c_str() , s.length() ) ;
				G_IGNORE_VARIABLE(rc) ;
			}
		}
		catch(...)
		{
		}
		::_exit( error_return ) ;
		return error_return ; // pacify the compiler
	}
	else
	{
		pipe.inParent() ;
		int exit_status = wait( child_pid , error_return ) ;
		if( pipe_result_p != NULL ) *pipe_result_p = pipe.read() ;
		return exit_status ;
	}
}

int G::NewProcess::execCore( const G::Path & exe , const Strings & args )
{
	char * env[3U] ;
	std::string path( "PATH=/usr/bin:/bin" ) ; // no "."
	std::string ifs( "IFS= \t\n" ) ;
	env[0U] = const_cast<char*>( path.c_str() ) ;
	env[1U] = const_cast<char*>( ifs.c_str() ) ;
	env[2U] = NULL ;

	char ** argv = new char* [ args.size() + 2U ] ;
	std::string str_exe = exe.str() ;
	argv[0U] = const_cast<char*>( str_exe.c_str() ) ;
	unsigned int argc = 1U ;
	for( Strings::const_iterator arg_p = args.begin() ; arg_p != args.end() ; ++arg_p , argc++ )
		argv[argc] = const_cast<char*>(arg_p->c_str()) ;
	argv[argc] = NULL ;

	::execve( exe.str().c_str() , argv , env ) ;
	const int error = Process::errno_() ;
	delete [] argv ;

	G_DEBUG( "G::NewProcess::exec: execve() returned: errno=" << error << ": " << exe ) ;
	return error ;
}

// ===

G::Pipe::Pipe( bool active ) : 
	m_fd(-1) 
{ 
	m_fds[0] = m_fds[1] = -1 ;
	if( active && ::pipe( m_fds ) < 0 ) 
		throw Error() ; 
	G_DEBUG( "G::Pipe::ctor: " << m_fds[0] << " " << m_fds[1] ) ;
}

G::Pipe::~Pipe()
{
	if( m_fds[0] >= 0 ) ::close( m_fds[0] ) ;
	if( m_fds[1] >= 0 ) ::close( m_fds[1] ) ;
}

void G::Pipe::inChild() 
{ 
	::close( m_fds[0] ) ; 
	m_fds[0] = -1 ;
	m_fd = m_fds[1] ; // writer
}

void G::Pipe::inParent() 
{ 
	::close( m_fds[1] ) ; 
	m_fds[1] = -1 ;
	m_fd = m_fds[0] ; // reader
}

int G::Pipe::fd() const
{ 
	return m_fd ; 
}

void G::Pipe::dup()
{ 
	if( m_fd != -1 && m_fd != STDOUT_FILENO )
	{
		if( ::dup2(m_fd,STDOUT_FILENO) != STDOUT_FILENO )
			throw Error() ;
		::close( m_fd ) ;
		m_fd = -1 ;
		m_fds[1] = -1 ;
		noCloseOnExec( STDOUT_FILENO ) ;
	}
}

std::string G::Pipe::read()
{
	char buffer[limits::pipe_buffer] ;
	ssize_t rc = m_fd == -1 ? 0 : ::read( m_fd , buffer , sizeof(buffer) ) ;
	if( rc < 0 ) throw Error("read") ;
    const size_t buffer_size = static_cast<size_t>(rc) ;
    return std::string(buffer,buffer_size) ;
}

