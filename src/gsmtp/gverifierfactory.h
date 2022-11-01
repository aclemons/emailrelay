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
/// \file gverifierfactory.h
///

#ifndef G_SMTP_VERIFIER_FACTORY_H
#define G_SMTP_VERIFIER_FACTORY_H

#include "gdef.h"
#include "gverifier.h"
#include "gexceptionsink.h"
#include "gfactoryparser.h"
#include <string>
#include <utility>
#include <memory>

namespace GSmtp
{
	class VerifierFactory ;
}

//| \class GSmtp::VerifierFactory
/// A factory for addresss verifiers.
///
class GSmtp::VerifierFactory
{
public:
	static std::unique_ptr<Verifier> newVerifier( GNet::ExceptionSink ,
		const FactoryParser::Result & spec , unsigned int timeout ) ;
			///< Returns a Verifier on the heap. Throws if
			///< an invalid or unsupported specification.

public:
	VerifierFactory() = delete ;
} ;

#endif
