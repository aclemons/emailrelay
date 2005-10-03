//
// Copyright (C) 2001-2005 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include <iostream>
#include <map>

namespace GPop
{
	class Secrets ;
	class SecretsImp ;
}

// temporary...
namespace GSmtp
{
	class Secrets ;
}

// Class: GPop::Secrets
// Description: A simple interface to a store of secrets as used in
// authentication.
//
class GPop::Secrets 
{
public:
	G_EXCEPTION( OpenError , "cannot read secrets file" ) ;

	static std::string defaultPath() ;
		// Returns the default path.

	explicit Secrets( const std::string & storage_path = defaultPath() ) ;
		// Constructor. In principle the storage_path can
		// be a path to a file, a database connection
		// string, etc.

	~Secrets() ;
		// Destructor.

	std::string path() const ;
		// Returns the storate path.

	bool valid() const ;
		// Returns true if a valid file.

	std::string secret(  const std::string & mechanism , const std::string & id ) const ;
		// Returns the given user's secret. Returns the
		// empty string if not a valid id.

	const GSmtp::Secrets & smtp() const ;
		// Temporary back-door.

private:
	Secrets( const Secrets & ) ; // not implemented
	void operator=( const Secrets & ) ; // not implemented

private:
	SecretsImp * m_imp ;
} ;

#endif

