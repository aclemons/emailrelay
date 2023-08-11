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
/// \file gprocess.h
///

#ifndef G_PROCESS_H
#define G_PROCESS_H

#include "gdef.h"
#include "gexception.h"
#include "gidentity.h"
#include "gpath.h"
#include "gstr.h"
#include "gstringarray.h"
#include "gsignalsafe.h"
#include <memory>
#include <utility>
#include <iostream>
#include <limits>
#include <type_traits>
#include <string>
#include <new>

namespace G
{
	class Process ;
	class NewProcess ;
}

//| \class G::Process
/// A static interface for doing things with processes.
/// \see G::Identity
///
class G::Process
{
public:
	G_EXCEPTION( CannotChangeDirectory , tx("cannot change directory") ) ;
	G_EXCEPTION( InvalidId , tx("invalid process-id string") ) ;
	G_EXCEPTION( UidError , tx("cannot set uid") ) ;
	G_EXCEPTION( GidError , tx("cannot set gid") ) ;
	G_EXCEPTION( GetCwdError , tx("cannot get current working directory") ) ;
	class Id ;
	class Umask ;
	class UmaskImp ;

	static void closeFiles( bool keep_stderr = false ) ;
		///< Closes all open file descriptors and reopen stdin,
		///< stdout and possibly stderr to the null device.

	static void closeStderr() ;
		///< Closes stderr and reopens it to the null device.

	static void closeOtherFiles( int fd_keep = -1 ) ;
		///< Closes all open file descriptors except the three
		///< standard ones and possibly one other.

	static void cd( const Path & dir ) ;
		///< Changes directory.

	static bool cd( const Path & dir , std::nothrow_t ) ;
		///< Changes directory. Returns false on error.

	static int errno_( const SignalSafe & = G::SignalSafe() ) noexcept ;
		///< Returns the process's current 'errno' value.
		///< (Beware of destructors of c++ temporaries disrupting
		///< the global errno value.)

	static void errno_( int e_new ) noexcept ;
		///< Sets the process's 'errno' value.

	static int errno_( const SignalSafe & , int e_new ) noexcept ;
		///< Sets the process's 'errno' value. Returns the old
		///< value. Typically used in signal handlers.

	static std::string strerror( int errno_ ) ;
		///< Translates an 'errno' value into a meaningful diagnostic string.
		///< The returned string is non-empty, even for a zero errno.

	static std::pair<Identity,Identity> beOrdinaryAtStartup( const std::string & nobody , bool change_group ) ;
		///< Revokes special privileges (root or suid) at startup, possibly
		///< including extra group membership, making the named user the
		///< effective identity. Returns the new effective identity and the
		///< original effective identity as a pair.
		///<
		///< \code
		///< auto pair = Process::beOrdinaryAtStartup( "daemon" , chgrp ) ;
		///< Process::beSpecial( pair.second , chgrp ) ;
		///< doPrivilegedStuff() ;
		///< Process::beOrdinary( pair.first , chgrp ) ;
		///< \endcode

	static void beOrdinary( Identity ordinary_id , bool change_group ) ;
		///< Releases special privileges.
		///<
		///< If the real-id is root then the effective user-id is changed to
		///< whatever is passed in. Otherwise the effective user-id is changed
		///< to the real user-id (optionally including the group), and the
		///< identity parameter is ignored.
		///<
		///< Logs an error message and throws on failure, resulting in a call
		///< to std::terminate() when called from a destructor (see G::Root).
		///<
		///< This affects all threads in the calling processes, with signal hacks
		///< used in some implementations to do the synchronisation. This can
		///< lead to surprising interruptions of sleep(), select() etc.
		///<
		///< See also class G::Root.

	static void beSpecial( Identity special_id , bool change_group = true ) ;
		///< Re-acquires special privileges (either root or suid). The
		///< parameter must have come from a previous call to
		///< beOrdinaryAtStartup() and use the same change_group value.
		///<
		///< See also class G::Root.

