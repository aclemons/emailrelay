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
// gnewprocess_unix.cpp
//

#include "gdef.h"
#include "gnewprocess.h"
#include "gprocess.h"
#include "genvironment.h"
#include "gfile.h"
#include "gidentity.h"
#include "gassert.h"
#include "gstr.h"
#include "glog.h"
#include <cerrno>
#include <array>
#include <algorithm> // std::swap()
#include <utility> // std::swap()
#include <iostream>
#include <csignal> // ::kill()
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h> // open()
#include <unistd.h> // setuid() etc

/// \class G::Pipe
/// A private implementation class used by G::NewProcess that wraps
/// a unix pipe.
///
class G::Pipe
{
public:
	Pipe() ;
	~Pipe() ;
	void inChild() ; // writer
	void inParent() ; // reader
	int fd() const ;
	void dupTo( int fd_std ) ; // onto stdout/stderr
	void write( const std::string & ) ;

public:
	Pipe( const Pipe & ) = delete ;
	Pipe( Pipe && ) = delete ;
	void operator=( const Pipe & ) = delete ;
	void operator=( Pipe && ) = delete ;

private:
	std::array<int,2U> m_fds ;
	int m_fd{-1} ;
} ;

/// \class G::NewProcessImp
/// A pimple-pattern implementation class used by G::NewProcess.
///
class G::NewProcessImp
{
public:
	using Fd = NewProcess::Fd ;
	NewProcessImp( const Path & exe , const StringArray & args , const Environment & env ,
		Fd fd_stdin , Fd fd_stdout , Fd fd_stderr , const G::Path & cd ,
		bool strict_path , Identity run_as_id , bool strict_id ,
		int exec_error_exit , const std::string & exec_error_format ,
		std::string (*exec_error_format_fn)(std::string,int) ) ;
	int id() const noexcept ;
	static std::pair<bool,pid_t> fork() ;
	NewProcessWaitFuture & wait() ;
	int run( const Path & , const StringArray & , const Environment & , bool strict_path ) ;
	void kill() noexcept ;
	static void printError( int , const std::string & s ) ;
	std::string execErrorFormat( const std::string & format , int errno_ ) ;
	static bool duplicate( Fd , int ) ;

public:
	~NewProcessImp() = default ;
	NewProcessImp( const NewProcessImp & ) = delete ;
	NewProcessImp( NewProcessImp && ) = delete ;
	void operator=( const NewProcessImp & ) = delete ;
	void operator=( NewProcessImp && ) = delete ;

private:
	Pipe m_pipe ;
	NewProcessWaitFuture m_wait_future ;
	pid_t m_child_pid{-1} ;
	bool m_killed{false} ;
} ;

// ==

G::NewProcess::NewProcess( const Path & exe , const StringArray & args , const Environment & env ,
	Fd fd_stdin , Fd fd_stdout , Fd fd_stderr , const G::Path & cd ,
	bool strict_path , Identity run_as_id , bool strict_id ,
	int exec_error_exit , const std::string & exec_error_format ,
	std::string (*exec_error_format_fn)(std::string,int) ) :
		m_imp(new NewProcessImp(exe,args,env,fd_stdin,fd_stdout,fd_stderr,cd,strict_path,
			run_as_id,strict_id,exec_error_exit,exec_error_format,exec_error_format_fn) )
{
}

G::NewProcess::~NewProcess()
= default;

G::NewProcessWaitFuture & G::NewProcess::wait()
{
	return m_imp->wait() ;
}

std::pair<bool,pid_t> G::NewProcess::fork()
{
	return NewProcessImp::fork() ;
}

int G::NewProcess::id() const noexcept
{
	return m_imp->id() ;
}

void G::NewProcess::kill( bool yield ) noexcept
{
	m_imp->kill() ;
	if( yield )
	{
		G::threading::yield() ;
		::close( ::open( "/dev/null" , O_RDONLY ) ) ; // hmm
		G::threading::yield() ;
	}
}

// ==

