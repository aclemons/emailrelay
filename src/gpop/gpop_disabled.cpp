//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gpop_disabled.cpp
///

#include "gdef.h"
#include "gpop.h"
#include "gsecrets.h"

bool GPop::enabled() noexcept
{
	return false ;
}

std::unique_ptr<GPop::Store> GPop::newStore( const G::Path & , const Store::Config & )
{
	return {} ;
}

std::unique_ptr<GAuth::SaslServerSecrets> GPop::newSecrets( const std::string & )
{
	return {} ;
}

std::unique_ptr<GPop::Server> GPop::newServer( GNet::EventState , Store & ,
	const GAuth::SaslServerSecrets & , const Server::Config & )
{
	return {} ;
}

void GPop::report( const Server * , const std::string & )
{
}
