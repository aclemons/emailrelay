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
/// \file gcleanup_unix.cpp
///

#include "gdef.h"
#include "gcleanup.h"
#include "gprocess.h"
#include "groot.h"
#include "glog.h"
#include <cstring> // ::strdup()
#include <csignal> // ::sigaction() etc
#include <array>

extern "C"
{
	void gcleanup_handler( int signum ) ;
	using Handler = void (*)(int) ;
}

namespace G
{
	class CleanupImp ;
}

//| \class G::CleanupImp
/// A static implementation class used by G::Cleanup.
///
class G::CleanupImp
{
public:
	static void add( bool (*fn)(SignalSafe,const char*) , const char * ) ;
		// Adds a cleanup function.

	static void installDefault( const SignalSafe & , int ) ;
		// Installs the SIG_DFL signal handler for the given signal.

	static void installDefault( int ) ;
		// Installs the SIG_DFL signal handler for the given signal.

	static void installIgnore( int ) ;
		// Installs the SIG_IGN signal handler for the given signal.

	static void callHandlers() ;
		// Calls all the cleanup functions. Any that fail are automatically
		// retried with Root::atExit().

	static bool callHandlersOnce( SignalSafe ) ;
		// Calls all the cleanup functions.

	static void atexit( bool active ) ;
		// Registers callHandlers() with atexit(3) if the active parameter is true.

	static void block() noexcept ;
		// Blocks signals until released.

	static void release() noexcept ;
		// Releases blocked signals.

	static const char * strdup_ignore_leaks( const char * p ) ;
		// A strdup() function.

private:
	struct Link /// A private linked-list structure used by G::CleanupImp.
	{
		bool (*fn)(SignalSafe,const char*) ;
		const char * arg ;
		Link * next ;
		bool done ;
	} ;

private:
	static void init() ;
	static void install( int , Handler , bool ) ;
	static void installHandler( int ) ;
	static bool ignored( int ) ;
	static void atexitHandler() ;
	static Link * new_link_ignore_leak() ;

private:
	static Link * m_head ;
	static Link * m_tail ;
	static bool m_atexit_active ;
	static bool m_atexit_installed ;
	static std::array<int,4U> m_signals ;
} ;

std::array<int,4U> G::CleanupImp::m_signals = {{ SIGTERM , SIGINT , SIGHUP , SIGQUIT }} ;
G::CleanupImp::Link * G::CleanupImp::m_head = nullptr ;
G::CleanupImp::Link * G::CleanupImp::m_tail = nullptr ;
bool G::CleanupImp::m_atexit_installed = false ;
bool G::CleanupImp::m_atexit_active = false ;

// ===

void G::Cleanup::init()
{
	CleanupImp::installIgnore( SIGPIPE ) ;
}

void G::Cleanup::add( bool (*fn)(SignalSafe,const char*) , const char * arg )
{
	CleanupImp::add( fn , arg ) ;
}

#ifndef G_LIB_SMALL
void G::Cleanup::atexit( bool active )
{
	CleanupImp::atexit( active ) ;
}
#endif

void G::Cleanup::block() noexcept
{
	CleanupImp::block() ;
}

void G::Cleanup::release() noexcept
{
	CleanupImp::release() ;
}

#ifndef G_LIB_SMALL
const char * G::Cleanup::strdup( const char * p )
{
	return CleanupImp::strdup_ignore_leaks( p ) ;
}
#endif

const char * G::Cleanup::strdup( const std::string & s )
{
	return CleanupImp::strdup_ignore_leaks( s.c_str() ) ;
}

// ===

void G::CleanupImp::init()
{
	// install our meta-handler for signals that normally terminate the process,
	// except for sigpipe which we ignore
	//
	installIgnore( SIGPIPE ) ;
	for( int s : m_signals )
		installHandler( s ) ;
}

