//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gnet.h"
#include "gnoncopyable.h"
#include "gevent.h"
#include "gexception.h"
#include "gstr.h"
#include "gsetter.h"
#include "gtimer.h"
#include "gtest.h"
#include "gdebug.h"
#include <sys/types.h>
#include <sys/time.h>

typedef struct timeval Timeval ; // std:: ??

namespace GNet
{
	class Select ;
	class Lock ;
	class FdSet ;
}

/// \class GNet::FdSet
/// An "fd_set" wrapper type.
/// 
class GNet::FdSet 
{
public:
	FdSet() ;
	void init( const EventHandlerList & ) ;
	void raiseEvents( EventHandlerList & , void (EventHandler::*method)() ) ;
	void raiseEvent( EventHandler * , void (EventHandler::*method)() ) ;
	void invalidate() ;
	int fdmax( int = 0 ) const ;
	fd_set * operator()() ;
private:
	bool m_valid ;
	int m_fdmax ;
	fd_set m_set_internal ;
	fd_set m_set_external ;
} ;
 
/// \class GNet::Select
/// A concrete implementation of GNet::EventLoop using
///  select() in the implementation.
/// 
class GNet::Select : public GNet::EventLoop , public G::noncopyable 
{
public:
	G_EXCEPTION( Error , "select() error" ) ;
	Select() ;
	virtual ~Select() ;
	virtual bool init() ;
	virtual std::string run() ;
	virtual bool running() const ;
	virtual void quit( std::string ) ;
	virtual void addRead( Descriptor fd , EventHandler &handler ) ;
	virtual void addWrite( Descriptor fd , EventHandler &handler ) ;
	virtual void addException( Descriptor fd , EventHandler &handler ) ;
	virtual void dropRead( Descriptor fd ) ;
	virtual void dropWrite( Descriptor fd ) ;
	virtual void dropException( Descriptor fd ) ;

private:
	void runOnce() ;
	virtual void setTimeout( G::DateTime::EpochTime t , bool & ) ;

private:
	bool m_quit ;
	std::string m_quit_reason ;
	bool m_running ;
	EventHandlerList m_read_list ;
	FdSet m_read_set ;
	EventHandlerList m_write_list ;
	FdSet m_write_set ;
	EventHandlerList m_exception_list ;
	FdSet m_exception_set ;
} ;

/// \class GNet::Lock
/// A private implementation class used by GNet::Select to
///  lock data structures in the face of reentrancy.
/// 
class GNet::Lock 
{
public:
	EventHandlerList & m_list ;
	explicit Lock( EventHandlerList & list ) ;
	~Lock() ;

private:
	Lock( const Lock & ) ; // not implemented
	void operator=( const Lock & ) ; // not implemented
} ;

// ===

GNet::Lock::Lock( EventHandlerList & list ) : 
	m_list(list) 
{ 
	m_list.lock() ; 
}

GNet::Lock::~Lock() 
{ 
	m_list.unlock() ; 
}

// ===

GNet::FdSet::FdSet() :
	m_valid(false) ,
	m_fdmax(1)
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
	if( !m_valid )
	{
		// copy the event-handler-list into the internal fd-set
		m_fdmax = 1 ;
		FD_ZERO( &m_set_internal ) ;
		const EventHandlerList::Iterator end = list.end() ;
		for( EventHandlerList::Iterator p = list.begin() ; p != end ; ++p )
		{
			Descriptor fd = EventHandlerList::fd( p ) ;
			FD_SET( fd.fd() , &m_set_internal ) ;
			if( (fd.fd()+1) > m_fdmax )
				m_fdmax = (fd.fd()+1) ;
		}
		m_valid = true ;
	}
	m_set_external = m_set_internal ; // hopefully fast, depending on the definition of fd_set
}

int GNet::FdSet::fdmax( int n ) const
{
	return n > m_fdmax ? n : m_fdmax ;
}

void GNet::FdSet::raiseEvents( EventHandlerList & list , void (EventHandler::*method)() )
{
	// call the event-handler for fds in fd-set which are ISSET()

	GNet::Lock lock( list ) ; // since event handlers may change the list while we iterate
	const EventHandlerList::Iterator end = list.end() ;
	for( EventHandlerList::Iterator p = list.begin() ; p != end ; ++p )
	{
		Descriptor fd = EventHandlerList::fd( p ) ;
		if( FD_ISSET( fd.fd() , &m_set_external ) )
		{
			EventHandler * h = EventHandlerList::handler( p ) ;
			if( h != NULL )
				raiseEvent( h , method ) ;
		}
	}
}

