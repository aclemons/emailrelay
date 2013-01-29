//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gauth.h"
#include "gvalid.h"
#include "gexception.h"
#include "gaddress.h"
#include "gstrings.h"
#include "gpath.h"
#include <map>
#include <memory>

/// \namespace GAuth
namespace GAuth
{
	class SaslClient ;
	class SaslClientImp ;
}

/// \class GAuth::SaslClient
/// A class for implementing the client-side SASL 
/// challenge/response concept. SASL is described in RFC4422,
/// and the SMTP extension for authentication is described 
/// in RFC2554.
/// \see GAuth::SaslServer, RFC4422, RFC2554.
///
class GAuth::SaslClient 
{
public:
	/// An interface used by GAuth::SaslClient to obtain authentication secrets.
	class Secrets : public virtual Valid 
	{
		public: virtual std::string id( const std::string & mechanism ) const = 0 ;
		public: virtual std::string secret( const std::string & id ) const = 0 ;
		public: virtual ~Secrets() ;
		private: void operator=( const Secrets & ) ; // not implemented
	} ;

	SaslClient( const Secrets & secrets , const std::string & server_name ) ;
		///< Constructor. The secrets reference is kept.

	~SaslClient() ;
		///< Destructor.

	bool active() const ;
		///< Returns true if the constructor's secrets object
		///< is valid.

	std::string response( const std::string & mechanism , const std::string & challenge , 
		bool & done , bool & error , bool & sensitive ) const ;
			///< Returns a response to the given challenge.
			///< Returns various boolean flags by reference.

	std::string preferred( const G::Strings & mechanisms ) const ;
		///< Returns the name of the preferred mechanism taken from
		///< the given set. Returns the empty string if none is
		///< supported or if not active().

private:
	SaslClient( const SaslClient & ) ; // not implemented
	void operator=( const SaslClient & ) ; // not implemented

private:
	SaslClientImp * m_imp ;
} ;

#endif
