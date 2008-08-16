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
// gsasl_none.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gsmtp.h"
#include "gsaslserver.h"
#include "gsaslclient.h"

std::string GSmtp::SaslServer::mechanisms( char c ) const
{
	return std::string() ;
}

std::string GSmtp::SaslServer::mechanism() const
{
	return std::string() ;
}

bool GSmtp::SaslServer::trusted( GNet::Address a ) const
{
	false ;
}

GSmtp::SaslServer::SaslServer( const SaslServer::Secrets & , bool , bool )
{
}

bool GSmtp::SaslServer::active() const
{
	return false ;
}

GSmtp::SaslServer::~SaslServer()
{
}

bool GSmtp::SaslServer::mustChallenge() const
{
	return false ;
}

bool GSmtp::SaslServer::init( const std::string & mechanism )
{
	return true ;
}

std::string GSmtp::SaslServer::initialChallenge() const
{
	return std::string() ;
}

std::string GSmtp::SaslServer::apply( const std::string & response , bool & done )
{
	done = true ;
	return std::string() ;
}

bool GSmtp::SaslServer::authenticated() const
{
	return false ;
}

std::string GSmtp::SaslServer::id() const
{
	return std::string() ;
}

// ===

GSmtp::SaslClient::SaslClient( const SaslClient::Secrets & , const std::string & )
{
}

GSmtp::SaslClient::~SaslClient()
{
}

bool GSmtp::SaslClient::active() const
{
	return false ;
}

std::string GSmtp::SaslClient::response( const std::string & mechanism , const std::string & challenge , 
	bool & done , bool & error , bool & sensitive ) const
{
	done = true ;
	error = false ;
	sensitive = false ;
	return std::string() ;
}

std::string GSmtp::SaslClient::preferred( const G::Strings & mechanism_list ) const
{
	return std::string() ;
}

// ===

GSmtp::Valid::~Valid() 
{
}

// ==

GSmtp::SaslServer::Secrets::~Secrets()
{
}

// ==

GSmtp::SaslClient::Secrets::~Secrets()
{
}

/// \file gsasl_none.cpp
