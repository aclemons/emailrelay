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

// Class: G::CleanupImp
// Description: A private implementation class used by G::Cleanup.
//
class G::CleanupImp 
{
public:
	static void add( void (*fn)(const char*) , const char * ) ;
	static void installDefault( int ) ;
	static void callHandlers() ;

private:
	static void init() ;
	static void install( int , Handler ) ;
	static void installHandler( int ) ;
	static bool ignored( int ) ;

private:
	typedef Cleanup::Error Error ;
	struct Link // A private linked-list structure used by G::CleanupImp.
	{
		void (*fn)(const char*) ;
		const char * arg ;
		Link * next ;
	} ;
	static Link * m_head ;
	static Link * m_tail ;
} ;

G::CleanupImp::Link * G::CleanupImp::m_head = NULL ;
G::CleanupImp::Link * G::CleanupImp::m_tail = NULL ;

// ===

void G::Cleanup::add( void (*fn)(const char*) , const char * arg )
{
	CleanupImp::add( fn , arg ) ;
}

// ===

void G::CleanupImp::init()
{
	installHandler( SIGTERM ) ;
	installHandler( SIGINT ) ;
	installHandler( SIGHUP ) ;
	installHandler( SIGPIPE ) ;
	installHandler( SIGQUIT ) ;
	//installHandler( SIGUSR1 ) ;
	//installHandler( SIGUSR2 ) ;
}

void G::CleanupImp::add( void (*fn)(const char*) , const char * arg )
{
	Link * p = new Link ;
	p->fn = fn ;
	p->arg = arg ;
	p->next = NULL ;

	// (this bit should be protected from signals)
	if( m_head == NULL ) init() ;
	if( m_tail != NULL ) m_tail->next = p ;
	m_tail = p ;
	if( m_head == NULL ) m_head = p ;
}

void G::CleanupImp::installHandler( int signum )
{
	if( ignored(signum) )
		G_DEBUG( "G::CleanupImp::installHandler: signal " << signum << " is ignored" ) ;
	else
		install( signum , gcleanup_unix_handler_ ) ;
}

void G::CleanupImp::installDefault( int signum )
{
	install( signum , NULL ) ;
}

bool G::CleanupImp::ignored( int signum )
{
	static struct sigaction zero_action ;
	struct sigaction action( zero_action ) ;
	if( ::sigaction( signum , NULL , &action ) )
		throw Error( "sigaction" ) ;
	return action.sa_handler == SIG_IGN ;
}

void G::CleanupImp::install( int signum , Handler fn )
{
	static struct sigaction zero_action ;
	struct sigaction action( zero_action ) ;
	action.sa_handler = fn ? fn : SIG_DFL ;
	if( ::sigaction( signum , &action , NULL ) && fn != NULL )
		throw Error( "sigaction" ) ;
}

void G::CleanupImp::callHandlers()
{
	G::Root claim_root ;
	for( const Link * p = m_head ; p != NULL ; p = p->next )
	{
		(*(p->fn))(p->arg) ;
	}
}

extern "C"
void gcleanup_unix_handler_( int signum )
{
	try
	{
		G::CleanupImp::callHandlers() ;
		G::CleanupImp::installDefault( signum ) ;
		::raise( signum ) ;
	}
	catch(...)
	{
	}
}

