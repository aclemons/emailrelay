//
// Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gsecrets.h
//

#ifndef G_SMTP_SECRETS_H
#define G_SMTP_SECRETS_H

#include "gdef.h"
#include "gsmtp.h"
#include "gpath.h"
#include "gexception.h"
#include <iostream>
#include <map>

namespace GSmtp
{
	class Secrets ;
}

// Class: GSmtp::Secrets
// Description: A simple interface to a store of secrets as used in
// authentication.
// See also: GSmtp::SaslClient, GSmtp::SaslServer
//
class GSmtp::Secrets 
{
public:
	G_EXCEPTION( OpenError , "cannot read secrets file" ) ;

	explicit Secrets( const G::Path & path , const std::string & debug_name = std::string() ) ;
		// Constructor.

	~Secrets() ;
		// Destructor.

	bool valid() const ;
		// Returns true if a valid file.

	std::string id( const std::string & mechanism ) const ;
		// Returns the default id for client-side
		// authentication.

	std::string secret( const std::string & mechanism ) const ;
		// Returns the default secret for client-side
		// authentication.

	std::string secret(  const std::string & mechanism , const std::string & id ) const ;
		// Returns the given user's secret. Returns the
		// empty string if not a valid id.

private:
	void read( std::istream & ) ;
	void process( std::string , std::string , std::string , std::string ) ;
	Secrets( const Secrets & ) ; // not implemented
	void operator=( const Secrets & ) ; // not implemented

private:
	typedef std::map<std::string,std::string> Map ;
	G::Path m_path ;
	std::string m_debug_name ;
	bool m_valid ;
	Map m_map ;
} ;

#endif

