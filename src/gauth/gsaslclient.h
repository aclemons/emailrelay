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
/// \file gsaslclient.h
///

#ifndef G_SASL_CLIENT_H
#define G_SASL_CLIENT_H

#include "gdef.h"
#include "gsaslclientsecrets.h"
#include "gexception.h"
#include "gstrings.h"

namespace GAuth
{
	class SaslClient ;
	class SaslClientImp ;
}

/// \class GAuth::SaslClient
/// A class that implements the client-side SASL challenge/response concept.
/// \see GAuth::SaslServer, RFC-4422, RFC-2554.
///
class GAuth::SaslClient
{
public:
	explicit SaslClient( const SaslClientSecrets & secrets ) ;
		///< Constructor. The secrets reference is kept.

	~SaslClient() ;
		///< Destructor.

	bool active() const ;
		///< Returns true if the constructor's secrets object is valid.

	std::string response( const std::string & mechanism , const std::string & challenge ,
		bool & done , bool & sensitive ) const ;
			///< Returns a response to the given challenge. The mechanism is
			///< used to choose the appropriate entry in the secrets file.
			///< Returns the empty string on error and returns boolean flags
			///< by reference.

	std::string preferred( const G::StringArray & mechanisms ) const ;
		///< Returns the name of the preferred mechanism taken from the given
		///< set, taking into account what client secrets are available.
		///< Returns the empty string if none is supported or if not active().

	bool next() ;
		///< Moves to the next preferred mechanism.

	std::string preferred() const ;
		///< Returns the name of the current preferred mechanism after
		///< next() returns true.

	std::string id() const ;
		///< Returns the authentication id, valid after the last
		///< response().

	std::string info() const ;
		///< Returns logging and diagnostic information, valid after
		///< the last response().

private:
	SaslClient( const SaslClient & ) ; // not implemented
	void operator=( const SaslClient & ) ; // not implemented

private:
	SaslClientImp * m_imp ;
} ;

#endif
