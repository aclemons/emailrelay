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
#include "gfile.h"
#include "gstr.h"
#include "gexception.h"

GVerifiers::VerifierFactory::VerifierFactory()
= default ;

GVerifiers::VerifierFactory::Spec GVerifiers::VerifierFactory::parse( const std::string & spec_in ,
	const G::Path & base_dir , const G::Path & app_dir , G::StringArray * warnings_p )
{
	std::string tail = G::Str::tail( spec_in , ":" ) ;
	Spec result ;
	if( spec_in.empty() )
	{
		result = Spec( "exit" , "0" ) ;
		checkExit( result ) ;
	}
	else if( spec_in.find(':') == std::string::npos )
	{
		result = Spec( "file" , spec_in ) ;
		fixFile( result , base_dir , app_dir ) ;
		checkFile( result , warnings_p ) ;
	}
	else if( spec_in.find("exit:") == 0U )
	{
		result = Spec( "exit" , tail ) ;
		checkExit( result ) ;
	}
	else if( spec_in.find("net:") == 0U )
	{
		result = Spec( "net" , tail ) ;
		checkNet( result ) ;
	}
	else if( spec_in.find("file:") == 0U )
	{
		result = Spec( "file" , tail ) ;
		fixFile( result , base_dir , app_dir ) ;
		checkFile( result , warnings_p ) ;
	}

	if( result.first.empty() )
		result.second = "[" + spec_in + "]" ;

	return result ;
}

std::unique_ptr<GSmtp::Verifier> GVerifiers::VerifierFactory::newVerifier( GNet::ExceptionSink es ,
	const Spec & spec , unsigned int timeout )
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

	throw G::Exception( "invalid verifier" , spec.second ) ;
	return {} ;
}

void GVerifiers::VerifierFactory::checkExit( Spec & result )
{
	if( !G::Str::isUInt(result.second) )
	{
		result.first.clear() ;
		result.second = "not a numeric exit code: " + G::Str::printable(result.second) ;
	}
}

void GVerifiers::VerifierFactory::checkNet( Spec & result )
{
	try
	{
		GNet::Location::nosocks( result.second ) ;
	}
	catch( std::exception & e )
	{
		result.first.clear() ;
		result.second = e.what() ;
	}
}

void GVerifiers::VerifierFactory::fixFile( Spec & result , const G::Path & base_dir , const G::Path & app_dir )
{
	if( !app_dir.empty() && result.second.find("@app") == 0U )
		G::Str::replace( result.second , "@app" , app_dir.str() ) ;
	else if( !base_dir.empty() && G::Path(result.second).isRelative() )
		result.second = (base_dir+result.second).str() ;
}

void GVerifiers::VerifierFactory::checkFile( Spec & result , G::StringArray * warnings_p )
{
	if( result.second.empty() )
	{
		result.first.clear() ;
		result.second = "empty file path" ;
	}
	else if( warnings_p && !G::File::exists(result.second) )
	{
		warnings_p->push_back( std::string("verifier program does not exist: ").append(result.second) ) ;
	}
	else if( warnings_p && G::File::isDirectory(result.second,std::nothrow) )
	{
		warnings_p->push_back( std::string("invalid program: ").append(result.second) ) ;
	}
}
