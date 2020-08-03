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
///
/// \file gnewprocess.h
///

#ifndef G_NEW_PROCESS_H
#define G_NEW_PROCESS_H

#include "gdef.h"
#include "gexception.h"
#include "genvironment.h"
#include "gidentity.h"
#include "gprocess.h"
#include "gpath.h"
#include "gstrings.h"
#include <string>

namespace G
{
	class NewProcess ;
	class NewProcessImp ;
	class NewProcessWaitFuture ;
	class Pipe ;
}

/// \class G::NewProcess
/// A class for creating new processes.
///
/// Eg:
/// \code
/// {
///   NewProcess task( "foo" , args ) ;
///   int rc = task.wait().run().get() ;
///   std::string s = task.wait().output() ;
///   ...
/// }
/// \endcode
///
/// \see G::Daemon, G::NewProcessWaitFuture
///
class G::NewProcess
{
public:
	G_EXCEPTION( Error , "cannot spawn new process" ) ;
	G_EXCEPTION( CannotFork , "cannot fork()" ) ;
	G_EXCEPTION( WaitError , "cannot wait()" ) ;
	G_EXCEPTION( ChildError , "child process terminated abnormally or stopped" ) ;
	G_EXCEPTION( Insecure , "refusing to exec() while the user-id is zero" ) ;
	G_EXCEPTION( PipeError , "pipe error" ) ;
	G_EXCEPTION( InvalidPath , "invalid executable path -- must be absolute" ) ;
	G_EXCEPTION( InvalidParameter , "invalid parameter" ) ;
	G_EXCEPTION( CreateProcessError , "CreateProcess() error" ) ; // windows

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

	NewProcess( const Path & exe , const StringArray & args , const Environment & env = Environment::minimal() ,
		Fd fd_stdin = Fd::devnull() , Fd fd_stdout = Fd::pipe() , Fd fd_stderr = Fd::devnull() ,
		const G::Path & cd = G::Path() , bool strict_path = true ,
		Identity run_as_id = Identity::invalid() , bool strict_id = true ,
		int exec_error_exit = 127 ,
		const std::string & exec_error_format = std::string() ,
		std::string (*exec_error_format_fn)(std::string,int) = nullptr ) ;
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
			///< If 'strict_path' then the program must be given as an absolute path.
			///< Otherwise it can be relative and the calling process's PATH is used
			///< to find it.
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
		///< Destructor. Kills the spawned process if the WaitFuture has
		///< not been resolved.

	int id() const noexcept ;
		///< Returns the process id.

	NewProcessWaitFuture & wait() ;
		///< Returns a reference to the WaitFuture sub-object so that the caller
		///< can wait for the child process to exit.

	void kill( bool yield = false ) noexcept ;
		///< Tries to kill the spawned process and optionally yield
		///< to a thread that might be waiting on it.

	static std::pair<bool,pid_t> fork() ;
		///< A utility function that forks the calling process and returns
		///< twice; once in the parent and once in the child. Returns
		///< an "is-in-child/child-pid" pair.
		/// \see G::Daemon

public:
	NewProcess( const NewProcess & ) = delete ;
	NewProcess( NewProcess && ) = delete ;
	void operator=( const NewProcess & ) = delete ;
	void operator=( NewProcess && ) = delete ;

private:
	static std::string execErrorFormat( const std::string & , int ) ;

private:
	std::unique_ptr<NewProcessImp> m_imp ;
} ;

/// \class G::NewProcessWaitFuture
/// A class that holds the parameters and the results of a waitpid() system
/// call. The run() method can be run from a worker thread and the results
/// collected by the main thread using get() once the worker thread has
/// signalled that it has finished. The signalling mechanism is outside the
/// scope of this class (see GNet::FutureEvent).
///
class G::NewProcessWaitFuture
{
public:
	NewProcessWaitFuture() ;
		///< Default constructor for an object where run() does nothing
		///< and get() returns zero.

	explicit NewProcessWaitFuture( pid_t pid , int fd = -1 ) ;
		///< Constructor taking a posix process-id and optional
		///< readable file descriptor.

	void assign( pid_t pid , int fd ) ;
		///< Reinitialises as if constructed with the given proces-id
		///< and file descriptor.

	NewProcessWaitFuture( HANDLE hprocess , HANDLE hpipe , int ) ;
		///< Constructor taking process and pipe handles.
		///< Used in the windows implementation.

	void assign( HANDLE hprocess , HANDLE hpipe , int ) ;
		///< Reinitialises as if constructed with the given proces
		///< handle and pipe handle.

	NewProcessWaitFuture & run() ;
		///< Waits for the process identified by the constructor
		///< parameter to exit. Returns *this. This method can be
		///< called from a separate worker thread.

	int get() ;
		///< Returns the result of the run() as either the process
		///< exit code or as a thrown exception. Typically called
		///< by the main thread after the run() worker thread has
		///< signalled its completion.

	std::string output() ;
		///< Returns the first bit of child-process output.
		///< Used after get().

public:
	~NewProcessWaitFuture() = default ;
	NewProcessWaitFuture( const NewProcessWaitFuture & ) = delete ;
	NewProcessWaitFuture( NewProcessWaitFuture && ) = delete ;
	void operator=( const NewProcessWaitFuture & ) = delete ;
	void operator=( NewProcessWaitFuture && ) = delete ;

private:
	std::vector<char> m_buffer ;
	HANDLE m_hprocess{0} ;
	HANDLE m_hpipe{0} ;
	pid_t m_pid{0} ;
	int m_fd{-1} ;
	int m_rc{0} ;
	int m_status{0} ;
	int m_error{0} ;
	int m_read_error{0} ;
} ;

#endif
