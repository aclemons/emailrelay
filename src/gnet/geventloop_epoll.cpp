//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include <sys/epoll.h>

namespace GNet
{
	class EventLoopImp ;
}

//| \class GNet::EventLoopImp
/// A concrete implementation of GNet::EventLoop using epoll() in its
/// implementation.
///
class GNet::EventLoopImp : public EventLoop
{
public:
	G_EXCEPTION( Error , tx("epoll error") ) ;
	EventLoopImp() ;
	~EventLoopImp() override ;

private: // overrides
	std::string run() override ;
	bool running() const override ;
	void quit( const std::string & ) override ;
	void quit( const G::SignalSafe & ) override ;
	void addRead( Descriptor fd , EventHandler & , ExceptionSink ) override ;
	void addWrite( Descriptor fd , EventHandler & , ExceptionSink ) override ;
	void addOther( Descriptor fd , EventHandler & , ExceptionSink ) override ;
	void dropRead( Descriptor fd ) noexcept override ;
	void dropWrite( Descriptor fd ) noexcept override ;
	void dropOther( Descriptor fd ) noexcept override ;
	void drop( Descriptor fd ) noexcept override ;
	void disarm( ExceptionHandler * ) noexcept override ;

public:
	EventLoopImp( const EventLoopImp & ) = delete ;
	EventLoopImp( EventLoopImp && ) = delete ;
	void operator=( const EventLoopImp & ) = delete ;
	void operator=( EventLoopImp && ) = delete ;

private:
	struct ListItem
	{
		unsigned int m_events {0U} ;
		EventEmitter m_read_emitter ;
		EventEmitter m_write_emitter ;
	} ;
	using List = std::vector<ListItem> ;

private:
	void runOnce() ;
	ListItem * find( int fd ) noexcept ;
	ListItem * findOrCreate( int fd ) ;
	void addReadWrite( int fd , EventHandler & , ExceptionSink , unsigned int new_events ) ;
	void dropReadWrite( int , unsigned int old_events ) noexcept ;
	void eadd( int fd , unsigned int events ) ;
	void emodify( int fd , unsigned int events ) ;
	int emodify( int fd , unsigned int events , std::nothrow_t ) noexcept ;
	void eremove( int fd ) noexcept ;
	int ms() const ;
	static int ms( unsigned int , unsigned int ) ;

private:
	std::vector<struct epoll_event> m_wait_events ;
	int m_wait_rc {0} ;
	bool m_quit {false} ;
	std::string m_quit_reason ;
	bool m_running {false} ;
	int m_fd {-1} ;
	List m_list ;
} ;

// ===

std::unique_ptr<GNet::EventLoop> GNet::EventLoop::create()
{
	return std::make_unique<EventLoopImp>() ;
}

GNet::EventLoopImp::EventLoopImp() :
	m_fd(epoll_create1(EPOLL_CLOEXEC))
{
	if( m_fd == -1 )
		throw Error( "epoll_create" ) ;
}

