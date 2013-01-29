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
/// \file gpopauth.h
///

#ifndef G_POP_AUTH_H
#define G_POP_AUTH_H

#include "gdef.h"
#include "gpop.h"
#include "gpopsecrets.h"

/// \namespace GPop
namespace GPop
{
	class Auth ;
	class AuthImp ;
}

/// \class GPop::Auth
/// An authenticator interface for POP3
/// sessions.
///
/// The implementation may use GSmtp::SaslServer.
///
/// \see GSmtp::SaslServer, RFC2222
///
class GPop::Auth 
{
public:
	explicit Auth( const Secrets & ) ;
		///< Constructor. Defaults to the APOP mechanism
		///< so that challenge() returns the APOP initial
		///< challege to go into the POP3 greeting.

	~Auth() ;
		///< Destructor.

	bool valid() const ;
		///< Returns true if the secrets are valid.

	bool init( const std::string & mechanism ) ;
		///< Initialises or reinitialises with the specified 
		///< mechanism. Returns false if not a supported mechanism.
		///< Updates the initial challenge() string as appropriate.

	bool mustChallenge() const ;
		///< Returns true if the init()ialised mechanism requires
		///< an initial challenge. Returns false if (in effect)
		///< the mechanism and the authetication can be supplied 
		///< together.

	std::string challenge() ;
		///< Returns an initial challenge appropriate to
		///< the current mechanism.

	bool authenticated( const std::string & rsp1 , const std::string & rsp2 ) ;
		///< Authenticates a one-step (APOP,PLAIN) or two-step (LOGIN)
		///< challenge-response sequence. Both steps in a two-step
		///< mechanism are done in one call to this method. The
		///< second parameter use used only if the current
		///< mechanism is a two-step mechanism. The second-step
		///< challenge itself is not accessible, which only really 
		///< makes sense for a LOGIN password prompt, since it is
		///< a fixed string.
		///<
		///< Returns true if authenticated.

	std::string id() const ;
		///< Returns the authenticated user id.
		///< Precondition: authenticated()

	std::string mechanisms() const ;
		///< Returns a space-separated list of standard, supported
		///< SASL mechanisms (so not including APOP).

	bool sensitive() const ;
		///< Returns true if the implementation requires
		///< authentication to be restricted to encrypted
		///< transports.

private:
	Auth( const Auth & ) ;
	void operator=( const Auth & ) ;

private:
	AuthImp * m_imp ;
} ;

#endif
