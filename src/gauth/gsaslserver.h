//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gvalid.h"
#include "gexception.h"
#include "gaddress.h"
#include "gstrings.h"
#include "gpath.h"
#include <map>
#include <memory>

namespace GAuth
{
	class SaslServer ;
	class SaslServerImp ;
}

/// \class GAuth::SaslServer
/// An interface for implementing the server-side SASL challenge/response
/// concept. Third-party libraries could be plumbed in at this interface in order
/// to get support for more exotic authentication mechanisms. In practice there is
/// one derived class for basic authentication mechanisms using a secrets file,
/// and another for PAM.
///
/// Usage:
/// \code
/// SaslServer sasl( secrets ) ;
/// peer.advertise( sasl.mechanisms() ) ;
/// if( sasl.init(peer.preferred()) )
/// {
///   if( peer.haveInitialResponse() && sasl.mustChallenge() ) throw ProtocolError() ;
///   bool done = false ;
///   string challenge = peer.haveInitialResponse() ?
///     sasl.apply(peer.initialResponse(),done) : sasl.initialChallenge() ;
///   while( !done )
///   {
/// \endcode
/// peer.send( challenge ) ;
/// \code
///     string response = peer.receive() ;
///     challenge = sasl.apply( response , done ) ;
///   }
///   bool ok = sasl.authenticated() ;
/// }
/// \endcode
///
/// \see GAuth::SaslClient, RFC-2554, RFC-4422
///
class GAuth::SaslServer
{
public:
	virtual ~SaslServer() ;
		///< Destructor.

	virtual bool requiresEncryption() const = 0 ;
		///< Returns true if the implementation requires that
		///< the challenge/response dialog should only take
		///< place over an encrypted transport.

	virtual bool active() const = 0 ;
		///< Returns true if the constructor's "secrets" object
		///< was valid. See also Secrets::valid().

	virtual std::string mechanisms( char space_separator ) const = 0 ;
		///< Returns a list of supported, standard mechanisms
		///< that can be advertised to the client.
		///<
		///< Some mechanisms (like "APOP") may be accepted by
		///< init() even though they are not advertised.

	virtual bool init( const std::string & mechanism ) = 0 ;
		///< Initialiser. Returns true if the mechanism is in the
		///< mechanisms() list, or if it is some other supported
		///< mechanism (like "APOP") that the derived-class object
		///< allows implicitly. May be used more than once.
		///< The initialChallenge() is re-initialised on each
		///< successful init().

	virtual std::string mechanism() const = 0 ;
		///< Returns the mechanism, as passed to the last init()
		///< call to return true.

	virtual bool mustChallenge() const = 0 ;
		///< Returns true if authentication using the current mechanism
		///< must always start with a non-empty server challenge, ie.
		///< it is a "server-first" mechanism as per RFC-4422.
		///<
		///< Returns false for the "LOGIN" mechanism since the initial
		///< challenge ("Username:") is not essential, ie. it is a
		///< "variable" mechanism.
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

	virtual bool trusted( const GNet::Address & ) const = 0 ;
		///< Returns true if a trusted client that
		///< does not need to authenticate.
} ;

#endif
