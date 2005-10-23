//
// Copyright (C) 2001-2005 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gpopauth.h
//

#ifndef G_POP_AUTH_H
#define G_POP_AUTH_H

#include "gdef.h"
#include "gpop.h"
#include "gpopsecrets.h"

namespace GPop
{
	class Auth ;
	class AuthImp ;
}

// Class: GPop::Auth
// Description: An authenticator using GSmtp::SaslServer in its
// implementation.
// See also: GSmtp::SaslServer, RFC2222
//
class GPop::Auth 
{
public:
	explicit Auth( const Secrets & ) ;
		// Constructor. Defaults to the APOP mechanism.

	~Auth() ;
		// Destructor.

	bool valid() const ;
		// Returns true if the secrets are valid.

	bool init( const std::string & mechanism ) ;
		// Initialises or reinitialises with
		// the specified mechanism. Returns
		// false if not a supported mechanism.

	std::string challenge() ;
		// Returns an initial challenge appropriate to
		// the current mechanism.

	bool authenticated( const std::string & rsp1 , const std::string & rsp2 ) ;
		// Authenticates a one-step (APOP) or two-step (LOGIN)
		// challenge-response sequence.
		//
		// Returns true if authenticated.
		//
		// The second parameter is used as the second response
		// in a two-step challenge. The challenge itself is
		// not accessible, which only really makes sense for a 
		// LOGIN password prompt.

	std::string id() const ;
		// Returns the authenticated user id.
		// Precondition: authenticated()

	std::string mechanisms() const ;
		// Returns a space-separated list of standard, supported
		// SASL mechanisms (so not including APOP).

private:
	Auth( const Auth & ) ;
	void operator=( const Auth & ) ;

private:
	AuthImp * m_imp ;
} ;

#endif
