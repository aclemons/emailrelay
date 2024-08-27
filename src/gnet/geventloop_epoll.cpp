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
/// \file geventloop_epoll.cpp
///

#include "gdef.h"
#include "gevent.h"
#include "gscope.h"
#include "gexception.h"
#include "gtimerlist.h"
#include "gprocess.h"
#include "glog.h"
#include "gassert.h"
#include <algorithm>
#include <vector>
#include <limits>
#include <sys/epoll.h>

namespace GNet
{
	class EventLoopImp ;
}

class GNet::EventLoopImp : public EventLoop
{
public:
	G_EXCEPTION( Error , tx("epoll error") )
	EventLoopImp() ;
	~EventLoopImp() override ;

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
	EventLoopImp( const EventLoopImp & ) = delete ;
	EventLoopImp( EventLoopImp && ) = delete ;
	EventLoopImp & operator=( const EventLoopImp & ) = delete ;
	EventLoopImp & operator=( EventLoopImp && ) = delete ;

private:
	struct ListItem
	{
		unsigned int m_events {0U} ;
		EventHandler * m_handler {nullptr} ;
		EventState m_es {EventState::Private(),nullptr,nullptr} ;
		int m_suppress_read {-1} ;
		int m_suppress_write {-1} ;
		void update( EventHandler * handler , EventState es ) noexcept { m_handler = handler ; m_es = es ; }
		void disarm( ExceptionHandler * eh ) noexcept { if( m_es.eh() == eh ) m_es.disarm() ; }
		void reset() noexcept { m_handler = nullptr ; }
	} ;
	using List = std::vector<ListItem> ;

private:
	void runOnce() ;
	ListItem * find( Descriptor ) noexcept ;
	ListItem & findOrCreate( Descriptor ) ;
	int ms() const ;
	static int ms( unsigned int , unsigned int ) noexcept ;
	static void fdupdate( int , int fd , unsigned int old_events , unsigned int new_events ) ;
	static void fdupdate( int , int fd , unsigned int old_events , unsigned int new_events , std::nothrow_t ) noexcept ;
	static void fdadd( int , int fd , unsigned int events ) ;
	static void fdmodify( int , int fd , unsigned int events ) ;
	static int fdmodify( int , int fd , unsigned int events , std::nothrow_t ) noexcept ;
	static void fdremove( int , int fd ) noexcept ;

private:
	std::vector<struct epoll_event> m_wait_events ;
	int m_epoll_fd {-1} ;
	bool m_running {false} ;
	bool m_quit {false} ;
	std::string m_quit_reason ;
	List m_list ;
	int m_wait_rc {0} ;
	int m_index {-1} ;
	int m_suppress_seq {0} ;
	EventState m_es_current ;
} ;

// ===

std::unique_ptr<GNet::EventLoop> GNet::EventLoop::create()
{
	return std::make_unique<EventLoopImp>() ;
}

// ===

GNet::EventLoopImp::EventLoopImp() :
	m_epoll_fd(epoll_create1(EPOLL_CLOEXEC)) ,
	m_es_current(EventState::Private(),nullptr,nullptr)
{
	if( m_epoll_fd == -1 )
		throw Error( "epoll_create" ) ;
	m_list.reserve( 1024U ) ;
}

GNet::EventLoopImp::~EventLoopImp()
{
	close( m_epoll_fd ) ;
}

bool GNet::EventLoopImp::running() const
{
	return m_running ;
}

std::string GNet::EventLoopImp::run()
{
	G::ScopeExitSetFalse running( m_running = true ) ;
	m_quit = false ;
	while( !m_quit )
	{
		runOnce() ;
	}
	std::string quit_reason = m_quit_reason ;
	m_quit_reason.clear() ;
	m_quit = false ;
	return quit_reason ;
}