GNet::EventLoopImp::~EventLoopImp()
{
	close( m_fd ) ;
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

void GNet::EventLoopImp::runOnce()
{
	// make the output array big enough for the largest file descriptor --
	// probably better than trying to count non-negative fds
	m_wait_events.resize( std::max(std::size_t(1U),m_list.size()) ) ;

	// extract the pending events
	int timeout_ms = ms() ;
	m_wait_rc = epoll_wait( m_fd , &m_wait_events[0] , m_wait_events.size() , timeout_ms ) ;
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

	// handle i/o events
	for( int i = 0 ; m_wait_rc > 0 && i < m_wait_rc ; i++ )
	{
		unsigned int e = m_wait_events[i].events ;
		int fd = m_wait_events[i].data.fd ;
		if( e & EPOLLIN )
		{
			ListItem * item = find( fd ) ;
			if( item )
				item->m_read_emitter.raiseReadEvent( Descriptor(fd) ) ;
		}
		if( e & EPOLLOUT )
		{
			ListItem * item = find( fd ) ; // again
			if( item )
				item->m_write_emitter.raiseWriteEvent( Descriptor(fd) ) ;
		}
	}
}

int GNet::EventLoopImp::ms() const
{
	if( TimerList::ptr() )
	{
		auto pair = TimerList::instance().interval() ;
		if( pair.second ) // infinite
			return -1 ;
		else if( pair.first.s() == 0 && pair.first.us() == 0U )
			return 0 ;
		else
			return std::max( 1 , ms( pair.first.s() , pair.first.us() ) ) ;
	}
	else
	{
		return -1 ;
	}
}

int GNet::EventLoopImp::ms( unsigned int s , unsigned int us )
{
	constexpr unsigned int s_max = static_cast<unsigned int>( std::numeric_limits<int>::max()/1000 - 1 ) ;
	static_assert( s_max > 600 , "" ) ; // sanity check that clipping at more than ten mins
	return
		s >= s_max ?
			std::numeric_limits<int>::max() :
			static_cast<int>( (s*1000U) + ((us+999U)/1000U) ) ;
}

bool GNet::EventLoopImp::running() const
{
	return m_running ;
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

GNet::EventLoopImp::ListItem * GNet::EventLoopImp::find( int fd ) noexcept
{
	std::size_t ufd = static_cast<unsigned int>(fd) ;
	return fd >= 0 && ufd < m_list.size() ? &m_list[ufd] : nullptr ;
}

GNet::EventLoopImp::ListItem * GNet::EventLoopImp::findOrCreate( int fd )
{
	ListItem * p = find( fd ) ;
	if( p == nullptr )
	{
		std::size_t ufd = static_cast<unsigned int>(fd) ;
		m_list.resize( std::max(m_list.size(),ufd+1U) ) ; // grow, not shrink
		p = &m_list[ufd] ;
		*p = ListItem() ;
	}
	return p ;
}

void GNet::EventLoopImp::addRead( Descriptor fd , EventHandler & handler , ExceptionSink es )
{
	handler.setDescriptor( fd ) ; // see EventHandler::dtor
	addReadWrite( fd.fd() , handler , es , EPOLLIN ) ;
}

void GNet::EventLoopImp::addWrite( Descriptor fd , EventHandler & handler , ExceptionSink es )
{
	handler.setDescriptor( fd ) ; // see EventHandler::dtor
	addReadWrite( fd.fd() , handler , es , EPOLLOUT ) ;
}

void GNet::EventLoopImp::addOther( Descriptor , EventHandler & , ExceptionSink )
{
	// no-op
}

void GNet::EventLoopImp::addReadWrite( int fd , EventHandler & handler , ExceptionSink es , unsigned int new_events )
{
	G_ASSERT( fd >= 0 ) ;
	G_ASSERT( new_events == EPOLLIN || new_events == EPOLLOUT ) ;
	ListItem * item = findOrCreate( fd ) ;
	if( item->m_events == 0U )
	{
		eadd( fd , new_events ) ;
		item->m_events = new_events ;
	}
	else
	{
		emodify( fd , item->m_events | new_events ) ;
		item->m_events |= new_events ;
	}
	EventEmitter & emitter = new_events == EPOLLIN ? item->m_read_emitter : item->m_write_emitter ;
	emitter.update( &handler , es ) ;
}

void GNet::EventLoopImp::dropRead( Descriptor fd ) noexcept
{
	dropReadWrite( fd.fd() , EPOLLIN ) ;
}

void GNet::EventLoopImp::dropWrite( Descriptor fd ) noexcept
{
	dropReadWrite( fd.fd() , EPOLLOUT ) ;
}

void GNet::EventLoopImp::dropOther( Descriptor ) noexcept
{
	// no-op
}

void GNet::EventLoopImp::dropReadWrite( int fd , unsigned int old_events ) noexcept
{
	ListItem * item = find( fd ) ;
	if( item && ( item->m_events & old_events ) )
	{
		unsigned int new_events = item->m_events & ~old_events ;
		if( new_events == 0U )
		{
			eremove( fd ) ;
		}
		else
		{
			emodify( fd , new_events , std::nothrow ) ;
		}
		item->m_events = new_events ;
		// the emitter is not messed with here because there may
		// be a pending exception that still needs to be delivered
	}
}

void GNet::EventLoopImp::drop( Descriptor fd ) noexcept
{
	ListItem * item = find( fd.fd() ) ;
	if( item )
	{
		if( item->m_events )
			eremove( fd.fd() ) ;
		item->m_events = 0U ;
		item->m_read_emitter.reset() ;
		item->m_write_emitter.reset() ;
	}
}

void GNet::EventLoopImp::disarm( ExceptionHandler * p ) noexcept
{
	for( auto & item : m_list )
	{
		item.m_read_emitter.disarm( p ) ;
		item.m_write_emitter.disarm( p ) ;
	}
}

void GNet::EventLoopImp::eadd( int fd , unsigned int events )
{
	epoll_event event {} ;
	event.data.fd = fd ;
	event.events = events ;
	int rc = epoll_ctl( m_fd , EPOLL_CTL_ADD , fd , &event ) ;
	if( rc == -1 )
	{
		int e = G::Process::errno_() ;
		throw Error( "epoll_ctl" , "add" , G::Process::strerror(e) ) ;
	}
}

void GNet::EventLoopImp::emodify( int fd , unsigned int events )
{
	epoll_event event {} ;
	event.data.fd = fd ;
	event.events = events ;
	int rc = epoll_ctl( m_fd , EPOLL_CTL_MOD , fd , &event ) ;
	if( rc == -1 )
	{
		int e = G::Process::errno_() ;
		throw Error( "epoll_ctl" , "modify" , G::Process::strerror(e) ) ;
	}
}

int GNet::EventLoopImp::emodify( int fd , unsigned int events , std::nothrow_t ) noexcept
{
	epoll_event event {} ;
	event.data.fd = fd ;
	event.events = events ;
	int rc = epoll_ctl( m_fd , EPOLL_CTL_MOD , fd , &event ) ;
	int e = G::Process::errno_() ;
	return rc == -1 ? e : 0 ;
}

void GNet::EventLoopImp::eremove( int fd ) noexcept
{
	epoll_event event {} ;
	epoll_ctl( m_fd , EPOLL_CTL_DEL , fd , &event ) ;
}