G::NewProcessImp::NewProcessImp( const Path & exe , const StringArray & args , const Environment & env ,
	Fd fd_stdin , Fd fd_stdout , Fd fd_stderr , const G::Path & cd ,
	bool strict_path , Identity run_as_id , bool strict_id ,
	int exec_error_exit , const std::string & exec_error_format ,
	std::string (*exec_error_format_fn)(std::string,int) )
{
	// sanity checks
	if( 1 != (fd_stdout==Fd::pipe()?1:0) + (fd_stderr==Fd::pipe()?1:0) || fd_stdin==Fd::pipe() )
		throw NewProcess::InvalidParameter() ;
	if( exe == G::Path() )
		throw NewProcess::InvalidParameter() ;

	// safety checks
	if( strict_path && exe.isRelative() )
		throw NewProcess::InvalidPath( exe.str() ) ;
	if( strict_id && run_as_id != Identity::invalid() &&
		( Identity::effective().isRoot() || run_as_id.isRoot() ) )
			throw NewProcess::Insecure() ;

	// fork
	std::pair<bool,pid_t> f = fork() ;
	bool in_child = f.first ;
	m_child_pid = f.second ;

	if( in_child )
	{
		try
		{
			// change directory
			if( cd != G::Path() )
				Process::cd( cd ) ;

			// set real id
			if( run_as_id != Identity::invalid() )
				Process::beOrdinaryForExec( run_as_id ) ;

			// set up standard streams
			m_pipe.inChild() ;
			if( fd_stdout == Fd::pipe() )
			{
				m_pipe.dupTo( STDOUT_FILENO ) ;
				duplicate( fd_stderr , STDERR_FILENO ) ;
			}
			else
			{
				duplicate( fd_stdout , STDOUT_FILENO ) ;
				m_pipe.dupTo( STDERR_FILENO ) ;
			}
			duplicate( fd_stdin , STDIN_FILENO ) ;
			Process::closeOtherFiles() ;

			// restore SIGPIPE handling so that writing to
			// the closed pipe should terminate the child
			::signal( SIGPIPE , SIG_DFL ) ;

			// start a new process group
			::setpgrp() ; // feature-tested -- see gdef.h

			// exec -- doesnt normally return from run()
			int e = run( exe , args , env , strict_path ) ;

			// execve() failed -- write an error message to the pipe
			int fd_pipe = fd_stdout == Fd::pipe() ? STDOUT_FILENO : STDERR_FILENO ;
			if( exec_error_format_fn != nullptr )
				printError( fd_pipe , (*exec_error_format_fn)(exec_error_format,e) ) ;
			else if( !exec_error_format.empty() )
				printError( fd_pipe , execErrorFormat(exec_error_format,e) ) ;
		}
		catch(...)
		{
		}
		std::_Exit( exec_error_exit ) ;
	}
	else
	{
		m_pipe.inParent() ;
		m_wait_future.assign( m_child_pid , m_pipe.fd() ) ;
	}
}

std::pair<bool,pid_t> G::NewProcessImp::fork()
{
	std::cout << std::flush ;
	std::cerr << std::flush ;
	pid_t rc = ::fork() ;
	const bool ok = rc != -1 ;
	if( !ok ) throw NewProcess::CannotFork() ;
	bool in_child = rc == 0 ;
	auto child_pid = static_cast<pid_t>(rc) ;
	return std::make_pair( in_child , child_pid ) ;
}

void G::NewProcessImp::printError( int stdxxx , const std::string & s )
{
	// write an exec-failure message back down the pipe
	if( stdxxx <= 0 ) return ;
	ssize_t rc = ::write( stdxxx , s.c_str() , s.length() ) ;
	G_IGNORE_VARIABLE( ssize_t , rc ) ;
}

int G::NewProcessImp::run( const G::Path & exe , const StringArray & args , const Environment & env , bool strict_path )
{
	char ** argv = new char* [ args.size() + 2U ] ;
	std::string str_exe = exe.str() ;
	argv[0U] = const_cast<char*>( str_exe.c_str() ) ;
	unsigned int argc = 1U ;
	for( auto arg_p = args.begin() ; arg_p != args.end() ; ++arg_p , argc++ )
		argv[argc] = const_cast<char*>(arg_p->c_str()) ;
	argv[argc] = nullptr ;

	const std::string exe_str = exe.str() ;
	const char * exe_p = exe_str.c_str() ;

	int e = 0 ;
	if( env.empty() )
	{
		if( strict_path )
		{
			::execv( exe_p , argv ) ;
			e = Process::errno_() ;
		}
		else
		{
			::execvp( exe_p , argv ) ;
			e = Process::errno_() ;
		}
	}
	else
	{
		if( strict_path )
		{
			::execve( exe_p , argv , env.v() ) ;
			e = Process::errno_() ;
		}
		else
		{
			::execvpe( exe_p , argv , env.v() ) ;
			e = Process::errno_() ;
		}
	}

	delete [] argv ;
	G_DEBUG( "G::NewProcess::run: execve() returned: errno=" << e << ": " << exe ) ;
	return e ;
}

int G::NewProcessImp::id() const noexcept
{
	return static_cast<int>(m_child_pid) ;
}

G::NewProcessWaitFuture & G::NewProcessImp::wait()
{
	return m_wait_future ;
}

void G::NewProcessImp::kill() noexcept
{
	if( !m_killed && m_child_pid != -1 )
	{
		// kill the group so the pipe is closed in all processes and the read returns zero
		::kill( -m_child_pid , SIGTERM ) ;
		m_killed = true ;
	}
}

std::string G::NewProcessImp::execErrorFormat( const std::string & format , int errno_ )
{
	std::string result = format ;
	Str::replaceAll( result , "__errno__" , Str::fromInt(errno_) ) ;
	Str::replaceAll( result , "__strerror__" , Process::strerror(errno_) ) ;
	return result ;
}

