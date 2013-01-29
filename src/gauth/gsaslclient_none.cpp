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
// gsaslclient_none.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gauth.h"
#include "gsaslclient.h"

GAuth::SaslClient::SaslClient( const SaslClient::Secrets & , const std::string & )
{
}

GAuth::SaslClient::~SaslClient()
{
}

bool GAuth::SaslClient::active() const
{
	return false ;
}

std::string GAuth::SaslClient::response( const std::string & , const std::string & , 
	bool & done , bool & error , bool & sensitive ) const
{
	done = true ;
	error = false ;
	sensitive = false ;
	return std::string() ;
}

std::string GAuth::SaslClient::preferred( const G::Strings & ) const
{
	return std::string() ;
}

// ==

GAuth::SaslClient::Secrets::~Secrets()
{
}

/// \file gsaslclient_none.cpp
