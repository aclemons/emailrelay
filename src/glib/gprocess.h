//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gprocess.h
///

#ifndef G_PROCESS_H
#define G_PROCESS_H

#include "gdef.h"
#include "gexception.h"
#include "gidentity.h"
#include "gpath.h"
#include "gstrings.h"
#include <iostream>
#include <sys/types.h>
#include <string>

/// \namespace G
namespace G
{
	class Process ;
}

/// \class G::Process
/// A static interface for doing things with processes.
/// \see G::Daemon
///
class G::Process : private G::IdentityUser 
{
public:
	G_EXCEPTION( CannotFork , "cannot fork()" ) ;
	G_EXCEPTION( CannotChangeDirectory , "cannot cd()" ) ;
	G_EXCEPTION( WaitError , "cannot wait()" ) ;
	G_EXCEPTION( ChildError , "child process terminated abnormally or stopped" ) ;
	G_EXCEPTION( InvalidPath , "invalid executable path -- must be absolute" ) ;
	G_EXCEPTION( Insecure , "refusing to exec() while the user-id is zero" ) ;
	G_EXCEPTION( InvalidId , "invalid process-id string" ) ;
	G_EXCEPTION( PipeError , "pipe error" ) ;
	G_EXCEPTION( NoExtension , "refusing to CreateProcess() without a file extension such as .exe" ) ; // windows

	enum Who { Parent , Child } ;
	class IdImp ;
	/// Process-id class.
	class Id 
	{
		public: Id() ;
		public: explicit Id( std::istream & ) ;
		public: Id( SignalSafe , const char * pid_file_path ) ; // (ctor for signal-handler)
		public: std::string str() const ;
		public: bool operator==( const Id & ) const ;
		private: pid_t m_pid ;
		friend class Process ;
	} ;
	/// Used to temporarily modify the process umask.
	class Umask 
	{
		public: enum Mode { Readable , Tighter , Tightest , GroupOpen } ;
		public: explicit Umask( Mode ) ;
		public: ~Umask() ;
		public: static void set( Mode ) ;
		private: Umask( const Umask & ) ; // not implemented
		private: void operator=( const Umask & ) ; // not implemented
		private: class UmaskImp ;
		private: UmaskImp * m_imp ;
	} ;
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
		friend class Process ;
	} ;
	/// An overload discriminator for Process.
	class NoThrow 
		{} ;

	static void closeFiles( bool keep_stderr = false ) ;
		///< Closes all open file descriptors.

	static void closeStderr() ;
		///< Closes stderr.

	static void cd( const Path & dir ) ;
		///< Changes directory.

	static bool cd( const Path & dir , NoThrow ) ;
		///< Changes directory. Returns false on error.

	static Who fork() ;
		///< Forks a child process.

	static Who fork( Id & child ) ;
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

	static int errno_() ;
		///< Returns the process's current 'errno' value.

	static std::string strerror( int errno_ ) ;
		///< Translates an 'errno' value into a meaningful diagnostic string.

	static void revokeExtraGroups() ;
		///< Revokes secondary group memberships if really root
		///< or if suid.

	static Identity beOrdinary( Identity nobody , bool change_group = true ) ;
		///< Revokes special privileges (root or suid).
		///<
		///< If really root (as opposed to suid root) then the effective 
		///< id is changed to that passed in. 
		///<
		///< If suid (including suid-root), then the effective id is 	
		///< changed to the real id, and the parameter is ignored. 
		///<
		///< Returns the old identity, which can be passed to beSpecial().
		///<
		///< See also class G::Root.

	static Identity beSpecial( Identity special , bool change_group = true ) ;
		///< Re-acquires special privileges (either root or suid). The 
		///< parameter must have come from a previous call to beOrdinary().
		///<
		///< Returns the old identity (which is normally ignored).
		///<
		///< See also class G::Root.

	static Identity beOrdinary( SignalSafe , Identity nobody , bool change_group = true ) ;
		///< A signal-safe overload.

	static Identity beSpecial( SignalSafe , Identity special , bool change_group = true ) ;
		///< A signal-safe overload.

private:
	friend class ChildProcess ;
	Process() ;
	static int wait( const Id & child ) ;
	static int wait( const Id & child , int error_return ) ;
	static int execCore( const Path & , const Strings & ) ;
	static void beNobody( Identity ) ;
	static void closeFiles( int ) ;
} ;

/// \namespace G
namespace G
{
	inline
	std::ostream & operator<<( std::ostream & stream , const G::Process::Id & id )
	{
		return stream << id.str() ;
	}

	inline
	std::istream & operator>>( std::istream & stream , G::Process::Id & id )
	{
		id = G::Process::Id( stream ) ;
		return stream ;
	}
}

#endif

