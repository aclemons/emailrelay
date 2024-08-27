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
/// \file geventloop_select.cpp
///

#include "gdef.h"
#include "gscope.h"
#include "gevent.h"
#include "geventemitter.h"
#include "gprocess.h"
#include "gexception.h"
#include "gstr.h"
#include "gtimer.h"
#include "gtimerlist.h"
#include "gtest.h"
#include "glog.h"
#include "gassert.h"
#include <memory>
#include <sys/types.h>
#include <sys/time.h>

namespace GNet
{
	class EventLoopImp ;
}

class GNet::EventLoopImp : public EventLoop
{
public:
	G_EXCEPTION( Error , tx("select error") )
	EventLoopImp() ;

private: // overrides
	std::string run() override ;
	bool running() const override ;
	void quit( const std::string & ) override ;
	void quit( const G::SignalSafe & ) override ;
	void addRead( Descriptor , EventHandler & , EventState ) override ;
	void addWrite( Descriptor , EventHandler & , EventState ) override ;
	void addOther( Descriptor , EventHandler & , EventState ) override ;
	void dropRead( Descriptor ) noexcept override ;
	void dropWrite( Descriptor ) noexcept override ;
	void dropOther( Descriptor ) noexcept override ;
	void drop( Descriptor ) noexcept override ;
	void disarm( ExceptionHandler * ) noexcept override ;

public:
	~EventLoopImp() override = default ;
	EventLoopImp( const EventLoopImp & ) = delete ;
	EventLoopImp( EventLoopImp && ) = delete ;
	EventLoopImp & operator=( const EventLoopImp & ) = delete ;
	EventLoopImp & operator=( EventLoopImp && ) = delete ;

private:
	struct ListItem
	{
		EventHandler * m_handler {nullptr} ;
		EventState m_es {EventState::Private(),nullptr,nullptr} ;
		void update( EventHandler * handler , EventState es ) noexcept { m_handler = handler ; m_es = es ; }
	} ;
	using List = std::vector<ListItem> ; // indexed by fd
	void runOnce() ;
	static void addImp( int fd , EventHandler & , EventState , fd_set & , List & , int & ) ;
	static void disarmImp( List & , ExceptionHandler * ) noexcept ;
	static void disarmImp( EventState & , ExceptionHandler * ) noexcept ;
	static void dropImp( int fd , fd_set & , fd_set & , int & ) noexcept ;
	static int events( int nfds , fd_set * ) noexcept ;
	static int fdmaxof( int nfds , fd_set * ) noexcept ;
	static int fdmaxof( std::size_t nfds , fd_set * sp ) noexcept ;

private:
	bool m_quit {false} ;
	std::string m_quit_reason ;
	bool m_running {false} ;
	fd_set m_read_set ;
	fd_set m_write_set ;
	fd_set m_other_set ;
	int m_read_fdmax {-1} ;
	int m_write_fdmax {-1} ;
	int m_other_fdmax {-1} ;
	fd_set m_read_set_copy ;
	fd_set m_write_set_copy ;
	fd_set m_other_set_copy ;
	List m_read_list ;
	List m_write_list ;
	List m_other_list ;
	EventState m_es_current {EventState::Private(),nullptr,nullptr} ;
} ;

// ===

std::unique_ptr<GNet::EventLoop> GNet::EventLoop::create()
{
	return std::make_unique<EventLoopImp>() ;
}

// ===

GNet::EventLoopImp::EventLoopImp() // NOLINT cppcoreguidelines-pro-type-member-init
{
	FD_ZERO( &m_read_set ) ; // NOLINT
	FD_ZERO( &m_write_set ) ; // NOLINT
	FD_ZERO( &m_other_set ) ; // NOLINT
	FD_ZERO( &m_read_set_copy ) ; // NOLINT
	FD_ZERO( &m_write_set_copy ) ; // NOLINT
	FD_ZERO( &m_other_set_copy ) ; // NOLINT
	m_read_list.reserve( FD_SETSIZE ) ;
	m_write_list.reserve( FD_SETSIZE ) ;
	m_other_list.reserve( FD_SETSIZE ) ;
}

std::string GNet::EventLoopImp::run()
{
	G::ScopeExitSetFalse running( m_running = true ) ;
	do
	{
		runOnce() ;
	} while( !m_quit ) ;
	std::string quit_reason = m_quit_reason ;
	m_quit_reason.clear() ;
	m_quit = false ;
	return quit_reason ;
}

bool GNet::EventLoopImp::running() const
{
	return m_running ;
}

void GNet::EventLoopImp::quit( const std::string & reason )
{
	m_quit = true ;
	m_quit_reason = reason ;
}

void GNet::EventLoopImp::quit( const G::SignalSafe & )
{
	m_quit = true ;
}

