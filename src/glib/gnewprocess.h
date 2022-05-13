//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
	struct NewProcessConfig ;
	class Pipe ;
}

//| \class G::NewProcess
/// A class for creating new processes.
///
/// Eg:
/// \code
/// {
///   NewProcess task( "foo" , args ) ;
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
		const std::string & exec_error_format = {} ,
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

	explicit NewProcess( const NewProcessConfig & ) ;
		///< Constructor overload with parameters packaged into a structure.

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

//| \class G::NewProcessWaitable
/// Holds the parameters and future results of a waitpid() system call,
/// as performed by the wait() method.
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
		///< int e = f.get() ;
		///< \endcode

	int get() const ;
		///< Returns the result of the wait() as either the process
		///< exit code or as a thrown exception. Typically called
		///< by the main thread after the wait() worker thread has
		///< signalled its completion. Returns zero if there is no
		///< process to wait for.

	int get( std::nothrow_t , int exit_code_on_error = 127 ) const ;
		///< Non-throwing overload.

	std::string output() const ;
		///< Returns the first bit of child-process output.
		///< Used after get().

public:
	~NewProcessWaitable() = default ;
	NewProcessWaitable( const NewProcessWaitable & ) = delete ;
	NewProcessWaitable( NewProcessWaitable && ) = delete ;
	void operator=( const NewProcessWaitable & ) = delete ;
	void operator=( NewProcessWaitable && ) = delete ;

private:
	std::vector<char> m_buffer ;
	std::size_t m_data_size{0U} ;
	HANDLE m_hprocess{0} ;
	HANDLE m_hpipe{0} ;
	pid_t m_pid{0} ;
	int m_fd{-1} ;
	int m_rc{0} ;
	int m_status{0} ;
	int m_error{0} ;
	int m_read_error{0} ;
	bool m_test_mode{false} ;
} ;

//| \class G::NewProcessConfig
/// Packages up the parameters of the multi-parameter G::NewProcess
/// constructor for its one-parameter overload.
///
struct G::NewProcessConfig
{
	explicit NewProcessConfig( const Path & exe ) ;
		///< Constructor.

	explicit NewProcessConfig( const ExecutableCommand & cmd ) ;
		///< Constructor.

	NewProcessConfig( const Path & exe , const std::string & argv1 ) ;
		///< Constructor.

	NewProcessConfig( const Path & exe , const std::string & argv1 , const std::string & argv2 ) ;
		///< Constructor.

	NewProcessConfig( const Path & exe , const StringArray & args ) ;
		///< Constructor.

	NewProcessConfig & set_args( const StringArray & args ) ;
		///< Sets the command-line arguments.

	NewProcessConfig & set_env( const Environment & env ) ;
		///< Sets the environment.

	NewProcessConfig & set_fd_stdin( NewProcess::Fd ) ;
		///< Sets the standard-input file descriptor.

	NewProcessConfig & set_fd_stdout( NewProcess::Fd ) ;
		///< Sets the standard-output file descriptor.

	NewProcessConfig & set_fd_stderr( NewProcess::Fd ) ;
		///< Sets the standard-error file descriptor.

	NewProcessConfig & set_cd( const G::Path & ) ;
		///< Sets the working directory.

	NewProcessConfig & set_strict_path( bool = true ) ;
		///< Sets the 'strict_path' value.

	NewProcessConfig & set_run_as_id( const G::Identity & ) ;
		///< Sets the run-as id.

	NewProcessConfig & set_strict_id( bool = true ) ;
		///< Sets the 'strict_id' value.

	NewProcessConfig & set_exec_error_exit( int ) ;
		///< Sets the 'exec_error_exit' value.

	NewProcessConfig & set_exec_error_format( const std::string & ) ;
		///< Sets the 'exec_error_format' value.

	NewProcessConfig & set_exec_error_format_fn( std::string (*)(std::string,int) ) ;
		///< Sets the 'exec_error_format_fn' value.

