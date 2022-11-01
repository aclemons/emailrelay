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
#include "ginternalverifier.h"
#include "gexecutableverifier.h"
#include "gnetworkverifier.h"
#include "gexception.h"

std::unique_ptr<GSmtp::Verifier> GSmtp::VerifierFactory::newVerifier( GNet::ExceptionSink es ,
	const FactoryParser::Result & spec , unsigned int timeout )
{
	if( spec.first == "exit" )
	{
		return std::make_unique<InternalVerifier>() ;
	}
	else if( spec.first == "net" )
	{
		return std::make_unique<NetworkVerifier>( es , spec.second , timeout , timeout ) ;
	}
	else if( spec.first == "file" )
	{
		return std::make_unique<ExecutableVerifier>( es , G::Path(spec.second) ) ;
	}
	else
	{
		throw G::Exception( "invalid verifier" , spec.second ) ; // never gets here
	}
}

