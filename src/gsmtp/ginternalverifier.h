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
/// \file ginternalverifier.h
///

#ifndef G_SMTP_INTERNAL_VERIFIER_H
#define G_SMTP_INTERNAL_VERIFIER_H

#include "gdef.h"
#include "gverifier.h"
#include "grequestclient.h"
#include "gclientptr.h"
#include <string>

namespace GSmtp
{
	class InternalVerifier ;
}

//| \class GSmtp::InternalVerifier
/// The standard internal Verifier that accepts all mailbox names.
///
class GSmtp::InternalVerifier : public Verifier
{
public:
	InternalVerifier() ;
		///< Constructor.

private: // overrides
	G::Slot::Signal<Command,const VerifierStatus&> & doneSignal() override ; // Override from GSmtp::Verifier.
	void cancel() override ; // Override from GSmtp::Verifier.
	void verify( Command , const std::string & rcpt_to_parameter ,
		const std::string & mail_from_parameter , const GNet::Address & client_ip ,
		const std::string & auth_mechanism , const std::string & auth_extra ) ; // Override from GSmtp::Verifier.

public:
	~InternalVerifier() override = default ;
	InternalVerifier( const InternalVerifier & ) = delete ;
	InternalVerifier( InternalVerifier && ) = delete ;
	void operator=( const InternalVerifier & ) = delete ;
	void operator=( InternalVerifier && ) = delete ;

private:
	G::Slot::Signal<Command,const VerifierStatus&> m_done_signal ;
} ;

#endif
