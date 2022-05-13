//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gverifierfactory.cpp
///

#include "gdef.h"
#include "gverifierfactory.h"
#include "gfactoryparser.h"
#include "ginternalverifier.h"
#include "gexecutableverifier.h"
#include "gnetworkverifier.h"
#include "gexception.h"

std::unique_ptr<GSmtp::Verifier> GSmtp::VerifierFactory::newVerifier( GNet::ExceptionSink es ,
	const std::string & identifier , unsigned int timeout )
{
	const bool allow_spam = false ;
	const bool allow_chain = false ;
	FactoryParser::Result p = FactoryParser::parse( identifier , allow_spam , allow_chain ) ;
	if( p.first == "exit" )
	{
		return std::make_unique<InternalVerifier>() ;
	}
	else if( p.first == "net" )
	{
		return std::make_unique<NetworkVerifier>( es , p.second , timeout , timeout ) ;
	}
	else if( p.first == "file" )
	{
		return std::make_unique<ExecutableVerifier>( es , G::Path(p.second) ) ;
	}
	else
	{
		throw G::Exception( "invalid verifier" ) ; // never gets here
	}
}

