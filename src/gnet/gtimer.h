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
#include "gtimerlist.h"

/// \namespace GNet
namespace GNet
{
	class AbstractTimer ;
}

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
} ;

/// \namespace GNet
namespace GNet
{

/// \class Timer
/// A timer class template in which the timeout
/// is delivered to the specified method. Any exception thrown
/// out of the timeout handler is delivered to the specified
/// EventHandler interface so that it can be handled or
/// rethrown.
///
/// Eg:
/// \code
/// struct Foo : public EventHandler
/// {
///   Timer<Foo> m_timer ;
///   Foo() : m_timer(*this,&Foo::onTimeout,*this) {}
///   void onTimeout() {}
///   void onException( std::exception & ) { throw ; }
/// } ;
/// \endcode
///
template <typename T>
class Timer : public AbstractTimer 
{
public:
	typedef void (T::*method_type)() ;

	Timer( T & t , method_type m , EventHandler & exception_handler ) ;
		///< Constructor. The EventHandler reference is required
		///< in case the timeout handler throws.

protected:
	virtual void onTimeout() ; 
		///< Final override from GNet::AbstractTimer.

	virtual void onTimeoutException( std::exception & ) ;
		///< Final override from GNet::AbstractTimer.

private:
	Timer( const Timer<T> & ) ; // not implemented
	void operator=( const Timer<T> & ) ; // not implemented

private:
	T & m_t ;
	method_type m_m ;
	EventHandler & m_event_handler ;
} ;

template <typename T>
Timer<T>::Timer( T & t , method_type m , EventHandler & e ) :
	m_t(t) ,
	m_m(m) ,
	m_event_handler(e)
{
}

template <typename T>
void Timer<T>::onTimeout()
{
	(m_t.*m_m)() ;
}

template <typename T>
void Timer<T>::onTimeoutException( std::exception & e )
{
	m_event_handler.onException( e ) ;
}

}

#endif
