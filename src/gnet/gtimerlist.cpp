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
//
// gtimerlist.cpp
//

#include "gdef.h"
#include "gtimerlist.h"
#include "gtimer.h"
#include "geventloop.h"
#include "glog.h"

GNet::TimerList * GNet::TimerList::m_this = NULL ;

GNet::TimerList::TimerList() :
	m_run_on_destruction(true) ,
	m_list_changed(false) ,
	m_empty_set_timeout_hint(false) ,
	m_soonest_changed(true) ,
	m_soonest(99U)
{
	if( m_this == NULL )
		m_this = this ;
}

GNet::TimerList::~TimerList()
{
	if( m_run_on_destruction )
	{
		try
		{
			doTimeouts() ;
		}
		catch(...)
		{
		}
	}

	if( m_this == this )
		m_this = NULL ;
}

void GNet::TimerList::add( AbstractTimer & t )
{
	m_list_changed = true ;
	m_list.push_back( &t ) ;
}

void GNet::TimerList::remove( AbstractTimer & t )
{
	for( List::iterator p = m_list.begin() ; p != m_list.end() ; ++p )
	{
		if( *p == &t )
		{
			*p = NULL ;
			break ;
		}
	}
}

void GNet::TimerList::update( G::DateTime::EpochTime t_old )
{
	// after any change in the soonest() time notify the event loop 

	G::DateTime::EpochTime t_new = soonest() ;
	if( t_old != t_new )
	{
		m_soonest_changed = true ;
		if( EventLoop::exists() )
		{
			G_DEBUG( "GNet::TimerList::update: " << t_old << " -> " << t_new ) ;
			EventLoop::instance().setTimeout( t_new , m_empty_set_timeout_hint ) ;
		}
	}
}

void GNet::TimerList::update()
{
	// this overload just assumes that the soonest() time has probably changed

	m_soonest_changed = true ;
	if( EventLoop::exists() )
	{
		G::DateTime::EpochTime t_new = soonest() ;
		G_DEBUG( "GNet::TimerList::update: ? -> " << t_new ) ;
		EventLoop::instance().setTimeout( t_new , m_empty_set_timeout_hint ) ;
	}
}

G::DateTime::EpochTime GNet::TimerList::soonest() const
{
	G::DateTime::EpochTime result = 0U ;
	const List::const_iterator end = m_list.end() ;
	for( List::const_iterator p = m_list.begin() ; p != end ; ++p )
	{
		if( *p != NULL && (*p)->t() != 0UL && ( result == 0U || (*p)->t() < result ) )
			result = (*p)->t() ;
	}
	return result ;
}

G::DateTime::EpochTime GNet::TimerList::soonest( int ) const
{
	// this optimised overload is for interval() which
	// gets called on _every_ fd event

	if( m_soonest_changed )
	{
		TimerList * This = const_cast<TimerList*>(this) ;
		This->m_soonest = soonest() ;
		This->m_soonest_changed = false ;
	}
	//G_ASSERT( valid() ) ; // optimisation lost if this is active
	return m_soonest ;
}

bool GNet::TimerList::valid() const
{
	if( soonest() != m_soonest )
	{
		G_ERROR( "GNet::TimerList::valid: soonest()=" << soonest() << ", m_soonest=" << m_soonest ) ;
		return false ;
	}
	return true ;
}

unsigned int GNet::TimerList::interval( bool & infinite ) const
{
	G::DateTime::EpochTime then = soonest(0) ; // fast
	infinite = then == 0U ;
	if( infinite )
	{
		return 0U ;
	}
	else
	{
		G::DateTime::EpochTime now = G::DateTime::now() ;
		return now >= then ? 0U : static_cast<unsigned int>(then-now) ;
	}
}

GNet::TimerList * GNet::TimerList::instance( const NoThrow & )
{
	return m_this ;
}

GNet::TimerList & GNet::TimerList::instance()
{
	if( m_this == NULL )
		throw NoInstance() ;

	return * m_this ;
}

void GNet::TimerList::doTimeouts()
{
	G_DEBUG( "GNet::TimerList::doTimeouts" ) ;
	G::DateTime::EpochTime now = G::DateTime::now() ;

	for( List::iterator p = m_list.begin() ; p != m_list.end() ; ++p )
	{
		if( *p != NULL )
		{
			G::DateTime::EpochTime t = (*p)->t() ;
			if( t != 0U && now >= t )
			{
				(*p)->doTimeout() ;
			}
		}
	}

	// deal with any change in the soonest() time
	update() ;
}

void GNet::TimerList::collectGarbage()
{
	for( List::iterator p = m_list.begin() ; p != m_list.end() ; )
	{
		if( *p == NULL )
			p = m_list.erase( p ) ;
		else
			++p ;
	}
}

/// \file gtimerlist.cpp
