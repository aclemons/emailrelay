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
/// \file gnullfilter.cpp
///

#include "gdef.h"
#include "gnullfilter.h"
#include "gassert.h"
#include "gstr.h"

#ifndef G_LIB_SMALL
GFilters::NullFilter::NullFilter( GNet::EventState es , GStore::FileStore & ,
	Filter::Type filter_type , const Filter::Config & ) :
		m_id("exit") ,
		m_exit(0,filter_type) ,
		m_quiet(true) ,
		m_timeout(0) ,
		m_timer(*this,&NullFilter::onTimeout,es)
{
}
#endif

GFilters::NullFilter::NullFilter( GNet::EventState es , GStore::FileStore & ,
	Filter::Type filter_type , const Filter::Config & ,
	unsigned int exit_code ) :
		m_id("exit:"+G::Str::fromUInt(exit_code)) ,
		m_exit(static_cast<int>(exit_code),filter_type) ,
		m_quiet(exit_code==0U) ,
		m_timeout(0) ,
		m_timer(*this,&NullFilter::onTimeout,es)
{
}

GFilters::NullFilter::NullFilter( GNet::EventState es , GStore::FileStore & ,
	Filter::Type filter_type , const Filter::Config & , G::TimeInterval sleep_time ) :
		m_id("sleep") ,
		m_exit(0,filter_type) ,
		m_quiet(false) ,
		m_timeout(sleep_time) ,
		m_timer(*this,&NullFilter::onTimeout,es)
{
	G_ASSERT( m_exit.ok() ) ;
}

std::string GFilters::NullFilter::id() const
{
	return m_id ;
}

bool GFilters::NullFilter::quiet() const
{
	return m_quiet ;
}

bool GFilters::NullFilter::special() const
{
	return m_exit.special ;
}

GSmtp::Filter::Result GFilters::NullFilter::result() const
{
	return m_exit.result ;
}

std::string GFilters::NullFilter::response() const
{
	return ( m_exit.ok() || m_exit.abandon() ) ? std::string() : std::string("rejected") ;
}

int GFilters::NullFilter::responseCode() const
{
	return 0 ;
}

std::string GFilters::NullFilter::reason() const
{
	return ( m_exit.ok() || m_exit.abandon() ) ? std::string() : m_id ;
}

G::Slot::Signal<int> & GFilters::NullFilter::doneSignal() noexcept
{
	return m_done_signal ;
}

void GFilters::NullFilter::cancel()
{
}

void GFilters::NullFilter::start( const GStore::MessageId & )
{
	m_timer.startTimer( m_timeout ) ;
}

void GFilters::NullFilter::onTimeout()
{
	m_done_signal.emit( static_cast<int>(m_exit.result) ) ;
}

