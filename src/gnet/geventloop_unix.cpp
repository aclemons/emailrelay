//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// geventloop_unix.cpp
//

#include "gdef.h"
#include "gevent.h"
#include "geventhandlerlist.h"
#include "gexception.h"
#include "gstr.h"
#include "gfile.h"
#include "gtimer.h"
#include "gtest.h"
#include "gdebug.h"
#include <sstream>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>

namespace GNet
{
	class EventLoopImp ;
	class FdSet ;
}

/// \class GNet::FdSet
/// An "fd_set" wrapper class used by GNet::EventLoopImp.
///
class GNet::FdSet
{
public:
	FdSet() ;
	void init( const EventHandlerList & ) ;
	void raiseEvents( EventHandlerList & , void (EventHandler::*method)() ) ;
	void raiseEvents( EventHandlerList & , void (EventHandler::*method)(EventHandler::Reason) , EventHandler::Reason ) ;
	void invalidate() ;
	int fdmax( int = 0 ) const ;
	fd_set * operator()() ;

private:
	bool m_valid ;
	int m_fdmax ;
	fd_set m_set_internal ; // set from EventHandlerList
	fd_set m_set_external ; // passed to select() and modified by it
} ;

/// \class GNet::EventLoopImp
/// A concrete implementation of GNet::EventLoop using select() in its
/// implementation.
///
class GNet::EventLoopImp : public EventLoop
{
public:
	G_EXCEPTION( Error , "select() error" ) ;
	EventLoopImp() ;
	virtual ~EventLoopImp() ;
	virtual std::string run() override ;
	virtual bool running() const override ;
	virtual void quit( std::string ) override ;
	virtual void quit( const G::SignalSafe & ) override ;
	virtual void addRead( Descriptor fd , EventHandler & , ExceptionHandler & ) override ;
	virtual void addWrite( Descriptor fd , EventHandler & , ExceptionHandler & ) override ;
	virtual void addOther( Descriptor fd , EventHandler & , ExceptionHandler & ) override ;
	virtual void dropRead( Descriptor fd ) override ;
	virtual void dropWrite( Descriptor fd ) override ;
	virtual void dropOther( Descriptor fd ) override ;
	virtual std::string report() const override ;
	virtual void setTimeout( G::EpochTime t ) override ;
	virtual void disarm( ExceptionHandler * ) override ;

private:
	EventLoopImp( const EventLoopImp & ) ;
	void operator=( const EventLoopImp & ) ;
	void runOnce() ;

private:
	bool m_quit ;
	std::string m_quit_reason ;
	bool m_running ;
	EventHandlerList m_read_list ;
	FdSet m_read_set ;
	EventHandlerList m_write_list ;
	FdSet m_write_set ;
	EventHandlerList m_other_list ;
	FdSet m_other_set ;
} ;

// ===

GNet::FdSet::FdSet() :
	m_valid(false) ,
	m_fdmax(0)
{
}

fd_set * GNet::FdSet::operator()()
{
	return &m_set_external ;
}

void GNet::FdSet::invalidate()
{
	m_valid = false ;
}

void GNet::FdSet::init( const EventHandlerList & list )
{
	// if the internal set has been inivalidate()d then re-initialise
	// it from the event-handler-list -- then copy the internal list
	// to the external list -- the external list is passed to select()
	// and modified by it -- this might look klunky but it is well
	// optimised on the high frequency code paths and it keeps the
	// choice of select()/fd_set hidden from client code
	//
	if( !m_valid )
	{
		// copy the event-handler-list into the internal fd-set
		m_fdmax = 0 ;
		FD_ZERO( &m_set_internal ) ;
		const EventHandlerList::Iterator end = list.end() ;
		for( EventHandlerList::Iterator p = list.begin() ; p != end ; ++p )
		{
			Descriptor fd = p.fd() ;
			G_ASSERT( fd.valid() && fd.fd() >= 0 ) ;
			if( fd.fd() < 0 ) continue ;
			FD_SET( fd.fd() , &m_set_internal ) ;
			if( (fd.fd()+1) > m_fdmax )
				m_fdmax = (fd.fd()+1) ;
		}
		m_valid = true ;
	}
	m_set_external = m_set_internal ; // fast structure copy
}

int GNet::FdSet::fdmax( int n ) const
{
	return n > m_fdmax ? n : m_fdmax ;
}

void GNet::FdSet::raiseEvents( EventHandlerList & list , void (EventHandler::*method)() )
{
	EventHandlerList::Lock lock( list ) ; // since event handlers may change the list while we iterate
	const EventHandlerList::Iterator end = list.end() ;
	for( EventHandlerList::Iterator p = list.begin() ; p != end ; ++p )
	{
		Descriptor fd = p.fd() ;
		if( fd.fd() >= 0 && FD_ISSET( fd.fd() , &m_set_external ) )
		{
			p.raiseEvent( method ) ;
		}
	}
}

