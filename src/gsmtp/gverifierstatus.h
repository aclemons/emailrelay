//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gverifierstatus.h
///

#ifndef G_SMTP_VERIFIER_STATUS_H
#define G_SMTP_VERIFIER_STATUS_H

#include "gdef.h"
#include "gexception.h"
#include <string>

namespace GSmtp
{
	class VerifierStatus ;
}

//| \class GSmtp::VerifierStatus
/// A structure returned by GSmtp::Verifier to describe the status of
/// a 'rcpt-to' or 'vrfy' recipient.
///
/// If describing an invalid recipient then 'is_valid' is set false
/// and a 'response' is supplied. The response is typically reported back
/// to the submitter, so it should not contain too much detail.
///
/// The 'reason' string can be added to give more context in the log
/// in addition to 'response'.
///
/// If a valid local recipient then 'is_local' is set true, 'full_name'
/// is set to the full description of the mailbox and 'address' is set
/// to the recipient's mailbox name (which should not have an at sign).
///
/// If a valid remote recipient then 'is_local' is set false, 'full_name'
/// is empty, and 'address' is typically a copy of the original recipient.
///
class GSmtp::VerifierStatus
{
public:
	G_EXCEPTION( InvalidStatus , tx("invalid verifier status") ) ;

	static VerifierStatus invalid( const std::string & recipient ,
		bool temporary = false ,
		const std::string & response = {} ,
		const std::string & reason = {} ) ;
			///< Factory function for an invalid address.

	static VerifierStatus remote( const std::string & recipient ,
		const std::string & address = {} ) ;
			///< Constructor for a valid remote mailbox.

	static VerifierStatus local( const std::string & recipient ,
		const std::string & full_name , const std::string & mbox ) ;
			///< Constructor for a valid local mailbox.

	static VerifierStatus parse( const std::string & str ) ;
		///< Parses a str() string into a structure.

	std::string str() const ;
		///< Returns a string representation of the structure.

	bool utf8address() const ;
		///< Returns true if 'address' is utf8 according to
		///< GSmtp::ServerParser::mailboxStyle().

public:
	bool is_valid {false} ;
	bool is_local {false} ;
	bool temporary {false} ;
	bool abort {false} ;
	std::string recipient ; // verifier input, even if not valid
	std::string full_name ; // description iff local
	std::string address ; // mailbox if local, output address if remote
	std::string response ;
	std::string reason ;

private:
	VerifierStatus() ;
} ;

#endif
