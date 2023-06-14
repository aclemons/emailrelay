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
/// \file gcram.h
///

#ifndef G_AUTH_CRAM_H
#define G_AUTH_CRAM_H

#include "gdef.h"
#include "gstringarray.h"
#include "gstringview.h"
#include "gsecret.h"
#include "gexception.h"
#include <string>

namespace GAuth
{
	class Cram ;
}

//| \class GAuth::Cram
/// Implements the standard challenge-response authentication
/// mechanism of RFC-2195.
///
/// The response can be built from a simple digest or a hmac.
/// It comprises the userid, followed by a space, followed by the
/// printable digest or hmac. This is normally base64 encoded
/// at higher protocol levels.
///
/// A hmac is (roughly) the hash of (1) the single-block shared
/// key and (2) the hash of (2a) the single-block shared key and
/// (2b) the challenge. The two intermediate hash states of
/// stages (1) and (2a) can be stored instead of the the plaintext
/// key (see GAuth::Secret::masked()).
///
class GAuth::Cram
{
public:
	G_EXCEPTION( BadType , tx("invalid secret type") ) ;
	G_EXCEPTION( Mismatch , tx("mismatched hash types") ) ;
	G_EXCEPTION( NoState , tx("no intermediate-state hash function available") ) ;
	G_EXCEPTION( InvalidState , tx("invalid hash function intermediate state") ) ;
	G_EXCEPTION( NoTls , tx("no tls library") ) ;

	static std::string response( G::string_view hash_type , bool hmac ,
		const Secret & secret , G::string_view challenge ,
		G::string_view response_prefix ) ;
			///< Constructs a response to a challenge comprising the
			///< response-prefix, space, and digest-or-hmac of
			///< secretkey-plus-challenge. Returns an empty string on
			///< error; does not throw.

	static std::string id( G::string_view response ) ;
		///< Returns the leading id part of the response. Returns
		///< the empty string on error.

	static bool validate( G::string_view hash_type , bool hmac ,
		const Secret & secret , G::string_view challenge ,
		G::string_view response ) ;
			///< Validates the response with respect to the original
			///< challenge. Returns false on error; does not throw.

	static G::StringArray hashTypes( G::string_view prefix = {} , bool require_state = false ) ;
		///< Returns a list of supported hash types, such as "MD5"
		///< and "SHA1", ordered with the strongest first. Optionally
		///< adds a prefix to each type, and optionally limits the
		///< list to those hash functions that support initialisation
		///< with intermediate state.

	static std::string challenge( unsigned int random , const std::string & challenge_domain ) ;
		///< Returns a challenge string that incorporates the given
		///< random number and the current time.

public:
	Cram() = delete ;

private:
	static std::string responseImp( G::string_view , bool , const Secret & , G::string_view ) ;
} ;

#endif