void GNet::EventLoopImp::runOnce()
{
	// get a timeout interval() from TimerList
	//
	using Timeval = struct timeval ;
	Timeval timeout ;
	Timeval * timeout_p = nullptr ;
	bool immediate = false ;
	if( TimerList::ptr() != nullptr )
	{
		G::TimeInterval interval = G::TimeInterval::zero() ;
		bool infinite = false ;
		std::tie( interval , infinite ) = TimerList::instance().interval() ;
		timeout.tv_sec = interval.s() ;
		timeout.tv_usec = interval.us() ;
		timeout_p = infinite ? nullptr : &timeout ;
		immediate = !infinite && interval.s() == 0 && interval.us() == 0U ;
	}

	// find the highest fd value to pass to select() -- probably unnecessary
	// that this is the exact maximum rather than an upper bound, but
	// that's what is specified -- the fdmax data members are maintained by
	// addImp() but invalidated by dropImp() so we need to re-evaluate if
	// invalid -- we can use the size of the list as as upper bound for
	// the required fd_set tests
	//
	if( m_read_fdmax == -1 ) m_read_fdmax = fdmaxof( m_read_list.size() , &m_read_set ) ;
	if( m_write_fdmax == -1 ) m_write_fdmax = fdmaxof( m_write_list.size() , &m_write_set ) ;
	if( m_other_fdmax == -1 ) m_other_fdmax = fdmaxof( m_other_list.size() , &m_other_set ) ;
	int nfds = 1 + std::max( {m_read_fdmax,m_write_fdmax,m_other_fdmax} ) ;

	G_ASSERT( fdmaxof(FD_SETSIZE,&m_read_set) == m_read_fdmax ) ;
	G_ASSERT( fdmaxof(FD_SETSIZE,&m_write_set) == m_write_fdmax ) ;
	G_ASSERT( fdmaxof(FD_SETSIZE,&m_other_set) == m_other_fdmax ) ;
	G_ASSERT( m_read_list.size() >= std::size_t(m_read_fdmax+1) ) ;
	G_ASSERT( m_write_list.size() >= std::size_t(m_write_fdmax+1) ) ;
	G_ASSERT( m_other_list.size() >= std::size_t(m_other_fdmax+1) ) ;

	// do the select() -- use fd_set copies for the select() parameters because select()
	// modifies them, and in any case our originals might be modified as we iterate over
	// the results and call event handlers
	//
	m_read_set_copy = m_read_set ;
	m_write_set_copy = m_write_set ;
	m_other_set_copy = m_other_set ;
	int rc = ::select( nfds , &m_read_set_copy , &m_write_set_copy , &m_other_set_copy , timeout_p ) ;
	if( rc < 0 )
	{
		int e = G::Process::errno_() ;
		if( e != EINTR ) // eg. when profiling
			throw Error( G::Str::fromInt(e) ) ;
	}
	G_ASSERT( rc < 0 ||
		rc == (events(nfds,&m_read_set_copy)+events(nfds,&m_write_set_copy)+events(nfds,&m_other_set_copy)) ) ;

	// call the timeout handlers
	//
	if( rc == 0 || immediate )
	{
		TimerList::instance().doTimeouts() ;
	}

	// call the fd event handlers -- count them as we go (ecount) so that we don't
	// have to iterate over the whole set if we have handled the expected number
	// as returned by select() -- note that event handlers can remove fds from
	// the 'copy' sets (see dropRead() etc) but not add them -- that means ecount
	// might never reach the expected value and we do end up iterating over the
	// whole set, but it still works as an optimisation in the common case
	//
	int ecount = 0 ;
	for( int fd = 0 ; ecount < rc && fd < nfds ; fd++ )
	{
		if( FD_ISSET(fd,&m_read_set_copy) )
		{
			G_ASSERT( static_cast<unsigned int>(fd) < m_read_list.size() ) ;
			ecount++ ;
			m_es_current = m_read_list[fd].m_es ; // see disarm()
			EventEmitter::raiseReadEvent( m_read_list[fd].m_handler , m_es_current ) ;
		}
		if( FD_ISSET(fd,&m_write_set_copy) )
		{
			G_ASSERT( static_cast<unsigned int>(fd) < m_write_list.size() ) ;
			ecount++ ;
			m_es_current = m_write_list[fd].m_es ; // see disarm()
			EventEmitter::raiseWriteEvent( m_write_list[fd].m_handler , m_es_current ) ;
		}
		if( FD_ISSET(fd,&m_other_set_copy) )
		{
			G_ASSERT( static_cast<unsigned int>(fd) < m_other_list.size() ) ;
			ecount++ ;
			m_es_current = m_other_list[fd].m_es ; // see disarm()
			EventEmitter::raiseOtherEvent( m_other_list[fd].m_handler , m_es_current , EventHandler::Reason::other ) ;
		}
	}
}

int GNet::EventLoopImp::events( int nfds , fd_set * sp ) noexcept
{
	int n = 0 ;
	for( int fd = 0 ; fd < nfds ; fd++ )
	{
		if( FD_ISSET(fd,sp) )
			n++ ;
	}
	return n ;
}

inline int GNet::EventLoopImp::fdmaxof( std::size_t nfds , fd_set * sp ) noexcept
{
	return fdmaxof( static_cast<int>(nfds) , sp ) ;
}

