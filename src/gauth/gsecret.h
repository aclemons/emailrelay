//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include <string>

namespace GAuth
{
	class Secret ;
}

//| \class GAuth::Secret
/// Encapsulates a shared secret from the secrets file plus the associated
/// userid. A secret is usually a plaintext shared key, but it may be masked
/// by a hash function. If masked then it can only be verified by an hmac
/// operation using the matching hash function. However, the hmac hash function
/// must be capable of accepting an intermediate hash state, and this is might
/// only be the case for md5.
///
class GAuth::Secret
{
public:
	G_EXCEPTION( Error , tx("invalid authorisation secret") ) ;
	G_EXCEPTION( BadId , tx("invalid authorisation id") ) ;

	Secret( const std::string & secret , const std::string & secret_encoding ,
		const std::string & id , bool id_encoding_xtext ,
		const std::string & context = {} ) ;
			///< Constructor used by the SecretsFile class. Throws on error,
			///< including if the encodings are invalid.

	static std::string check( const std::string & secret , const std::string & secret_encoding ,
		const std::string & id , bool id_encoding_xtext ) ;
			///< Does a non-throwing check of the constructor parameters,
			///< returning an error message or the empty string.

	bool valid() const ;
		///< Returns true if the secret is valid.

	std::string key() const ;
		///< Returns the key. Throws if not valid().

	bool masked() const ;
		///< Returns true if key() is masked.

	std::string maskType() const ;
		///< Returns the masking function name, such as "MD5", or the
		///< empty string if not masked(). Throws if not valid().

	std::string id() const ;
		///< Returns the associated identity. Throws if not valid().

	static Secret none( const std::string & id ) ;
		///< Factory function that returns a secret that is not valid(),
		///< as used by the SecretsFile class.

	static Secret none() ;
		///< Factory function that returns a secret that is not valid() and
		///< has an empty id().

	std::string info( const std::string & id = {} ) const ;
		///< Returns information for logging, excluding anything
		///< sensitive. The secret may be in-valid().

private:
	Secret() ; // Secret::none()
	explicit Secret( const std::string & ) ;
	static bool isDotted( const std::string & ) ;
	static std::string undotted( const std::string & ) ;

private:
	std::string m_server_type ;
	std::string m_key ;
	std::string m_mask_type ;
	std::string m_id ;
	std::string m_context ;
} ;

#endif