void GNet::EventLoopImp::runOnce()
{
	// make the output array big enough for the largest file descriptor --
	// probably better than trying to count non-negative fds
	m_wait_events.resize( std::max(std::size_t(1U),m_list.size()) ) ;

	// extract the pending events
	int timeout_ms = ms() ;
	m_wait_rc = epoll_wait( m_epoll_fd , m_wait_events.data() , m_wait_events.size() , timeout_ms ) ; // NOLINT narrowing
	if( m_wait_rc < 0 )
	{
		int e = G::Process::errno_() ;
		if( e != EINTR )
			throw Error( "epoll_wait" , G::Process::strerror(e) ) ;
	}

	// handle timer events
	if( m_wait_rc == 0 || timeout_ms == 0 )
	{
		TimerList::instance().doTimeouts() ;
	}

	// shenanigans for O(1) callback suppression after drop()
	m_suppress_seq++ ;
	if( m_suppress_seq == std::numeric_limits<int>::max() )
	{
		m_suppress_seq = 0 ;
		std::for_each( m_list.begin() , m_list.end() ,
			[](ListItem & i_){ i_.m_suppress_read = i_.m_suppress_write = -1 ; } ) ;
	}

	// handle i/o events
	G_ASSERT( m_index == -1 ) ;
	G::ScopeExitSet<int,-1> resetter( m_index ) ;
	auto wait_event = m_wait_events.begin() ;
	for( m_index = 0 ; m_wait_rc > 0 && m_index < m_wait_rc ; m_index++ , ++wait_event )
	{
		Descriptor fdd( wait_event->data.fd ) ;
		if( wait_event->events & EPOLLIN )
		{
			ListItem * item = find( fdd ) ;
			if( item && item->m_suppress_read != m_suppress_seq && item->m_handler != nullptr )
			{
				m_es_current = item->m_es ; // see disarm()
				EventEmitter::raiseReadEvent( item->m_handler , m_es_current ) ;
			}
		}
		if( wait_event->events & EPOLLOUT )
		{
			ListItem * item = find( fdd ) ; // again
			if( item && item->m_suppress_write != m_suppress_seq && item->m_handler != nullptr )
			{
				m_es_current = item->m_es ; // see disarm()
				EventEmitter::raiseWriteEvent( item->m_handler , m_es_current ) ;
			}
		}
	}
}

int GNet::EventLoopImp::ms() const
{
	constexpr int infinite = -1 ;
	if( TimerList::ptr() )
	{
		auto pair = TimerList::instance().interval() ;
		if( pair.second )
			return infinite ;
		else if( pair.first.s() == 0 && pair.first.us() == 0U )
			return 0 ;
		else
			return std::max( 1 , ms(pair.first.s(),pair.first.us()) ) ;
	}
	else
	{
		return infinite ;
	}
}

int GNet::EventLoopImp::ms( unsigned int s , unsigned int us ) noexcept
{
	constexpr unsigned int s_max = static_cast<unsigned int>( std::numeric_limits<int>::max()/1000 - 1 ) ;
	static_assert( s_max > 600 , "" ) ; // sanity check that clipping at more than ten mins
	return
		s >= s_max ?
			std::numeric_limits<int>::max() :
			static_cast<int>( (s*1000U) + ((us+999U)/1000U) ) ;
}

void GNet::EventLoopImp::quit( const std::string & reason )
{
	m_quit_reason = reason ;
	m_quit = true ;
}

void GNet::EventLoopImp::quit( const G::SignalSafe & )
{
	m_quit = true ;
}

GNet::EventLoopImp::ListItem * GNet::EventLoopImp::find( Descriptor fdd ) noexcept
{
	std::size_t ufd = static_cast<unsigned int>(fdd.fd()) ;
	return fdd.fd() >= 0 && ufd < m_list.size() ? &m_list[ufd] : nullptr ;
}

GNet::EventLoopImp::ListItem & GNet::EventLoopImp::findOrCreate( Descriptor fdd )
{
	ListItem * p = find( fdd ) ;
	if( p == nullptr )
	{
		std::size_t ufd = static_cast<unsigned int>(fdd.fd()) ;
		m_list.resize( std::max(m_list.size(),ufd+1U) ) ; // grow, not shrink
		p = &m_list[ufd] ;
		*p = ListItem() ;
	}
	return *p ;
}

void GNet::EventLoopImp::addRead( Descriptor fdd , EventHandler & handler , EventState es )
{
	G_ASSERT( fdd.fd() >= 0 ) ;
	handler.setDescriptor( fdd ) ; // see EventHandler::dtor
	unsigned int new_events = EPOLLIN ;
	ListItem & item = findOrCreate( fdd ) ;
	fdupdate( m_epoll_fd , fdd.fd() , item.m_events , item.m_events | new_events ) ;
	item.m_events |= new_events ;
	item.update( &handler , es ) ;
}

void GNet::EventLoopImp::addWrite( Descriptor fdd , EventHandler & handler , EventState es )
{
	G_ASSERT( fdd.fd() >= 0 ) ;
	handler.setDescriptor( fdd ) ; // see EventHandler::dtor
	unsigned int new_events = EPOLLOUT ;
	ListItem & item = findOrCreate( fdd ) ;
	fdupdate( m_epoll_fd , fdd.fd() , item.m_events , item.m_events | new_events ) ;
	item.m_events |= new_events ;
	item.update( &handler , es ) ;
}

