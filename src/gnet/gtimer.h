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
/// \file gtimer.h
///

#ifndef G_NET_TIMER__H
#define G_NET_TIMER__H

#include "gdef.h"
#include "gdatetime.h"
#include "geventhandler.h"
#include "gexception.h"
#include "gexceptionsink.h"

namespace GNet
{
	class TimerBase ;
}

/// \class GNet::TimerBase
/// An interface used by GNet::TimerList to keep track of pending timeouts
/// and to deliver timeout events. The public methods to start and cancel
/// the timer are normally used via GNet::Timer<>.
///
class GNet::TimerBase
{
protected:
	explicit TimerBase( ExceptionSink es ) ;
		///< Constructor. The ExceptionSink receives an onException()
		///< call if the onTimeout() implementation throws.

public:
	virtual ~TimerBase() ;
		///< Destructor.

	void startTimer( unsigned int interval_s , unsigned int interval_us = 0U ) ;
		///< Starts or restarts the timer so that it expires
		///< after the given interval.

	void cancelTimer() ;
		///< Cancels the timer. Does nothing if not running.

	bool active() const g__noexcept ;
		///< Returns true if the timer is started and not cancelled.

	bool immediate() const ;
		///< Used by TimerList. Returns true if the timer is active()
		///< and zero-length.

	void doTimeout() ;
		///< Used by TimerList to execute the onTimeout() callback.

	G::EpochTime t() const ;
		///< Used by TimerList to get the expiry epoch time. Zero-length
		///< timers return a value corresponding to some time in ancient
		///< history (1970).

	void adjust( unsigned int us ) ;
		///< Used by TimerList to set the fractional part of the expiry
		///< time of immediate() timers so that t() is ordered by
		///< startTimer() time.

	bool expired( G::EpochTime & ) const ;
		///< Used by TimerList. Returns true if expired when compared
		///< to the given epoch time.

protected:
	virtual void onTimeout() = 0 ;
		///< Called when the timer expires (or soon after).

private:
	TimerBase( const TimerBase & ) g__eq_delete ;
	void operator=( const TimerBase & ) g__eq_delete ;
	static G::EpochTime history() ;

private:
	G::EpochTime m_time ;
} ;

inline
bool GNet::TimerBase::active() const g__noexcept
{
	return m_time.s != 0 ; // sic
}

namespace GNet
{

/// \class Timer
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
///   Foo( ExceptionSink es ) : m_timer(*this,&Foo::onTimeout,es) {}
///   void onTimeout() { throw "oops" ; }
/// } ;
/// \endcode
///
template <typename T>
class Timer : private TimerBase
{
public:
	typedef void (T::*method_type)() ;

	Timer( T & t , method_type m , ExceptionSink ) ;
		///< Constructor.

	void startTimer( unsigned int interval_s , unsigned int interval_us = 0U ) ;
		///< Starts or restarts the timer so that it expires
		///< after the given interval.

	void startTimer( const G::TimeInterval & ) ;
		///< Starts or restarts the timer so that it expires
		///< after the given interval.

	void cancelTimer() ;
		///< Cancels the timer. Does nothing if not running.

	bool active() const g__noexcept ;
		///< Returns true if the timer is running.

private: // overrides
	virtual void onTimeout() override ; // Override from GNet::TimerBase.

private:
	Timer( const Timer<T> & ) g__eq_delete ;
	void operator=( const Timer<T> & ) g__eq_delete ;

private:
	T & m_t ; // callback target object
	method_type m_m ;
} ;

template <typename T>
Timer<T>::Timer( T & t , method_type m , GNet::ExceptionSink es ) :
	TimerBase(es) ,
	m_t(t) ,
	m_m(m)
{
}

template <typename T>
void Timer<T>::startTimer( unsigned int s , unsigned int us )
{
	TimerBase::startTimer( s , us ) ;
}

template <typename T>
void Timer<T>::startTimer( const G::TimeInterval & i )
{
	TimerBase::startTimer( i.s , i.us ) ;
}

template <typename T>
void Timer<T>::cancelTimer()
{
	TimerBase::cancelTimer() ;
}

template <typename T>
bool Timer<T>::active() const g__noexcept
{
	return TimerBase::active() ;
}

template <typename T>
void Timer<T>::onTimeout()
{
	(m_t.*m_m)() ;
}

}

#endif
