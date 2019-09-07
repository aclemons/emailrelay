//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
//
// gcleanup_unix.cpp
//

#include "gdef.h"
#include "gcleanup.h"
#include "gprocess.h"
#include "groot.h"
#include "glog.h"
#include <signal.h>

extern "C"
{
	void gcleanup_unix_handler_( int signum ) ;
	typedef void (*Handler)( int ) ;
}

namespace G
{
	class CleanupImp ;
}

/// \class G::CleanupImp
/// A pimple-pattern implementation class used by G::Cleanup.
///
class G::CleanupImp
{
public:
	static void add( void (*fn)(SignalSafe,const char*) , const char * ) ;
		// Adds a cleanup function.

	static void installDefault( const SignalSafe & , int ) ;
		// Installs the SIG_DFL signal handler for the given signal.

	static void installDefault( int ) ;
		// Installs the SIG_DFL signal handler for the given signal.

	static void installIgnore( int ) ;
		// Installs the SIG_IGN signal handler for the given signal.

	static void callHandlers( SignalSafe ) ;
		// Calls all the cleanup functions.

	static void atexit( bool active ) ;
		// Registers callHandlers() with atexit(3) if the active parameter is true.

private:
	struct Link /// A private linked-list structure used by G::CleanupImp.
	{
		void (*fn)(SignalSafe,const char*) ;
		const char * arg ;
		Link * next ;
	} ;
	struct BlockSignals /// A private implementation class that temporarily blocks signals.
	{
		BlockSignals() ;
		~BlockSignals() ;
	} ;

private:
	CleanupImp( const CleanupImp & ) g__eq_delete ;
	void operator=( const CleanupImp & ) g__eq_delete ;
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
} ;

G::CleanupImp::Link * G::CleanupImp::m_head = nullptr ;
G::CleanupImp::Link * G::CleanupImp::m_tail = nullptr ;
bool G::CleanupImp::m_atexit_installed = false ;
bool G::CleanupImp::m_atexit_active = false ;

// ===

void G::Cleanup::init()
{
	CleanupImp::installIgnore( SIGPIPE ) ;
}

void G::Cleanup::add( void (*fn)(SignalSafe,const char*) , const char * arg )
{
	CleanupImp::add( fn , arg ) ; // was if(arg!=nullptr)
}

void G::Cleanup::atexit( bool active )
{
	CleanupImp::atexit( active ) ;
}

// ===

void G::CleanupImp::init()
{
	// install our meta-handler for signals that normally terminate the process,
	// except for sigpipe which we ignore
	//
	installIgnore( SIGPIPE ) ;
	installHandler( SIGTERM ) ;
	installHandler( SIGINT ) ;
	installHandler( SIGHUP ) ;
	installHandler( SIGQUIT ) ;
	//installHandler( SIGUSR1 ) ;
	//installHandler( SIGUSR2 ) ;
}

void G::CleanupImp::add( void (*fn)(SignalSafe,const char*) , const char * arg )
{
	Link * p = new_link_ignore_leak() ;
	p->fn = fn ;
	p->arg = arg ;
	p->next = nullptr ;

	// (all the signal-handling code treats the m_head/m_tail
	// list as immutable, so there are no locking issues here)

	if( m_head == nullptr ) init() ;
	if( m_tail != nullptr ) m_tail->next = p ;
	m_tail = p ;
	if( m_head == nullptr ) m_head = p ;
}

G::CleanupImp::Link * G::CleanupImp::new_link_ignore_leak()
{
	return new Link ;
}

void G::CleanupImp::installHandler( int signum )
{
	if( ignored(signum) )
		G_DEBUG( "G::CleanupImp::installHandler: signal " << signum << " is ignored" ) ;
	else
		install( signum , gcleanup_unix_handler_ , true ) ;
}

bool G::CleanupImp::ignored( int signum )
{
	static struct sigaction zero_action ;
	struct sigaction action( zero_action ) ;
	if( ::sigaction( signum , nullptr , &action ) )
		throw Cleanup::Error( "sigaction" ) ;
	return action.sa_handler == SIG_IGN ;
}

void G::CleanupImp::installDefault( int signum )
{
	install( signum , SIG_DFL , true ) ;
}

void G::CleanupImp::installDefault( const G::SignalSafe & , int signum )
{
	install( signum , SIG_DFL , false ) ;
}

void G::CleanupImp::installIgnore( int signum )
{
	install( signum , SIG_IGN , true ) ;
}

void G::CleanupImp::install( int signum , Handler fn , bool do_throw )
{
	// install the given handler, or the system default if null
	static struct sigaction zero_action ;
	struct sigaction action( zero_action ) ;
	action.sa_handler = fn ;
	if( ::sigaction( signum , &action , nullptr ) && do_throw )
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
		callHandlers( SignalSafe() ) ;
}

void G::CleanupImp::callHandlers( SignalSafe )
{
	Identity identity = Root::start( SignalSafe() ) ;
	for( const Link * p = m_head ; p != nullptr ; p = p->next )
	{
		try
		{
			(*(p->fn))(SignalSafe(),p->arg) ;
		}
		catch(...)
		{
		}
	}
	Root::stop( SignalSafe() , identity ) ;
}

extern "C"
void gcleanup_unix_handler_( int signum )
{
	// call the registered handler(s) and then do the system default action
	try
	{
		int e = G::Process::errno_( G::SignalSafe() ) ;
		G::CleanupImp::callHandlers( G::SignalSafe() ) ;
		G::CleanupImp::installDefault( G::SignalSafe() , signum ) ;
		G::Process::errno_( G::SignalSafe() , e ) ;
		::raise( signum ) ;
	}
	catch(...)
	{
	}
}

