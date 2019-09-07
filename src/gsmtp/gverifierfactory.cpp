//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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

unique_ptr<GSmtp::Verifier> GSmtp::VerifierFactory::newVerifier( GNet::ExceptionSink es ,
	const std::string & identifier , unsigned int timeout )
{
	unique_ptr<Verifier> ptr ;
	FactoryParser::Result p = FactoryParser::parse( identifier , false ) ;
	if( p.first.empty() || p.first == "exit" )
	{
		ptr.reset( new InternalVerifier ) ;
	}
	else if( p.first == "net" )
	{
		ptr.reset( new NetworkVerifier( es , p.second , timeout , timeout ) ) ;
	}
	else
	{
		ptr.reset( new ExecutableVerifier( es , G::Path(p.second) ) ) ;
	}
	return ptr ;
}

/// \file gverifierfactory.cpp
