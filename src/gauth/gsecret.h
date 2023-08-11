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
/// \file gsecret.h
///

#ifndef G_AUTH_SECRET_H
#define G_AUTH_SECRET_H

#include "gdef.h"
#include "gexception.h"
#include <utility>
#include <string>

namespace GAuth
{
	class Secret ;
}

//| \class GAuth::Secret
/// Encapsulates a userid/shared-secret/hash-function tuple from the secrets file.
/// The shared secret can be a plaintext password or it can be a masked password
/// using the given hash function. A masked secret can only be verified by an hmac
/// operation using that hash function. However, the implementation of the hash
/// function must be capable of accepting an intermediate hash state, and this
/// might only be the case for md5.
///
class GAuth::Secret
{
public:
	G_EXCEPTION( Error , tx("invalid authorisation secret") ) ;
	G_EXCEPTION( BadId , tx("invalid authorisation id") ) ;
	using Value = std::pair<G::string_view,G::string_view> ; // encoded value and encoding

	Secret( Value id , Value secret , G::string_view masking_hash_function = {} ,
		G::string_view context = {} ) ;
			///< Constructor used by the SecretsFile class. Throws on error,
			///< including if the encodings are invalid. Encodings should be
			///< empty (raw) or "xtext" or "base64" or "dotted".

	static std::string check( Value id , Value secret ,
		G::string_view masking_hash_function ) ;
			///< Does a non-throwing check of the constructor parameters,
			///< returning an error message or the empty string.

	bool valid() const ;
		///< Returns true if the secret is valid.

	std::string id() const ;
		///< Returns the associated identity. Throws if not valid().

	std::string secret() const ;
		///< Returns the secret shared key. Throws if not valid().

	bool masked() const ;
		///< Returns true if a non-empty hash function was passed
		///< to the ctor.

	std::string maskHashFunction() const ;
		///< Returns the masking function name as passed to the ctor,
		///< such as "md5", or the empty string if not masked().
		///< Throws if not valid().

	static Secret none() ;
		///< Factory function that returns a secret that is not valid().

	std::string info( const std::string & id = {} ) const ;
		///< Returns information for logging, excluding anything
		///< sensitive. The secret may be in-valid().

	static bool isDotted( G::string_view ) ;
		///< Returns true if the given secret string looks like it is in
		///< the old dotted format rather than base64.

	static std::string decode( Value ) ;
		///< Decodes a value.

private:
	enum class Encoding { xtext , base64 , raw , dotted } ;
	Secret() ; // Secret::none()
	static std::string undotted( G::string_view ) ;
	static bool validEncodingType( Value ) ;
	static bool validEncoding( Value ) ;
	static Encoding encoding( Value ) ;

private:
	std::string m_id ;
	std::string m_secret ;
	std::string m_hash_function ;
	std::string m_context ;
} ;

#endif
