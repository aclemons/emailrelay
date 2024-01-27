//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
///
/// \file gpam_none.cpp
///

#include "gdef.h"
#include "gpam.h"
#include <string>

class G::PamImp
{
} ;

G::Pam::Pam( const std::string & , const std::string & , bool )
{
}

G::Pam::~Pam()
= default;

bool G::Pam::authenticate( bool )
{
	throw Error( "authenticate" , 0 ) ;
}

void G::Pam::checkAccount( bool )
{
}

void G::Pam::establishCredentials()
{
}

void G::Pam::openSession()
{
}

void G::Pam::closeSession()
{
}

void G::Pam::deleteCredentials()
{
}

void G::Pam::reinitialiseCredentials()
{
}

void G::Pam::refreshCredentials()
{
}

std::string G::Pam::name() const
{
	return std::string() ;
}

