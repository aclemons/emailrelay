//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#ifndef G_LIB_SMALL
GNet::TimerList::Value::Value()
= default;
#endif

GNet::TimerList::Value::Value( TimerBase * t , ExceptionSink es ) :
	m_timer(t) ,
	m_es(es)
{
}

inline bool GNet::TimerList::Value::operator==( const Value & v ) const noexcept
{
	return m_timer == v.m_timer ;
}

inline void GNet::TimerList::Value::resetIf( TimerBase * p ) noexcept
{
	if( m_timer == p )
		m_timer = nullptr ;
}

void GNet::TimerList::Value::disarmIf( ExceptionHandler * eh ) noexcept
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

void GNet::TimerList::add( TimerBase & t , ExceptionSink es )
{
	(m_locked?m_list_added:m_list).push_back( Value(&t,es) ) ;
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
	for( auto & value : list )
		value.resetIf( timer_p ) ;
}

void GNet::TimerList::disarm( ExceptionHandler * eh ) noexcept
{
	disarmIn( m_list , eh ) ;
	disarmIn( m_list_added , eh ) ;
}

void GNet::TimerList::disarmIn( List & list , ExceptionHandler * eh ) noexcept
{
	for( auto & value : list )
		value.disarmIf( eh ) ;
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
		m_list.erase( std::remove( m_list.begin() , m_list.end() , Value(nullptr,{}) ) , m_list.end() ) ;
	}
}

void GNet::TimerList::doTimeouts()
{
	G_ASSERT( m_list_added.empty() ) ;
	Lock lock( *this ) ;
	m_adjust = 0 ;
	G::TimerTime now = G::TimerTime::zero() ; // lazy initialisation to G::TimerTime::now() in G::Timer::expired()

	auto expired_end = std::partition( m_list.begin() , m_list.end() ,
		[&now](const Value &value){ return value.m_timer != nullptr && value.m_timer->active() && value.m_timer->expired(now) ; } ) ;

	std::sort( m_list.begin() , expired_end ,
		[](const Value &a,const Value &b){ return a.m_timer->tref() < b.m_timer->tref() ; } ) ;

	if( expired_end != m_list.begin() )
		m_soonest = nullptr ; // the soonest timer will in the expired list, so invalidate it

	for( List::iterator value_p = m_list.begin() ; value_p != expired_end ; ++value_p )
	{
		if( value_p->m_timer != nullptr && value_p->m_timer->active() && value_p->m_timer->expired(now) ) // still
			doTimeout( *value_p ) ;
	}

	unlock() ; // avoid doing possibly-throwing operations in Lock dtor
}

void GNet::TimerList::doTimeout( Value & value )
{
	// see also GNet::EventEmitter::raiseEvent()
	EventLoggingContext set_logging_context( value.m_es.esrc() ) ;
	try
	{
		value.m_timer->doTimeout() ;
	}
	catch( GNet::Done & e ) // (caught separately to avoid requiring rtti)
	{
		if( value.m_es.set() )
			value.m_es.call( e , true ) ; // call onException()
		else
			throw ; // (new)
	}
	catch( std::exception & e )
	{
		if( value.m_es.set() )
			value.m_es.call( e , false ) ; // call onException()
		else
			throw ; // (new)
	}
}