	Path m_path ;
	StringArray m_args ;
	Environment m_env{Environment::minimal()} ;
	NewProcess::Fd m_stdin{NewProcess::Fd::devnull()} ;
	NewProcess::Fd m_stdout{NewProcess::Fd::pipe()} ;
	NewProcess::Fd m_stderr{NewProcess::Fd::devnull()} ;
	Path m_cd ;
	bool m_strict_path{true} ;
	Identity m_run_as{Identity::invalid()} ;
	bool m_strict_id{true} ;
	int m_exec_error_exit{127} ;
	std::string m_exec_error_format ;
	std::string (*m_exec_error_format_fn)( std::string , int ){nullptr} ;
} ;

inline
G::NewProcessConfig::NewProcessConfig( const Path & exe ) :
	m_path(exe)
{
}

inline
G::NewProcessConfig::NewProcessConfig( const Path & exe , const G::StringArray & args ) :
	m_path(exe) ,
	m_args(args)
{
}

inline
G::NewProcessConfig::NewProcessConfig( const ExecutableCommand & cmd ) :
	m_path(cmd.exe()) ,
	m_args(cmd.args())
{
}

inline
G::NewProcessConfig::NewProcessConfig( const Path & exe , const std::string & argv1 ) :
	m_path(exe)
{
	m_args.push_back( argv1 ) ;
}

inline
G::NewProcessConfig::NewProcessConfig( const Path & exe , const std::string & argv1 , const std::string & argv2 ) :
	m_path(exe)
{
	m_args.push_back( argv1 ) ;
	m_args.push_back( argv2 ) ;
}

inline G::NewProcessConfig & G::NewProcessConfig::set_args( const StringArray & args ) { m_args = args ; return *this ; }
inline G::NewProcessConfig & G::NewProcessConfig::set_env( const Environment & env ) { m_env = env ; return *this ; }
inline G::NewProcessConfig & G::NewProcessConfig::set_fd_stdin( NewProcess::Fd fd ) { m_stdin = fd ; return *this ; }
inline G::NewProcessConfig & G::NewProcessConfig::set_fd_stdout( NewProcess::Fd fd ) { m_stdout = fd ; return *this ; }
inline G::NewProcessConfig & G::NewProcessConfig::set_fd_stderr( NewProcess::Fd fd ) { m_stderr = fd ; return *this ; }
inline G::NewProcessConfig & G::NewProcessConfig::set_cd( const G::Path & cd ) { m_cd = cd ; return *this ; }
inline G::NewProcessConfig & G::NewProcessConfig::set_strict_path( bool strict_path ) { m_strict_path = strict_path ; return *this ; }
inline G::NewProcessConfig & G::NewProcessConfig::set_run_as_id( const G::Identity & run_as ) { m_run_as = run_as ; return *this ; }
inline G::NewProcessConfig & G::NewProcessConfig::set_strict_id( bool strict_id ) { m_strict_id = strict_id ; return *this ; }
inline G::NewProcessConfig & G::NewProcessConfig::set_exec_error_exit( int exec_error_exit ) { m_exec_error_exit = exec_error_exit ; return *this ; }
inline G::NewProcessConfig & G::NewProcessConfig::set_exec_error_format( const std::string & exec_error_format ) { m_exec_error_format = exec_error_format ; return *this ; }
inline G::NewProcessConfig & G::NewProcessConfig::set_exec_error_format_fn( std::string (*exec_error_format_fn)(std::string,int) ) { m_exec_error_format_fn = exec_error_format_fn ; return *this ; }

inline
G::NewProcess::NewProcess( const NewProcessConfig & config ) :
	NewProcess(
		config.m_path ,
		config.m_args ,
		config.m_env ,
		config.m_stdin ,
		config.m_stdout ,
		config.m_stderr ,
		config.m_cd ,
		config.m_strict_path ,
		config.m_run_as ,
		config.m_strict_id ,
		config.m_exec_error_exit ,
		config.m_exec_error_format ,
		config.m_exec_error_format_fn )
{
}

#endif
