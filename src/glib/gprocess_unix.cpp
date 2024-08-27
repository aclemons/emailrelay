//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gprocess_unix.cpp
///

#include "gdef.h"
#include "gprocess.h"
#include "gidentity.h"
#include "gstr.h"
#include "gfile.h"
#include "gpath.h"
#include "glimits.h"
#include "glog.h"
#include <iostream>
#include <stdexcept>
#include <array>
#include <cstring> // std::strerror()
#include <climits> // PATH_MAX
#include <cerrno> // errno
#include <fcntl.h>

namespace G
{
	namespace ProcessImp
	{
		G_EXCEPTION_CLASS( DevNullError , tx("cannot open /dev/null") )
		G_EXCEPTION_CLASS( IdentityError , tx("cannot change process identity") )
		void noCloseOnExec( int fd ) noexcept ;
		enum class Mode { read_only , write_only } ;
		void reopen( int fd , Mode mode ) ;
		mode_t umaskValue( G::Process::Umask::Mode mode ) ;
		bool readlink_( std::string_view path , std::string & value ) ;
		bool setRealUser( Identity id , std::nothrow_t ) noexcept ;
		bool setRealGroup( Identity id , std::nothrow_t ) noexcept ;
		void setEffectiveUser( Identity id ) ;
		bool setEffectiveUser( Identity id , std::nothrow_t ) noexcept ;
		void setEffectiveGroup( Identity id ) ;
		bool setEffectiveGroup( Identity id , std::nothrow_t ) noexcept ;
		void throwError() ;
		void beSpecial( Identity special_identity , bool change_group ) ;
		void beSpecialForExit( SignalSafe , Identity special_identity ) noexcept ;
		Identity beOrdinaryAtStartup( Identity , bool change_group ) ;
		Identity beOrdinary( Identity , bool change_group ) ;
		void beOrdinaryForExec( Identity run_as_id ) noexcept ;
		void revokeExtraGroups() ;
	}
}

class G::Process::UmaskImp
{
public:
	mode_t m_old_mode ;
	static mode_t set( Process::Umask::Mode ) noexcept ;
	static void set( mode_t ) noexcept ;
} ;

// ==

void G::Process::cd( const Path & dir )
{
	if( ! cd(dir,std::nothrow) )
		throw CannotChangeDirectory( dir.str() ) ;
}

bool G::Process::cd( const Path & dir , std::nothrow_t )
{
	return 0 == ::chdir( dir.cstr() ) ;
}

void G::Process::closeStderr()
{
	ProcessImp::reopen( STDERR_FILENO , ProcessImp::Mode::write_only ) ;
}

void G::Process::closeFiles( bool keep_stderr )
{
	std::cout << std::flush ;
	std::cerr << std::flush ;

	ProcessImp::reopen( STDIN_FILENO , ProcessImp::Mode::read_only ) ;
	ProcessImp::reopen( STDOUT_FILENO , ProcessImp::Mode::write_only ) ;
	if( !keep_stderr )
		ProcessImp::reopen( STDERR_FILENO , ProcessImp::Mode::write_only ) ;

	closeOtherFiles() ;
	inheritStandardFiles() ;
}

void G::Process::closeOtherFiles( int fd_keep )
{
	int n = 256U ;
	long rc = ::sysconf( _SC_OPEN_MAX ) ;
	if( rc > 0L )
		n = static_cast<int>( rc ) ;

	for( int fd = 0 ; fd < n ; fd++ )
	{
		if( fd != STDIN_FILENO && fd != STDOUT_FILENO && fd != STDERR_FILENO && fd != fd_keep )
			::close( fd ) ;
	}
}

void G::Process::inheritStandardFiles()
{
	ProcessImp::noCloseOnExec( STDIN_FILENO ) ;
	ProcessImp::noCloseOnExec( STDOUT_FILENO ) ;
	ProcessImp::noCloseOnExec( STDERR_FILENO ) ;
}