void GNet::EventLoopImp::addOther( Descriptor , EventHandler & , EventState )
{
	// no-op
}

void GNet::EventLoopImp::dropRead( Descriptor fdd ) noexcept
{
	ListItem * item = find( fdd ) ;
	if( item && ( item->m_events & EPOLLIN ) )
	{
		unsigned int new_events = item->m_events & ~EPOLLIN ;
		fdupdate( m_epoll_fd , fdd.fd() , item->m_events , new_events , std::nothrow ) ;
		item->m_events = new_events ;
		item->m_suppress_read = m_suppress_seq ;
	}
}

void GNet::EventLoopImp::dropWrite( Descriptor fdd ) noexcept
{
	ListItem * item = find( fdd ) ;
	if( item && ( item->m_events & EPOLLOUT ) )
	{
		unsigned int new_events = item->m_events & ~EPOLLOUT ;
		fdupdate( m_epoll_fd , fdd.fd() , item->m_events , new_events , std::nothrow ) ;
		item->m_events = new_events ;
		item->m_suppress_write = m_suppress_seq ;
	}
}

void GNet::EventLoopImp::dropOther( Descriptor ) noexcept
{
	// no-op
}

void GNet::EventLoopImp::drop( Descriptor fdd ) noexcept
{
	ListItem * item = find( fdd ) ;
	if( item )
	{
		if( item->m_events )
			fdremove( m_epoll_fd , fdd.fd() ) ;
		item->m_events = 0U ;
		item->reset() ;
		item->m_suppress_read = m_suppress_seq ;
		item->m_suppress_write = m_suppress_seq ;
	}
}

void GNet::EventLoopImp::disarm( ExceptionHandler * eh ) noexcept
{
	if( m_es_current.eh() == eh )
		m_es_current.disarm() ;

	for( auto & list_item : m_list )
		list_item.disarm( eh ) ;
}

// --

void GNet::EventLoopImp::fdupdate( int epoll_fd , int fd , unsigned int old_events , unsigned int new_events )
{
	if( new_events == 0U )
		fdremove( epoll_fd , fd ) ;
	else if( old_events == 0U )
		fdadd( epoll_fd , fd , new_events ) ;
	else
		fdmodify( epoll_fd , fd , new_events ) ;
}

void GNet::EventLoopImp::fdupdate( int epoll_fd , int fd , unsigned int , unsigned int new_events , std::nothrow_t ) noexcept
{
	if( new_events == 0U )
		fdremove( epoll_fd , fd ) ;
	else
		fdmodify( epoll_fd , fd , new_events , std::nothrow ) ;
}

void GNet::EventLoopImp::fdadd( int epoll_fd , int fd , unsigned int events )
{
	epoll_event event {} ;
	event.data.fd = fd ;
	event.events = events ;
	int rc = epoll_ctl( epoll_fd , EPOLL_CTL_ADD , fd , &event ) ;
	if( rc == -1 )
	{
		int e = G::Process::errno_() ;
		throw Error( "epoll_ctl" , "add" , G::Process::strerror(e) ) ;
	}
}

void GNet::EventLoopImp::fdmodify( int epoll_fd , int fd , unsigned int events )
{
	epoll_event event {} ;
	event.data.fd = fd ;
	event.events = events ;
	int rc = epoll_ctl( epoll_fd , EPOLL_CTL_MOD , fd , &event ) ;
	if( rc == -1 )
	{
		int e = G::Process::errno_() ;
		throw Error( "epoll_ctl" , "modify" , G::Process::strerror(e) ) ;
	}
}

int GNet::EventLoopImp::fdmodify( int epoll_fd , int fd , unsigned int events , std::nothrow_t ) noexcept
{
	epoll_event event {} ;
	event.data.fd = fd ;
	event.events = events ;
	int rc = epoll_ctl( epoll_fd , EPOLL_CTL_MOD , fd , &event ) ;
	int e = G::Process::errno_() ;
	return rc == -1 ? e : 0 ;
}

void GNet::EventLoopImp::fdremove( int epoll_fd , int fd ) noexcept
{
	epoll_event event {} ;
	epoll_ctl( epoll_fd , EPOLL_CTL_DEL , fd , &event ) ;
}

