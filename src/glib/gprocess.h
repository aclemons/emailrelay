//
// Copyright (C) 2001-2004 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gprocess.h
//

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

namespace G
{
	class Process ;
}

// Class: G::Process
// Description: A static interface for doing things with processes.
// See also: G::Daemon
//
class G::Process : private G::IdentityUser 
{
public:
	G_EXCEPTION( CannotFork , "cannot fork()" ) ;
	G_EXCEPTION( CannotChroot , "cannot chroot()" ) ;
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
	class Id // Process-id class.
	{
		public: Id() ;
		public: explicit Id( std::istream & ) ;
		public: explicit Id( const char * pid_file_path ) ; // (re-entrant ctor)
		public: std::string str() const ;
		public: bool operator==( const Id & ) const ;
		private: pid_t m_pid ;
		friend class Process ;
	} ;
	class Umask // Used to temporarily modify the process umask.
	{
		public: enum Mode { Readable , Tighter , Tightest } ;
		public: explicit Umask( Mode ) ;
		public: ~Umask() ;
		public: static void set( Mode ) ;
		private: Umask( const Umask & ) ; // not implemented
		private: void operator=( const Umask & ) ; // not implemented
		private: class UmaskImp ;
		private: UmaskImp * m_imp ;
	} ;
	class NoThrow // An overload discriminator for Process.
		{} ;

	static void closeFiles( bool keep_stderr = false ) ;
		// Closes all open file descriptors.

	static void closeStderr() ;
		// Closes stderr.

	static void cd( const Path & dir ) ;
		// Changes directory.

	static bool cd( const Path & dir , NoThrow ) ;
		// Changes directory. Returns false on
		// error.

	static void chroot( const Path & dir ) ;
		// Does a chroot. Throws on error, or if not implemented.

	static Who fork() ;
		// Forks a child process.

	static Who fork( Id & child ) ;
		// Forks a child process. Returns the child
		// pid by reference to the parent.

	static int spawn( Identity nobody , const Path & exe , const Strings & args , 
		std::string * pipe_result_p = NULL , int error_return = 127 ,
		std::string (*error_decode_fn)(int) = 0 ) ;
			// Runs a command in an unprivileged child process. Returns the
			// child process's exit code, or 'error_return' on error.
			//
			// The identity should have come from beOrdinary().
			//
			// If the 'pipe_result_p' pointer is supplied then a pipe
			// is used to read the first bit of whatever the child process 
			// writes to stdout. 
			//
			// If the function pointer is supplied then it is used
			// to prepare a pipe-result string if the underlying
			// exec() fails in the fork()ed child process.

	static int errno_() ;
		// Returns the process's current 'errno' value.

	static std::string strerror( int errno_ ) ;
		// Translates an 'errno' value into a meaningful diagnostic string.

	static void revokeExtraGroups() ;
		// Revokes secondary group memberships if really root
		// or if suid.

	static Identity beOrdinary( Identity nobody , bool change_group = true ) ;
		// Revokes special privileges (root or suid).
		//
		// If really root (as opposed to suid root)
		// then the effective id is changed to that
		// passed in. 
		//
		// If suid (including suid-root), then the effective
		// id is changed to the real id, and the parameter 
		// is ignored. 
		//
		// Returns the old identity, which can be passed to
		// beSpecial().
		//
		// See also class G::Root.

	static void beSpecial( Identity special , bool change_group = true ) ;
		// Re-aquires special privileges (either root
		// or suid). The parameter must have come from
		// a previous call to beOrdinary().
		//
		// See also class G::Root.

private:
	Process() ;
	static int wait( const Id & child ) ;
	static int wait( const Id & child , int error_return ) ;
	static int execCore( const Path & , const Strings & ) ;
	static void beNobody( Identity ) ;
	static void closeFiles( int ) ;
} ;

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