int G::Process::errno_( const SignalSafe & ) noexcept
{
	return errno ; // possible macro, not ::errno or std::errno
}

void G::Process::errno_( int e_new ) noexcept
{
	errno = e_new ;
}

int G::Process::errno_( const SignalSafe & , int e_new ) noexcept
{
	int e_old = errno ;
	errno = e_new ;
	return e_old ;
}

std::string G::Process::strerror( int errno_ )
{
	char * p = std::strerror( errno_ ) ;
	std::string s( p ? p : "" ) ;
	if( s.empty() ) s = "unknown error" ;
	return Str::isPrintableAscii(s) ? Str::lower(s) : s ;
}

#ifndef G_LIB_SMALL
std::string G::Process::errorMessage( DWORD e )
{
	return std::string("error ").append( std::to_string(e) ) ;
}
#endif

void G::Process::beSpecial( Identity special_identity , bool change_group )
{
	ProcessImp::beSpecial( special_identity , change_group ) ;
}

void G::Process::beSpecialForExit( SignalSafe , Identity special_identity ) noexcept
{
	ProcessImp::beSpecialForExit( SignalSafe() , special_identity ) ;
}

std::pair<G::Identity,G::Identity> G::Process::beOrdinaryAtStartup( const std::string & ordinary_name ,
	bool change_group )
{
	Identity ordinary_id( ordinary_name ) ;

	// revoke extra groups, but not if we are leaving groups alone
	// or we have been given root as the non-root user or we
	// are running vanilla
	if( change_group && !ordinary_id.isRoot() )
	{
		Identity real = Identity::real() ;
		if( real.isRoot() || real != Identity::effective() )
			ProcessImp::revokeExtraGroups() ;
	}

	Identity startup_id = ProcessImp::beOrdinary( ordinary_id , change_group ) ;
	return { ordinary_id , startup_id } ;
}

void G::Process::beOrdinary( Identity ordinary_id , bool change_group )
{
	ProcessImp::beOrdinary( ordinary_id , change_group ) ;
}

void G::Process::beOrdinaryForExec( Identity run_as_id ) noexcept
{
	ProcessImp::beOrdinaryForExec( run_as_id ) ;
}

#ifndef G_LIB_SMALL
void G::Process::setEffectiveUser( Identity id )
{
	G::ProcessImp::setEffectiveUser( id ) ;
}
#endif

#ifndef G_LIB_SMALL
void G::Process::setEffectiveGroup( Identity id )
{
	G::ProcessImp::setEffectiveGroup( id ) ;
}
#endif

G::Path G::Process::cwd()
{
	return cwdImp( false ) ;
}

G::Path G::Process::cwd( std::nothrow_t )
{
	return cwdImp( true ) ;
}

G::Path G::Process::cwdImp( bool no_throw )
{
	std::string result ;
	std::array<std::size_t,2U> sizes = {{ G::Limits<>::path_buffer , PATH_MAX+1U }} ;
	for( std::size_t n : sizes )
	{
		std::vector<char> buffer( n ) ;
		char * p = getcwd( buffer.data() , buffer.size() ) ;
		int error = errno_() ;
		if( p != nullptr )
		{
			buffer.push_back( '\0' ) ;
			result.assign( buffer.data() ) ;
			break ;
		}
		else if( error != ERANGE )
		{
			break ;
		}
	}
	if( result.empty() && !no_throw )
		throw GetCwdError() ;
	return {result} ;
}

#ifdef G_UNIX_MAC
#include <libproc.h>
G::Path G::Process::exe()
{
	// (see also _NSGetExecutablePath())
	std::vector<char> buffer( std::max(100,PROC_PIDPATHINFO_MAXSIZE) ) ;
	buffer[0] = '\0' ;
	int rc = proc_pidpath( getpid() , buffer.data() , buffer.size() ) ;
	if( rc > 0 )
	{
		std::size_t n = static_cast<std::size_t>(rc) ;
		if( n > buffer.size() ) n = buffer.size() ;
		return Path( std::string_view(buffer.data(),n) ) ;
	}
	else
	{
		return {} ;
	}
}
#else
G::Path G::Process::exe()
{
	// best effort, not guaranteed
	std::string result ;
	ProcessImp::readlink_( "/proc/self/exe" , result ) ||
	ProcessImp::readlink_( "/proc/curproc/file" , result ) ||
	ProcessImp::readlink_( "/proc/curproc/exe" , result ) ;
	return {result} ;
}
#endif

