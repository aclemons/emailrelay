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
/// \file gnewprocess.h
///

#ifndef G_NEW_PROCESS_H
#define G_NEW_PROCESS_H

#include "gdef.h"
#include "gexception.h"
#include "genvironment.h"
#include "gexecutablecommand.h"
#include "gidentity.h"
#include "gprocess.h"
#include "gpath.h"
#include "gstringarray.h"
#include <memory>
#include <new>
#include <string>
#include <future>

namespace G
{
	class NewProcess ;
	class NewProcessImp ;
	class NewProcessWaitable ;
}

//| \class G::NewProcess
/// A class for creating new processes.
///
/// Eg:
/// \code
/// {
///   NewProcess task( "exe" , args , {} ) ;
///   auto& waitable = task.waitable() ;
///   waitable.wait() ;
///   int rc = waitable.get() ;
///   std::string s = waitable.output() ;
///   ...
/// }
/// \endcode
///
/// \see G::Daemon, G::NewProcessWaitable
///
class G::NewProcess
{
public:
	G_EXCEPTION( Error , tx("cannot spawn new process") ) ;
	G_EXCEPTION( CannotFork , tx("cannot fork") ) ;
	G_EXCEPTION( WaitError , tx("failed waiting for child process") ) ;
	G_EXCEPTION( ChildError , tx("child process terminated abnormally") ) ;
	G_EXCEPTION( Insecure , tx("refusing to exec while the user-id is zero") ) ;
	G_EXCEPTION( PipeError , tx("pipe error") ) ;
	G_EXCEPTION( InvalidPath , tx("invalid executable path -- must be absolute") ) ;
	G_EXCEPTION( InvalidParameter , tx("invalid parameter") ) ;
	G_EXCEPTION( CreateProcessError , tx("CreateProcess error") ) ; // windows
	G_EXCEPTION( SystemError , tx("system error") ) ; // windows

	struct Fd /// Wraps up a file descriptor for passing to G::NewProcess.
	{
		bool m_null ;
		bool m_pipe ;
		int m_fd ;
		Fd( bool null_ , bool pipe_ , int fd_ ) : m_null(null_) , m_pipe(pipe_) , m_fd(fd_) {}
		static Fd pipe() { return {false,true,-1} ; }
		static Fd devnull() { return {true,false,-1} ; }
		static Fd fd(int fd_) { return fd_ < 0 ? devnull() : Fd(false,false,fd_) ; }
		bool operator==( const Fd & other ) const { return m_null == other.m_null && m_pipe == other.m_pipe && m_fd == other.m_fd ; }
		bool operator!=( const Fd & other ) const { return !(*this == other) ; }
	} ;

	struct Config /// Configuration structure for G::NewProcess.
	{
		using Fd = NewProcess::Fd ;
		using FormatFn = std::string (*)(std::string,int) ;
		Environment env {Environment::minimal()} ; // execve() envp parameter
		NewProcess::Fd stdin {Fd::devnull()} ;
		NewProcess::Fd stdout {Fd::pipe()} ;
		NewProcess::Fd stderr {Fd::devnull()} ;
		Path cd ; // cd in child process before exec
		bool strict_exe {true} ; // require 'exe' is absolute
		std::string exec_search_path ; // PATH in child process before execvpe()
		Identity run_as {Identity::invalid()} ; // see Process::beOrdinaryForExec()
		bool strict_id {true} ; // dont allow run_as root
		int exec_error_exit {127} ; // exec failure error code
		std::string exec_error_format ; // exec failure error message with substitution of strerror and errno
		FormatFn exec_error_format_fn {nullptr} ; // exec failure error message function passed exec_error_format and errno

