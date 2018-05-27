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
// gverifierfactory.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gfactoryparser.h"
#include "gexception.h"
#include "gverifierfactory.h"
#include "ginternalverifier.h"
#include "gexecutableverifier.h"
#include "gnetworkverifier.h"

std::string GSmtp::VerifierFactory::check( const std::string & identifier )
{
	return FactoryParser::check( identifier , false ) ;
}

GSmtp::Verifier * GSmtp::VerifierFactory::newVerifier( GNet::ExceptionHandler & exception_handler ,
	const std::string & identifier , unsigned int timeout , bool compatible )
{
	FactoryParser::Result p = FactoryParser::parse( identifier , false ) ;
	if( p.first.empty() || p.first == "exit" )
	{
		return new InternalVerifier ;
	}
	else if( p.first == "net" )
	{
		return new NetworkVerifier( exception_handler , p.second , timeout , timeout , compatible ) ;
	}
	else
	{
		return new ExecutableVerifier( exception_handler , G::Path(p.second) , compatible ) ;
	}
}

/// \file gverifierfactory.cpp