// ==

G::Process::Id::Id() noexcept :
	m_pid(::getpid())
{
}

std::string G::Process::Id::str() const
{
	std::ostringstream ss ;
	ss << m_pid ;
	return ss.str() ;
}

#ifndef G_LIB_SMALL
bool G::Process::Id::operator==( const Id & other ) const noexcept
{
	return m_pid == other.m_pid ;
}
#endif

bool G::Process::Id::operator!=( const Id & other ) const noexcept
{
	return m_pid != other.m_pid ;
}

// ==

void G::Process::UmaskImp::set( mode_t mode ) noexcept
{
	GDEF_IGNORE_RETURN ::umask( mode ) ;
}

mode_t G::Process::UmaskImp::set( Umask::Mode mode ) noexcept
{
	mode_t old = ::umask( 2 ) ;
	mode_t new_ = old ;
	if( mode == Umask::Mode::Tightest )
		new_ = 0077 ; // -rw-------
	else if( mode == Umask::Mode::Tighter )
		new_ = 0007 ;  // -rw-rw----
	else if( mode == Umask::Mode::Readable )
		new_ = 0022 ; // -rw-r--r--
	else if( mode == Umask::Mode::GroupOpen )
		new_ = 0002 ; // -rw-rw-r--
	else if( mode == Umask::Mode::TightenOther )
		new_ = old | mode_t(007) ;
	else if( mode == Umask::Mode::LoosenGroup )
		new_ = old & ~mode_t(070) ;
	else if( mode == Umask::Mode::Open )
		new_ = 0 ; // -rw-rw-rw-
	set( new_ ) ;
	return old ;
}

G::Process::Umask::Umask( Mode mode ) :
	m_imp(std::make_unique<UmaskImp>())
{
	m_imp->m_old_mode = UmaskImp::set( mode ) ;
}

G::Process::Umask::~Umask()
{
	UmaskImp::set( m_imp->m_old_mode ) ;
}

void G::Process::Umask::set( Mode mode )
{
	UmaskImp::set( mode ) ;
}

#ifndef G_LIB_SMALL
void G::Process::Umask::tightenOther()
{
	set( Mode::TightenOther ) ;
}
#endif

#ifndef G_LIB_SMALL
void G::Process::Umask::loosenGroup()
{
	set( Mode::LoosenGroup ) ;
}
#endif

// ==

void G::ProcessImp::noCloseOnExec( int fd ) noexcept
{
	::fcntl( fd , F_SETFD , 0 ) ;
}

void G::ProcessImp::reopen( int fd , Mode mode_in )
{
	auto mode = mode_in == Mode::read_only ? File::InOutAppend::In : File::InOutAppend::OutNoCreate ;
	int fd_null = File::open( Path::nullDevice() , mode ) ;
	if( fd_null < 0 ) throw DevNullError() ;
	::dup2( fd_null , fd ) ;
	::close( fd_null ) ;
}

#ifndef G_LIB_SMALL
mode_t G::ProcessImp::umaskValue( Process::Umask::Mode mode )
{
	mode_t m = 0 ;
	if( mode == Process::Umask::Mode::Tightest ) m = 0177 ; // -rw-------
	if( mode == Process::Umask::Mode::Tighter ) m = 0117 ;  // -rw-rw----
	if( mode == Process::Umask::Mode::Readable ) m = 0133 ; // -rw-r--r--
	if( mode == Process::Umask::Mode::GroupOpen ) m = 0113 ;// -rw-rw-r--
	return m ;
}
#endif

