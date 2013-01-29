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
/// \file gverifierstatus.h
///

#ifndef G_SMTP_VERIFIER_STATUS_H
#define G_SMTP_VERIFIER_STATUS_H

#include "gdef.h"
#include "gsmtp.h"
#include <string>

/// \namespace GSmtp
namespace GSmtp
{
	class VerifierStatus ;
}

/// \class GSmtp::VerifierStatus
/// A structure returned by GSmtp::Verifier to describe 
/// the status of a rcpt-to recipient.
///
/// If describing an invalid recipient then 'is_valid' is set false 
/// and a 'reason' is supplied.
///
/// If a valid local recipient then 'is_local' is set true, 'full_name' 
/// is set to the full description of the mailbox and 'address' is set 
/// to the recipient's mailbox name (which should not have an at sign).
///
/// If a valid remote recipient then 'is_local' is set false, 'full_name' 
/// is empty, and 'address' is copied from the original recipient 'to' address.
///
/// The 'help' string can be added by the user of the verifier to give 
/// more context in the log in addition to 'reason'.
///
class GSmtp::VerifierStatus  
{ 
public:
	bool is_valid ;
	bool is_local ; 
	bool temporary ;
	std::string full_name ; 
	std::string address ; 
	std::string reason ;
	std::string help ;

	VerifierStatus() ;
		///< Default constructor for an invalid remote mailbox.

	explicit VerifierStatus( const std::string & ) ;
		///< Constructor for a valid remote mailbox with the
		///< given 'address' field.

	static VerifierStatus parse( const std::string & str , std::string & to_ref ) ;
		///< Parses a str() string into a structure and a 
		///< recipient 'to' address (by reference).

	std::string str( const std::string & to ) const ;
		///< Returns a string representation of the structure
		///< plus the original recipient 'to' address.
} ;

#endif
