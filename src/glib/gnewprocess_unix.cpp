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
/// \file gnewprocess_unix.cpp
///

#include "gdef.h"
#include "gnewprocess.h"
#include "gprocess.h"
#include "genvironment.h"
#include "gfile.h"
#include "gidentity.h"
#include "gassert.h"
#include "gtest.h"
#include "gstr.h"
#include "glog.h"
#include <cerrno>
#include <array>
#include <algorithm> // std::swap()
#include <utility> // std::swap()
#include <tuple> // std::tie()
#include <iostream>
#include <csignal> // ::kill()
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h> // open()
#include <unistd.h> // setuid() etc

#ifndef WIFCONTINUED
#define WIFCONTINUED(wstatus) 0
#endif

namespace G
{
	namespace NewProcessUnixImp
	{
		class Pipe ;
	}
}

class G::NewProcessUnixImp::Pipe
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
	Pipe & operator=( const Pipe & ) = delete ;
	Pipe & operator=( Pipe && ) = delete ;

private:
	std::array<int,2U> m_fds{{-1,-1}} ;
	int m_fd{-1} ;
} ;

class G::NewProcessImp
{
public:
	using Fd = NewProcess::Fd ;
	using Pipe = NewProcessUnixImp::Pipe ;
	NewProcessImp( const Path & , const StringArray & , const NewProcess::Config & ) ;
	int id() const noexcept ;
	static std::pair<bool,pid_t> fork() ;
	NewProcessWaitable & waitable() noexcept ;
	int run( const Path & , const StringArray & , const Environment & , bool strict_exe ) ;
	void kill() noexcept ;
	static void printError( int , const std::string & s ) ;
	std::string execErrorFormat( const std::string & format , int errno_ ) ;
	static bool duplicate( Fd , int ) ;

public:
	~NewProcessImp() = default ;
	NewProcessImp( const NewProcessImp & ) = delete ;
	NewProcessImp( NewProcessImp && ) = delete ;
	NewProcessImp & operator=( const NewProcessImp & ) = delete ;
	NewProcessImp & operator=( NewProcessImp && ) = delete ;

private:
	Pipe m_pipe ;
	NewProcessWaitable m_waitable ;
	pid_t m_child_pid {-1} ;
	bool m_killed {false} ;
} ;

// ==

G::NewProcess::NewProcess( const Path & exe , const StringArray & args , const Config & config ) :
	m_imp(std::make_unique<NewProcessImp>(exe,args,config))
{
}

G::NewProcess::~NewProcess()
= default;

G::NewProcessWaitable & G::NewProcess::waitable() noexcept
{
	return m_imp->waitable() ;
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
		::close( ::open( "/dev/null" , O_RDONLY ) ) ; // hmm // NOLINT
		G::threading::yield() ;
	}
}

// ==

