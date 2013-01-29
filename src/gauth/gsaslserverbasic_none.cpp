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
// gsaslserverbasic_none.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gauth.h"
#include "gsaslserverbasic.h"

std::string GAuth::SaslServerBasic::mechanisms( char ) const
{
	return std::string() ;
}

std::string GAuth::SaslServerBasic::mechanism() const
{
	return std::string() ;
}

bool GAuth::SaslServerBasic::trusted( GNet::Address ) const
{
	return false ;
}

GAuth::SaslServerBasic::SaslServerBasic( const SaslServer::Secrets & , bool , bool ) :
	m_imp(NULL)
{
}

bool GAuth::SaslServerBasic::active() const
{
	return false ;
}

GAuth::SaslServerBasic::~SaslServerBasic()
{
}

bool GAuth::SaslServerBasic::mustChallenge() const
{
	return false ;
}

bool GAuth::SaslServerBasic::init( const std::string & )
{
	return true ;
}

std::string GAuth::SaslServerBasic::initialChallenge() const
{
	return std::string() ;
}

std::string GAuth::SaslServerBasic::apply( const std::string & , bool & done )
{
	done = true ;
	return std::string() ;
}

bool GAuth::SaslServerBasic::authenticated() const
{
	return false ;
}

std::string GAuth::SaslServerBasic::id() const
{
	return std::string() ;
}

bool GAuth::SaslServerBasic::requiresEncryption() const
{
	return false ;
}

/// \file gsaslserverbasic_none.cpp
