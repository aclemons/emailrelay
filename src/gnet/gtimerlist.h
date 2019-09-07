//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gtimer.h"
#include "geventhandler.h"
#include "gexception.h"
#include "gexceptionsink.h"
#include <utility>
#include <vector>

namespace GNet
{
	class TimerList ;
	class TimerBase ;
	class TimerListTest ;
}

/// \class GNet::TimerList
/// A singleton which maintains a list of all Timer objects, and interfaces
/// to the event loop on their behalf.
///
/// Event loops should call TimerList::interval() to determine how long to
/// wait before the first Timer goes off. If the timed-wait times-out or
/// if the interval was zero then they must call TimerList::doTimeouts().
///
/// There can be a race where this class sees no expired timers in
/// doTimeouts(), perhaps because the system clock is being stretched.
/// However, the next interval() or setTimer() time will be very small
/// and the race will resolve itself naturally.
///
/// Every timer has an associated exception handler, typically a
/// more long-lived object that has the timer as a sub-object.
/// If the timer callback throws an exception then timer list catches
/// it and invokes the exception handler -- and if that throws then the
/// exception escapes the event loop. This is safe even if the exception
/// handler object is destroyed by the original exception because the
/// exception handler base-class destructor uses the timer list's disarm()
/// mechanism. This is the same behaviour as in the EventHandlerList.
///
/// Exception handlers are combined with an additional 'source' pointer
/// in an ExceptionSink tuple. The source pointer can be used to
/// provide additional information to the exception handler, typically
/// as a pointer to the event handler (sic) object.
///
/// The implementation maintains a pointer to the timer that will
/// expire soonest so that interval() is fast and O(1) when the set
/// of timers is stable and most events are non-timer events.
///
/// Zero-length timers expire in the same order as they were started,
/// which allows them to be used as a mechanism for asynchronous
/// message-passing.
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
		///< Destructor.

	void add( TimerBase & , ExceptionSink ) ;
		///< Adds a timer. Called from the Timer constructor.

	void remove( TimerBase & ) ;
		///< Removes a timer from the list. Called from the
		///< Timer destructor.

	void updateOnStart( TimerBase & ) ;
		///< Called by Timer when a timer is started.

	void updateOnCancel( TimerBase & ) ;
		///< Called by Timer when a timer is cancelled.

	std::pair<G::TimeInterval,bool> interval() const ;
		///< Returns the interval to the first timer expiry. The second
		///< part is an 'infinite' flag that is set if there are no
		///< timers running. In pathalogical cases the interval
		///< will be capped at the type's maximum value.

	void doTimeouts() ;
		///< Triggers the timeout callbacks of any expired timers.
		///< Called by the event loop (GNet::EventLoop). Any exception
		///< thrown out of an expired timer's callback is caught and
		///< delivered back to the ExceptionSink associated with
		///< the timer.

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
	struct Value /// A value type for the GNet::TimerList.
	{
		TimerBase * m_timer ; // handler for the timeout event
		ExceptionSink m_es ; // handler for any exception thrown
		Value() ; // for uclibc++
		Value( TimerBase * t , ExceptionSink es ) ;
		bool operator==( const Value & v ) const g__noexcept ;
		void resetIf( TimerBase * p ) g__noexcept ;
		void disarmIf( ExceptionHandler * eh ) g__noexcept ;
	} ;
	typedef std::vector<Value> List ;
	struct Lock /// A raii class to lock and unlock GNet::TimerList.
	{
		explicit Lock( TimerList & ) ;
		~Lock() ;
		TimerList & m_timer_list ;
	} ;

private:
	friend class GNet::TimerListTest ;
	friend struct Lock ;
	TimerList( const TimerList & ) g__eq_delete ;
	void operator=( const TimerList & ) g__eq_delete ;
	const TimerBase * findSoonest() const ;
	void lock() ;
	void unlock() ;
	void purgeRemoved() ;
	void mergeAdded() ;
	static void removeFrom( List & , TimerBase * ) ;
	static void disarmIn( List & , ExceptionHandler * ) ;

private:
	static TimerList * m_this ;
	mutable const TimerBase * m_soonest ;
	unsigned int m_adjust ;
	bool m_locked ;
	bool m_removed ;
	List m_list ;
	List m_list_added ; // temporary list for when add()ed from within doTimeouts()
} ;

#endif
