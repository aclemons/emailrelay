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
// gprocess_unix.cpp
//

#include "gdef.h"
#include "gprocess.h"
#include "gidentity.h"
#include "gassert.h"
#include "gfs.h"
#include "glog.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h> // open()
#include <unistd.h> // setuid() etc

namespace
{
	void noCloseOnExec( int fd )
	{
		::fcntl( fd , F_SETFD , 0 ) ;
	}
}

namespace G
{
	class Pipe ;
}

// Class: G::Pipe
// Description: A private implementation class used by G::Process.
//
class G::Pipe 
{
public:
	explicit Pipe( bool active ) ;
	~Pipe() ;
	void inChild() ; // writer
	void inParent() ; // reader
	int fd() const ;
	void dup() ; // onto stdout
	std::string read() ; // size-limited
	void write( const std::string & ) ;
private:
	G_EXCEPTION( Error , "pipe error" ) ;
	int m_fds[2] ;
	int m_fd ;
} ;

// Class: G::Process::IdImp
// Description: A private implementation class used by G::Process.
//
class G::Process::IdImp 
{
public: 
	pid_t m_pid ;
} ;

// ===

//static
void G::Process::cd( const Path & dir )
{
	if( ! cd(dir,NoThrow()) )
		throw CannotChangeDirectory( dir.str() ) ;
}

//static
bool G::Process::cd( const Path & dir , NoThrow )
{
	return 0 == ::chdir( dir.str().c_str() ) ;
}

//static
void G::Process::chroot( const Path & dir )
{
	cd( dir ) ;
	if( 0 != ::chroot( dir.str().c_str() ) )
	{
		int error = errno_() ;
		G_WARNING( "G::Process::chroot: cannot chroot to \"" << dir << "\": " << error ) ;
		throw CannotChroot( dir.str() ) ;
	}
	cd( "/" ) ;
}

//static
void G::Process::closeStderr()
{
	::close( STDERR_FILENO ) ;
	::open( G::FileSystem::nullDevice() , O_WRONLY ) ;
	noCloseOnExec( STDERR_FILENO ) ;
}

//static
void G::Process::closeFiles( bool keep_stderr )
{
	closeFiles( keep_stderr ? STDERR_FILENO : -1 ) ;
}

//static
void G::Process::closeFiles( int keep )
{
	G_ASSERT( keep == -1 || keep >= STDERR_FILENO ) ;

	int n = 256U ;
	long rc = ::sysconf( _SC_OPEN_MAX ) ;
	if( rc > 0L )
		n = static_cast<int>( rc ) ;

	for( int fd = 0 ; fd < n ; fd++ )
	{
		if( fd != keep )
			::close( fd ) ;
	}

	// reopen standard fds to prevent accidental use 
	// of arbitrary files or sockets as standard
	// streams
	//
	::open( G::FileSystem::nullDevice() , O_RDONLY ) ;
	::open( G::FileSystem::nullDevice() , O_WRONLY ) ;
	if( keep != STDERR_FILENO )
	{
		::open( G::FileSystem::nullDevice() , O_WRONLY ) ;
	}
	noCloseOnExec( STDIN_FILENO ) ;
	noCloseOnExec( STDOUT_FILENO ) ;
	noCloseOnExec( STDERR_FILENO ) ;
}

G::Process::Who G::Process::fork()
{
	Id id ;
	return fork( id ) ;
}

G::Process::Who G::Process::fork( Id & child_pid )
{
	std::cout << std::flush ;
	std::cerr << std::flush ;
	pid_t rc = ::fork() ;
	const bool ok = rc != -1 ;
	if( ok )
	{
		if( rc != 0 )
			child_pid.m_pid = rc ;
	}
	else
	{
		throw CannotFork() ;
	}
	return rc == 0 ? Child : Parent ;
}