bool G::ProcessImp::readlink_( std::string_view path , std::string & value )
{
	Path target = File::readlink( Path(path) , std::nothrow ) ;
	if( !target.empty() ) value = target.str() ;
	return !target.empty() ;
}

// ==

G::Identity G::ProcessImp::beOrdinary( Identity nobody_id , bool change_group )
{
	Identity old_id = Identity::effective() ;
	Identity real_id = Identity::real() ;
	if( real_id.isRoot() )
	{
		if( change_group )
		{
			// make sure we have privilege to change group
			if( !setEffectiveUser( Identity::root() , std::nothrow ) )
				throwError() ;

			if( !setEffectiveGroup( nobody_id , std::nothrow ) )
			{
				setEffectiveUser( old_id , std::nothrow ) ; // rollback
				throwError() ;
			}
		}
		if( !setEffectiveUser( nobody_id , std::nothrow ) )
			throwError() ;
	}
	else
	{
		// change to real id -- drops suid privileges
		if( !setEffectiveUser( real_id , std::nothrow ) )
			throwError() ;

		if( change_group && !setEffectiveGroup( real_id , std::nothrow ) )
		{
			setEffectiveUser( old_id , std::nothrow ) ; // rollback
			throwError() ;
		}
	}
	return old_id ;
}

void G::ProcessImp::beOrdinaryForExec( Identity run_as_id ) noexcept
{
	if( run_as_id != Identity::invalid() )
	{
		setEffectiveUser( Identity::root() , std::nothrow ) ; // for root-suid
		setRealGroup( run_as_id , std::nothrow ) ;
		setEffectiveGroup( run_as_id , std::nothrow ) ;
		setRealUser( run_as_id , std::nothrow ) ;
		setEffectiveUser( run_as_id , std::nothrow ) ;
	}
}

void G::ProcessImp::beSpecial( Identity special_identity , bool change_group )
{
	setEffectiveUser( special_identity ) ;
	if( change_group )
		setEffectiveGroup( special_identity ) ;
}

void G::ProcessImp::beSpecialForExit( SignalSafe , Identity special_identity ) noexcept
{
	// changing effective ids is not strictly signal-safe :-<
	setEffectiveUser( special_identity , std::nothrow ) ;
	setEffectiveGroup( special_identity , std::nothrow ) ;
}

void G::ProcessImp::revokeExtraGroups()
{
	if( Identity::real().isRoot() || Identity::effective() != Identity::real() )
	{
		// set supplementary group-ids to a zero-length list
		gid_t dummy = 0 ;
		GDEF_IGNORE_RETURN ::setgroups( 0U , &dummy ) ; // (only works for root, so ignore the return code)
	}
}

bool G::ProcessImp::setRealUser( Identity id , std::nothrow_t ) noexcept
{
	return 0 == ::setuid( id.userid() ) ;
}

void G::ProcessImp::setEffectiveUser( Identity id )
{
	if( ::seteuid(id.userid()) )
	{
		int e = errno ;
		throw Process::UidError( Process::strerror(e) ) ;
	}
}

bool G::ProcessImp::setEffectiveUser( Identity id , std::nothrow_t ) noexcept
{
	return 0 == ::seteuid( id.userid() ) ;
}

bool G::ProcessImp::setRealGroup( Identity id , std::nothrow_t ) noexcept
{
	return 0 == ::setgid( id.groupid() ) ;
}

void G::ProcessImp::setEffectiveGroup( Identity id )
{
	if( ::setegid(id.groupid()) )
	{
		int e = errno ;
		throw Process::GidError( Process::strerror(e) ) ;
	}
}

bool G::ProcessImp::setEffectiveGroup( Identity id , std::nothrow_t ) noexcept
{
	return 0 == ::setegid( id.groupid() ) ;
}

void G::ProcessImp::throwError()
{
	// typically we are about to std::terminate() so make sure there is an error message
	G_ERROR( "G::ProcessImp::throwError: failed to change process identity" ) ;
	throw IdentityError() ;
}

