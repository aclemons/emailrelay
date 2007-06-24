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
///
/// \file gtimer.h
///

#ifndef G_NET_TIMER_H
#define G_NET_TIMER_H

#include "gdef.h"
#include "gnet.h"
#include "gdatetime.h"
#include "geventhandler.h"
#include "gexception.h"
#include <list>

/// \namespace GNet
namespace GNet
{
	class ConcreteTimer ;
	class AbstractTimer ;
	class TimeoutHandler ;
	class TimerList ;
}

/// \class GNet::TimeoutHandler
/// An interface used by GNet::ConcreteTimer.
///
class GNet::TimeoutHandler 
{
public:
	virtual ~TimeoutHandler() ;
		///< Destructor.

	virtual void onTimeout( AbstractTimer & ) = 0 ;
		///< Called when the associated timer expires.
		///<
		///< If an exception is thrown out of the
		///< override then the event loop will call the 
		///< timer object's onTimeoutException() method.

private:
	void operator=( const TimeoutHandler & ) ; // not implemented
} ;

/// \class GNet::AbstractTimer
/// A timer base class that calls a pure virtual
/// method on expiry.
///
class GNet::AbstractTimer 
{
public:
	virtual ~AbstractTimer() ;
		///< Destructor.

	void startTimer( unsigned int time ) ;
		///< Starts the timer.

	void cancelTimer() ;
		///< Cancels the timer.

protected:
	AbstractTimer() ;
		///< Default constructor.

	virtual void onTimeout() = 0 ;
		///< Called when the timer expires (or soon after).

	virtual void onTimeoutException( std::exception & ) = 0 ;
		///< Called by the event loop when the onTimeout() 
		///< override throws. 
		///<
		///< The implementation can just throw the current
		///< exception so that the event loop terminates.

private:
	AbstractTimer( const AbstractTimer & ) ; // not implemented
	void operator=( const AbstractTimer & ) ; // not implemented

private:
	friend class TimerList ;
	void doTimeout() ; // called by friendly TimerList
	G::DateTime::EpochTime t() const ; // called by friendly TimerList

private:
	G::DateTime::EpochTime m_time ;
	TimeoutHandler * m_handler ;
} ;

/// \class GNet::ConcreteTimer
/// A concrete timer class that calls
/// TimeoutHandler::onTimeout() on expiry and 
/// EventHandler::onException() on error.
///
class GNet::ConcreteTimer : public GNet::AbstractTimer 
{
public:
	ConcreteTimer( TimeoutHandler & , EventHandler & ) ;
		///< Constructor. The EventHandler reference is required
		///< in case the timeout handler throws.

private:
	ConcreteTimer( const ConcreteTimer & ) ; // not implemented
	void operator=( const ConcreteTimer & ) ; // not implemented
	virtual void onTimeout() ; // from AbstractTimer
	virtual void onTimeoutException( std::exception & ) ; // from AbstractTimer

private:
	TimeoutHandler & m_timeout_handler ;
	EventHandler & m_event_handler ;
} ;

/// \class GNet::TimerList
/// A singleton which maintains a list of all Timer
/// objects, and interfaces to the event loop on their behalf.
///
class GNet::TimerList 
{
public:
	G_EXCEPTION( NoInstance , "no TimerList instance" ) ;
	/// Overload discriminator class for TimerList.
	class NoThrow 
		{} ;

	TimerList() ;
		///< Default constructor.

	~TimerList() ;
		///< Destructor.

	void add( AbstractTimer & ) ;
		///< Adds a timer. Used by Timer::Timer().

	void remove( AbstractTimer & ) ;
		///< Removes a timer from the list. Used by 
		///< Timer::~Timer().

	void update( G::DateTime::EpochTime previous_soonest ) ;
		///< Called when one of the list's timers has changed.

	G::DateTime::EpochTime soonest() const ;
		///< Returns the time of the first timer to expire,
		///< or zero if none.

	unsigned int interval( bool & infinite ) const ;
		///< Returns the interval to the next timer expiry. 
		///< The 'infinite' value is set to true if there 
		///< are no timers running.

	void doTimeouts() ;
		///< Triggers the timeout callbacks of any expired
		///< timers. Called by the event loop (GNet::EventLoop).

	static TimerList * instance( const NoThrow & ) ;
		///< Singleton access. Returns NULL if none.

	static TimerList & instance() ;
		///< Singleton access. Throws an exception if none.

private:
	TimerList( const TimerList & ) ; // not implemented
	void operator=( const TimerList & ) ; // not implemented
	void collectGarbage() ;
	G::DateTime::EpochTime soonest( int ) const ; // fast overload
	void update() ;
	bool valid() const ;

private:
	static TimerList * m_this ;
	typedef std::list<AbstractTimer*> List ;
	List m_list ;
	bool m_list_changed ;
	bool m_empty_set_timeout_hint ;
	bool m_soonest_changed ; // mutable
	G::DateTime::EpochTime m_soonest ;
} ;

#endif
