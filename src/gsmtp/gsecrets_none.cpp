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
// gsecrets_none.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gsecrets.h"

GSmtp::Secrets::Secrets( const std::string & , const std::string & , const std::string & )
{
}

GSmtp::Secrets::Secrets()
{
}

GSmtp::Secrets::~Secrets()
{
}

bool GSmtp::Secrets::valid() const
{
	return false ;
}

std::string GSmtp::Secrets::id( const std::string & mechanism ) const
{
	return std::string() ;
}

std::string GSmtp::Secrets::secret( const std::string & mechanism ) const
{
	return std::string() ;
}

std::string GSmtp::Secrets::secret(  const std::string & mechanism , const std::string & id ) const
{
	return std::string() ;
}

bool GSmtp::Secrets::contains( const std::string & mechanism ) const
{
	return false ;
}

/// \file gsecrets_none.cpp
