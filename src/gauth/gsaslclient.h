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
/// \file gsaslclient.h
///

#ifndef G_SASL_CLIENT_H
#define G_SASL_CLIENT_H

#include "gdef.h"
#include "gsaslclientsecrets.h"
#include "gexception.h"
#include "gstringview.h"
#include "gstringarray.h"
#include <memory>

namespace GAuth
{
	class SaslClient ;
	class SaslClientImp ;
}

//| \class GAuth::SaslClient
/// A class that implements the client-side SASL challenge/response concept.
/// \see GAuth::SaslServer, RFC-4422, RFC-2554.
///
class GAuth::SaslClient
{
public:
	struct Response /// Result structure returned from GAuth::SaslClient::response
	{
		bool sensitive{true} ; // don't log
		bool error{true} ; // abort the sasl dialog
		bool final{false} ; // final response, server's decision time
		std::string data ;
	} ;

	SaslClient( const SaslClientSecrets & secrets , const std::string & config ) ;
		///< Constructor. The secrets reference is kept.

	~SaslClient() ;
		///< Destructor.

	bool validSelector( G::string_view selector ) const ;
		///< Returns true if the selector is valid.

	bool mustAuthenticate( G::string_view selector ) const ;
		///< Returns true if authentication is required.

	Response response( G::string_view mechanism , G::string_view challenge , G::string_view selector ) const ;
		///< Returns a response to the given challenge. The mechanism is
		///< used to choose the appropriate entry in the secrets file.

	Response initialResponse( G::string_view selector , std::size_t limit = 0U ) const ;
		///< Returns an optional initial response. Always returns the empty
		///< string if the mechanism is 'server-first'. Returns the empty
		///< string, with no side-effects, if the initial response is longer
		///< than the specified limit. Zero-length initial-responses are not
		///< distinguishable from absent initial-responses.

	std::string mechanism( const G::StringArray & mechanisms , G::string_view selector ) const ;
		///< Returns the name of the preferred mechanism taken from the given
		///< set, taking into account what client secrets are available.
		///< Returns the empty string if none is supported or if not active().

	bool next() ;
		///< Moves to the next preferred mechanism. Returns false if there
		///< are no more mechanisms.

	std::string next( const std::string & ) ;
		///< A convenience overload that moves to the next() mechanism
		///< and returns it. Returns the empty string if the given string
		///< is empty or if there are no more mechanisms.

	std::string mechanism() const ;
		///< Returns the name of the current mechanism once next() has
		///< returned true.

	std::string id() const ;
		///< Returns the authentication id, valid after the last
		///< response().

	std::string info() const ;
		///< Returns logging and diagnostic information, valid after
		///< the last response().

public:
	SaslClient( const SaslClient & ) = delete ;
	SaslClient( SaslClient && ) = delete ;
	SaslClient & operator=( const SaslClient & ) = delete ;
	SaslClient & operator=( SaslClient && ) = delete ;

private:
	std::unique_ptr<SaslClientImp> m_imp ;
} ;

#endif
