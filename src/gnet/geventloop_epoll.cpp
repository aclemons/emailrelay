//
// Copyright (C) 2001-2021 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "geventhandlerlist.h"
#include "gtimerlist.h"
#include "gprocess.h"
#include "gtest.h"
#include "gfile.h"
#include "gstr.h"
#include "glog.h"
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
	G_EXCEPTION( Error , "epoll error" ) ;
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
	void disarm( ExceptionHandler * ) noexcept override ;

public:
	EventLoopImp( const EventLoopImp & ) = delete ;
	EventLoopImp( EventLoopImp && ) = delete ;
	void operator=( const EventLoopImp & ) = delete ;
	void operator=( EventLoopImp && ) = delete ;

private:
	void runOnce() ;
	bool add( int fd , int events ) ;
	bool modify( int fd , int events ) ;
	void remove( int fd ) noexcept ;
	void clear( int fd ) noexcept ;
	int ms() const ;
	static int ms( unsigned int , unsigned int ) ;

private:
	std::vector<struct epoll_event> m_wait_events ;
	int m_wait_rc{0} ;
	bool m_quit{false} ;
	std::string m_quit_reason ;
	bool m_running{false} ;
	int m_fd{-1} ;
	EventHandlerList m_read_list ;
	EventHandlerList m_write_list ;
} ;

// ===

std::unique_ptr<GNet::EventLoop> GNet::EventLoop::create()
{
	return std::make_unique<EventLoopImp>() ;
}

GNet::EventLoopImp::EventLoopImp() :
	m_read_list("read") ,
	m_write_list("write")
{
	m_fd = epoll_create1( EPOLL_CLOEXEC ) ;
	if( m_fd == -1 )
		throw Error( "epoll_create" ) ;
}

GNet::EventLoopImp::~EventLoopImp()
{
	close( m_fd ) ;
}

std::string GNet::EventLoopImp::run()
{
	G::ScopeExitSetFalse _( m_running = true ) ;
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
	// resize the output array -- a small fixed-size array works but might lead to starvation
	m_wait_events.resize( std::max(std::size_t(1U),m_read_list.size()+m_write_list.size()) ) ;

	// extract the pending events
	int timeout_ms = ms() ;
	m_wait_rc = epoll_wait( m_fd , &m_wait_events[0] , m_wait_events.size() , timeout_ms ) ;
	if( m_wait_rc == -1 )
	{
		int e = G::Process::errno_() ;
		if( e != EINTR )
			throw Error( "epoll_wait" , G::Process::strerror(e) ) ;
	}

	// handle timer events
	if( m_wait_rc <= 0 || timeout_ms == 0 )
	{
		TimerList::instance().doTimeouts() ;
	}

	// handle read events
	for( int i = 0 ; m_wait_rc > 0 && i < m_wait_rc ; i++ )
	{
		int fd = m_wait_events[i].data.fd ;
		int e = m_wait_events[i].events ;
		if( fd >= 0 && (e & EPOLLIN) ) // see clear()
		{
			auto p = m_read_list.find( Descriptor(fd) ) ;
			if( p != m_read_list.end() )
				p.raiseEvent( &EventHandler::readEvent ) ;
		}
	}

	// handle write events
	for( int i = 0 ; m_wait_rc > 0 && i < m_wait_rc ; i++ )
	{
		int fd = m_wait_events[i].data.fd ;
		int e = m_wait_events[i].events ;
		if( fd >= 0 && (e & EPOLLOUT) ) // see clear()
		{
			auto p = m_write_list.find( Descriptor(fd) ) ;
			if( p != m_write_list.end() )
				p.raiseEvent( &EventHandler::writeEvent ) ;
		}
	}
}

