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
// gtimer.cpp
//

#include "gdef.h"
#include "gtimer.h"
#include "gtimerlist.h"
#include "gevent.h"
#include "glog.h"
#include "gassert.h"
#include <algorithm>

GNet::TimerBase::TimerBase( ExceptionSink es ) :
	m_time(0)
{
	TimerList::instance().add( *this , es ) ;
}

GNet::TimerBase::~TimerBase()
{
	try
	{
		if( TimerList::instance(TimerList::NoThrow()) != nullptr )
			TimerList::instance().remove( *this ) ;
	}
	catch(...) // dtor
	{
	}
}

bool GNet::TimerBase::expired( G::EpochTime & now ) const
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
		if( now.s == 0 ) now = G::DateTime::now() ;
		return m_time <= now ;
	}
}

void GNet::TimerBase::startTimer( unsigned int time , unsigned int time_us )
{
	m_time = (time==0U && time_us==0U) ? history() : ( G::DateTime::now() + G::TimeInterval(time,time_us) ) ;
	TimerList::instance().updateOnStart( *this ) ;
}

G::EpochTime GNet::TimerBase::history()
{
	return G::EpochTime( 1 ) ;
}

bool GNet::TimerBase::immediate() const
{
	return m_time.s == 1 ; // history() with any adjust()ment
}

void GNet::TimerBase::adjust( unsigned int us )
{
	G_ASSERT( m_time.s == 1 ) ;
	m_time.us = std::min( 999999U , us ) ;
}

void GNet::TimerBase::cancelTimer()
{
	if( active() )
	{
		m_time = G::EpochTime(0) ;
		TimerList::instance().updateOnCancel( *this ) ;
	}
}

void GNet::TimerBase::doTimeout()
{
	G_ASSERT( active() ) ;
	m_time = G::EpochTime(0) ;
	onTimeout() ;
}

G::EpochTime GNet::TimerBase::t() const
{
	return m_time ;
}

/// \file gtimer.cpp