		Config & set_env( const Environment & ) ;
		Config & set_stdin( Fd ) ;
		Config & set_stdout( Fd ) ;
		Config & set_stderr( Fd ) ;
		Config & set_cd( const Path & ) ;
		Config & set_strict_exe( bool = true ) ;
		Config & set_exec_search_path( const std::string & ) ;
		Config & set_run_as( Identity ) ;
		Config & set_strict_id( bool = true ) ;
		Config & set_exec_error_exit( int ) ;
		Config & set_exec_error_format( const std::string & ) ;
		Config & set_exec_error_format_fn( FormatFn ) ;
	} ;

	NewProcess( const Path & exe , const G::StringArray & args , const Config & ) ;
		///< Constructor. Spawns the given program to run independently in a
		///< child process.
		///<
		///< The child process's stdin, stdout and stderr are connected
		///< as directed, but exactly one of stdout and stderr must be the
		///< internal pipe since it is used to detect process termination.
		///< To inherit the existing file descriptors use Fd(STDIN_FILENO)
		///< etc. Using Fd::fd(-1) is equivalent to Fd::devnull().
		///<
		///< The child process is given the new environment, unless the
		///< environment given is empty() in which case the environment is
		///< inherited from the calling process (see G::Environment::inherit()).
		///<
		///< If 'strict_exe' then the program must be given as an absolute path.
		///< Otherwise it can be a relative path and the calling process's PATH
		///< variable or the 'exec_search_path' is used to find it.
		///<
		///< If a valid identity is supplied then the child process runs as
		///< that identity. If 'strict_id' is also true then the id is not
		///< allowed to be root. See G::Process::beOrdinaryForExec().
		///<
		///< If the exec() fails then the 'exec_error_exit' argument is used as
		///< the child process exit code.
		///<
		///< The internal pipe can be used for error messages in the situation
		///< where the exec() in the forked child process fails. This requires
		///< that one of the 'exec_error_format' parameters is given; by default
		///< nothing is sent over the pipe when the exec() fails.
		///<
		///< The exec error message is assembled by the given callback function,
		///< with the 'exec_error_format' argument passed as its first parameter.
		///< The second parameter is the exec() errno. The default callback
		///< function does text substitution for "__errno__" and "__strerror__"
		///< substrings that appear within the error format string.

	~NewProcess() ;
		///< Destructor. Kills the spawned process if the Waitable has
		///< not been resolved.

	int id() const noexcept ;
		///< Returns the process id.

	NewProcessWaitable & waitable() noexcept ;
		///< Returns a reference to the Waitable sub-object so that the caller
		///< can wait for the child process to exit.

	void kill( bool yield = false ) noexcept ;
		///< Tries to kill the spawned process and optionally yield
		///< to a thread that might be waiting on it.

	static std::pair<bool,pid_t> fork() ;
		///< A utility function that forks the calling process and returns
		///< twice; once in the parent and once in the child. Not implemented
		///< on windows. Returns an "is-in-child/child-pid" pair.
		/// \see G::Daemon

public:
	NewProcess( const NewProcess & ) = delete ;
	NewProcess( NewProcess && ) = delete ;
	NewProcess & operator=( const NewProcess & ) = delete ;
	NewProcess & operator=( NewProcess && ) = delete ;

private:
	static std::string execErrorFormat( const std::string & , int ) ;

private:
	std::unique_ptr<NewProcessImp> m_imp ;
} ;

//| \class G::NewProcessWaitable
/// Holds the parameters and future results of a waitpid() system call.
///
/// The wait() method can be called from a worker thread and the results
/// collected by the main thread using get() once the worker thread has
/// signalled that it has finished. The signalling mechanism is outside
/// the scope of this class (see std::promise, GNet::FutureEvent).
///
class G::NewProcessWaitable
{
public:
	NewProcessWaitable() ;
		///< Default constructor for an object where wait() does nothing
		///< and get() returns zero.

	explicit NewProcessWaitable( pid_t pid , int fd = -1 ) ;
		///< Constructor taking a posix process-id and optional
		///< readable file descriptor. Only used by the unix
		///< implementation of G::NewProcess.

	NewProcessWaitable( HANDLE hprocess , HANDLE hpipe , int ) ;
		///< Constructor taking process and pipe handles. Only
		///< used by the windows implementation of G::NewProcess.

