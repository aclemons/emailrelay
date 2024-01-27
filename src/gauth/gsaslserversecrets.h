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
/// \file gsaslserversecrets.h
///

#ifndef GAUTH_SASL_SERVER_SECRETS_H
#define GAUTH_SASL_SERVER_SECRETS_H

#include "gdef.h"
#include "gsecret.h"
#include "gstringview.h"
#include <string>
#include <utility>

namespace GAuth
{
	class SaslServerSecrets ;
}

//| \class GAuth::SaslServerSecrets
/// An interface used by GAuth::SaslServer to obtain authentication secrets.
/// \see GAuth::Secret
///
class GAuth::SaslServerSecrets
{
public:
	virtual ~SaslServerSecrets() = default ;
		///< Destructor.

	virtual bool valid() const = 0 ;
		///< Returns true if the secrets are valid.

	virtual Secret serverSecret( G::string_view type , G::string_view id ) const = 0 ;
		///< Returns the server secret for the given client id.
		///< The type is "plain" or the CRAM hash algorithm.
		///< Returns an invalid secret if not found.

	virtual std::pair<std::string,std::string> serverTrust( const std::string & address_range ) const = 0 ;
		///< Returns a non-empty trustee name if the server trusts
		///< the given address range (eg. "192.168.0.0/24"), together
		///< with context information for logging purposes.

	virtual std::string source() const = 0 ;
		///< Returns the source identifier (eg. file name).

	virtual bool contains( G::string_view type , G::string_view id ) const = 0 ;
		///< Returns true if there is a secret of the given type
		///< either for one user in particular or for any user if
		///< the id is empty.
} ;

#endif
