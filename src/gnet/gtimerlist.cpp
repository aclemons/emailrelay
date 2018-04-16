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
//
// gtimerlist.cpp
//

#include "gdef.h"
#include "gtimerlist.h"
#include "gtimer.h"
#include "geventloop.h"
#include "glog.h"
#include "gassert.h"
#include <algorithm>
#include <sstream>

GNet::TimerList * GNet::TimerList::m_this = nullptr ;

GNet::TimerList::TimerList() :
	m_soonest(nullptr)
{
	if( m_this == nullptr )
		m_this = this ;
}

GNet::TimerList::~TimerList()
{
	if( m_this == this )
		m_this = nullptr ;
}

void GNet::TimerList::add( TimerBase & t , ExceptionHandler & eh )
{
	G_ASSERT( !t.active() ) ; // (called by ctor)
	m_list.push_back( Value(&t,&eh) ) ;
}

void GNet::TimerList::remove( TimerBase & timer )
{
	const List::iterator end = m_list.end() ;
	for( List::iterator p = m_list.begin() ; p != end ; ++p )
	{
		if( (*p).first == &timer )
			(*p).first = nullptr ;
	}

	if( m_soonest == &timer )
	{
		m_soonest = findSoonest() ;
		setTimeout() ;
	}
}

void GNet::TimerList::disarm( ExceptionHandler * eh )
{
	const List::iterator end = m_list.end() ;
	for( List::iterator p = m_list.begin() ; p != end ; ++p )
	{
		if( (*p).second == eh )
			(*p).second = nullptr ;
	}
}

void GNet::TimerList::update( TimerBase & timer )
{
	if( !timer.active() && m_soonest != &timer )
	{
		; // no-op -- cancelled a non-soonest timer
	}
	else if( timer.active() && m_soonest != nullptr && timer.t() > m_soonest->t() )
	{
		; // no-op -- started a non-soonest timer
	}
	else
	{
		G::EpochTime old_soonest = soonestTime() ;
		m_soonest = (timer.active() && timer.immediate()) ? &timer : findSoonest() ;
		if( soonestTime() != old_soonest )
			setTimeout() ;
	}
}

G::EpochTime GNet::TimerList::soonestTime() const
{
	return m_soonest == nullptr ? G::EpochTime(0) : m_soonest->t() ;
}

GNet::TimerBase * GNet::TimerList::findSoonest()
{
	// (we could keep the list sorted to make this O(1), but keep it simple for now)
	TimerBase * result = nullptr ;
	const List::const_iterator end = m_list.end() ;
	for( List::const_iterator p = m_list.begin() ; p != end ; ++p )
	{
		if( (*p).first != nullptr && (*p).first->active() && ( result == nullptr || (*p).first->t() < result->t() ) )
			result = (*p).first ;
	}
	return result ;
}

G::EpochTime GNet::TimerList::interval( bool & infinite ) const
{
	if( m_soonest == nullptr )
	{
		infinite = true ;
		return G::EpochTime(0) ;
	}
	else if( m_soonest->immediate() )
	{
		infinite = false ;
		return G::EpochTime(0) ;
	}
	else
	{
		infinite = false ;
		G::EpochTime now = G::DateTime::now() ;
		G::EpochTime then = m_soonest->t() ;
		return now >= then ? G::EpochTime(0) : (then-now) ;
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

void GNet::TimerList::doTimeouts()
{
	G::EpochTime now( 0 ) ; // lazy initialisation to G::DateTime::now() in G::Timer::expired()
	for( List::iterator p = m_list.begin() ; p != m_list.end() ; ++p )
	{
		if( (*p).first != nullptr && (*p).first->active() && (*p).first->expired(now) )
		{
			try
			{
				(*p).first->doTimeout() ;
			}
			catch( std::exception & e )
			{
				if( (*p).second )
					(*p).second->onException( e ) ;
			}
		}
	}

	collectGarbage() ;

	m_soonest = findSoonest() ;
	setTimeout() ;
}

void GNet::TimerList::setTimeout()
{
	if( EventLoop::exists() )
		EventLoop::instance().setTimeout( soonestTime() ) ;
}

void GNet::TimerList::collectGarbage()
{
	for( List::iterator p = m_list.begin() ; p != m_list.end() ; )
	{
		if( (*p).first == nullptr )
			p = m_list.erase( p ) ;
		else
			++p ;
	}
}

std::string GNet::TimerList::report() const
{
	std::ostringstream ss ;
	ss << m_list.size() ;
	return ss.str() ;
}

/// \file gtimerlist.cpp
