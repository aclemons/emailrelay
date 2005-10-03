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
// gpopauth.h
//

#ifndef G_POP_AUTH_H
#define G_POP_AUTH_H

#include "gdef.h"
#include "gpop.h"
#include "gpopsecrets.h"

namespace GPop
{
	class Auth ;
	class AuthImp ;
}

// Class: GPop::Auth
// Description: An authenticator.
// See also: GSmtp::SaslServer
//
class GPop::Auth 
{
public:
	explicit Auth( const Secrets & ) ;
	~Auth() ;
	bool valid() const ;
	bool init( const std::string & mechanism ) ;
	bool authenticated( const std::string & , const std::string & ) ;
	std::string challenge() ;

private:
	Auth( const Auth & ) ;
	void operator=( const Auth & ) ;

private:
	AuthImp * m_imp ;
} ;

#endif
