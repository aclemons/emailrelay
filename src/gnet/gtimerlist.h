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
///
/// \file gtimerlist.h
///

#ifndef G_NET_TIMER_LIST__H
#define G_NET_TIMER_LIST__H

#include "gdef.h"
#include "gdatetime.h"
#include "geventhandler.h"
#include "gexception.h"
#include <list>

namespace GNet
{
	class TimerList ;
	class TimerBase ;
}

/// \class GNet::TimerList
/// A singleton which maintains a list of all Timer objects, and interfaces
/// to the event loop on their behalf.
///
/// Event loops that can do a timed-wait should call TimerList::interval()
/// to determine how long to wait before the first G::Timer goes off.
/// If the timed-wait times-out or if the interval was zero then they must
/// call TimerList::doTimeouts().
///
/// Event loops that cannot do timed waits efficiently (ie. the traditional
/// Windows message queue) have their setTimeout() method called by this
/// class and they are expected to call doTimeouts() when the event-loop
/// timer expires.
///
/// In both models there can be a race where this class sees no expired
/// timers in doTimeouts(), perhaps because the system clock is being
/// stretched. However, the next interval() or setTimer() time will be
/// very small and the race will resolve itself naturally.
///
/// If a timer callback throws an exception then the associated
/// exception handler is called. This is the same behaviour as in the
/// EventHandlerList. This is made safe even if the event handler object
/// is destroyed by the original exception because of the disarm()
/// mechanism.
///
class GNet::TimerList
{
public:
	G_EXCEPTION( NoInstance , "no TimerList instance" ) ;
	class NoThrow /// Overload discriminator class for TimerList.
		{} ;

	TimerList() ;
		///< Default constructor.

	~TimerList() ;
		///< Destructor. Any expired timers have their timeouts called.

	void add( TimerBase & , ExceptionHandler & ) ;
		///< Adds a timer.

	void remove( TimerBase & ) ;
		///< Removes a timer from the list.

	void update( TimerBase & ) ;
		///< Called when a timer changes its value.

	G::EpochTime interval( bool & infinite ) const ;
		///< Returns the interval to the first timer expiry. The 'infinite'
		///< value is set to true if there are no timers running.

	void doTimeouts() ;
		///< Triggers the timeout callbacks of any expired timers.
		///< Called by the event loop (GNet::EventLoop).

	static bool exists() ;
		///< Returns true if instance() exists.

	static TimerList * instance( const NoThrow & ) ;
		///< Singleton access. Returns nullptr if none.

	static TimerList & instance() ;
		///< Singleton access. Throws an exception if none.

	std::string report() const ;
		///< Returns a line of text reporting the status of the timer list.
		///< Used in debugging and diagnostics.

	void disarm( ExceptionHandler * ) ;
		///< Resets any matching ExceptionHandler pointers.

private:
	TimerList( const TimerList & ) ;
	void operator=( const TimerList & ) ;
	G::EpochTime soonestTime() const ;
	TimerBase * findSoonest() ;
	void collectGarbage() ;
	void setTimeout() ;

private:
	struct Value /// A value type for the GNet::TimerList.
	{
		TimerBase * first ;
		ExceptionHandler * second ;
		Value( TimerBase * t , ExceptionHandler *eh ) : first(t) , second(eh) {}
	} ;
	typedef std::list<Value> List ;
	static TimerList * m_this ;
	TimerBase * m_soonest ;
	List m_list ;
} ;

#endif