	void assign( pid_t pid , int fd ) ;
		///< Reinitialises as if constructed with the given proces-id
		///< and file descriptor.

	void assign( HANDLE hprocess , HANDLE hpipe , int ) ;
		///< Reinitialises as if constructed with the given proces
		///< handle and pipe handle.

	NewProcessWaitable & wait() ;
		///< Waits for the process identified by the constructor
		///< parameter to exit. Returns *this. This method can be
		///< called from a separate worker thread.

	void waitp( std::promise<std::pair<int,std::string>> ) noexcept ;
		///< Calls wait() and then sets the given promise with the get() and
		///< output() values or an exception:
		///< \code
		///< std::promise<int,std::string> p ;
		///< std::future<int,std::string> f = p.get_future() ;
		///< std::thread t( std::bind(&NewProcessWaitable::waitp,waitable,_1) , std::move(p) ) ;
		///< f.wait() ;
		///< t.join() ;
		///< int e = f.get().first ;
		///< \endcode

	int get() const ;
		///< Returns the result of the wait() as either the process
		///< exit code or as a thrown exception. Typically called
		///< by the main thread after the wait() worker thread has
		///< signalled its completion. Returns zero if there is no
		///< process to wait for.

	int get( std::nothrow_t , int exit_code_on_error = 127 ) const noexcept ;
		///< Non-throwing overload.

	std::string output() const ;
		///< Returns the first bit of child-process output.
		///< Used after get().

public:
	~NewProcessWaitable() = default ;
	NewProcessWaitable( const NewProcessWaitable & ) = delete ;
	NewProcessWaitable( NewProcessWaitable && ) = delete ;
	NewProcessWaitable & operator=( const NewProcessWaitable & ) = delete ;
	NewProcessWaitable & operator=( NewProcessWaitable && ) = delete ;

private:
	std::vector<char> m_buffer ;
	std::size_t m_data_size {0U} ;
	HANDLE m_hprocess {0} ;
	HANDLE m_hpipe {0} ;
	pid_t m_pid {0} ;
	int m_fd {-1} ;
	int m_rc {0} ;
	int m_status {0} ;
	int m_error {0} ;
	int m_read_error {0} ;
	bool m_test_mode {false} ;
} ;

inline G::NewProcess::Config & G::NewProcess::Config::set_env( const Environment & e ) { env = e ; return *this ; }
inline G::NewProcess::Config & G::NewProcess::Config::set_stdin( Fd fd ) { stdin = fd ; return *this ; }
inline G::NewProcess::Config & G::NewProcess::Config::set_stdout( Fd fd ) { stdout = fd ; return *this ; }
inline G::NewProcess::Config & G::NewProcess::Config::set_stderr( Fd fd ) { stderr = fd ; return *this ; }
inline G::NewProcess::Config & G::NewProcess::Config::set_cd( const Path & p ) { cd = p ; return *this ; }
inline G::NewProcess::Config & G::NewProcess::Config::set_strict_exe( bool b ) { strict_exe = b ; return *this ; }
inline G::NewProcess::Config & G::NewProcess::Config::set_exec_search_path( const std::string & s ) { exec_search_path = s ; return *this ; }
inline G::NewProcess::Config & G::NewProcess::Config::set_run_as( Identity i ) { run_as = i ; return *this ; }
inline G::NewProcess::Config & G::NewProcess::Config::set_strict_id( bool b ) { strict_id = b ; return *this ; }
inline G::NewProcess::Config & G::NewProcess::Config::set_exec_error_exit( int n ) { exec_error_exit = n ; return *this ; }
inline G::NewProcess::Config & G::NewProcess::Config::set_exec_error_format( const std::string & s ) { exec_error_format = s ; return *this ; }
inline G::NewProcess::Config & G::NewProcess::Config::set_exec_error_format_fn( FormatFn f ) { exec_error_format_fn = f ; return *this ; }

#endif
