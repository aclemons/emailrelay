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
//
// gtimerlist.cpp
//

#include "gdef.h"
#include "gtimerlist.h"
#include "gtimer.h"
#include "gnetdone.h"
#include "geventloop.h"
#include "glog.h"
#include "gassert.h"
#include <algorithm>
#include <functional>
#include <sstream>

GNet::TimerList::Value::Value() :
	m_timer(nullptr)
{
}

GNet::TimerList::Value::Value( TimerBase * t , ExceptionSink es ) :
	m_timer(t) ,
	m_es(es)
{
}

inline bool GNet::TimerList::Value::operator==( const Value & v ) const g__noexcept
{
	return m_timer == v.m_timer ;
}

inline void GNet::TimerList::Value::resetIf( TimerBase * p ) g__noexcept
{
	if( m_timer == p )
		m_timer = nullptr ;
}

void GNet::TimerList::Value::disarmIf( ExceptionHandler * eh ) g__noexcept
{
	if( m_es.eh() == eh )
		m_es.reset() ;
}

// ==

GNet::TimerList::Lock::Lock( TimerList & timer_list ) :
	m_timer_list(timer_list)
{
	m_timer_list.lock() ;
}

GNet::TimerList::Lock::~Lock()
{
	m_timer_list.unlock() ;
}

// ==

GNet::TimerList * GNet::TimerList::m_this = nullptr ;

GNet::TimerList::TimerList() :
	m_soonest(nullptr) ,
	m_adjust(0) ,
	m_locked(false) ,
	m_removed(false)
{
	if( m_this == nullptr )
		m_this = this ;
}

GNet::TimerList::~TimerList()
{
	if( m_this == this )
		m_this = nullptr ;
}

void GNet::TimerList::add( TimerBase & t , ExceptionSink es )
{
	(m_locked?m_list_added:m_list).push_back( Value(&t,es) ) ;
}

void GNet::TimerList::remove( TimerBase & timer )
{
	m_removed = true ;
	removeFrom( m_list , &timer ) ;
	removeFrom( m_list_added , &timer ) ;
	if( m_soonest == &timer ) m_soonest = nullptr ;
}

void GNet::TimerList::removeFrom( List & list , TimerBase * timer_p )
{
	//std::for_each( list.begin() , list.end() , std::bind2nd(std::mem_fun_ref(&Value::resetIf),timer_p) ) ;
	for( List::iterator p = list.begin() ; p != list.end() ; ++p )
		(*p).resetIf( timer_p ) ;
}

void GNet::TimerList::disarm( ExceptionHandler * eh )
{
	disarmIn( m_list , eh ) ;
	disarmIn( m_list_added , eh ) ;
}

void GNet::TimerList::disarmIn( List & list , ExceptionHandler * eh )
{
	//std::for_each( list.begin() , list.end() , std::bind2nd(std::mem_fun_ref(&Value::disarmIf),eh) ) ;
	for( List::iterator p = list.begin() ; p != list.end() ; ++p )
		(*p).disarmIf( eh ) ;
}

void GNet::TimerList::updateOnStart( TimerBase & timer )
{
	if( timer.immediate() )
		timer.adjust( m_adjust++ ) ; // well-defined t() order for immediate timers

	if( m_soonest != nullptr && timer.t() < m_soonest->t() )
		m_soonest = &timer ;
}

void GNet::TimerList::updateOnCancel( TimerBase & timer )
{
	if( m_soonest == &timer )
		m_soonest = nullptr ;
}

const GNet::TimerBase * GNet::TimerList::findSoonest() const
{
	G_ASSERT( !m_locked ) ;
	TimerBase * result = nullptr ;
	const List::const_iterator end = m_list.end() ;
	for( List::const_iterator p = m_list.begin() ; p != end ; ++p )
	{
		if( (*p).m_timer != nullptr && (*p).m_timer->active() && ( result == nullptr || (*p).m_timer->t() < result->t() ) )
			result = (*p).m_timer ;
	}
	return result ;
}

std::pair<G::TimeInterval,bool> GNet::TimerList::interval() const
{
	if( m_soonest == nullptr )
		m_soonest = findSoonest() ;

	if( m_soonest == nullptr )
	{
		return std::make_pair( G::TimeInterval(0) , true ) ;
	}
	else if( m_soonest->immediate() )
	{
		return std::make_pair( G::TimeInterval(0) , false ) ;
	}
	else
	{
		G::EpochTime now = G::DateTime::now() ;
		G::EpochTime then = m_soonest->t() ;
		return std::make_pair( G::TimeInterval(now,then) , false ) ;
	}
}

GNet::TimerList * GNet::TimerList::instance( const NoThrow & )
{
	return m_this ;
}

bool GNet::TimerList::exists()
{
	return m_this != nullptr ;
}

GNet::TimerList & GNet::TimerList::instance()
{
	if( m_this == nullptr )
		throw NoInstance() ;
	return * m_this ;
}

void GNet::TimerList::lock()
{
	m_locked = true ;
}

void GNet::TimerList::unlock()
{
	if( m_locked )
	{
		m_locked = false ;
		mergeAdded() ; // accept any add()ed while locked
		purgeRemoved() ; // collect garbage created by remove()
	}
}

void GNet::TimerList::mergeAdded()
{
	if( !m_list_added.empty() )
	{
		m_list.reserve( m_list.size() + m_list_added.size() ) ;
		m_list.insert( m_list.end() , m_list_added.begin() , m_list_added.end() ) ;
		m_list_added.clear() ;
	}
}

void GNet::TimerList::purgeRemoved()
{
	if( m_removed )
	{
		m_list.erase( std::remove( m_list.begin() , m_list.end() , Value(nullptr,ExceptionSink()) ) , m_list.end() ) ;
		m_removed = false ;
	}
}

void GNet::TimerList::doTimeouts()
{
	G_ASSERT( m_list_added.empty() ) ;
	Lock lock( *this ) ;
	m_adjust = 0 ;
	G::EpochTime now( 0 ) ; // lazy initialisation to G::DateTime::now() in G::Timer::expired()
	for( List::iterator p = m_list.begin() ; p != m_list.end() ; ++p )
	{
		if( (*p).m_timer != nullptr && (*p).m_timer->active() && (*p).m_timer->expired(now) )
		{
			try
			{
				if( (*p).m_timer == m_soonest ) m_soonest = nullptr ;
				(*p).m_timer->doTimeout() ;
			}
			catch( GNet::Done & e ) // (caught separately to avoid requiring rtti)
			{
				if( (*p).m_es.set() )
					(*p).m_es.call( e , true ) ; // call onException()
				else
					throw ; // (new)
			}
			catch( std::exception & e )
			{
				if( (*p).m_es.set() )
					(*p).m_es.call( e , false ) ; // call onException()
				else
					throw ; // (new)
			}
		}
	}
	unlock() ; // avoid doing possibly-throwing operations in Lock dtor
}

std::string GNet::TimerList::report() const
{
	std::ostringstream ss ;
	ss << m_list.size() ;
	return ss.str() ;
}

/// \file gtimerlist.cpp
