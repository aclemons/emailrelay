//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gtimer.cpp
//

#include "gdef.h"
#include "gtimer.h"
#include "gevent.h"
#include "gdebug.h"
#include "gassert.h"

namespace GNet
{
	class TimerUpdate ;
}

/// \class GNet::TimerUpdate
/// A private implementation class used by GNet::Timer.
/// 
class GNet::TimerUpdate 
{
public:
	TimerUpdate() ;
	~TimerUpdate() ;
private:
	TimerUpdate( const TimerUpdate & ) ; // not implemented
	void operator=( const TimerUpdate & ) ; // not implemented
private:
	G::DateTime::EpochTime m_old_soonest ;
} ;

// ===

GNet::TimeoutHandler::~TimeoutHandler()
{
}

// ===

GNet::Timer::Timer( TimeoutHandler & handler ) :
	m_time(0UL) ,
	m_handler(&handler)
{
	G_ASSERT( TimerList::instance(TimerList::NoThrow()) != NULL ) ;
	G_ASSERT( EventLoop::exists() ) ;
	TimerList::instance().add( *this ) ;
	// no List::update() here since this timer has not started
}

GNet::Timer::Timer() :
	m_time(0UL) ,
	m_handler(NULL)
{
	G_ASSERT( TimerList::instance(TimerList::NoThrow()) != NULL ) ;
	G_ASSERT( EventLoop::exists() ) ;
	TimerList::instance().add( *this ) ;
	// no List::update() here since this timer has not started
}

GNet::Timer::~Timer()
{
	try
	{
		if( TimerList::instance(TimerList::NoThrow()) != NULL )
		{
			TimerUpdate update ;
			TimerList::instance().remove( *this ) ;
			// List::update() here
		}
	}
	catch(...)
	{
	}
}

void GNet::Timer::startTimer( unsigned int time )
{
	TimerUpdate update ;
	m_time = G::DateTime::now() + time ;
	// List::update() here
}

void GNet::Timer::cancelTimer()
{
	TimerUpdate update ;
	m_time = 0U ;
	// List::update() here
}

void GNet::Timer::doTimeout()
{
	G_ASSERT( m_time != 0U ) ;

	m_time = 0U ;
	onTimeout() ;
	if( m_handler != NULL )
		m_handler->onTimeout(*this) ;
}

void GNet::Timer::onTimeout()
{
	// no-op
}

G::DateTime::EpochTime GNet::Timer::t() const
{
	return m_time ;
}

// ===

GNet::TimerList * GNet::TimerList::m_this = NULL ;

GNet::TimerList::TimerList() :
	m_list_changed(false) ,
	m_empty_set_timeout_hint(false) ,
	m_soonest_changed(true) ,
	m_soonest(99U)
{
	if( m_this == NULL )
		m_this = this ;
}

GNet::TimerList::~TimerList()
{
	if( m_this == this )
		m_this = NULL ;
}

void GNet::TimerList::add( Timer & t )
{
	m_list_changed = true ;
	m_list.push_back( &t ) ;
}

void GNet::TimerList::remove( Timer & t )
{
	for( List::iterator p = m_list.begin() ; p != m_list.end() ; ++p )
	{
		if( *p == &t )
		{
			m_list.erase( p ) ;
			m_list_changed = true ;
			break ;
		}
	}
}

void GNet::TimerList::update( G::DateTime::EpochTime t_old )
{
	// after any change in the soonest() time notify the event loop 

	G::DateTime::EpochTime t_new = soonest() ;
	if( t_old != t_new )
	{
		m_soonest_changed = true ;
		if( EventLoop::exists() )
		{
			G_DEBUG( "GNet::TimerList::update: " << t_old << " -> " << t_new ) ;
			EventLoop::instance().setTimeout( t_new , m_empty_set_timeout_hint ) ;
		}
	}
}

void GNet::TimerList::update()
{
	// this overload just assumes that the soonest() time has probably changed

	m_soonest_changed = true ;
	if( EventLoop::exists() )
	{
		G::DateTime::EpochTime t_new = soonest() ;
		G_DEBUG( "GNet::TimerList::update: ? -> " << t_new ) ;
		EventLoop::instance().setTimeout( t_new , m_empty_set_timeout_hint ) ;
	}
}

G::DateTime::EpochTime GNet::TimerList::soonest() const
{
	G::DateTime::EpochTime result = 0U ;
	const List::const_iterator end = m_list.end() ;
	for( List::const_iterator p = m_list.begin() ; p != end ; ++p )
	{
		if( (*p)->t() != 0UL && ( result == 0U || (*p)->t() < result ) )
			result = (*p)->t() ;
	}
	return result ;
}

G::DateTime::EpochTime GNet::TimerList::soonest( int ) const
{
	// this optimised overload is for interval() which
	// gets called on _every_ fd event

	if( m_soonest_changed )
	{
		TimerList * This = const_cast<TimerList*>(this) ;
		This->m_soonest = soonest() ;
		This->m_soonest_changed = false ;
	}
	//G_ASSERT( valid() ) ; // optimisation lost if this is active
	return m_soonest ;
}

bool GNet::TimerList::valid() const
{
	if( soonest() != m_soonest )
	{
		G_ERROR( "GNet::TimerList::valid: soonest()=" << soonest() << ", m_soonest=" << m_soonest ) ;
		return false ;
	}
	return true ;
}

unsigned int GNet::TimerList::interval( bool & infinite ) const
{
	G::DateTime::EpochTime then = soonest(0) ; // fast
	infinite = then == 0U ;
	if( infinite )
	{
		return 0U ;
	}
	else
	{
		G::DateTime::EpochTime now = G::DateTime::now() ;
		return now >= then ? 0U : (then-now) ;
	}
}

GNet::TimerList * GNet::TimerList::instance( const NoThrow & )
{
	return m_this ;
}

GNet::TimerList & GNet::TimerList::instance()
{
	if( m_this == NULL )
		throw NoInstance() ;

	return * m_this ;
}

void GNet::TimerList::doTimeouts()
{
	G_DEBUG( "GNet::TimerList::doTimeouts" ) ;
	G::DateTime::EpochTime now = G::DateTime::now() ;

	// if the list changes break the loop and start again
	do
	{
		m_list_changed = false ;
		for( List::iterator p = m_list.begin() ; p != m_list.end() ; ++p )
		{
			G::DateTime::EpochTime t = (*p)->t() ;
			if( t != 0U && now >= t )
			{
				(*p)->doTimeout() ;
				if( m_list_changed ) break ;
			}
		}
	} while( m_list_changed ) ;

	// deal with any change in the soonest() time
	update() ;
}

// ===

GNet::TimerUpdate::TimerUpdate()
{
	m_old_soonest = TimerList::instance().soonest() ;
}

GNet::TimerUpdate::~TimerUpdate()
{
	try
	{
		if( TimerList::instance(TimerList::NoThrow()) != NULL )
			TimerList::instance().update( m_old_soonest ) ;
	}
	catch(...)
	{
	}
}

