//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gtimer.cpp
///

#include "gdef.h"
#include "gtimer.h"
#include "gtimerlist.h"
#include "gevent.h"
#include "glog.h"
#include "gassert.h"
#include <algorithm>

GNet::TimerBase::TimerBase( ExceptionSink es ) :
	m_time(G::TimerTime::zero())
{
	TimerList::instance().add( *this , es ) ;
	G_ASSERT( !active() ) ;
	G_ASSERT( history() < G::TimerTime::now() ) ;
}

GNet::TimerBase::~TimerBase()
{
	try
	{
		if( TimerList::ptr() != nullptr )
			TimerList::ptr()->remove( *this ) ;
	}
	catch(...) // dtor
	{
	}
}

bool GNet::TimerBase::expired( G::TimerTime & now ) const
{
	if( !active() )
	{
		return false ;
	}
	else if( immediate() )
	{
		return true ;
	}
	else
	{
		// lazy evaluation of caller's idea of now -- no call
		// to TimerTime::now() if there is a zero-length
		// timer or no timers at all
		if( now == G::TimerTime::zero() )
			now = G::TimerTime::now() ;

		return m_time <= now ;
	}
}

void GNet::TimerBase::startTimer( unsigned int time , unsigned int time_us )
{
	m_time = (time==0U && time_us==0U) ? history() : ( G::TimerTime::now() + G::TimeInterval(time,time_us) ) ;
	TimerList::instance().updateOnStart( *this ) ; // adjust()
	G_ASSERT( active() ) ;
}

void GNet::TimerBase::startTimer( const G::TimeInterval & i )
{
	m_time = i == G::TimeInterval(0U) ? history() : ( G::TimerTime::now() + i ) ;
	TimerList::instance().updateOnStart( *this ) ; // adjust()
	G_ASSERT( active() ) ;
}

G::TimerTime GNet::TimerBase::history()
{
	// an arbitrary historical non-zero epoch time used for all immediate() timers
	return G::TimerTime::zero() + G::TimeInterval( 1U ) ;
}

bool GNet::TimerBase::immediate() const
{
	return m_time.sameSecond( history() ) ; // history() ignoring any adjust()ment
}

void GNet::TimerBase::adjust( unsigned int us )
{
	G_ASSERT( immediate() ) ;
	G_ASSERT( us == 0U || (history()+G::TimeInterval(0U,std::min(999999U,us))) != m_time ) ;
	m_time = history() + G::TimeInterval( 0U , std::min( 999999U , us ) ) ;
	G_ASSERT( immediate() ) ;
}

void GNet::TimerBase::cancelTimer()
{
	if( active() )
	{
		m_time = G::TimerTime::zero() ;
		TimerList::instance().updateOnCancel( *this ) ;
	}
	G_ASSERT( !active() ) ;
}

void GNet::TimerBase::doTimeout()
{
	G_ASSERT( active() ) ;
	m_time = G::TimerTime::zero() ;
	onTimeout() ;
}

G::TimerTime GNet::TimerBase::t() const
{
	return m_time ;
}

