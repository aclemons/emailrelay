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
/// \file gtimer.h
///

#ifndef G_NET_TIMER__H
#define G_NET_TIMER__H

#include "gdef.h"
#include "gdatetime.h"
#include "geventhandler.h"
#include "gexception.h"
#include "gtimerlist.h"

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
	explicit TimerBase( ExceptionHandler & ) ;
		///< Constructor.

public:
	virtual ~TimerBase() ;
		///< Destructor.

	void startTimer( unsigned int interval_s , unsigned int interval_us = 0U ) ;
		///< Starts the timer so that it goes off after the
		///< given time interval.

	void startTimer( const G::EpochTime & interval_time ) ;
		///< Overload for an interval expressed as an G::EpochTime.

	void cancelTimer() ;
		///< Cancels the timer.

	bool active() const ;
		///< Returns true if the timer is started and not cancelled.

	bool immediate() const ;
		///< Used by TimerList. Returns true if the timer is active()
		///< and zero-length.

	void doTimeout() ;
		///< Used by TimerList to execute the onTimeout() callback.

	G::EpochTime t() const ;
		///< Used by TimerList to get the expiry epoch time.

	bool expired( G::EpochTime & ) const ;
		///< Used by TimerList. Returns true if expired when compared
		///< to the given epoch time.

protected:
	virtual void onTimeout() = 0 ;
		///< Called when the timer expires (or soon after).

private:
	TimerBase( const TimerBase & ) ; // not implemented
	void operator=( const TimerBase & ) ; // not implemented

private:
	G::EpochTime m_time ;
} ;

inline
bool GNet::TimerBase::active() const
{
	return m_time.s != 0 ;
}

namespace GNet
{

/// \class Timer
/// A timer class template in which the timeout is delivered to the specified
/// method. Any exception thrown out of the timeout handler is delivered to
/// the specified EventHandler interface so that it can be handled or rethrown.
///
/// Eg:
/// \code
/// struct Foo : public ExceptionHandler
/// {
///   Timer<Foo> m_timer ;
///   Foo() : m_timer(*this,&Foo::onTimeout,*this) {}
///   void onTimeout() {}
///   void onException( std::exception & ) { throw ; }
/// } ;
/// \endcode
///
template <typename T>
class Timer : public TimerBase
{
public:
	typedef void (T::*method_type)() ;

	Timer( T & t , method_type m , ExceptionHandler & ) ;
		///< Constructor. The ExceptionHandler reference is used when
		///< the timeout handler throws; the onException() implementation
		///< can simply rethrow to exit the event loop.

protected:
	virtual void onTimeout() override ;
		///< Override from GNet::TimerBase.

private:
	Timer( const Timer<T> & ) ; // not implemented
	void operator=( const Timer<T> & ) ; // not implemented

private:
	T & m_t ;
	method_type m_m ;
	ExceptionHandler & m_event_exception_handler ;
} ;

template <typename T>
Timer<T>::Timer( T & t , method_type m , ExceptionHandler & e ) :
	TimerBase(e) ,
	m_t(t) ,
	m_m(m) ,
	m_event_exception_handler(e)
{
}

template <typename T>
void Timer<T>::onTimeout()
{
	(m_t.*m_m)() ;
}

}

#endif
