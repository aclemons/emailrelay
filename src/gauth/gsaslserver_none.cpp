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
// gsaslserver_none.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gauth.h"
#include "gsaslserver.h"

std::string GAuth::SaslServer::mechanisms( char ) const
{
	return std::string() ;
}

std::string GAuth::SaslServer::mechanism() const
{
	return std::string() ;
}

bool GAuth::SaslServer::trusted( GNet::Address ) const
{
	return false ;
}

GAuth::SaslServer::SaslServer( const SaslServer::Secrets & , bool , bool )
{
}

bool GAuth::SaslServer::active() const
{
	return false ;
}

GAuth::SaslServer::~SaslServer()
{
}

bool GAuth::SaslServer::mustChallenge() const
{
	return false ;
}

bool GAuth::SaslServer::init( const std::string & )
{
	return true ;
}

std::string GAuth::SaslServer::initialChallenge() const
{
	return std::string() ;
}

std::string GAuth::SaslServer::apply( const std::string & , bool & done )
{
	done = true ;
	return std::string() ;
}

bool GAuth::SaslServer::authenticated() const
{
	return false ;
}

std::string GAuth::SaslServer::id() const
{
	return std::string() ;
}

bool GAuth::SaslServer::requiresEncryption()
{
	return false ;
}

// ==

GAuth::SaslServer::Secrets::~Secrets()
{
}

/// \file gsaslserver_none.cpp
