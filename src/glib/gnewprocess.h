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
///
/// \file gnewprocess.h
///

#ifndef G_NEW_PROCESS_H
#define G_NEW_PROCESS_H

#include "gdef.h"
#include "gexception.h"
#include "gidentity.h"
#include "gprocess.h"
#include "gpath.h"
#include "gstrings.h"
#include <string>

/// \namespace G
namespace G
{
	class NewProcess ;
}

/// \class G::NewProcess
/// A static interface for creating new processes.
/// \see G::Daemon, G::NewProcess
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
	G_EXCEPTION( NoExtension , "refusing to CreateProcess() without a file extension such as .exe" ) ; // windows

	enum Who { Parent , Child } ;
	class ChildProcessImp ;
	/// Represents the state of a child process.
	class ChildProcess 
	{
		private: explicit ChildProcess( ChildProcessImp * ) ;
		public: ChildProcess( const ChildProcess & ) ;
		public: ~ChildProcess() ;
		public: void operator=( const ChildProcess & ) ;
		public: int wait() ;
		public: std::string read() ;
		private: ChildProcessImp * m_imp ;
		friend class NewProcess ;
	} ;

	static Who fork() ;
		///< Forks a child process.

	static Who fork( Process::Id & child ) ;
		///< Forks a child process. Returns the child
		///< pid by reference to the parent.

	static int spawn( Identity nobody , const Path & exe , const Strings & args , 
		std::string * pipe_result_p = NULL , int error_return = 127 ,
		std::string (*error_decode_fn)(int) = 0 ) ;
			///< Runs a command in an unprivileged child process. Returns the
			///< child process's exit code, or 'error_return' on error.
			///<
			///< The 'nobody' identity should have come from beOrdinary().
			///<
			///< If the 'pipe_result_p' pointer is supplied then the child
			///< process is given a pipe as its stdout and this is used
			///< to read the first bit of whatever it writes.
			///<
			///< If the function pointer is supplied then it is used
			///< to generate a string that is written into the pipe if 
			///< the exec() fails in the fork()ed child process.

	static ChildProcess spawn( const Path & exe , const Strings & args ) ;
		///< A simple overload to spawn a child process asynchronously.
		///< Does no special security checks.

private:
	friend class ChildProcess ;
	NewProcess() ;
	static int wait( const Process::Id & child ) ;
	static int wait( const Process::Id & child , int error_return ) ;
	static int execCore( const Path & , const Strings & ) ;
} ;

#endif