int GNet::EventLoopImp::ms() const
{
	if( TimerList::ptr() )
	{
		auto pair = TimerList::instance().interval() ;
		if( pair.second )
			return -1 ;
		else if( pair.first.s() == 0 && pair.first.us() == 0U )
			return 0 ;
		else
			return ms( pair.first.s() , pair.first.us() ) ;
	}
	else
	{
		return -1 ;
	}
}

int GNet::EventLoopImp::ms( unsigned int s , unsigned int us )
{
	constexpr unsigned int s_max = static_cast<unsigned int>( std::numeric_limits<int>::max()/1000 - 1 ) ;
	return
		s >= s_max ?
			std::numeric_limits<int>::max() :
			static_cast<int>( s * 1000U + ( us >> 10 ) ) ;
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

bool GNet::EventLoopImp::add( int fd , int events )
{
	epoll_event event {} ;
	event.events = events ;
	event.data.fd = fd ;
	int rc = epoll_ctl( m_fd , EPOLL_CTL_ADD , fd , &event ) ;
	if( rc == -1 )
	{
		int e = G::Process::errno_() ;
		if( e != EEXIST )
			throw Error( "epoll_ctl(add)" ) ;
	}
	return rc != -1 ;
}

bool GNet::EventLoopImp::modify( int fd , int events )
{
	epoll_event event {} ;
	event.events = events ;
	event.data.fd = fd ;
	int rc = epoll_ctl( m_fd , EPOLL_CTL_MOD , fd , &event ) ;
	if( rc == -1 )
		throw Error( "epoll_ctl(mod)" ) ;
	return true ;
}

void GNet::EventLoopImp::remove( int fd ) noexcept
{
	epoll_event event {} ;
	epoll_ctl( m_fd , EPOLL_CTL_DEL , fd , &event ) ;
}

void GNet::EventLoopImp::clear( int fd ) noexcept
{
	for( int i = 0 ; m_wait_rc > 0 && i < m_wait_rc ; i++ )
	{
		if( m_wait_events[i].data.fd == fd )
		{
			m_wait_events[i].data.fd = -1 ;
			break ;
		}
	}
}

void GNet::EventLoopImp::addRead( Descriptor fd , EventHandler & handler , ExceptionSink es )
{
	if( !m_read_list.contains(fd) )
	{
		if( m_write_list.contains(fd) )
			modify( fd.fd() , EPOLLIN|EPOLLOUT ) ;
		else
			add( fd.fd() , EPOLLIN ) ;
		m_read_list.add( fd , &handler , es ) ;
	}
}

void GNet::EventLoopImp::addWrite( Descriptor fd , EventHandler & handler , ExceptionSink es )
{
	if( !m_write_list.contains(fd) )
	{
		if( m_read_list.contains(fd) )
			modify( fd.fd() , EPOLLIN|EPOLLOUT ) ;
		else
			add( fd.fd() , EPOLLOUT ) ;
		m_write_list.add( fd , &handler , es ) ;
	}
}

void GNet::EventLoopImp::dropRead( Descriptor fd ) noexcept
{
	m_read_list.remove( fd ) ;
	if( m_write_list.contains(fd) )
	{
		modify( fd.fd() , EPOLLOUT ) ;
	}
	else
	{
		remove( fd.fd() ) ;
		clear( fd.fd() ) ;
	}
}

void GNet::EventLoopImp::dropWrite( Descriptor fd ) noexcept
{
	m_write_list.remove( fd ) ;
	if( m_read_list.contains(fd) )
	{
		modify( fd.fd() , EPOLLIN ) ;
	}
	else
	{
		remove( fd.fd() ) ;
		clear( fd.fd() ) ;
	}
}

void GNet::EventLoopImp::addOther( Descriptor , EventHandler & , ExceptionSink )
{
	// no-op
}

void GNet::EventLoopImp::dropOther( Descriptor ) noexcept
{
	// no-op
}

void GNet::EventLoopImp::disarm( ExceptionHandler * p ) noexcept
{
	m_read_list.disarm( p ) ;
	m_write_list.disarm( p ) ;
}

