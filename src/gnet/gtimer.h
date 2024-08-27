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
/// \file gtimer.h
///

#ifndef G_NET_TIMER_H
#define G_NET_TIMER_H

#include "gdef.h"
#include "gdatetime.h"
#include "geventhandler.h"
#include "gexception.h"
#include "geventstate.h"

namespace GNet
{
	class TimerBase ;
	template <typename T> class Timer ;
}

//| \class GNet::TimerBase
/// An interface used by GNet::TimerList to keep track of pending timeouts
/// and to deliver timeout events. The public methods to start and cancel
/// the timer are normally used via GNet::Timer<>.
///
class GNet::TimerBase
{
protected:
	explicit TimerBase( EventState es ) ;
		///< Constructor. The EventState receives an onException()
		///< call if the onTimeout() implementation throws.

public:
	virtual ~TimerBase() ;
		///< Destructor.

	void startTimer( unsigned int interval_s , unsigned int interval_us = 0U ) ;
		///< Starts or restarts the timer so that it expires
		///< after the given interval.

	void startTimer( const G::TimeInterval & ) ;
		///< Starts or restarts the timer so that it expires
		///< after the given interval.

	void cancelTimer() ;
		///< Cancels the timer. Does nothing if not running.

	bool active() const noexcept ;
		///< Returns true if the timer is started and not cancelled.

	bool immediate() const ;
		///< Used by TimerList. Returns true if the timer is active()
		///< and zero-length.

	void doTimeout() ;
		///< Used by TimerList to execute the onTimeout() callback.

	G::TimerTime t() const ;
		///< Used by TimerList to get the expiry epoch time. Zero-length
		///< timers return TimerTime::zero() plus any adjust()ment,
		///< ~guaranteed to be less than the t() of any non-immediate
		///< timer.

	const G::TimerTime & tref() const noexcept ;
		///< An inline noexcept alternative to t().

	void adjust( unsigned long ) ;
		///< Used by TimerList to set the order of immedate() timer
		///< expiry.

	bool expired( G::TimerTime & ) const ;
		///< Used by TimerList. Returns true if expired when compared
		///< to the given epoch time. If the given epoch time is
		///< TimerTime::zero() then it is initialised with
		///< TimerTime::now().

protected:
	virtual void onTimeout() = 0 ;
		///< Called when the timer expires (or soon after).

public:
	TimerBase( const TimerBase & ) = delete ;
	TimerBase( TimerBase && ) = delete ;
	TimerBase & operator=( const TimerBase & ) = delete ;
	TimerBase & operator=( TimerBase && ) = delete ;

private:
	static G::TimerTime history() ;

private:
	bool m_active {false} ;
	bool m_immediate {false} ;
	G::TimerTime m_time ;
} ;

inline
const G::TimerTime & GNet::TimerBase::tref() const noexcept
{
	return m_time ;
}

inline
bool GNet::TimerBase::active() const noexcept
{
	return m_active ;
}

//| \class GNet::Timer
/// A timer class template in which the timeout is delivered to the specified
/// method. Any exception thrown out of the timeout handler is delivered to
/// the specified ExceptionHandler interface so that it can be handled or
/// rethrown.
///
/// Eg:
/// \code
/// struct Foo
/// {
///   Timer<Foo> m_timer ;
///   Foo( EventState es ) : m_timer(*this,&Foo::onTimeout,es) {}
///   void onTimeout() { throw "oops" ; }
/// } ;
/// \endcode
///
template <typename T>
class GNet::Timer : private TimerBase
{
public:
	using method_type = void (T::*)() ;

	Timer( T & t , method_type m , EventState ) ;
		///< Constructor.

	void startTimer( unsigned int interval_s , unsigned int interval_us = 0U ) ;
		///< Starts or restarts the timer so that it expires
		///< after the given interval.

	void startTimer( const G::TimeInterval & ) ;
		///< Starts or restarts the timer so that it expires
		///< after the given interval.

	void cancelTimer() ;
		///< Cancels the timer. Does nothing if not running.

	bool active() const noexcept ;
		///< Returns true if the timer is running.

public:
	~Timer() override = default ;
	Timer( const Timer<T> & ) = delete ;
	Timer( Timer<T> && ) = delete ;
	Timer<T> & operator=( const Timer<T> & ) = delete ;
	Timer<T> & operator=( Timer<T> && ) = delete ;

private: // overrides
	void onTimeout() override ; // Override from GNet::TimerBase.

private:
	T & m_t ; // callback target object
	method_type m_m ;
} ;

template <typename T>
GNet::Timer<T>::Timer( T & t , method_type m , GNet::EventState es ) :
	TimerBase(es) ,
	m_t(t) ,
	m_m(m)
{
}

template <typename T>
void GNet::Timer<T>::startTimer( unsigned int s , unsigned int us )
{
	TimerBase::startTimer( s , us ) ;
}

template <typename T>
void GNet::Timer<T>::startTimer( const G::TimeInterval & i )
{
	TimerBase::startTimer( i ) ;
}

template <typename T>
void GNet::Timer<T>::cancelTimer()
{
	TimerBase::cancelTimer() ;
}

template <typename T>
void GNet::Timer<T>::onTimeout()
{
	(m_t.*m_m)() ;
}

template <typename T>
bool GNet::Timer<T>::active() const noexcept
{
	return TimerBase::active() ;
}

#endif