void G::CleanupImp::add( bool (*fn)(SignalSafe,const char*) , const char * arg )
{
	Link * p = new_link_ignore_leak() ;
	p->fn = fn ;
	p->arg = arg ;
	p->next = nullptr ;
	p->done = false ;

	Cleanup::Block block ;
	if( m_head == nullptr ) init() ;
	if( m_tail != nullptr ) m_tail->next = p ;
	m_tail = p ;
	if( m_head == nullptr ) m_head = p ;
}

G::CleanupImp::Link * G::CleanupImp::new_link_ignore_leak()
{
	return new Link ; // ignore leak
}

void G::CleanupImp::installHandler( int signum )
{
	if( ignored(signum) )
		G_DEBUG( "G::CleanupImp::installHandler: signal " << signum << " is ignored" ) ;
	else
		install( signum , gcleanup_handler , true ) ;
}

bool G::CleanupImp::ignored( int signum )
{
	struct ::sigaction action {} ;
	if( ::sigaction( signum , nullptr , &action ) != 0 )
		throw Cleanup::Error( "sigaction" ) ;
	return action.sa_handler == SIG_IGN ; // NOLINT
}

#ifndef G_LIB_SMALL
void G::CleanupImp::installDefault( int signum )
{
	install( signum , SIG_DFL , true ) ;
}
#endif

#ifndef G_LIB_SMALL
void G::CleanupImp::installDefault( const G::SignalSafe & , int signum )
{
	install( signum , SIG_DFL , false ) ;
}
#endif

void G::CleanupImp::installIgnore( int signum )
{
	install( signum , SIG_IGN , true ) ; // NOLINT
}

void G::CleanupImp::install( int signum , Handler fn , bool do_throw )
{
	// install the given handler, or the system default if null
	struct ::sigaction action {} ;
	action.sa_handler = fn ;
	if( ::sigaction( signum , &action , nullptr ) != 0 && do_throw )
		throw Cleanup::Error( "sigaction" ) ;
}

void G::CleanupImp::atexit( bool active )
{
	if( active && !m_atexit_installed )
	{
		m_atexit_installed = true ;
		::atexit( atexitHandler ) ;
	}
	m_atexit_active = active ;
}

void G::CleanupImp::atexitHandler()
{
	if( m_atexit_active )
		callHandlers() ;
}

void G::CleanupImp::callHandlers()
{
	if( !callHandlersOnce( SignalSafe() ) )
	{
		Root::atExit( SignalSafe() ) ;
		callHandlersOnce( SignalSafe() ) ;
	}
}

bool G::CleanupImp::callHandlersOnce( SignalSafe )
{
	bool all_ok = true ;
	for( Link * p = m_head ; p != nullptr ; p = p->next )
	{
		try
		{
			if( !p->done && (*(p->fn))(SignalSafe(),p->arg) )
				p->done = true ;
			else
				all_ok = false ;
		}
		catch(...)
		{
		}
	}
	return all_ok ;
}

extern "C" void gcleanup_handler( int signum )
{
	// call the registered handler(s) and exit
	try
	{
		G::CleanupImp::callHandlers() ;
		std::_Exit( signum + 128 ) ;
	}
	catch(...)
	{
	}
}

void G::CleanupImp::block() noexcept
{
	sigset_t set ;
	sigemptyset( &set ) ;
	for( int s : m_signals )
	{
		sigaddset( &set , s ) ;
	}
	gdef_pthread_sigmask( SIG_BLOCK , &set , nullptr ) ; // gdef.h
}

void G::CleanupImp::release() noexcept
{
	sigset_t emptyset ;
	sigemptyset( &emptyset ) ;
	sigset_t set ;
	sigemptyset( &set ) ;
	gdef_pthread_sigmask( SIG_BLOCK , &emptyset , &set ) ;
	for( int s : m_signals )
	{
		sigdelset( &set , s ) ;
	}
	gdef_pthread_sigmask( SIG_SETMASK , &set , nullptr ) ;
}

const char * G::CleanupImp::strdup_ignore_leaks( const char * p )
{
	return ::strdup( p ) ; // NOLINT
}

