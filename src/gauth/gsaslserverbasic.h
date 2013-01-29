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
/// \file gsaslserverbasic.h
///

#ifndef G_SASL_SERVER_BASIC_H
#define G_SASL_SERVER_BASIC_H

#include "gdef.h"
#include "gauth.h"
#include "gvalid.h"
#include "gsecrets.h"
#include "gsaslserver.h"
#include "gexception.h"
#include "gaddress.h"
#include "gstrings.h"
#include "gpath.h"
#include <map>
#include <memory>

/// \namespace GAuth
namespace GAuth
{
	class SaslServerBasicImp ;
	class SaslServerBasic ;
}

/// \class GAuth::SaslServerBasic
/// An implementation of the SaslServer
/// interface.
///
class GAuth::SaslServerBasic : public GAuth::SaslServer 
{
public:

	SaslServerBasic( const Secrets & , bool ignored , bool force_one_mechanism ) ;
		///< Constructor.

	virtual ~SaslServerBasic() ;
		///< Destructor.

	virtual bool requiresEncryption() const ;
		///< Final override from GAuth::SaslServer.

	virtual bool active() const ;
		///< Final override from GAuth::SaslServer.

	virtual std::string mechanisms( char sep = ' ' ) const ;
		///< Final override from GAuth::SaslServer.

	virtual bool init( const std::string & mechanism ) ;
		///< Final override from GAuth::SaslServer.

	virtual std::string mechanism() const ;
		///< Final override from GAuth::SaslServer.

	virtual bool mustChallenge() const ;
		///< Final override from GAuth::SaslServer.

	virtual std::string initialChallenge() const ;
		///< Final override from GAuth::SaslServer.

	virtual std::string apply( const std::string & response , bool & done ) ;
		///< Final override from GAuth::SaslServer.

	virtual bool authenticated() const ;
		///< Final override from GAuth::SaslServer.

	virtual std::string id() const ;
		///< Final override from GAuth::SaslServer.

	virtual bool trusted( GNet::Address ) const ;
		///< Final override from GAuth::SaslServer.

private:
	SaslServerBasic( const SaslServerBasic & ) ; // not implemented
	void operator=( const SaslServerBasic & ) ; // not implemented

private:
	SaslServerBasicImp * m_imp ;
} ;

#endif
