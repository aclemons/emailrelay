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
	m_server_side(server_side) ,
	m_exit_code(0) ,
	m_special_cancelled(false) ,
	m_special_other(false) ,
	m_ok(true) ,
	m_timer(*this,&NullFilter::onDoneTimeout,eh)
{
}

GSmtp::NullFilter::NullFilter( GNet::ExceptionHandler & eh , bool server_side , unsigned int exit_code ) :
	m_server_side(server_side) ,
	m_exit_code(exit_code) ,
	m_special_cancelled(false) ,
	m_special_other(false) ,
	m_ok(false) ,
	m_timer(*this,&NullFilter::onDoneTimeout,eh)
{
	Exit exit( exit_code , server_side ) ;
	m_ok = exit.ok ;
	m_special_cancelled = exit.cancelled ;
	m_special_other = exit.other ;
}

GSmtp::NullFilter::~NullFilter()
{
}

std::string GSmtp::NullFilter::id() const
{
	return m_exit_code == 0 ? "null" : ( "exit:" + G::Str::fromUInt(m_exit_code) ) ;
}

bool GSmtp::NullFilter::simple() const
{
	return true ;
}

bool GSmtp::NullFilter::specialCancelled() const
{
	return m_special_cancelled ;
}

bool GSmtp::NullFilter::specialOther() const
{
	return m_special_other ;
}

std::string GSmtp::NullFilter::text() const
{
	return m_ok ? std::string() : std::string("error") ;
}

G::Slot::Signal1<bool> & GSmtp::NullFilter::doneSignal()
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

void GSmtp::NullFilter::onDoneTimeout()
{
	m_done_signal.emit( m_ok ) ;
}

/// \file gnullfilter.cpp
