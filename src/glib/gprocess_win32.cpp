//
// Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gprocess_win32.cpp
//

#include "gdef.h"
#include "gprocess.h"
#include "gexception.h"
#include "gstr.h"
#include "glog.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <process.h>
#include <io.h>
#include <fcntl.h>

namespace G
{
	const int g_stderr_fileno = 2 ;
	const int g_sc_open_max = 256 ; // 32 in limits.h !?
	class Pipe ;
} ;

class G::Pipe 
{
public:
	G_EXCEPTION( Error , "pipe error" ) ;
	explicit Pipe( bool active ) ;
	~Pipe() ;
	int fd() const ;
	std::string read() ;
private:
	bool m_active ;
	int m_fds[2] ;
	int m_fd_writer ;
} ;

class G::Process::IdImp 
{
public: 
	unsigned int m_pid ;
} ;

// ===

G::Pipe::Pipe( bool active ) :
	m_active(active) ,
	m_fd_writer(-1)
{
	m_fds[0] = m_fds[1] = -1 ;
	if( m_active )
	{
		int rc = ::_pipe( m_fds , 256 , _O_BINARY | _O_NOINHERIT ) ;
		if( rc < 0 ) throw Error() ;
		m_fd_writer = ::_dup( m_fds[1] ) ; // inherited
		::_close( m_fds[1] ) ;
		m_fds[1] = -1 ;
	}
}

G::Pipe::~Pipe()
{
	if( m_active )
	{
		::_close( m_fds[0] ) ;
		::_close( m_fds[1] ) ;
		::_close( m_fd_writer ) ;
	}
}

int G::Pipe::fd() const
{
	return m_fd_writer ;
}

std::string G::Pipe::read()
{
	if( ! m_active ) return std::string() ;
	::_close( m_fd_writer ) ;
	char buffer[4096] ;
	int rc = m_fds[0] == -1 ? 0 : ::_read( m_fds[0] , buffer , sizeof(buffer) ) ;
	if( rc < 0 ) throw Error() ;
	return std::string( buffer , rc ) ;
}

// ===

G::Process::Id::Id() 
{
	m_pid = static_cast<unsigned int>(::_getpid()) ; // or ::GetCurrentProcessId()
}

G::Process::Id::Id( const char * path ) :
	m_pid(0)
{
	std::ifstream file( path ? path : "" ) ;
	file >> m_pid ;
	if( !file.good() )
		m_pid = 0 ;
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

bool G::Process::Id::operator==( const Id & rhs ) const
{
	return m_pid == rhs.m_pid ;
}

// not implemented...
//G::Process::Id::Id( const char * pid_file_path ) {} 

// ===

void G::Process::closeFiles( bool keep_stderr )
{
	const int n = g_sc_open_max ;
	for( int fd = 0 ; fd < n ; fd++ )
	{
		if( !keep_stderr || fd != g_stderr_fileno )
			::_close( fd ) ;
	}
}

void G::Process::closeStderr()
{
	int fd = g_stderr_fileno ;
	::_close( fd ) ;
}

void G::Process::cd( const Path & dir )
{
	if( !cd(dir,NoThrow()) )
		throw CannotChangeDirectory( dir.str() ) ;
}

bool G::Process::cd( const Path & dir , NoThrow )
{
	return 0 == ::_chdir( dir.str().c_str() ) ;
}

int G::Process::errno_()
{
	return errno ;
}

int G::Process::spawn( Identity , const Path & exe , const Strings & args_ , 
	std::string * pipe_result_p , int error_return )
{
	// open file descriptors are inherited across ::_spawn() --
	// no fcntl() is available to set close-on-exec -- but see 
	// also ::CreateProcess()

	Strings args( args_ ) ; // non-const copy
	Pipe pipe( pipe_result_p != NULL ) ;
	if( pipe_result_p != NULL )
		args.push_front( Str::fromInt(pipe.fd()) ) ; // kludge -- child must write on fd passed as argv[1]

	G_DEBUG( "G::Process::spawn: \"" << exe << "\": \"" << Str::join(args,"\",\"") << "\"" ) ;

	char ** argv = new char* [ args.size() + 2U ] ;
	argv[0U] = const_cast<char*>( exe.pathCstr() ) ;
	unsigned int argc = 1U ;
	for( Strings::const_iterator arg_p = args.begin() ; arg_p != args.end() ; ++arg_p , argc++ )
		argv[argc] = const_cast<char*>(arg_p->c_str()) ;
	argv[argc] = NULL ;

	const int mode = _P_WAIT ;
	::_flushall() ;
	int rc = ::_spawnv( mode , exe.str().c_str() , argv ) ;
	int error = errno_() ;
	delete [] argv ;
	G_DEBUG( "G::Process::spawn: _spawnv(): rc=" << rc << ": errno=" << error ) ;

	if( pipe_result_p != NULL )
		*pipe_result_p = pipe.read() ;

	return rc < 0 ? error_return : rc ;
}

G::Identity G::Process::beOrdinary( Identity identity , bool )
{
	// not implemented
	return identity ;
}

void G::Process::beSpecial( Identity , bool )
{
	// not implemented
}

// not implemented...
// Who G::Process::fork() {}
// Who G::Process::fork( Id & child ) {}
// void G::Process::exec( const Path & exe , const std::string & arg ) {}
// int G::Process::wait( const Id & child ) {}
// int G::Process::wait( const Id & child , int error_return ) {}

// ===

G::Process::Umask::Umask( G::Process::Umask::Mode ) :
	m_imp(0)
{
}

G::Process::Umask::~Umask()
{
}

void G::Process::Umask::set( G::Process::Umask::Mode )
{
	// not implemented
}

