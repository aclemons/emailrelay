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
/// \file gpopsecrets.h
///

#ifndef G_POP_SECRETS_H
#define G_POP_SECRETS_H

#include "gdef.h"
#include "gpop.h"
#include "gsecrets.h"
#include "gsaslserver.h"
#include "gpath.h"
#include "gexception.h"
#include <iostream>
#include <map>

/// \namespace GPop
namespace GPop
{
	class Secrets ;
	class SecretsImp ;
}

/// \class GPop::Secrets
/// A simple interface to a store of secrets as used in
/// authentication.
///
class GPop::Secrets : public GAuth::SaslServer::Secrets 
{
public:
	G_EXCEPTION( OpenError , "cannot open pop secrets file" ) ;

	static std::string defaultPath() ;
		///< Returns the default path.

	explicit Secrets( const std::string & storage_path = defaultPath() ) ;
		///< Constructor. In principle the storage_path can
		///< be a path to a file, a database connection
		///< string, etc. Throws on error.

	virtual ~Secrets() ;
		///< Destructor.

	std::string path() const ;
		///< Returns the storage path.

	virtual bool valid() const ;
		///< Returns true. 
		///< Final override from GSmtp::Valid virtual base class.

	virtual std::string source() const ;
		///< Returns the storage path, as passed in to the constructor.
		///<
		///< Final override from GAuth::SaslServer::Secrets.

	virtual std::string secret( const std::string & mechanism , const std::string & id ) const ;
		///< Returns the given user's secret. Returns the
		///< empty string if not a valid id.
		///<
		///< Final override from GSmtp::SaslServer::Secrets.

	bool contains( const std::string & mechanism ) const ;
		///< Returns true if there is one or more secrets 
		///< using the given mechanism.

private:
	Secrets( const Secrets & ) ; // not implemented
	void operator=( const Secrets & ) ; // not implemented

private:
	SecretsImp * m_imp ;
} ;

#endif

