//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#define G_NEW_PROCESS_WITH_WAIT_FUTURE

#include "gdef.h"
#include "gexception.h"
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
	G_EXCEPTION( CannotFork , "cannot fork()" ) ;
	G_EXCEPTION( WaitError , "cannot wait()" ) ;
	G_EXCEPTION( ChildError , "child process terminated abnormally or stopped" ) ;
	G_EXCEPTION( Insecure , "refusing to exec() while the user-id is zero" ) ;
	G_EXCEPTION( PipeError , "pipe error" ) ;
	G_EXCEPTION( InvalidPath , "invalid executable path -- must be absolute" ) ;
	G_EXCEPTION( CreateProcessError , "CreateProcess() error" ) ; // windows

	NewProcess( const Path & exe , const StringArray & args ,
		int capture_stdxxx = 1 , bool clean_environment = true , bool strict_path = true ,
		Identity run_as_id = Identity::invalid() , bool strict_id = true ,
		int exec_error_exit = 127 ,
		const std::string & exec_error_format = std::string() ,
		std::string (*exec_error_format_fn)(std::string,int) = 0 ) ;
			///< Constructor. Spawns the given program to run independently in a
			///< child process.
			///<
			///< The parent process can capture the child process's stdout or
			///< stderr (ie. stdxxx) by redirecting it to an internal pipe going
			///< from child to parent.
			///<
			///< The child process is optionally given a clean, minimalist
			///< environment.
			///<
			///< If 'strict_path' then the program must be given as an absolute path.
			///< Otherwise it can be relative and the PATH environment is used
			///< to find it. (The PATH search is done after the environment has
			///< been either cleaned or not.)
			///<
			///< If a valid identity is supplied then the child process runs as
			///< that identity. If 'strict_id' is also true then the id is not
			///< allowed to be root. See Process::beOrdinaryForExec().
			///<
			///< By default the child process runs with stdin and stderr attached to
			///< the null device and stdout attached to the internal pipe. The
			///< internal pipe is also used for error messages in case the exec()
			///< fails.
			///<
			///< If the exec() fails then the 'exec_error_exit' argument is used as
			///< the child process exit code, and if either of the other 'exec_whatever'
			///< arguments is supplied then an exec error message is written into
			///< the child process's end of the internal pipe.
			///<
			///< The exec error message is assembled by the given callback function,
			///< with the 'exec_error_format' argument passed as its second parameter.
			///< The first parameter is the exec() errno. The default callback function
			///< does text substitution for "__errno__" and "__strerror__"
			///< substrings that appear within the error format string.

	~NewProcess() ;
		///< Destructor. Kills the spawned process if the WaitFuture has
		///< not been resolved.

	int id() const ;
		///< Returns the process id.

	NewProcessWaitFuture & wait() ;
		///< Returns a reference to the WaitFuture sub-object so that the caller
		///< can wait for the child process to exit.

	void kill() ;
		///< Tries to kill the spawned process.

	static std::pair<bool,pid_t> fork() ;
		///< A utility function that forks the calling process and returns
		///< twice; once in the parent and once in the child. Returns
		///< an "is-in-child/child-pid" pair.
		/// \see G::Daemon

private:
	NewProcess( const NewProcess & ) ;
	void operator=( const NewProcess & ) ;
	static std::string execErrorFormat( const std::string & , int ) ;

private:
	NewProcessImp * m_imp ;
} ;

/// \class G::NewProcessWaitFuture
/// A class that holds the parameters and the results of a waitpid() system
/// call. The run() method can be run from a worker thread and the results
/// collected by the main thread using get() once the worker thread has
/// signalled that it has finished. The signalling mechanism is outside the
/// scope of this class.
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

	NewProcessWaitFuture( HANDLE hprocess , HANDLE hpipe , int ) ;
		///< Constructor taking process and pipe handles.
		///< Used in the windows implementation.

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

private:
	std::vector<char> m_buffer ;
	HANDLE m_hprocess ;
	HANDLE m_hpipe ;
	pid_t m_pid ;
	int m_fd ;
	int m_rc ;
	int m_status ;
	int m_error ;
	int m_read_error ;
} ;

#endif