int GNet::EventLoopImp::fdmaxof( int nfds , fd_set * sp ) noexcept
{
	int fdmax = -1 ;
	for( int fd = 0 ; fd < nfds ; fd++ )
	{
		if( FD_ISSET(fd,sp) )
			fdmax = fd ;
	}
	return fdmax ;
}

void GNet::EventLoopImp::addRead( Descriptor fdd , EventHandler & handler , EventState es )
{
	if( fdd.fd() >= 0 )
		addImp( fdd.fd() , handler , es , m_read_set , m_read_list , m_read_fdmax ) ;
}

void GNet::EventLoopImp::addWrite( Descriptor fdd , EventHandler & handler , EventState es )
{
	if( fdd.fd() >= 0 )
		addImp( fdd.fd() , handler , es , m_write_set , m_write_list , m_write_fdmax ) ;
}

void GNet::EventLoopImp::addOther( Descriptor fdd , EventHandler & handler , EventState es )
{
	if( fdd.fd() >= 0 )
		addImp( fdd.fd() , handler , es , m_other_set , m_other_list , m_other_fdmax ) ;
}

void GNet::EventLoopImp::addImp( int fd , EventHandler & handler , EventState es , fd_set & set , List & list , int & fdmax )
{
	G_ASSERT( fd >= 0 ) ;
	G_ASSERT( fdmax >= -1 ) ;
	if( fd >= FD_SETSIZE )
		throw EventLoop::Overflow( "too many open file descriptors for select()" ) ;

	// make sure drop() is called if the EventHandler goes away -- see EventHandler::dtor
	handler.setDescriptor( Descriptor(fd) ) ;

	// update the list
	if( list.size() < std::size_t(fd+1) )
		list.resize( fd+1 ) ;
	list[fd].update( &handler , es ) ;

	// update the set
	FD_SET( fd , &set ) ;
	fdmax = std::max( fdmax , fd ) ;

	G_ASSERT( list.size() >= static_cast<std::size_t>(fdmax+1) ) ;
}

void GNet::EventLoopImp::dropRead( Descriptor fdd ) noexcept
{
	if( fdd.fd() >= 0 )
		dropImp( fdd.fd() , m_read_set , m_read_set_copy , m_read_fdmax ) ;
}

void GNet::EventLoopImp::dropWrite( Descriptor fdd ) noexcept
{
	if( fdd.fd() >= 0 )
		dropImp( fdd.fd() , m_write_set , m_write_set_copy , m_write_fdmax ) ;
}

void GNet::EventLoopImp::dropOther( Descriptor fdd ) noexcept
{
	if( fdd.fd() >= 0 )
		dropImp( fdd.fd() , m_read_set , m_read_set_copy , m_read_fdmax ) ;
}

void GNet::EventLoopImp::dropImp( int fd , fd_set & set , fd_set & set_copy , int & fdmax ) noexcept
{
	G_ASSERT( fd >= 0 ) ;
	G_ASSERT( fdmax >= -1 ) ;

	// update the set
	FD_CLR( fd , &set ) ;
	FD_CLR( fd , &set_copy ) ; // dont deliver from the current result set
	if( fd == fdmax )
		fdmax = -1 ; // invalidate, force a re-evaluation before next use
}

void GNet::EventLoopImp::drop( Descriptor fdd ) noexcept
{
	if( fdd.fd() >= 0 )
	{
		// update the sets
		dropImp( fdd.fd() , m_read_set , m_read_set_copy , m_read_fdmax ) ;
		dropImp( fdd.fd() , m_write_set , m_write_set_copy , m_write_fdmax ) ;
		dropImp( fdd.fd() , m_other_set , m_other_set_copy , m_other_fdmax ) ;

		// update the lists since the handler is going away
		std::size_t ufd = static_cast<unsigned int>(fdd.fd()) ;
		if( ufd < m_read_list.size() )
			m_read_list[ufd].m_handler = nullptr ;
		if( ufd < m_write_list.size() )
			m_write_list[ufd].m_handler = nullptr ;
		if( ufd < m_other_list.size() )
			m_other_list[ufd].m_handler = nullptr ;
	}
}

void GNet::EventLoopImp::disarm( ExceptionHandler * eh ) noexcept
{
	// stop EventEmitter calling the specified exception handler
	disarmImp( m_es_current , eh ) ;

	// remove any other references -- this is overkill is most cases
	// because if the exception handler is going away then all
	// event handlers that might refer to it will have already
	// been dropped -- exception handlers tend to be long-lived
	// so any performance penalty is likely insignificant
	disarmImp( m_read_list , eh ) ;
	disarmImp( m_write_list , eh ) ;
	disarmImp( m_other_list , eh ) ;
}

void GNet::EventLoopImp::disarmImp( List & list , ExceptionHandler * eh ) noexcept
{
	for( auto & list_item : list )
	{
		if( list_item.m_es.eh() == eh )
			list_item.m_es.disarm() ;
	}
}

void GNet::EventLoopImp::disarmImp( EventState & es , ExceptionHandler * eh ) noexcept
{
	if( es.eh() == eh )
		es.disarm() ;
}