int G::Process::wait( const Id & child_pid )
{
	int status ;
	for(;;)
	{
		G_DEBUG( "G::Process::wait: waiting" ) ;
		int rc = ::waitpid( child_pid.m_pid , &status , 0 ) ;
		if( rc == -1 && errno_() == EINTR )
		{
			; // signal in parent -- keep waiting
		}
		else if( rc == -1 )
		{
			int error = errno_() ;
			std::ostringstream ss ;
			ss << "errno=" << error ;
			throw WaitError( ss.str() ) ;
		}
		else
		{
			break ;
		}
	}
	G_DEBUG( "G::Process::wait: done" ) ;

	if( ! WIFEXITED(status) )
	{
		// uncaught signal or stopped
		std::ostringstream ss ;
		ss << "status=" << status ;
		throw ChildError( ss.str() ) ;
	}

	const int exit_status = WEXITSTATUS(status) ;
	return exit_status ;
}

int G::Process::wait( const Id & child_pid , int error_return )
{
	try
	{
		return wait( child_pid ) ;
	}
	catch(...)
	{
	}
	return error_return ;
}

//static
int G::Process::errno_()
{
	return errno ; // not ::errno or std::errno for gcc2.95
}

int G::Process::spawn( Identity nobody , const Path & exe , const Strings & args , 
	std::string * pipe_result_p , int error_return , std::string (*fn)(int) )
{
	if( exe.isRelative() )
		throw InvalidPath( exe.str() ) ;

	if( Identity::effective().isRoot() || nobody.isRoot() )
		throw Insecure() ;

	Pipe pipe( pipe_result_p != NULL ) ;
	Id child_pid ;
	if( fork(child_pid) == Child )
	{
		try
		{
			beNobody( nobody ) ;
			G_ASSERT( ::getuid() != 0U && ::geteuid() != 0U ) ;
			pipe.inChild() ;
			closeFiles( pipe.fd() ) ;
			pipe.dup() ; // dup() onto stdout
			int error = execCore( exe , args ) ;
			if( fn != 0 )
			{
				std::string s = (*fn)(error) ;
				::write( STDOUT_FILENO , s.c_str() , s.length() ) ;
			}
		}
		catch(...)
		{
		}
		::_exit( error_return ) ;
		return error_return ; // pacify the compiler
	}
	else
	{
		pipe.inParent() ;
		int exit_status = wait( child_pid , error_return ) ;
		if( pipe_result_p != NULL ) *pipe_result_p = pipe.read() ;
		return exit_status ;
	}
}

int G::Process::execCore( const G::Path & exe , const Strings & args )
{
	char * env[3U] ;
	std::string path( "PATH=/usr/bin:/bin" ) ; // no "."
	std::string ifs( "IFS= \t\n" ) ;
	env[0U] = const_cast<char*>( path.c_str() ) ;
	env[1U] = const_cast<char*>( ifs.c_str() ) ;
	env[2U] = NULL ;

	char ** argv = new char* [ args.size() + 2U ] ;
	argv[0U] = const_cast<char*>( exe.pathCstr() ) ;
	unsigned int argc = 1U ;
	for( Strings::const_iterator arg_p = args.begin() ; arg_p != args.end() ; ++arg_p , argc++ )
		argv[argc] = const_cast<char*>(arg_p->c_str()) ;
	argv[argc] = NULL ;

	::execve( exe.str().c_str() , argv , env ) ;
	const int error = errno_() ;
	delete [] argv ;

	G_DEBUG( "G::Process::exec: execve() returned: errno=" << error << ": " << exe ) ;
	return error ;
}

std::string G::Process::strerror( int errno_ )
{
	char * p = ::strerror( errno_ ) ;
	return std::string( p ? p : "" ) ;
}

void G::Process::beSpecial( Identity identity , bool change_group )
{
	const bool do_throw = false ; // only works if really root or if executable is suid
	setEffectiveUserTo( identity , do_throw ) ;
	if( change_group) setEffectiveGroupTo( identity , do_throw ) ;
}

void G::Process::revokeExtraGroups()
{
	if( Identity::real().isRoot() || Identity::effective() != Identity::real() )
	{
		gid_t dummy ;
		G_IGNORE ::setgroups( 0U , &dummy ) ; // (only works for root, so ignore the return code)
	}
}

G::Identity G::Process::beOrdinary( Identity nobody , bool change_group )
{
	Identity special_identity( Identity::effective() ) ;
	if( Identity::real().isRoot() )
	{
		setEffectiveUserTo( Identity::root() ) ;
		if( change_group ) 
		setEffectiveGroupTo( nobody ) ;
		setEffectiveUserTo( nobody ) ;
	}
	else
	{
		setEffectiveUserTo( Identity::real() ) ;
		if( change_group )
		setEffectiveGroupTo( Identity::real() ) ;
	}
	return special_identity ;
}

