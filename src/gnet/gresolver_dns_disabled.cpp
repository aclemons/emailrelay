//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gresolver_dns_disabled.cpp
//

#include "gdef.h"
#include "gresolver.h"
#include "gstr.h"
#include "gaddress.h"
#include "glocation.h"

class GNet::ResolverImp
{
} ;

// ==

GNet::Resolver::Resolver( Resolver::Callback & callback , EventHandler & exception_handler ) :
	m_callback(callback) ,
	m_exception_handler(exception_handler) ,
	m_busy(false)
{
}

GNet::Resolver::~Resolver()
{
}

std::string GNet::Resolver::resolve( Location & location , bool )
{
	try
	{
		Address address( location.host() , G::Str::toUInt(location.service()) ) ;
		location.update( address , std::string() ) ;
		return std::string() ;
	}
	catch( std::exception & e )
	{
		return "invalid address" ;
	}
}

void GNet::Resolver::start( const Location & , bool )
{
	// never gets here
	throw std::runtime_error( "not implemented" ) ;
}

bool GNet::Resolver::busy() const
{
	return false ;
}

bool GNet::Resolver::async()
{
	return false ;
}

/// \file gresolver_dns_disabled.cpp
