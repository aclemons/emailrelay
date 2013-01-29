//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gsmtp.h"
#include "gverifier.h"
#include "grequestclient.h"
#include "gclientptr.h"
#include <string>

/// \namespace GSmtp
namespace GSmtp
{
	class InternalVerifier ;
}

/// \class GSmtp::InternalVerifier
/// The standard internal Verifier.
///
class GSmtp::InternalVerifier : public GSmtp::Verifier 
{
public:
	InternalVerifier() ;
		///< Constructor.

	virtual ~InternalVerifier() ;
		///< Destructor.

	virtual void verify( const std::string & rcpt_to_parameter ,
		const std::string & mail_from_parameter , const GNet::Address & client_ip ,
		const std::string & auth_mechanism , const std::string & auth_extra ) ;
			///< Final override from GSmtp::Verifier.

	virtual G::Signal2<std::string,VerifierStatus> & doneSignal() ;
		///< Final override from GSmtp::Verifier.

	virtual void reset() ;
		///< Final override from GSmtp::Verifier.

private:
	InternalVerifier( const InternalVerifier & ) ; // not implemented
	void operator=( const InternalVerifier & ) ; // not implemented
	G::Signal2<std::string,VerifierStatus> m_done_signal ;
	VerifierStatus verifyInternal( const std::string & address ) const ;
} ;

#endif
