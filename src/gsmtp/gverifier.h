//
// Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// gverifier.h
//

#ifndef G_SMTP_VERIFIER_H
#define G_SMTP_VERIFIER_H

#include "gdef.h"
#include "gsmtp.h"
#include "gaddress.h"
#include "gpath.h"
#include <string>

namespace GSmtp
{
	class Verifier ;
}

// Class: GSmtp::Verifier
// Description: A class which verifies recipient addresses.
// This functionality is used in the VRFY and RCPT commands
// in the SMTP server-side protocol.
// See also: ServerProtocol
//
class GSmtp::Verifier 
{
public:
	struct Status // A structure returned by GSmtp::Verifier::verify().
	{ 
		bool is_valid ;
		bool is_local ; 
		std::string full_name ; 
		std::string address ; 
		std::string reason ;
	} ;

	Verifier( const G::Path & exe , bool deliver_to_postmaster , bool reject_local ) ;
		// Constructor. If an executable path is given (ie. not 
		// G::Path()) then it is used for external verification.
		// Otherwise the internal verifier is used, controlled
		// by the two boolean flags. The deliver-to-postmaster 
		// flag enables the special treatment of local "postmaster" 
		// addresses, as dictated by the RFC. If reject-local
		// is true then local mailbox addresses (ie. without
		// an at-sign) are rejected. If reject-local is false 
		// then all addresses are treated as remote, and no 
		// local delivery is attempted.

	Status verify( const std::string & rcpt_to_parameter ,
		const std::string & mail_from_parameter , const GNet::Address & client_ip ,
		const std::string & auth_mechanism , const std::string & auth_extra ) const ;
			// Checks a recipient address returning
			// a structure which indicates whether the
			// address is local, what the full name is,
			// and the canonical address.
			//
			// If invalid then 'is_valid' is set false
			// and a 'reason' is supplied.
			//
			// If valid and syntactically local then
			// 'is_local' is set true, 'full_name' is
			// set to the full description
			// and 'address' is set to the
			// canonical local address (without an
			// at sign).
			//
			// If valid and syntactically remote, then
			// 'is_local' is set false, 'full_name' is
			// empty, and 'address' is copied from
			// 'recipient_address'.
			//
			// The 'from' address is passed in for
			// RCPT commands, but not VRFY.

private:
	Status verifyInternal( const std::string & , const std::string & , const std::string & , 
		const std::string & ) const ;
	Status verifyExternal( const std::string & , const std::string & , const std::string & , 
		const std::string & , const std::string & , const GNet::Address & ,
		const std::string & , const std::string & ) const ;

private:
	G::Path m_path ;
	bool m_deliver_to_postmaster ;
	bool m_reject_local ;
} ;

#endif
