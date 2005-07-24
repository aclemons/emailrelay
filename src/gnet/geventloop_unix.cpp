//
// Copyright (C) 2001-2005 Graeme Walker <graeme_walker@users.sourceforge.net>
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

// Class: GNet::Select
// Description: A concrete implementation of GNet::EventLoop using
// ::select() in the implementation.
//
class GNet::Select : public GNet::EventLoop , public G::noncopyable 
{
public:
	G_EXCEPTION( Error , "select() error" ) ;
	Select() ;
	virtual ~Select() ;
	virtual bool init() ;
	virtual void run() ;
	virtual bool running() const ;
	virtual void quit() ;
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
	bool m_running ;
	EventHandlerList m_read_list ;
	EventHandlerList m_write_list ;
	EventHandlerList m_exception_list ;
} ;

// Class: GNet::Lock
// Description: A private implementation class used by GNet::Select to
// lock data structures in the face of reentrancy.
//
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

// Class: GNet::FdSet
// Description: A static implementation interface used by GNet::Select
// to do fd_set iteration.
//
class GNet::FdSet 
{
public:
	static int init( int n , fd_set * set , const EventHandlerList & list ) ;
	static void raiseEvents( fd_set * set , EventHandlerList & list , 
		void (EventHandler::*method)() ) ;
private:
	FdSet() ; // not implemented
} ;
 
// ===

inline
GNet::Lock::Lock( EventHandlerList & list ) : 
	m_list(list) 
{ 
	m_list.lock() ; 
}

inline
GNet::Lock::~Lock() 
{ 
	m_list.unlock() ; 
}

// ===

//static
int GNet::FdSet::init( int n , fd_set * set , const EventHandlerList & list )
{
	// copy the event-handler-list into the fd-set

	FD_ZERO( set ) ;
	const EventHandlerList::Iterator end = list.end() ;
	for( EventHandlerList::Iterator p = list.begin() ; p != end ; ++p )
	{
		Descriptor fd = EventHandlerList::fd( p ) ;
		FD_SET( fd , set ) ;
		if( (fd+1) > n )
			n = (fd+1) ;
	}
	return n ;
}

//static 
void GNet::FdSet::raiseEvents( fd_set * set , EventHandlerList & list , void (EventHandler::*method)() )
{
	// call the event-handler for fds in fd-set which are ISSET()

	GNet::Lock lock( list ) ; // since event handlers may change the list while we iterate
	const EventHandlerList::Iterator end = list.end() ;
	for( EventHandlerList::Iterator p = list.begin() ; p != end ; ++p )
	{
		Descriptor fd = EventHandlerList::fd( p ) ;
		if( FD_ISSET( fd , set ) )
		{
			//G_DEBUG( "raiseEvents: " << type << " event on fd " << fd ) ;
			EventHandler * h = EventHandlerList::handler( p ) ;
			if( h != NULL )
				(h->*method)() ;
		}
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
	m_read_list(std::string("read")) ,
	m_write_list(std::string("write")) ,
	m_exception_list(std::string("exception"))
{
}

GNet::Select::~Select()
{
}

bool GNet::Select::init()
{
	return true ;
}

void GNet::Select::run()
{
	G::Setter setter( m_running ) ;
	while( !m_quit )
	{
		runOnce() ;
	}
	m_quit = false ;
}

bool GNet::Select::running() const
{
	return m_running ;
}

void GNet::Select::quit()
{
	m_quit = true ;
}

void GNet::Select::runOnce()
{
	// build fd-sets from handler lists
	//
	int n = 1 ;
	fd_set r ; n = FdSet::init( n , &r , m_read_list ) ;
	fd_set w ; n = FdSet::init( n , &w , m_write_list ) ;
	fd_set e ; n = FdSet::init( n , &e , m_exception_list ) ;

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
	int rc = ::select( n , &r , &w , &e , timeout_p ) ;
	if( rc < 0 )
		throw Error() ;

	// call the event handlers
	//
	if( rc == 0 )
	{
		G_DEBUG( "GNet::Select::runOnce: select() timeout" ) ;
		TimerList::instance().doTimeouts() ;
	}
	else // rc > 0
	{
		G_DEBUG( "GNet::Select::runOnce: detected event(s) on " << rc << " fd(s)" ) ;
		FdSet::raiseEvents( &r , m_read_list , & EventHandler::readEvent ) ;
		FdSet::raiseEvents( &w , m_write_list , & EventHandler::writeEvent ) ;
		FdSet::raiseEvents( &e , m_exception_list , & EventHandler::exceptionEvent ) ;
	}
}

void GNet::Select::addRead( Descriptor fd , EventHandler & handler )
{
	m_read_list.add( fd , & handler ) ;
}

void GNet::Select::addWrite( Descriptor fd , EventHandler & handler )
{
	m_write_list.add( fd , & handler ) ;
}

void GNet::Select::addException( Descriptor fd , EventHandler & handler )
{
	m_exception_list.add( fd , & handler ) ;
}

void GNet::Select::dropRead( Descriptor fd )
{
	m_read_list.remove( fd ) ;
}

void GNet::Select::dropWrite( Descriptor fd )
{
	m_write_list.remove( fd ) ;
}

void GNet::Select::dropException( Descriptor fd )
{
	m_exception_list.remove( fd ) ;
}

void GNet::Select::setTimeout( G::DateTime::EpochTime , bool & empty_hint )
{
	// does nothing -- interval() in runOnce() suffices
	empty_hint = true ;
}

