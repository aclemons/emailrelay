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
/// \file verifierfactory.h
///

#ifndef MAIN_VERIFIER_FACTORY_H
#define MAIN_VERIFIER_FACTORY_H

#include "gdef.h"
#include "gverifierfactory.h"
#include <memory>
#include <cstddef> // std::nullptr_t

namespace Main
{
	class VerifierFactory ;
	class Run ;
	class Unit ;
}

//| \class Main::VerifierFactory
/// A VerifierFactory that knows about classes in the Main namespace.
///
class Main::VerifierFactory : public GVerifiers::VerifierFactory
{
public:
	VerifierFactory( Run & , Unit & ) ;
		///< Constructor.

    static Spec parse( const std::string & spec , const G::Path & base_dir = {} ,
		const G::Path & app_dir = {} , G::StringArray * warnings_p = nullptr ) ;
			///< Parses the verifier spec calling the base class
			///< as necessary.

private: // overrides
	std::unique_ptr<GSmtp::Verifier> newVerifier( GNet::ExceptionSink ,
		const Spec & spec , unsigned int timeout ) override ;

private:
	Run & m_run ;
	Unit & m_unit ;
} ;

#endif
