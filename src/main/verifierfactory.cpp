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
/// \file verifierfactory.cpp
///

#include "gdef.h"
#include "verifierfactory.h"
#include "unit.h"
#include "gstr.h"

#if !defined(VERIFIER_DEMO) && defined(GCONFIG_VERIFIER_MASK) && (GCONFIG_VERIFIER_MASK & 0x08)
#define VERIFIER_DEMO 1
#endif

#ifdef VERIFIER_DEMO
#include "demoverifier.h"
#endif

Main::VerifierFactory::VerifierFactory( Run & run , Unit & unit ) :
	m_run(run) ,
	m_unit(unit)
{
	GDEF_IGNORE_VARIABLE( m_run ) ;
	GDEF_IGNORE_VARIABLE( m_unit ) ;
}

GSmtp::VerifierFactoryBase::Spec Main::VerifierFactory::parse( const std::string & spec ,
	const G::Path & base_dir , const G::Path & app_dir , G::StringArray * warnings_p )
{
	#ifdef VERIFIER_DEMO
		if( !spec.empty() && spec.find("demo:") == 0U )
			return { "demo" , G::Str::tail(spec,":") } ;
	#endif
	return GVerifiers::VerifierFactory::parse( spec , base_dir , app_dir , warnings_p ) ;
}

std::unique_ptr<GSmtp::Verifier> Main::VerifierFactory::newVerifier( GNet::ExceptionSink es ,
	const GSmtp::Verifier::Config & config , const Spec & spec )
{
	#ifdef VERIFIER_DEMO
		if( spec.first == "demo" )
			return std::make_unique<DemoVerifier>( es , m_run , m_unit , config , spec.second ) ;
	#endif
	return GVerifiers::VerifierFactory::newVerifier( es , config , spec ) ;
}

