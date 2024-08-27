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
/// \file gverifier.h
///

#ifndef G_SMTP_VERIFIER_H
#define G_SMTP_VERIFIER_H

#include "gdef.h"
#include "gverifierstatus.h"
#include "gbasicaddress.h"
#include "gslot.h"
#include "gexception.h"
#include <string>

namespace GSmtp
{
	class Verifier ;
}

//| \class GSmtp::Verifier
/// An asynchronous interface that verifies recipient 'to' addresses.
/// This is used in the VRFY and RCPT commands in the smtp server
/// protocol.
/// \see GSmtp::ServerProtocol
///
class GSmtp::Verifier
{
public:
	enum class Command { VRFY , RCPT } ;
	struct Request /// Verification request passed to various GSmtp::Verifier::verify() overrides.
	{
		Command command {Command::RCPT} ;
		std::string raw_address ; // recipient address as received
		std::string address ; // recipient address to verify
		std::string from_address ; // MAIL-FROM address if RCPT, not VRFY
		G::BasicAddress client_ip ;
		std::string auth_mechanism ;
		std::string auth_extra ;
	} ;
	struct Config /// Configuration passed to address verifier constructors.
	{
		unsigned int timeout {60U} ;
		std::string domain ;
		Config & set_timeout( unsigned int ) noexcept ;
		Config & set_domain( const std::string & ) ;
	} ;

	virtual void verify( const Request & ) = 0 ;
		///< Checks a recipient address and asynchronously returns a
		///< GSmtp::VerifierStatus structure to indicate whether the address
		///< is a local mailbox, what the full name is, and the canonical
		///< address.

	virtual G::Slot::Signal<Command,const VerifierStatus&> & doneSignal() = 0 ;
		///< Returns a signal that is emit()ed when the verify() request
		///< is complete.

	virtual void cancel() = 0 ;
		///< Aborts any current processing.

	virtual ~Verifier() = default ;
		///< Destructor.
} ;

inline GSmtp::Verifier::Config & GSmtp::Verifier::Config::set_timeout( unsigned int n ) noexcept { timeout = n ; return *this ; }
inline GSmtp::Verifier::Config & GSmtp::Verifier::Config::set_domain( const std::string & s ) { domain = s ; return *this ; }

#endif
