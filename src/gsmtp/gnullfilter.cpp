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
// gnullfilter.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gnullfilter.h"
#include "gstr.h"

GSmtp::NullFilter::NullFilter( GNet::ExceptionHandler & eh , bool server_side ) :
	m_exit(0,server_side) ,
	m_id("none") ,
	m_timer(*this,&NullFilter::onTimeout,eh)
{
}

GSmtp::NullFilter::NullFilter( GNet::ExceptionHandler & eh , bool server_side , unsigned int exit_code ) :
	m_exit(exit_code,server_side) ,
	m_id("exit:"+G::Str::fromUInt(exit_code)) ,
	m_timer(*this,&NullFilter::onTimeout,eh)
{
}

GSmtp::NullFilter::~NullFilter()
{
}

std::string GSmtp::NullFilter::id() const
{
	return m_id ;
}

bool GSmtp::NullFilter::simple() const
{
	return true ;
}

bool GSmtp::NullFilter::special() const
{
	return m_exit.special ;
}

std::string GSmtp::NullFilter::response() const
{
	return ( m_exit.ok() || m_exit.abandon() ) ? std::string() : std::string("rejected") ;
}

std::string GSmtp::NullFilter::reason() const
{
	return ( m_exit.ok() || m_exit.abandon() ) ? std::string() : m_id ;
}

G::Slot::Signal1<int> & GSmtp::NullFilter::doneSignal()
{
	return m_done_signal ;
}

void GSmtp::NullFilter::cancel()
{
}

void GSmtp::NullFilter::start( const std::string & )
{
	m_timer.startTimer( 0U ) ;
}

void GSmtp::NullFilter::onTimeout()
{
	m_done_signal.emit( m_exit.result ) ;
}

bool GSmtp::NullFilter::abandoned() const
{
	return m_exit.abandon() ;
}

/// \file gnullfilter.cpp