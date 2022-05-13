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
/// \file gsaslclientsecrets.h
///

#ifndef G_SASL_CLIENT_SECRETS_H
#define G_SASL_CLIENT_SECRETS_H

#include "gdef.h"
#include "gvalid.h"
#include "gsecret.h"

namespace GAuth
{
	class SaslClientSecrets ;
}

//| \class GAuth::SaslClientSecrets
/// An interface used by GAuth::SaslClient to obtain a client id and
/// its authentication secret. Conceptually there is one client and
/// they can have secrets encoded in multiple ways.
///
class GAuth::SaslClientSecrets : public virtual Valid
{
public:
	virtual Secret clientSecret( const std::string & type ) const = 0 ;
		///< Returns the client secret for the given type.
		///< The type is "plain" or the CRAM hash algorithm or "oauth".
		///< Returns an invalid secret if none.
} ;

#endif
