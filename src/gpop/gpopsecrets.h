//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// gpopsecrets.h
//

#ifndef G_POP_SECRETS_H
#define G_POP_SECRETS_H

#include "gdef.h"
#include "gpop.h"
#include "gpath.h"
#include "gexception.h"
#include "gsasl.h"
#include <iostream>
#include <map>

namespace GPop
{
	class Secrets ;
	class SecretsImp ;
}

namespace GSmtp
{
	class Secrets ;
}

// Class: GPop::Secrets
// Description: A simple interface to a store of secrets as used in
// authentication.
//
class GPop::Secrets : public GSmtp::SaslServer::Secrets 
{
public:
	G_EXCEPTION( OpenError , "cannot open pop secrets file" ) ;

	static std::string defaultPath() ;
		// Returns the default path.

	explicit Secrets( const std::string & storage_path = defaultPath() ) ;
		// Constructor. In principle the storage_path can
		// be a path to a file, a database connection
		// string, etc. Throws on error.

	virtual ~Secrets() ;
		// Destructor.

	std::string path() const ;
		// Returns the storage path.

	virtual bool valid() const ;
		// Returns true. 
		//
		// Override from Valid virtual base class.

	virtual std::string secret(  const std::string & mechanism , const std::string & id ) const ;
		// Returns the given user's secret. Returns the
		// empty string if not a valid id.
		//
		// Override from SaslServer::Secrets.

	bool contains( const std::string & mechanism ) const ;
		// Returns true if there is one or more secrets 
		// using the given mechanism.

private:
	Secrets( const Secrets & ) ; // not implemented
	void operator=( const Secrets & ) ; // not implemented

private:
	SecretsImp * m_imp ;
} ;

#endif
