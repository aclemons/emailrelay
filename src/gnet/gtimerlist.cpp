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
/// \file gtimerlist.cpp
///

#include "gdef.h"
#include "gtimerlist.h"
#include "gtimer.h"
#include "gnetdone.h"
#include "geventloop.h"
#include "geventloggingcontext.h"
#include "glog.h"
#include "gassert.h"
#include <algorithm>
#include <functional>
#include <sstream>

GNet::TimerList::ListItem::ListItem( TimerBase * t , EventState es ) :
	m_timer(t) ,
	m_es(es)
{
}

#ifndef G_LIB_SMALL
inline bool GNet::TimerList::ListItem::operator==( const ListItem & rhs ) const noexcept
{
	return m_timer == rhs.m_timer ;
}
#endif

inline void GNet::TimerList::ListItem::resetIf( TimerBase * p ) noexcept
{
	if( m_timer == p )
		m_timer = nullptr ;
}

void GNet::TimerList::ListItem::disarmIf( ExceptionHandler * eh ) noexcept
{
	if( m_es.eh() == eh )
		m_es.disarm() ;
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

GNet::TimerList::TimerList()
{
	if( m_this == nullptr )
		m_this = this ;
}

GNet::TimerList::~TimerList()
{
	if( m_this == this )
		m_this = nullptr ;
}

void GNet::TimerList::add( TimerBase & t , EventState es )
{
	(m_locked?m_list_added:m_list).emplace_back( &t , es ) ;
}

void GNet::TimerList::remove( TimerBase & timer ) noexcept
{
	m_removed = true ;
	removeFrom( m_list , &timer ) ;
	removeFrom( m_list_added , &timer ) ;
	if( m_soonest == &timer ) m_soonest = nullptr ;
}

void GNet::TimerList::removeFrom( List & list , TimerBase * timer_p ) noexcept
{
	for( auto & list_item : list )
		list_item.resetIf( timer_p ) ;
}

void GNet::TimerList::disarm( ExceptionHandler * eh ) noexcept
{
	disarmIn( m_list , eh ) ;
	disarmIn( m_list_added , eh ) ;
}

void GNet::TimerList::disarmIn( List & list , ExceptionHandler * eh ) noexcept
{
	for( auto & list_item : list )
		list_item.disarmIf( eh ) ;
}

void GNet::TimerList::updateOnStart( TimerBase & timer )
{
	if( timer.immediate() )
		timer.adjust( m_adjust++ ) ; // well-defined t() order for immediate timers

	if( m_soonest == &timer )
		m_soonest = nullptr ;

	if( m_soonest != nullptr && timer.tref() < m_soonest->tref() )
		m_soonest = &timer ;
}

void GNet::TimerList::updateOnCancel( TimerBase & timer )
{
	G_ASSERT( !timer.active() ) ;
	if( m_soonest == &timer )
		m_soonest = nullptr ;
}

const GNet::TimerBase * GNet::TimerList::findSoonest() const
{
	G_ASSERT( !m_locked ) ;
	TimerBase * result = nullptr ;
	for( const auto & t : m_list )
	{
		if( t.m_timer != nullptr && t.m_timer->active() && ( result == nullptr || t.m_timer->tref() < result->tref() ) )
			result = t.m_timer ;
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
		G::TimerTime now = G::TimerTime::now() ;
		G::TimerTime then = m_soonest->t() ;
		return std::make_pair( G::TimeInterval(now,then) , false ) ;
	}
}

GNet::TimerList * GNet::TimerList::ptr() noexcept
{
	return m_this ;
}

#ifndef G_LIB_SMALL
bool GNet::TimerList::exists()
{
	return m_this != nullptr ;
}
#endif

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
		if( m_soonest != nullptr && (m_list.size()+m_list_added.size()) > m_list.capacity() )
			m_soonest = nullptr ; // about to be invalidated
		m_list.reserve( m_list.size() + m_list_added.size() ) ;
		m_list.insert( m_list.end() , m_list_added.begin() , m_list_added.end() ) ;
		m_list_added.clear() ;
	}
}

void GNet::TimerList::purgeRemoved()
{
	if( m_removed )
	{
		m_removed = false ;
		m_soonest = nullptr ; // about to be invalidated
		m_list.erase( std::remove_if( m_list.begin() , m_list.end() ,
			[](const ListItem &v_){ return v_.m_timer == nullptr ; } ) , m_list.end() ) ;
	}
}

void GNet::TimerList::doTimeouts()
{
	G_ASSERT( m_list_added.empty() ) ;
	Lock lock( *this ) ;
	m_adjust = 0 ;
	G::TimerTime now = G::TimerTime::zero() ; // lazy initialisation to G::TimerTime::now() in G::Timer::expired()

	// move expired timers to the front
	auto expired_end = std::partition( m_list.begin() , m_list.end() ,
		[&now](const ListItem &li_){ return li_.m_timer != nullptr && li_.m_timer->active() && li_.m_timer->expired(now) ; } ) ;

	// sort expired timers so that they are handled in time order
	std::sort( m_list.begin() , expired_end ,
		[](const ListItem &a,const ListItem &b){ return a.m_timer->tref() < b.m_timer->tref() ; } ) ;

	// invalidate the soonest pointer, except in the degenerate case where nothing changes
	if( expired_end != m_list.begin() )
		m_soonest = nullptr ;

	// call each expired timer's handler
	for( List::iterator item_p = m_list.begin() ; item_p != expired_end ; ++item_p )
	{
		// (make sure the timer is still valid and expired in case another timer's handler has changed it)
		if( item_p->m_timer != nullptr && item_p->m_timer->active() && item_p->m_timer->expired(now) )
			doTimeout( *item_p ) ;
	}

	// unlock the list explicitly to avoid the Lock dtor throwing
	unlock() ;
}

void GNet::TimerList::doTimeout( ListItem & item )
{
	// see also GNet::EventEmitter::raiseEvent()
	EventLoggingContext set_logging_context( item.m_es ) ;
	try
	{
		item.m_timer->doTimeout() ;
	}
	catch( GNet::Done & e ) // (caught separately to avoid requiring rtti)
	{
		if( item.m_es.hasExceptionHandler() )
			item.m_es.doOnException( e , true ) ;
		else
			throw ;
	}
	catch( std::exception & e )
	{
		if( item.m_es.hasExceptionHandler() )
			item.m_es.doOnException( e , false ) ;
		else
			throw ;
	}
}

