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
// glocal.h
//

#ifndef G_GNET_LOCAL_SYSTEM_H
#define G_GNET_LOCAL_SYSTEM_H

#include "gdef.h"
#include "gnet.h"
#include "gaddress.h"
#include "gexception.h"

namespace GNet
{
	class Local ;
}

// Class: GNet::Local
// Description: A static class for getting information
// about the local system.
//
class GNet::Local 
{
public:
	G_EXCEPTION( Error , "local domainname/hostname error" ) ;

	static std::string hostname() ;
		// Returns the hostname.

	static Address canonicalAddress() ;
		// Returns the address associated with 
		// hostname().

	static std::string fqdn() ;
		// Returns the fully-qualified-domain-name.
		// (Typically implemented by doing a DNS
		// lookup on hostname(), but overridable by
		// fdqn(string).)

	static std::string domainname() ;
		// Returns the fqdn()'s domainname.

	static void fqdn( const std::string & fqdn_override ) ;
		// Sets the fqdn() (and therefore domainname()) 
		// override.

	static Address localhostAddress() ;
		// A convenience function returning the
		// "127.0.0.1:0" address.

private:
	static std::string m_fqdn_override ;
	static std::string fqdnImp() ;
	Local() ;
} ;

#endif