	static void beSpecialForExit( SignalSafe , Identity special_id ) noexcept ;
		///< A signal-safe version of beSpecial() that should only be used
		///< just before process exit.

	static void beOrdinaryForExec( Identity run_as_id ) noexcept ;
		///< Sets the real and effective user-id and group-ids to those
		///< given, on a best-effort basis. Errors are ignored.

	static void setEffectiveUser( Identity ) ;
		///< Sets the effective user-id. Throws on error.

	static void setEffectiveGroup( Identity ) ;
		///< Sets the effective group-id. Throws on error.

	static std::string cwd( bool no_throw = false ) ;
		///< Returns the current working directory. Throws on error
		///< by default or returns the empty string.

	static std::string exe() ;
		///< Returns the absolute path of the current executable,
		///< independent of the argv array passed to main(). Returns
		///< the empty string if unknown.

	class Id /// Process-id class.
	{
	public:
		Id() noexcept ;
		explicit Id( const char * , const char * end ) noexcept ;
		explicit Id( int ) noexcept ;
		explicit Id( std::istream & ) ;
		static Id invalid() noexcept ;
		std::string str() const ;
		bool operator==( const Id & ) const noexcept ;
		bool operator!=( const Id & ) const noexcept ;
		template <typename T> T value(
			typename std::enable_if
				<std::numeric_limits<T>::max() >= std::numeric_limits<pid_t>::max()>
			::type * = 0 ) const noexcept
		{
			static_assert( sizeof(T) >= sizeof(pid_t) , "" ) ;
			return static_cast<T>( m_pid ) ;
		}
		template <typename T> T seed() const noexcept
		{
			return static_cast<T>( m_pid ) ;
		}

	private:
		friend class NewProcess ;
		friend class Process ;
		pid_t m_pid{0} ;
	} ;

	class Umask /// Used to temporarily modify the process umask.
	{
	public:
		enum class Mode
		{
			NoChange , // typically 0022
			TightenOther , // -......---
			LoosenGroup , // -...rwx...
			Readable , // 0022 -rw-r--r-- for open(0666) and -rwxr-xr-x for mkdir(0777)
			Tighter ,  // 0007 -rw-rw---- for open(0666) and -rwxrwx--- for mkdir(0777)
			Tightest , // 0077 -rw------- for open(0666) and -rwx------ for mkdir(0777)
			GroupOpen ,// 0002 -rw-rw-r-- for open(0666) and -rwxrwxr-x for mkdir(0777)
			Open       // 0000 -rw-rw-rw- for open(0666) and -rwxrwxrwx for mkdir(0777)
		} ;
		explicit Umask( Mode ) ;
		~Umask() ;
		static void set( Mode ) ;
		static void tightenOther() ; // deny "other" access, user and group unchanged
		static void loosenGroup() ; // allow group access, user and "other" unchanged
		Umask( const Umask & ) = delete ;
		Umask( Umask && ) = delete ;
		Umask & operator=( const Umask & ) = delete ;
		Umask & operator=( Umask && ) = delete ;

	private:
		std::unique_ptr<UmaskImp> m_imp ;
	} ;

public:
	Process() = delete ;
} ;

inline G::Process::Id::Id( int n ) noexcept :
	m_pid(static_cast<pid_t>(n))
{
}

inline G::Process::Id::Id( std::istream & stream )
{
	stream >> m_pid ;
}

inline G::Process::Id::Id( const char * p , const char * end ) noexcept
{
	bool overflow = false ;
	m_pid = G::Str::toUnsigned<pid_t>( p , end , overflow ) ;
	if( overflow )
		m_pid = static_cast<pid_t>(-1) ;
}

inline G::Process::Id G::Process::Id::invalid() noexcept
{
	return Id(-1) ;
}

namespace G
{
	inline
	std::ostream & operator<<( std::ostream & stream , const G::Process::Id & id )
	{
		return stream << id.str() ;
	}
}

#endif
