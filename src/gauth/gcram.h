//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#ifndef G_AUTH_CRAM__H
#define G_AUTH_CRAM__H

#include "gdef.h"
#include "gstrings.h"
#include "gsecret.h"
#include "gexception.h"
#include <string>

namespace GAuth
{
	class Cram ;
}

/// \class GAuth::Cram
/// Implements the standard challenge-response authentication
/// mechanism of RFC-2195.
///
class GAuth::Cram
{
public:
	G_EXCEPTION( BadType , "invalid secret type" ) ;
	G_EXCEPTION( Mismatch , "mismatched hash types" ) ;
	G_EXCEPTION( NoState , "no implementation for extracting hash function state" ) ;

	static std::string response( const std::string & hash_type , bool hmac ,
		const Secret & secret , const std::string & challenge ,
		const std::string & response_prefix ) ;
			///< Constructs a response to a challenge comprising the
			///< response-prefix, space, and digest-or-hmac of
			///< key-plus-challenge. Returns an empty string on
			///< error; does not throw.

	static std::string id( const std::string & response ) ;
		///< Returns the leading id part of the response. Returns
		///< the empty string on error.

	static bool validate( const std::string & hash_type , bool hmac ,
		const Secret & secret , const std::string & challenge ,
		const std::string & response ) ;
			///< Validates the response with respect to the original
			///< challenge. Returns false on error; does not throw.

	static G::StringArray hashTypes( const std::string & prefix = std::string() , bool require_state = false ) ;
		///< Returns a list of supported hash types, such as "MD5"
		///< and "SHA1", ordered with the strongest first. Optionally
		///< adds a prefix to each type, and optionally limits the
		///< list to those hash functions that support initialisation
		///< with intermediate state.

	static std::string challenge() ;
		///< Returns a challenge string that incorporates a random number
		///< and the current time.

private:
	static std::string responseImp( const std::string & , bool , const Secret & , const std::string & ) ;
	Cram() ;
} ;

#endif
