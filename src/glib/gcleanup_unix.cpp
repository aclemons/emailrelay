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
/// \file gcleanup_unix.cpp
///

#include "gdef.h"
#include "gcleanup.h"
#include "gprocess.h"
#include "groot.h"
#include "glog.h"
#include "gassert.h"
#include <limits>
#include <new> // operator new()
#include <cstring> // std::memcpy()
#include <csignal> // ::sigaction() etc
#include <array>

extern "C"
{
	void gcleanup_handler( int signum ) ;
	using Handler = void (*)(int) ;
}

namespace G
{
	class CleanupImp
	{
		public:
			using Arg = Cleanup::Arg ;
			struct Link
			{
				Cleanup::Fn fn ;
				Arg arg ;
				Link * next ;
				bool done ;
			} ;

			static void add( Cleanup::Fn , Arg ) ;
			static void installDefault( const SignalSafe & , int ) noexcept ;
			static void installDefault( int ) ;
			static void installIgnore( int ) ;
			static void callHandlers() noexcept ;
			static bool callHandlersOnce( SignalSafe ) noexcept ;
			static void atexit( bool active ) ;
			static void block() noexcept ;
			static void release() noexcept ;
			static void init() ;
			static void install( int , Handler ) ;
			static void install( int , Handler , std::nothrow_t ) noexcept ;
			static void installHandler( int ) ;
			static bool ignored( int ) ;
			static void atexitHandler() noexcept ;
			static Arg duplicate( const char * , std::size_t , bool = false ) ;
			static Link * new_link_ignore_leak() ;
			static char * new_arg_ignore_leak( std::size_t ) ;

		private:
			static Link * m_head ;
			static Link * m_tail ;
			static bool m_atexit_active ;
			static bool m_atexit_installed ;
			static std::array<int,4U> m_signals ;
	} ;
}

G::CleanupImp::Link * G::CleanupImp::m_head = nullptr ;
G::CleanupImp::Link * G::CleanupImp::m_tail = nullptr ;
bool G::CleanupImp::m_atexit_active = false ;
bool G::CleanupImp::m_atexit_installed = false ;
std::array<int,4U> G::CleanupImp::m_signals = {{ SIGTERM , SIGINT , SIGHUP , SIGQUIT }} ;

// ===

void G::Cleanup::init()
{
	CleanupImp::installIgnore( SIGPIPE ) ;
}

void G::Cleanup::add( Fn fn , Arg arg )
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
G::Cleanup::Arg G::Cleanup::arg( const char * p )
{
	return CleanupImp::duplicate( p?p:"" , p?std::strlen(p):0U ) ;
}
#endif

#ifndef G_LIB_SMALL
G::Cleanup::Arg G::Cleanup::arg( const std::string & s )
{
	return CleanupImp::duplicate( s.data() , s.size() ) ;
}
#endif

G::Cleanup::Arg G::Cleanup::arg( const Path & p )
{
	std::string s = p.str() ;
	return CleanupImp::duplicate( s.data() , s.size() , true ) ;
}

#ifndef G_LIB_SMALL
G::Cleanup::Arg G::Cleanup::arg( std::nullptr_t )
{
	return CleanupImp::duplicate( nullptr , 0U ) ;
}
#endif

// ==

#ifndef G_LIB_SMALL
bool G::Cleanup::Arg::isPath() const noexcept
{
	return m_is_path ;
}
#endif

const char * G::Cleanup::Arg::str() const noexcept
{
	return m_ptr ;
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

void G::CleanupImp::add( Cleanup::Fn fn , Arg arg )
{
	// simple c-style data structures (with leaks) to avoid static destructors

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
	return new Link ; // NOLINT // ignore leak
}

char * G::CleanupImp::new_arg_ignore_leak( std::size_t n )
{
    return static_cast<char*>( operator new(n) ) ; // NOLINT // ignore leak
}

G::Cleanup::Arg G::CleanupImp::duplicate( const char * p , std::size_t n , bool is_path )
{
	G_ASSERT( (n+std::size_t(1U)) > n ) ;
	if( (n+std::size_t(1U)) <= n )
		throw Cleanup::Error( "numeric overflow" ) ;

    char * pp = new_arg_ignore_leak( n+1U ) ;
    if( p && n ) std::memcpy( pp , p , n ) ;
    pp[n] = '\0' ;

	Arg a ;
	a.m_ptr = pp ; // friend
	a.m_is_path = is_path ;
	return a ;
}

void G::CleanupImp::installHandler( int signum )
{
	if( ignored(signum) )
		G_DEBUG( "G::CleanupImp::installHandler: signal " << signum << " is ignored" ) ;
	else
		install( signum , gcleanup_handler ) ;
}

bool G::CleanupImp::ignored( int signum )
{
	struct ::sigaction action {} ;
	if( ::sigaction( signum , nullptr , &action ) != 0 )
		throw Cleanup::Error( "sigaction" ) ;
	return action.sa_handler == SIG_IGN ; // NOLINT macro shenanigans
}

#ifndef G_LIB_SMALL
void G::CleanupImp::installDefault( int signum )
{
	install( signum , SIG_DFL ) ;
}
#endif

#ifndef G_LIB_SMALL
void G::CleanupImp::installDefault( const G::SignalSafe & , int signum ) noexcept
{
	install( signum , SIG_DFL , std::nothrow ) ;
}
#endif

void G::CleanupImp::installIgnore( int signum )
{
	install( signum , SIG_IGN ) ; // NOLINT macro shenanigans
}

void G::CleanupImp::install( int signum , Handler fn )
{
	// install the given handler, or the system default if null
	struct ::sigaction action {} ;
	action.sa_handler = fn ;
	if( ::sigaction( signum , &action , nullptr ) != 0 )
		throw Cleanup::Error( "sigaction" ) ;
}

void G::CleanupImp::install( int signum , Handler fn , std::nothrow_t ) noexcept
{
	struct ::sigaction action {} ;
	action.sa_handler = fn ;
	::sigaction( signum , &action , nullptr ) ;
}

void G::CleanupImp::atexit( bool active )
{
	if( active && !m_atexit_installed )
	{
		m_atexit_installed = true ;
		::atexit( atexitHandler ) ; // NOLINT
	}
	m_atexit_active = active ;
}

void G::CleanupImp::atexitHandler() noexcept
{
	if( m_atexit_active )
		callHandlers() ;
}

void G::CleanupImp::callHandlers() noexcept
{
	if( !callHandlersOnce( SignalSafe() ) )
	{
		Root::atExit( SignalSafe() ) ;
		callHandlersOnce( SignalSafe() ) ;
	}
}

bool G::CleanupImp::callHandlersOnce( SignalSafe ) noexcept
{
	bool all_ok = true ;
	for( Link * p = m_head ; p != nullptr ; p = p->next )
	{
		try
		{
			if( !p->done && (*(p->fn))(p->arg) )
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
	static_assert( noexcept(G::CleanupImp::callHandlers()) , "" ) ;
	G::CleanupImp::callHandlers() ;
	std::_Exit( signum + 128 ) ;
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

