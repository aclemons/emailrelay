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
/// \file gverifier.h
///

#ifndef G_SMTP_VERIFIER_H
#define G_SMTP_VERIFIER_H

#include "gdef.h"
#include "gsmtp.h"
#include "gverifierstatus.h"
#include "gaddress.h"
#include "gslot.h"
#include "gexception.h"
#include <string>

/// \namespace GSmtp
namespace GSmtp
{
	class Verifier ;
}

/// \class GSmtp::Verifier
/// An asynchronous interface that verifies recipient 'to'
/// addresses. This functionality is used in the VRFY and RCPT commands 
/// in the SMTP server-side protocol.
/// \see GSmtp::ServerProtocol
///
class GSmtp::Verifier 
{
public:
	G_EXCEPTION( AbortRequest , "verifier abort request" ) ;

	virtual void verify( const std::string & rcpt_to_parameter ,
		const std::string & mail_from_parameter , const GNet::Address & client_ip ,
		const std::string & auth_mechanism , const std::string & auth_extra ) = 0 ;
			///< Checks a recipient address and asynchronously returns a 
			///< structure to indicate whether the address is a local
			///< mailbox, what the full name is, and the canonical address.
			///<
			///< The 'mail-from' address is passed in for RCPT commands, but 
			///< not VRFY.
			///<
			///< Throws an AbortRequest if the verifier wants to terminate 
			///< the connection.

	virtual G::Signal2<std::string,VerifierStatus> & doneSignal() = 0 ;
		///< Returns a signal that is emit()ed when the verify() request
		///< is complete. The first signal parameter is the mailbox
		///< name (ie. rcpt_to_parameter).

	virtual void reset() = 0 ;
		///< Aborts any current processing.

	virtual ~Verifier() ;
		///< Destructor.
} ;

#endif
