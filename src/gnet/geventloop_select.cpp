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
/// \file geventloop_select.cpp
///

#include "gdef.h"
#include "gscope.h"
#include "gevent.h"
#include "geventemitter.h"
#include "gprocess.h"
#include "gexception.h"
#include "gstr.h"
#include "gfile.h"
#include "gtimer.h"
#include "gtimerlist.h"
#include "gtest.h"
#include "glog.h"
#include "gassert.h"
#include <tuple>
#include <sstream>
#include <sys/types.h>
#include <sys/time.h>

namespace GNet
{
	class EventLoopImp ;
}

//| \class GNet::EventLoopImp
/// A concrete implementation of GNet::EventLoop using select() in its
/// implementation.
///
class GNet::EventLoopImp : public EventLoop
{
public:
	G_EXCEPTION( Error , tx("select error") ) ;
	EventLoopImp() ;

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
	~EventLoopImp() override = default ;
	EventLoopImp( const EventLoopImp & ) = delete ;
	EventLoopImp( EventLoopImp && ) = delete ;
	void operator=( const EventLoopImp & ) = delete ;
	void operator=( EventLoopImp && ) = delete ;

private:
	using Emitters = std::vector<EventEmitter> ;
	void runOnce() ;
	void addImp( int fd , Emitters & , const EventEmitter & ) ;
	void dropImp( int fd , Emitters & ) noexcept ;
	void disarmImp( Emitters & , ExceptionHandler * ) noexcept ;
	void shrink( int ) noexcept ;
	static int events( int nfds , fd_set * ) ;

private:
	bool m_quit{false} ;
	std::string m_quit_reason ;
	bool m_running{false} ;
	int m_nfds{0} ;
	fd_set m_read_set ; // NOLINT cppcoreguidelines-pro-type-member-init
	fd_set m_write_set ; // NOLINT cppcoreguidelines-pro-type-member-init
	fd_set m_other_set ; // NOLINT cppcoreguidelines-pro-type-member-init
	Emitters m_read_emitters ;
	Emitters m_write_emitters ;
	Emitters m_other_emitters ;
	fd_set m_read_set_copy ;
	fd_set m_write_set_copy ;
	fd_set m_other_set_copy ;
} ;

// ===

std::unique_ptr<GNet::EventLoop> GNet::EventLoop::create()
{
	return std::make_unique<EventLoopImp>() ;
}

// ===

GNet::EventLoopImp::EventLoopImp() // NOLINT cppcoreguidelines-pro-type-member-init
{
	FD_ZERO( &m_read_set ) ;
	FD_ZERO( &m_write_set ) ;
	FD_ZERO( &m_other_set ) ;
	FD_ZERO( &m_read_set_copy ) ;
	FD_ZERO( &m_write_set_copy ) ;
	FD_ZERO( &m_other_set_copy ) ;
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
		bool infinite ;
		std::tie( interval , infinite ) = TimerList::instance().interval() ;
		timeout.tv_sec = interval.s() ;
		timeout.tv_usec = interval.us() ;
		timeout_p = infinite ? nullptr : &timeout ;
		immediate = !infinite && interval.s() == 0 && interval.us() == 0U ;
	}

	if( G::Test::enabled("event-loop-quitfile") ) // esp. for profiling
	{
		if( G::File::remove(".quit",std::nothrow) )
			m_quit = true ;
		if( timeout_p == nullptr || timeout.tv_sec > 0 )
		{
			timeout.tv_sec = 0 ;
			timeout.tv_usec = 999999U ;
		}
		timeout_p = &timeout ;
	}

	// do the select()
	//
	int nfds = m_nfds ; // make copies since modified via event handling
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

	// call the fd event handlers -- note that event handlers can
	// remove fds from the copy sets but not add them -- that means
	// that ecount might be smaller that expected, but it still
	// serves as a valid optimisation
	//
	int ecount = 0 ; // optimisation to stop when all events accounted for
	for( int fd = 0 ; ecount < rc && fd < nfds ; fd++ )
	{
		if( FD_ISSET(fd,&m_read_set_copy) )
		{
			ecount++ ;
			G_ASSERT( static_cast<unsigned int>(fd) < m_read_emitters.size() ) ;
			m_read_emitters[fd].raiseReadEvent( Descriptor(fd) ) ;
		}
		if( FD_ISSET(fd,&m_write_set_copy) )
		{
			ecount++ ;
			G_ASSERT( static_cast<unsigned int>(fd) < m_write_emitters.size() ) ;
			m_write_emitters[fd].raiseWriteEvent( Descriptor(fd) ) ;
		}
		if( FD_ISSET(fd,&m_other_set_copy) )
		{
			ecount++ ;
			G_ASSERT( static_cast<unsigned int>(fd) < m_other_emitters.size() ) ;
			m_other_emitters[fd].raiseOtherEvent( Descriptor(fd) , EventHandler::Reason::other ) ;
		}
	}

	if( G::Test::enabled("event-loop-slow") )
	{
		Timeval timeout_slow ;
		timeout_slow.tv_sec = 0 ;
		timeout_slow.tv_usec = 100000 ;
		::select( 0 , nullptr , nullptr , nullptr , &timeout_slow ) ;
	}
}

