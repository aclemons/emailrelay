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
// admin_disabled.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "admin.h"
#include <stdexcept>

bool Main::Admin::enabled()
{
	return false ;
}

std::auto_ptr<GSmtp::AdminServer> Main::Admin::newServer( const Configuration & , 
	GSmtp::MessageStore & , const GSmtp::Client::Config & , 
	const GAuth::Secrets & , const std::string & )
{
	throw std::runtime_error( "admin interface not supported: not enabled at build time" ) ;
}

void Main::Admin::notify( GSmtp::AdminServer & , const std::string & , const std::string & , const std::string & )
{
}

void Main::Admin::report( const GSmtp::AdminServer & )
{
}

/// \file admin_disabled.cpp
