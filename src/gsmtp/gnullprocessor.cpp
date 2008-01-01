//
// Copyright (C) 2001-2008 Graeme Walker <graeme_walker@users.sourceforge.net>
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

GSmtp::NullProcessor::NullProcessor()
{
}

GSmtp::NullProcessor::~NullProcessor()
{
}

bool GSmtp::NullProcessor::cancelled() const
{
	return false ;
}

bool GSmtp::NullProcessor::repoll() const
{
	return false ;
}

std::string GSmtp::NullProcessor::text() const
{
	return std::string() ;
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
	m_done_signal.emit( true ) ;
}

/// \file gnullprocessor.cpp
