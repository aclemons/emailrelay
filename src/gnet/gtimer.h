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
// gtimer.h
//

#ifndef G_NET_TIMER_H
#define G_NET_TIMER_H

#include "gdef.h"
#include "gnet.h"
#include "gdatetime.h"
#include "gexception.h"
#include <list>

namespace GNet
{
	class Timer ;
	class TimeoutHandler ;
	class TimerList ;
}

// Class: GNet::TimeoutHandler
// Description: An interface used by GNet::Timer.
//
class GNet::TimeoutHandler 
{
public:
	virtual ~TimeoutHandler() ;
		// Destructor.

	virtual void onTimeout( Timer & ) = 0 ;
		// Called when the associated timer
		// expires.

private:
	void operator=( const TimeoutHandler & ) ; // not implemented
} ;

// Class: GNet::Timer
// Description: A timer class.
//
class GNet::Timer 
{
public:
	explicit Timer( TimeoutHandler & handler ) ;
		// Constructor.

	Timer() ;
		// Default constructor.

	virtual ~Timer() ;
		// Destructor.

	void startTimer( unsigned int time ) ;
		// Starts the timer.

	void cancelTimer() ;
		// Cancels the timer.

protected:
	virtual void onTimeout() ;
		// Called when the timer expires (or soon
		// after).
private:
	Timer( const Timer & ) ; // not implemented
	void operator=( const Timer & ) ; // not implemented

private:
	friend class TimerList ;
	void doTimeout() ; // called by friendly TimerList
	G::DateTime::EpochTime t() const ; // called by friendly TimerList

private:
	G::DateTime::EpochTime m_time ;
	TimeoutHandler * m_handler ;
} ;

// Class: GNet::TimerList
// Description: A singleton which maintains a list of all Timer
// objects, and interfaces to the event loop on their behalf.
//
class GNet::TimerList 
{
public:
	G_EXCEPTION( NoInstance , "no TimerList instance" ) ;
	class NoThrow // Overload discriminator class for TimerList.
		{} ;

	TimerList() ;
		// Default constructor.

	~TimerList() ;
		// Destructor.

	void add( Timer & ) ;
		// Adds a timer. Used by Timer::Timer().

	void remove( Timer & ) ;
		// Removes a timer from the list.
		// Used by Timer::~Timer().

	void update( G::DateTime::EpochTime previous_soonest , 
		const std::string & why ) ;
			// Called when one of the list's timers
			// has changed.

	G::DateTime::EpochTime soonest() const ;
		// Returns the time of the first timer to expire,
		// or zero if none.

	unsigned int interval( bool & infinite ) const ;
		// Returns the interval to the next
		// timer expiry. The 'infinite' value is
		// set to true if there are no timers 
		// running.

	void doTimeouts() ;
		// Triggers the timeout callbacks of any expired
		// timers. Called by the event loop (GNet::EventLoop).

	static TimerList * instance( const NoThrow & ) ;
		// Singleton access. Returns NULL if none.

	static TimerList & instance() ;
		// Singleton access. Throws an exception if none.

private:
	TimerList( const TimerList & ) ; // not implemented
	void operator=( const TimerList & ) ; // not implemented

private:
	static TimerList * m_this ;
	typedef std::list<Timer*> List ;
	List m_list ;
	bool m_changed ;
} ;


#endif
