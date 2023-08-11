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
/// \file gsaslserver.h
///

#ifndef G_SASL_SERVER_H
#define G_SASL_SERVER_H

#include "gdef.h"
#include "gexception.h"
#include "gaddress.h"
#include "gstringarray.h"
#include "gpath.h"
#include <memory>
#include <utility>

namespace GAuth
{
	class SaslServer ;
	class SaslServerImp ;
}

//| \class GAuth::SaslServer
/// An interface for implementing the server-side SASL challenge/response
/// concept. In practice there is one derived class for basic authentication
/// mechanisms using a secrets file, and another for PAM.
///
/// Usage:
/// \code
/// SaslServer sasl( secrets ) ;
/// if( !sasl.mechanisms(peer.secure()).empty() )
/// {
///   peer.advertise( sasl.mechanisms(peer.secure()) ) ;
///   if( sasl.init(peer.secure(),peer.preferred()) )
///   {
///     if( peer.haveInitialResponse() && sasl.mustChallenge() )
///       throw ProtocolError() ;
///     bool done = false ;
///     string challenge = peer.haveInitialResponse() ?
///       sasl.apply(peer.initialResponse(),done) : sasl.initialChallenge() ;
///     while( !done )
///     {
///       peer.send( challenge ) ;
///       string response = peer.receive() ;
///       challenge = sasl.apply( response , done ) ;
///     }
///     bool ok = sasl.authenticated() ;
///   }
/// }
/// \endcode
///
/// \see GAuth::SaslClient, RFC-2554, RFC-4422
///
/// Available mechanisms depend on the encryption state ('secure'). In practice
/// there can often be no mechanisms when in the insecure state. If there are no
/// mechanisms then the protocol might advertise a mechanism that always fails
/// to authenticate, returning a 'secure connection required' error to the
/// client -- but that behaviour is out of scope at this interface.
///
class GAuth::SaslServer
{
public:
	virtual ~SaslServer() = default ;
		///< Destructor.

	virtual G::StringArray mechanisms( bool secure ) const = 0 ;
		///< Returns a list of supported, standard mechanisms that
		///< can be advertised to the client. The parameter
		///< indicates whether the transport connection is currently
		///< encrypted.
		///<
		///< Returns the empty set if authentication is not possible
		///< for the given encryption state.

	virtual void reset() = 0 ;
		///< Clears the internal state as if just constructed.
		///< Postcondition: mechanism().empty() && id().empty() && !authenticated() && !trusted()

	virtual bool init( bool secure , const std::string & mechanism ) = 0 ;
		///< Initialiser for the given mechanism. Returns true iff
		///< the requested mechanism is in the mechanisms() list.
		///< May be used more than once. The initialChallenge() is
		///< re-initialised on each successful init().

	virtual std::string mechanism() const = 0 ;
		///< Returns the current mechanism, as selected by the last
		///< successful init().

	virtual std::string preferredMechanism( bool secure ) const = 0 ;
		///< Returns a preferred mechanism if authentication with the
		///< current mechanism has failed. Returns the empty string if
		///< there is no preference. This allows the negotiation of the
		///< mechanism to be user-specific, perhaps by having the
		///< first mechanism a probe mechanism that fails for all users.

	virtual bool mustChallenge() const = 0 ;
		///< Returns true if authentication using the current mechanism
		///< must always start with a non-empty server challenge, ie.
		///< it is a "server-first" mechanism as per RFC-4422.
		///<
		///< Returns false for the "LOGIN" mechanism since the initial
		///< challenge ("Username:") is not essential, ie. it is a
		///< RFC-4422 "variable" mechanism.
		///<
		///< The server should call initialChallenge() to decide whether
		///< to send an initial challenge; this method is only to
		///< stop a client providing an initial response before an
		///< initial challenge has been sent.

	virtual std::string initialChallenge() const = 0 ;
		///< Returns the possibly-empty initial server challenge.

	virtual std::string apply( const std::string & response , bool & done ) = 0 ;
		///< Applies the client response and returns the next
		///< challenge and a 'done' flag by reference.
		///<
		///< Note that some mechanisms generate an extra round-trip
		///< even after the authentication status has been settled.
		///< In this case the 'done' flag will be set true only
		///< when the final empty response from the client is
		///< apply()d.

	virtual bool authenticated() const = 0 ;
		///< Returns true if authenticated sucessfully.
		///< Precondition: apply() 'done'

	virtual std::string id() const = 0 ;
		///< Returns the authenticated or trusted identity. Returns the
		///< empty string if not authenticated and not trusted.

	virtual bool trusted( const G::StringArray & address_wildcards ,
		const std::string & address_display ) const = 0 ;
			///< Returns true if a trusted client that does not need
			///< to authenticate. Pass Address::wildcards() and
			///< Address::hostPartString().
} ;

#endif
