//
// Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
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

// Class: GNet::TimerUpdate
// Description: A private implementation class used by GNet::Timer.
//
class GNet::TimerUpdate 
{
public:
	TimerUpdate( Timer & , const std::string & ) ;
	~TimerUpdate() ;
private:
	TimerUpdate( const TimerUpdate & ) ; // not implemented
	void operator=( const TimerUpdate & ) ; // not implemented
private:
	Timer & m_timer ;
	std::string m_type ;
	G::DateTime::EpochTime m_soonest ;
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
	TimerUpdate update( *this , "ctor" ) ;
	TimerList::instance().add( *this ) ;
}

GNet::Timer::Timer() :
	m_time(0UL) ,
	m_handler(NULL)
{
	G_ASSERT( TimerList::instance(TimerList::NoThrow()) != NULL ) ;
	G_ASSERT( EventLoop::exists() ) ;
	TimerUpdate update( *this , "ctor" ) ;
	TimerList::instance().add( *this ) ;
}

GNet::Timer::~Timer()
{
	try
	{
		if( TimerList::instance(TimerList::NoThrow()) != NULL )
		{
			TimerUpdate update( *this , "dtor" ) ;
			TimerList::instance().remove( *this ) ;
		}
	}
	catch(...)
	{
	}
}

void GNet::Timer::startTimer( unsigned int time )
{
	TimerUpdate update( *this , "start" ) ;
	m_time = G::DateTime::now() + time ;
}

void GNet::Timer::cancelTimer()
{
	TimerUpdate update( *this , "cancel" ) ;
	m_time = 0U ;
}

void GNet::Timer::doTimeout()
{
	if( m_time != 0U )
	{
		m_time = 0U ;
		G_DEBUG( "GNet::Timer::doTimeout" ) ;
		onTimeout() ;
		if( m_handler != NULL )
			m_handler->onTimeout(*this) ;
	}
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
	m_changed(false)
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
	m_changed = true ;
	m_list.push_back( &t ) ;
}

void GNet::TimerList::remove( Timer & t )
{
	for( List::iterator p = m_list.begin() ; p != m_list.end() ; ++p )
	{
		if( *p == &t )
		{
			m_list.erase( p ) ;
			m_changed = true ;
			break ;
		}
	}
}

void GNet::TimerList::update( G::DateTime::EpochTime t_old , 
	const std::string & op )
{
	G::DateTime::EpochTime t_new = soonest() ;
	//G_DEBUG( "GNet::TimerList::update: " << op << ": " << t_old << " -> " << t_new ) ;
	G_IGNORE op.length() ; // pacify the compiler
	if( t_old != t_new && EventLoop::exists() )
	{
		EventLoop::instance().setTimeout( t_new ) ;
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

unsigned int GNet::TimerList::interval( bool & infinite ) const
{
	G::DateTime::EpochTime then = soonest() ;
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

	do
	{
		m_changed = false ;
		for( List::iterator p = m_list.begin() ; p != m_list.end() ; ++p )
		{
			if( now >= (*p)->t() ) 
			{
				(*p)->doTimeout() ;
				if( m_changed ) break ;
			}
		}
	} while( m_changed ) ;

	if( EventLoop::exists() )
		EventLoop::instance().setTimeout( soonest() ) ;
}

// ===

GNet::TimerUpdate::TimerUpdate( Timer & timer , const std::string & type ) :
	m_timer(timer) ,
	m_type(type)
{
	m_soonest = TimerList::instance().soonest() ;
}

GNet::TimerUpdate::~TimerUpdate()
{
	try
	{
		if( TimerList::instance(TimerList::NoThrow()) != NULL )
			TimerList::instance().update( m_soonest , m_type ) ;
	}
	catch(...)
	{
	}
}