void G::Process::beNobody( Identity nobody )
{
	// this private method is only used before an exec()
	// so the effective ids are lost anyway -- the
	// Identity will only be valid if getuid() is zero

	if( Identity::real().isRoot() )
	{
		setEffectiveUserTo( Identity::root() ) ;
		setRealGroupTo( nobody ) ;
		setRealUserTo( nobody ) ;
	}
}

// ===

G::Process::Id::Id()
{
	m_pid = ::getpid() ;
}

G::Process::Id::Id( const char * path ) :
	m_pid(0)
{
	// reentrant implementation suitable for a signal handler...
	int fd = ::open( path ? path : "" , O_RDONLY ) ;
	const size_t buffer_size = 11U ;
	char buffer[buffer_size] ;
	buffer[0U] = '\0' ;
	ssize_t rc = ::read( fd , buffer , buffer_size - 1U ) ;
	::close( fd ) ;
	for( const char * p = buffer ; rc > 0 && *p >= '0' && *p <= '9' ; p++ , rc-- )
	{
		m_pid *= 10 ;
		m_pid += ( *p - '0' ) ;
	}
}

G::Process::Id::Id( std::istream & stream )
{
	stream >> m_pid ;
	if( !stream.good() )
		throw Process::InvalidId() ;
}

std::string G::Process::Id::str() const
{
	std::ostringstream ss ;
	ss << m_pid ;
	return ss.str() ;
}

bool G::Process::Id::operator==( const Id & other ) const
{
	return m_pid == other.m_pid ;
}

// ===

class G::Process::Umask::UmaskImp // A private implementation class used by G::Process::Umask. 
{
public:
	mode_t m_old_mode ;
} ;

G::Process::Umask::Umask( Mode mode ) :
	m_imp(new UmaskImp)
{
	m_imp->m_old_mode = 
		::umask( mode==Readable?0133:(mode==Tighter?0117:0177) ) ;
}

G::Process::Umask::~Umask()
{
	G_IGNORE ::umask( m_imp->m_old_mode ) ;
	delete m_imp ;
}

//static
void G::Process::Umask::set( Mode mode )
{
	// Tightest: -rw------- 
	// Tighter:  -rw-rw----
	// Readable: -rw-r--r--
	::umask( mode==Readable?0133:(mode==Tighter?0117:0177) ) ;
}

// ===

G::Pipe::Pipe( bool active ) : 
	m_fd(-1) 
{ 
	m_fds[0] = m_fds[1] = -1 ;
	if( active && ::pipe( m_fds ) < 0 ) 
		throw Error() ; 
	G_DEBUG( "G::Pipe::ctor: " << m_fds[0] << " " << m_fds[1] ) ;
}

G::Pipe::~Pipe()
{
	if( m_fds[0] >= 0 ) ::close( m_fds[0] ) ;
	if( m_fds[1] >= 0 ) ::close( m_fds[1] ) ;
}

void G::Pipe::inChild() 
{ 
	::close( m_fds[0] ) ; 
	m_fds[0] = -1 ;
	m_fd = m_fds[1] ; // writer
}

void G::Pipe::inParent() 
{ 
	::close( m_fds[1] ) ; 
	m_fds[1] = -1 ;
	m_fd = m_fds[0] ; // reader
}

int G::Pipe::fd() const
{ 
	return m_fd ; 
}

void G::Pipe::dup()
{ 
	if( m_fd != -1 && m_fd != STDOUT_FILENO )
	{
		if( ::dup2(m_fd,STDOUT_FILENO) != STDOUT_FILENO )
			throw Error() ;
		::close( m_fd ) ;
		m_fd = -1 ;
		m_fds[1] = -1 ;
		noCloseOnExec( STDOUT_FILENO ) ;
	}
}

std::string G::Pipe::read()
{
	char buffer[4096] ;
	int rc = m_fd == -1 ? 0 : ::read( m_fd , buffer , sizeof(buffer) ) ;
	if( rc < 0 ) throw Error("read") ;
	return std::string(buffer,rc) ;
}

