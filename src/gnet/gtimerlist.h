//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#ifndef G_NET_TIMER_LIST_H
#define G_NET_TIMER_LIST_H

#include "gdef.h"
#include "gdatetime.h"
#include "gtimer.h"
#include "geventhandler.h"
#include "gexception.h"
#include "geventstate.h"
#include <utility>
#include <vector>

namespace GNet
{
	class TimerList ;
	class TimerBase ;
	class TimerListTest ;
}

//| \class GNet::TimerList
/// A singleton which maintains a list of all Timer objects, and interfaces
/// to the event loop on their behalf.
///
/// Event loops should call TimerList::interval() to determine how long to
/// wait before the first Timer goes off. If this is zero, or after their
/// event-waiting times-out, they should call TimerList::doTimeouts().
///
/// There can be a race where this class incorrectly sees no expired timers
/// in doTimeouts() if, for example, the system clock is being stretched.
/// However, the interval() or setTimer() time will be very small and the
/// race will resolve itself naturally.
///
/// Every timer has an associated exception handler, typically a
/// more long-lived object that has the timer as a sub-object.
/// If the timer callback throws an exception then timer list catches
/// it and invokes the exception handler -- and if that throws then the
/// exception escapes the event loop. This is safe even if the exception
/// handler object is destroyed by the original exception because the
/// exception handler base-class destructor uses the timer list's disarm()
/// mechanism. This is the same behaviour as in the EventLoop.
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
	G_EXCEPTION( NoInstance , tx("no TimerList instance") )

	TimerList() ;
		///< Default constructor.

	~TimerList() ;
		///< Destructor.

	void add( TimerBase & , EventState ) ;
		///< Adds a timer. Called from the Timer constructor.

	void remove( TimerBase & ) noexcept ;
		///< Removes a timer from the list. Called from the
		///< Timer destructor.

	void updateOnStart( TimerBase & ) ;
		///< Called by Timer when a timer is started.

	void updateOnCancel( TimerBase & ) ;
		///< Called by Timer when a timer is cancelled.

	std::pair<G::TimeInterval,bool> interval() const ;
		///< Returns the interval to the first timer expiry. The second
		///< part is an 'infinite' flag that is set if there are no
		///< timers running. In pathological cases the interval
		///< will be capped at the type's maximum value.

	void doTimeouts() ;
		///< Triggers the timeout callbacks of any expired timers.
		///< Called by the event loop (GNet::EventLoop). Any exception
		///< thrown out of an expired timer's callback is caught and
		///< delivered back to the EventState associated with
		///< the timer.

	static bool exists() ;
		///< Returns true if instance() exists.

	static TimerList * ptr() noexcept ;
		///< Singleton access. Returns nullptr if none.

	static TimerList & instance() ;
		///< Singleton access. Throws an exception if none.

	void disarm( ExceptionHandler * ) noexcept ;
		///< Resets any matching ExceptionHandler pointers.

public:
	TimerList( const TimerList & ) = delete ;
	TimerList( TimerList && ) = delete ;
	TimerList & operator=( const TimerList & ) = delete ;
	TimerList & operator=( TimerList && ) = delete ;

private:
	struct ListItem /// A value type for the GNet::TimerList.
	{
		TimerBase * m_timer{nullptr} ; // handler for the timeout event
		EventState m_es ; // handler for any exception thrown
		ListItem( TimerBase * t , EventState es ) ;
		bool operator==( const ListItem & v ) const noexcept ;
		void resetIf( TimerBase * p ) noexcept ;
		void disarmIf( ExceptionHandler * eh ) noexcept ;
	} ;
	using List = std::vector<ListItem> ;
	struct Lock /// A RAII class to lock and unlock GNet::TimerList.
	{
		explicit Lock( TimerList & ) ;
		~Lock() ;
		Lock( const Lock & ) = delete ;
		Lock( Lock && ) = delete ;
		Lock & operator=( const Lock & ) = delete ;
		Lock & operator=( Lock && ) = delete ;
		TimerList & m_timer_list ;
	} ;
	friend class GNet::TimerListTest ;
	friend struct Lock ;

private:
	const TimerBase * findSoonest() const ;
	void lock() ;
	void unlock() ;
	void purgeRemoved() ;
	void mergeAdded() ;
	void doTimeout( ListItem & ) ;
	static void removeFrom( List & , TimerBase * ) noexcept ;
	static void disarmIn( List & , ExceptionHandler * ) noexcept ;

private:
	static TimerList * m_this ;
	mutable const TimerBase * m_soonest{nullptr} ;
	unsigned int m_adjust{0} ;
	bool m_locked{false} ;
	bool m_removed{false} ;
	List m_list ;
	List m_list_added ; // temporary list for when add()ed from within doTimeouts()
} ;

#endif