bool G::NewProcessImp::duplicate( Fd fd , int fd_std )
{
	G_ASSERT( !(fd==Fd::pipe()) ) ;
	if( fd == Fd::devnull() )
	{
		int fd_null = ::open( G::Path::nullDevice().str().c_str() , fd_std == STDIN_FILENO ? O_RDONLY : O_WRONLY ) ;
		if( fd_null < 0 ) throw NewProcess::Error( "failed to open /dev/null" ) ;
		::dup2( fd_null , fd_std ) ;
		return true ;
	}
	else if( fd.m_fd != fd_std )
	{
		if( ::dup2(fd.m_fd,fd_std) != fd_std )
			throw NewProcess::Error( "dup failed" ) ;
		::close( fd.m_fd ) ;
		return true ;
	}
	else
	{
		return false ;
	}
}

// ==

G::Pipe::Pipe() :
	m_fds{{-1,-1}}
{
	if( ::socketpair( AF_UNIX , SOCK_STREAM , 0 , &m_fds[0] ) < 0 ) // must be a stream to dup() onto stdout
		throw NewProcess::PipeError() ;
	G_DEBUG( "G::Pipe::ctor: " << m_fds[0] << " " << m_fds[1] ) ;
}

G::Pipe::~Pipe()
{
	if( m_fds[0] >= 0 ) ::close( m_fds[0] ) ;
	if( m_fds[1] >= 0 ) ::close( m_fds[1] ) ;
}

void G::Pipe::inChild()
{
	::close( m_fds[0] ) ; // close read end
	m_fds[0] = -1 ;
	m_fd = m_fds[1] ; // writer
}

void G::Pipe::inParent()
{
	::close( m_fds[1] ) ; // close write end
	m_fds[1] = -1 ;
	m_fd = m_fds[0] ; // reader
}

int G::Pipe::fd() const
{
	return m_fd ;
}

void G::Pipe::dupTo( int fd_std )
{
	if( NewProcessImp::duplicate( NewProcess::Fd::fd(m_fd) , fd_std ) )
	{
		m_fd = -1 ;
		m_fds[1] = -1 ;
	}
}

// ==

G::NewProcessWaitFuture::NewProcessWaitFuture()
= default;

G::NewProcessWaitFuture::NewProcessWaitFuture( pid_t pid , int fd ) :
	m_buffer(1024U) ,
	m_pid(pid) ,
	m_fd(fd)
{
}

void G::NewProcessWaitFuture::assign( pid_t pid , int fd )
{
	m_buffer.resize( 1024U ) ;
	m_hprocess = 0 ;
	m_hpipe = 0 ;
	m_pid = pid ;
	m_fd = fd ;
	m_rc = 0 ;
	m_status = 0 ;
	m_error = 0 ;
	m_read_error = 0 ;
}

G::NewProcessWaitFuture & G::NewProcessWaitFuture::run()
{
	// (worker thread - keep it simple - read then wait)
	{
		std::array<char,64U> more ; // NOLINT cppcoreguidelines-pro-type-member-init
		char * p = &m_buffer[0] ;
		std::size_t space = m_buffer.size() ;
		std::size_t size = 0U ;
		while( m_fd >= 0 )
		{
			ssize_t n = ::read( m_fd , p?p:&more[0] , p?space:more.size() ) ;
			m_read_error = errno ;
			if( n < 0 && m_error == EINTR )
			{
				; // keep reading
			}
			else if( n < 0 )
			{
				m_buffer.clear() ;
				break ;
			}
			else if( n == 0 )
			{
				m_buffer.resize( size ) ;
				m_read_error = 0 ;
				break ;
			}
			else if( p )
			{
				p += n ;
				size += n ;
				space -= n ;
				if( space == 0U )
					p = nullptr ;
			}
		}
	}
	while( m_pid != 0 )
	{
		errno = 0 ;
		m_rc = ::waitpid( m_pid , &m_status , 0 ) ;
		m_error = errno ;
		if( m_rc == -1 && m_error == EINTR )
			; // signal in parent -- keep waiting
		else
			break ;
	}
	return *this ;
}

int G::NewProcessWaitFuture::get()
{
	int result = 0 ;
	if( m_pid != 0 )
	{
		if( m_error || m_read_error )
		{
			std::ostringstream ss ;
			ss << "errno=" << (m_read_error?m_read_error:m_error) ;
			throw NewProcess::WaitError( ss.str() ) ;
		}
		if( ! WIFEXITED(m_status) )
		{
			// uncaught signal or stopped
			std::ostringstream ss ;
			ss << "status=" << m_status ;
			throw NewProcess::ChildError( ss.str() ) ;
		}
		result = WEXITSTATUS(m_status) ;
	}
	return result ;
}

std::string G::NewProcessWaitFuture::output()
{
	if( m_fd < 0 || m_read_error != 0 )
		return std::string() ;
	else
		return std::string( &m_buffer[0] , m_buffer.size() ) ;
}

