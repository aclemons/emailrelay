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
/// \file gverifierfactorybase.h
///

#ifndef G_SMTP_VERIFIER_FACTORY_BASE_H
#define G_SMTP_VERIFIER_FACTORY_BASE_H

#include "gdef.h"
#include "gverifier.h"
#include "geventstate.h"
#include "gstringview.h"
#include <string>
#include <utility>
#include <memory>

namespace GSmtp
{
	class VerifierFactoryBase ;
}

//| \class GSmtp::VerifierFactoryBase
/// A factory interface for addresss verifiers.
///
class GSmtp::VerifierFactoryBase
{
public:
	struct Spec /// Verifier specification tuple for GSmtp::VerifierFactoryBase::newVerifier().
	{
		Spec() ;
		Spec( std::string_view , std::string_view ) ;
		std::string first ; // "exit", "file", "net", empty on error
		std::string second ; // reason on error, or eg. "/bin/a" if "file"
	} ;

	virtual std::unique_ptr<Verifier> newVerifier( GNet::EventState ,
		const Verifier::Config & config , const Spec & spec ) = 0 ;
			///< Returns a Verifier on the heap. Throws if an invalid
			///< or unsupported specification.

	virtual ~VerifierFactoryBase() = default ;
		///< Destructor.
} ;

#endif