void GNet::FdSet::raiseEvents( EventHandlerList & list , void (EventHandler::*method)(EventHandler::Reason) , EventHandler::Reason reason )
{
	EventHandlerList::Lock lock( list ) ; // since event handlers may change the list while we iterate
	const EventHandlerList::Iterator end = list.end() ;
	for( EventHandlerList::Iterator p = list.begin() ; p != end ; ++p )
	{
		Descriptor fd = p.fd() ;
		if( fd.fd() >= 0 && FD_ISSET( fd.fd() , &m_set_external ) )
		{
			p.raiseEvent( method , reason ) ;
		}
	}
}

// ===

GNet::EventLoop * GNet::EventLoop::create()
{
	return new EventLoopImp ;
}

// ===

GNet::EventLoopImp::EventLoopImp() :
	m_quit(false) ,
	m_running(false) ,
	m_read_list("read") ,
	m_write_list("write") ,
	m_other_list("other")
{
}

GNet::EventLoopImp::~EventLoopImp()
{
}

std::string GNet::EventLoopImp::run()
{
	EventLoop::Running running( m_running ) ;
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

void GNet::EventLoopImp::quit( std::string reason )
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
	// build fd-sets from handler lists
	//
	m_read_set.init( m_read_list ) ;
	m_write_set.init( m_write_list ) ;
	m_other_set.init( m_other_list ) ;
	int n = m_read_set.fdmax( m_write_set.fdmax(m_other_set.fdmax()) ) ;

	// get a timeout interval() from TimerList
	//
	typedef struct timeval Timeval ;
	Timeval timeout ;
	Timeval * timeout_p = nullptr ;
	bool timeout_immediate = false ;
	if( TimerList::instance(TimerList::NoThrow()) != nullptr )
	{
		bool timeout_infinite = false ;
		G::EpochTime interval = TimerList::instance().interval( timeout_infinite ) ;
		timeout_immediate = !timeout_infinite && interval.s == 0 && interval.us == 0U ;
		timeout.tv_sec = interval.s ;
		timeout.tv_usec = interval.us ;
		timeout_p = timeout_infinite ? nullptr : &timeout ;
	}

	if( G::Test::enabled("event-loop-quitfile") ) // esp. for profiling
	{
		if( G::File::remove(".quit",G::File::NoThrow()) )
			m_quit = true ;
		if( timeout_p == nullptr || timeout.tv_sec > 0 )
		{
			timeout.tv_sec = 0 ;
			timeout.tv_usec = 999999 ;
		}
		timeout_p = &timeout ;
	}

	// do the select()
	//
	int rc = ::select( n , m_read_set() , m_write_set() , m_other_set() , timeout_p ) ;
	if( rc < 0 )
	{
		int e = errno ;
		if( e != EINTR ) // eg. when profiling
			throw Error( G::Str::fromInt(e) ) ;
	}

	// call the timeout handlers
	//
	if( rc == 0 || timeout_immediate )
	{
		//G_DEBUG( "GNet::EventLoopImp::runOnce: select() timeout" ) ;
		TimerList::instance().doTimeouts() ;
	}

	// call the fd event handlers
	//
	if( rc > 0 )
	{
		//G_DEBUG( "GNet::EventLoopImp::runOnce: detected event(s) on " << rc << " fd(s)" ) ;
		m_read_set.raiseEvents( m_read_list , &EventHandler::readEvent ) ;
		m_write_set.raiseEvents( m_write_list , &EventHandler::writeEvent ) ;
		m_other_set.raiseEvents( m_other_list , &EventHandler::otherEvent , EventHandler::reason_other ) ;
	}

	if( G::Test::enabled("event-loop-slow") )
	{
		Timeval timeout_slow ;
		timeout_slow.tv_sec = 0 ;
		timeout_slow.tv_usec = 100000 ;
		::select( 0 , nullptr , nullptr , nullptr , &timeout_slow ) ;
	}
}

void GNet::EventLoopImp::addRead( Descriptor fd , EventHandler & handler , ExceptionHandler & eh )
{
	m_read_list.add( fd , &handler , &eh ) ;
	m_read_set.invalidate() ;
}

void GNet::EventLoopImp::addWrite( Descriptor fd , EventHandler & handler , ExceptionHandler & eh )
{
	m_write_list.add( fd , &handler , &eh ) ;
	m_write_set.invalidate() ;
}

void GNet::EventLoopImp::addOther( Descriptor fd , EventHandler & handler , ExceptionHandler & eh )
{
	m_other_list.add( fd , &handler , &eh ) ;
	m_other_set.invalidate() ;
}

void GNet::EventLoopImp::dropRead( Descriptor fd )
{
	m_read_list.remove( fd ) ;
	m_read_set.invalidate() ;
}

void GNet::EventLoopImp::dropWrite( Descriptor fd )
{
	m_write_list.remove( fd ) ;
	m_write_set.invalidate() ;
}

void GNet::EventLoopImp::dropOther( Descriptor fd )
{
	m_other_list.remove( fd ) ;
	m_other_set.invalidate() ;
}

void GNet::EventLoopImp::disarm( ExceptionHandler * p )
{
	m_read_list.disarm( p ) ;
	m_write_list.disarm( p ) ;
	m_other_list.disarm( p ) ;
}

void GNet::EventLoopImp::setTimeout( G::EpochTime )
{
	// does nothing -- interval() in runOnce() suffices
}

std::string GNet::EventLoopImp::report() const
{
	return std::string() ;
}