int GNet::EventLoopImp::events( int nfds , fd_set * sp )
{
	int n = 0 ;
	for( int fd = 0 ; fd < nfds ; fd++ )
	{
		if( FD_ISSET(fd,sp) )
			n++ ;
	}
	return n ;
}

void GNet::EventLoopImp::addRead( Descriptor fd , EventHandler & handler , ExceptionSink es )
{
	addImp( fd.fd() , m_read_emitters , EventEmitter(&handler,es) ) ;
	FD_SET( fd.fd() , &m_read_set ) ;
}

void GNet::EventLoopImp::addWrite( Descriptor fd , EventHandler & handler , ExceptionSink es )
{
	addImp( fd.fd() , m_write_emitters , EventEmitter(&handler,es) ) ;
	FD_SET( fd.fd() , &m_write_set ) ;
}

void GNet::EventLoopImp::addOther( Descriptor fd , EventHandler & handler , ExceptionSink es )
{
	addImp( fd.fd() , m_other_emitters , EventEmitter(&handler,es) ) ;
	FD_SET( fd.fd() , &m_other_set ) ;
}

void GNet::EventLoopImp::addImp( int fd , Emitters & emitters , const EventEmitter & emitter )
{
	if( fd >= FD_SETSIZE )
		throw EventLoop::Overflow( "too many open file descriptors for select()" ) ;

	m_nfds = std::max( m_nfds , fd+1 ) ;
	emitters.resize( m_nfds ) ;
	emitters[fd] = emitter ;
}

void GNet::EventLoopImp::dropRead( Descriptor fd ) noexcept
{
	FD_CLR( fd.fd() , &m_read_set ) ;
	FD_CLR( fd.fd() , &m_read_set_copy ) ;
	dropImp( fd.fd() , m_read_emitters ) ;
}

void GNet::EventLoopImp::dropWrite( Descriptor fd ) noexcept
{
	FD_CLR( fd.fd() , &m_write_set ) ;
	FD_CLR( fd.fd() , &m_write_set_copy ) ;
	dropImp( fd.fd() , m_write_emitters ) ;
}

void GNet::EventLoopImp::dropOther( Descriptor fd ) noexcept
{
	FD_CLR( fd.fd() , &m_other_set ) ;
	FD_CLR( fd.fd() , &m_other_set_copy ) ;
	dropImp( fd.fd() , m_other_emitters ) ;
}

void GNet::EventLoopImp::dropImp( int fd , Emitters & ) noexcept
{
	G_ASSERT( (fd+1) <= m_nfds ) ;
	if( m_nfds && fd >= 0 && (fd+1) >= m_nfds ) // if dropping biggest fd
	{
		// count defunct fds at the top of the fd range
		int i = m_nfds - 1 ;
		while( i >= 0 &&
			!FD_ISSET(i,&m_read_set) &&
			!FD_ISSET(i,&m_write_set) &&
			!FD_ISSET(i,&m_other_set) )
		{
			i-- ;
		}

		// garbage-collect big fds
		shrink( i < 0 ? 0 : (i+1) ) ;
	}
}

void GNet::EventLoopImp::shrink( int nfds ) noexcept
{
	G_ASSERT( m_nfds >= 0 ) ;
	G_ASSERT( nfds >= 0 ) ;
	if( nfds < m_nfds )
	{
		m_nfds = nfds ;
		std::size_t n = static_cast<unsigned int>( nfds ) ;
		if( n < m_read_emitters.size() )
			m_read_emitters.resize( n ) ;
		if( n < m_write_emitters.size() )
			m_write_emitters.resize( n ) ;
		if( n < m_other_emitters.size() )
			m_other_emitters.resize( n ) ;
	}
}

void GNet::EventLoopImp::disarm( ExceptionHandler * p ) noexcept
{
	disarmImp( m_read_emitters , p ) ;
	disarmImp( m_write_emitters , p ) ;
	disarmImp( m_other_emitters , p ) ;
}

void GNet::EventLoopImp::disarmImp( Emitters & emitters , ExceptionHandler * p ) noexcept
{
	for( auto & emitter : emitters )
	{
		if( emitter.es().eh() == p )
			emitter.disarm() ;
	}
}

