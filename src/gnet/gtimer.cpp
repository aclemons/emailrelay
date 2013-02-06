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
/// A private implementation class used by GNet::AbstractTimer.
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

GNet::AbstractTimer::AbstractTimer() :
	m_time(0UL)
{
	G_ASSERT( TimerList::instance(TimerList::NoThrow()) != NULL ) ;
	G_ASSERT( EventLoop::exists() ) ;
	TimerList::instance().add( *this ) ;
	// no List::update() here since this timer has not started
}

GNet::AbstractTimer::~AbstractTimer()
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
	catch(...) // dtor
	{
	}
}

void GNet::AbstractTimer::startTimer( unsigned int time )
{
	TimerUpdate update ;
	m_time = G::DateTime::now() + time ;
	// List::update() here
}

void GNet::AbstractTimer::cancelTimer()
{
	TimerUpdate update ;
	m_time = 0U ;
	// List::update() here
}

void GNet::AbstractTimer::doTimeout()
{
	G_ASSERT( m_time != 0U ) ;
	m_time = 0U ;

	try
	{
		onTimeout() ;
	}
	catch( std::exception & e ) // strategy
	{
		G_DEBUG( "GNet::AbstractTimer::doTimeout: exception from timeout handler being passed back: " << e.what() ) ;
		onTimeoutException( e ) ;
	}
}

G::DateTime::EpochTime GNet::AbstractTimer::t() const
{
	return m_time ;
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
	catch(...) // dtor
	{
	}
}