void GNet::FdSet::raiseEvent( EventHandler * h , void (EventHandler::*method)() )
{
	try
	{
		(h->*method)() ;
	}
	catch( std::exception & e )
	{
		h->onException( e ) ;
	}
}

// ===

GNet::EventLoop * GNet::EventLoop::create()
{
	// factory-method pattern
	return new Select ;
}

// ===

GNet::Select::Select() :
	m_quit(false) ,
	m_running(false) ,
	m_read_list("read") ,
	m_write_list("write") ,
	m_exception_list("exception")
{
}

GNet::Select::~Select()
{
}

bool GNet::Select::init()
{
	return true ;
}

std::string GNet::Select::run()
{
	G::Setter setter( m_running ) ;
	do
	{
		runOnce() ;
	} while( !m_quit ) ;
	std::string quit_reason = m_quit_reason ;
	m_quit_reason.clear() ;
	m_quit = false ;
	return quit_reason ;
}

bool GNet::Select::running() const
{
	return m_running ;
}

void GNet::Select::quit( std::string reason )
{
	m_quit = true ;
	m_quit_reason = reason ;
}

void GNet::Select::runOnce()
{
	// build fd-sets from handler lists
	//
	m_read_set.init( m_read_list ) ;
	m_write_set.init( m_write_list ) ;
	m_exception_set.init( m_exception_list ) ;
	int n = m_read_set.fdmax( m_write_set.fdmax(m_exception_set.fdmax()) ) ;

	// get a timeout interval() from TimerList
	//
	Timeval timeout ;
	Timeval * timeout_p = NULL ;
	if( TimerList::instance(TimerList::NoThrow()) != NULL )
	{
		bool infinite = false ;
		timeout.tv_sec = TimerList::instance().interval( infinite ) ;
		timeout.tv_usec = 0 ; // micro seconds
		timeout_p = infinite ? NULL : &timeout ;
	}

	// do the select()
	//
	int rc = ::select( n , m_read_set() , m_write_set() , m_exception_set() , timeout_p ) ;
	if( rc < 0 )
		throw Error() ;

	// call the event handlers
	//
	if( rc == 0 || ( timeout_p != NULL && timeout_p->tv_sec == 0 ) )
	{
		G_DEBUG( "GNet::Select::runOnce: select() timeout" ) ;
		TimerList::instance().doTimeouts() ;
	}
	if( rc > 0 )
	{
		G_DEBUG( "GNet::Select::runOnce: detected event(s) on " << rc << " fd(s)" ) ;
		m_read_set.raiseEvents( m_read_list , & EventHandler::readEvent ) ;
		m_write_set.raiseEvents( m_write_list , & EventHandler::writeEvent ) ;
		m_exception_set.raiseEvents( m_exception_list , & EventHandler::exceptionEvent ) ;
	}

	if( G::Test::enabled("slow-event-loop") )
	{
		Timeval timeout_slow ;
		timeout_slow.tv_sec = 0 ;
		timeout_slow.tv_usec = 100000 ;
		::select( 0 , NULL , NULL , NULL , &timeout_slow ) ;
	}
}

void GNet::Select::addRead( Descriptor fd , EventHandler & handler )
{
	m_read_list.add( fd , & handler ) ;
	m_read_set.invalidate() ;
}

void GNet::Select::addWrite( Descriptor fd , EventHandler & handler )
{
	m_write_list.add( fd , & handler ) ;
	m_write_set.invalidate() ;
}

void GNet::Select::addException( Descriptor fd , EventHandler & handler )
{
	m_exception_list.add( fd , & handler ) ;
	m_exception_set.invalidate() ;
}

void GNet::Select::dropRead( Descriptor fd )
{
	m_read_list.remove( fd ) ;
	m_read_set.invalidate() ;
}

void GNet::Select::dropWrite( Descriptor fd )
{
	m_write_list.remove( fd ) ;
	m_write_set.invalidate() ;
}

void GNet::Select::dropException( Descriptor fd )
{
	m_exception_list.remove( fd ) ;
	m_exception_set.invalidate() ;
}

void GNet::Select::setTimeout( G::DateTime::EpochTime , bool & empty_hint )
{
	// does nothing -- interval() in runOnce() suffices
	empty_hint = true ;
}