G::NewProcessImp::NewProcessImp( const Path & exe , const StringArray & args , const NewProcess::Config & config )
{
	// sanity checks
	if( 1 != (config.stdout==Fd::pipe()?1:0) + (config.stderr==Fd::pipe()?1:0) || config.stdin==Fd::pipe() )
		throw NewProcess::InvalidParameter() ;
	if( exe.empty() )
		throw NewProcess::InvalidParameter() ;

	// safety checks
	if( config.strict_exe && exe.isRelative() )
		throw NewProcess::InvalidPath( exe.str() ) ;
	if( config.strict_id && config.run_as != Identity::invalid() &&
		( Identity::effective().isRoot() || config.run_as.isRoot() ) )
			throw NewProcess::Insecure() ;

	// fork
	bool in_child {} ;
	std::tie(in_child,m_child_pid) = fork() ;
	if( in_child )
	{
		try
		{
			// change directory
			if( !config.cd.empty() )
				Process::cd( config.cd ) ; // throws on error

			// set real id
			if( config.run_as != Identity::invalid() )
				Process::beOrdinaryForExec( config.run_as ) ;

			// set up standard streams
			m_pipe.inChild() ;
			if( config.stdout == Fd::pipe() )
			{
				m_pipe.dupTo( STDOUT_FILENO ) ;
				duplicate( config.stderr , STDERR_FILENO ) ;
			}
			else
			{
				duplicate( config.stdout , STDOUT_FILENO ) ;
				m_pipe.dupTo( STDERR_FILENO ) ;
			}
			duplicate( config.stdin , STDIN_FILENO ) ;
			Process::closeOtherFiles() ;

			// restore SIGPIPE handling so that writing to
			// the closed pipe should terminate the child
			::signal( SIGPIPE , SIG_DFL ) ;

			// start a new process group
			::setpgrp() ; // feature-tested -- see gdef.h

			// set the execvp() path
			if( !config.strict_exe && !config.exec_search_path.empty() )
				G::Environment::put( "PATH" , config.exec_search_path ) ;

			// exec -- doesnt normally return from run()
			int e = run( exe , args , config.env , config.strict_exe ) ;

			// execve() failed -- write an error message to the pipe
			int fd_pipe = config.stdout == Fd::pipe() ? STDOUT_FILENO : STDERR_FILENO ;
			if( config.exec_error_format_fn != nullptr )
				printError( fd_pipe , (config.exec_error_format_fn)(config.exec_error_format,e) ) ;
			else if( !config.exec_error_format.empty() )
				printError( fd_pipe , execErrorFormat(config.exec_error_format,e) ) ;
		}
		catch(...)
		{
		}
		std::_Exit( config.exec_error_exit ) ;
	}
	else
	{
		m_pipe.inParent() ;
		m_waitable.assign( m_child_pid , m_pipe.fd() ) ;
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
	return { in_child , child_pid } ;
}

void G::NewProcessImp::printError( int stdxxx , const std::string & s )
{
	// write an exec-failure message back down the pipe
	if( stdxxx <= 0 ) return ;
	GDEF_IGNORE_RETURN ::write( stdxxx , s.c_str() , s.length() ) ;
}

int G::NewProcessImp::run( const G::Path & exe , const StringArray & args ,
	const Environment & env , bool strict_exe )
{
	char ** argv = new char* [ args.size() + 2U ] ;
	argv[0U] = const_cast<char*>( exe.cstr() ) ;
	unsigned int argc = 1U ;
	for( auto arg_p = args.begin() ; arg_p != args.end() ; ++arg_p , argc++ )
		argv[argc] = const_cast<char*>(arg_p->c_str()) ;
	argv[argc] = nullptr ;

	int e = 0 ;
	if( env.empty() )
	{
		if( strict_exe )
		{
			::execv( exe.cstr() , argv ) ;
			e = Process::errno_() ;
		}
		else
		{
			::execvp( exe.cstr() , argv ) ;
			e = Process::errno_() ;
		}
	}
	else
	{
		if( strict_exe )
		{
			::execve( exe.cstr() , argv , env.v() ) ;
			e = Process::errno_() ;
		}
		else
		{
			::execvpe( exe.cstr() , argv , env.v() ) ;
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

G::NewProcessWaitable & G::NewProcessImp::waitable() noexcept
{
	return m_waitable ;
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
	Str::replaceAll( result , "__""errno""__" , Str::fromInt(errno_) ) ;
	Str::replaceAll( result , "__""strerror""__" , Process::strerror(errno_) ) ;
	return result ;
}

bool G::NewProcessImp::duplicate( Fd fd , int fd_std )
{
	G_ASSERT( !(fd==Fd::pipe()) ) ;
	if( fd == Fd::devnull() )
	{
		int fd_null = ::open( G::Path::nullDevice().cstr() , fd_std == STDIN_FILENO ? O_RDONLY : O_WRONLY ) ; // NOLINT
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

G::NewProcessUnixImp::Pipe::Pipe()
{
	if( ::socketpair( AF_UNIX , SOCK_STREAM , 0 , &m_fds[0] ) < 0 ) // must be a stream to dup() onto stdout
		throw NewProcess::PipeError() ;
	G_DEBUG( "G::Pipe::ctor: " << m_fds[0] << " " << m_fds[1] ) ;
}

G::NewProcessUnixImp::Pipe::~Pipe()
{
	if( m_fds[0] >= 0 ) ::close( m_fds[0] ) ;
	if( m_fds[1] >= 0 ) ::close( m_fds[1] ) ;
}

void G::NewProcessUnixImp::Pipe::inChild()
{
	::close( m_fds[0] ) ; // close read end
	m_fds[0] = -1 ;
	m_fd = m_fds[1] ; // writer
}

void G::NewProcessUnixImp::Pipe::inParent()
{
	::close( m_fds[1] ) ; // close write end
	m_fds[1] = -1 ;
	m_fd = m_fds[0] ; // reader
}

int G::NewProcessUnixImp::Pipe::fd() const
{
	return m_fd ;
}

void G::NewProcessUnixImp::Pipe::dupTo( int fd_std )
{
	if( NewProcessImp::duplicate( NewProcess::Fd::fd(m_fd) , fd_std ) )
	{
		m_fd = -1 ;
		m_fds[1] = -1 ;
	}
}

// ==

G::NewProcessWaitable::NewProcessWaitable() :
	m_test_mode(G::Test::enabled("waitpid-slow"))
{
}

#ifndef G_LIB_SMALL
G::NewProcessWaitable::NewProcessWaitable( pid_t pid , int fd ) :
	m_buffer(1024U) ,
	m_pid(pid) ,
	m_fd(fd) ,
	m_test_mode(G::Test::enabled("waitpid-slow"))
{
}
#endif

void G::NewProcessWaitable::assign( pid_t pid , int fd )
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

#ifndef G_LIB_SMALL
void G::NewProcessWaitable::waitp( std::promise<std::pair<int,std::string>> p ) noexcept
{
	try
	{
		wait() ;
		int rc = get() ;
		p.set_value( std::make_pair(rc,output()) ) ;
	}
	catch(...)
	{
		try { p.set_exception( std::current_exception() ) ; } catch(...) {}
	}
}
#endif

G::NewProcessWaitable & G::NewProcessWaitable::wait()
{
	// (worker thread - keep it simple - never throws - does read then waitpid)
	{
		std::array<char,64U> discard {} ;
		char * p = &m_buffer[0] ;
		std::size_t space = m_buffer.size() ;
		std::size_t size = 0U ;
		while( m_fd >= 0 )
		{
			ssize_t n = ::read( m_fd , p?p:&discard[0] , p?space:discard.size() ) ;
			m_read_error = errno ;
			if( n < 0 && m_read_error == EINTR )
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
				m_buffer.resize( std::min(size,m_buffer.size()) ) ; // shrink, so no-throw
				m_read_error = 0 ;
				break ;
			}
			else if( p )
			{
				std::size_t nn = static_cast<std::size_t>(n) ; // n>0
				nn = std::min( nn , space ) ; // just in case
				p += nn ;
				size += nn ;
				space -= nn ;
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
		if( m_rc >= 0 && ( WIFSTOPPED(m_status) || WIFCONTINUED(m_status) ) ) // NOLINT
			; // keep waiting
		else if( m_rc == -1 && m_error == EINTR )
			; // keep waiting
		else
			break ;
	}
	if( m_test_mode )
		sleep( 10 ) ;
	return *this ;
}

int G::NewProcessWaitable::get() const
{
	int result = 0 ;
	if( m_pid != 0 )
	{
		if( m_error == ECHILD )
		{
			// only here if SIGCHLD is explicitly ignored, but in that case
			// we get no zombie process and cannot recover the exit code
			result = 126 ;
		}
		else if( m_error || m_read_error )
		{
			std::ostringstream ss ;
			ss << "errno=" << (m_read_error?m_read_error:m_error) ;
			throw NewProcess::WaitError( ss.str() ) ;
		}
		else if( !WIFEXITED(m_status) ) // NOLINT
		{
			// uncaught signal
			std::ostringstream ss ;
			ss << "pid=" << m_pid ;
			if( WIFSIGNALED(m_status) ) // NOLINT
				ss << " signal=" << WTERMSIG(m_status) ; // NOLINT
			throw NewProcess::ChildError( ss.str() ) ;
		}
		else
		{
			result = WEXITSTATUS( m_status ) ; // NOLINT
		}
	}
	return result ;
}

int G::NewProcessWaitable::get( std::nothrow_t , int ec ) const noexcept
{
	int result = 0 ;
	if( m_pid != 0 )
	{
		if( m_error || m_read_error )
			result = ec ;
		else if( !WIFEXITED(m_status) ) // NOLINT
			result = 128 + WTERMSIG(m_status) ; // NOLINT
		else
			result = WEXITSTATUS( m_status ) ; // NOLINT
	}
	return result ;
}

std::string G::NewProcessWaitable::output() const
{
	if( m_fd < 0 || m_read_error != 0 )
		return {} ;
	else
		return { &m_buffer[0] , m_buffer.size() } ;
}

