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
// gnullprocessor.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gnullprocessor.h"

GSmtp::NullProcessor::NullProcessor() :
	m_cancelled(false) ,
	m_repoll(false) ,
	m_ok(true)
{
}

GSmtp::NullProcessor::NullProcessor( unsigned int exit_code ) :
	m_cancelled(false) ,
	m_repoll(false) ,
	m_ok(false)
{
	bool is_special = exit_code >= 100U && exit_code <= 107U ;
	m_repoll = is_special && ((exit_code-100U)&2U) != 0U ;
	m_cancelled = is_special && ((exit_code-100U)&1U) == 0U ;
	m_ok = exit_code == 0 || ( is_special && !m_cancelled ) ;
}

GSmtp::NullProcessor::~NullProcessor()
{
}

bool GSmtp::NullProcessor::cancelled() const
{
	return m_cancelled ;
}

bool GSmtp::NullProcessor::repoll() const
{
	return m_repoll ;
}

std::string GSmtp::NullProcessor::text() const
{
	return m_ok ? std::string() : std::string("error") ;
}

G::Signal1<bool> & GSmtp::NullProcessor::doneSignal()
{
	return m_done_signal ;
}

void GSmtp::NullProcessor::abort()
{
}

void GSmtp::NullProcessor::start( const std::string & )
{
	m_done_signal.emit( m_ok ) ;
}

/// \file gnullprocessor.cpp
