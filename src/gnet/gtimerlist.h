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
/// \file gtimerlist.h
///

#ifndef G_NET_TIMER_LIST_H
#define G_NET_TIMER_LIST_H

#include "gdef.h"
#include "gnet.h"
#include "gdatetime.h"
#include "geventhandler.h"
#include "gexception.h"
#include <list>

/// \namespace GNet
namespace GNet
{
	class TimerList ;
	class AbstractTimer ;
}

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
		///< Destructor. Any expired timers have their timeouts
		///< called.

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
	bool m_run_on_destruction ;
	List m_list ;
	bool m_list_changed ;
	bool m_empty_set_timeout_hint ;
	bool m_soonest_changed ; // mutable
	G::DateTime::EpochTime m_soonest ;
} ;

#endif
