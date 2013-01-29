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
/// \file glocal.h
///

#ifndef G_GNET_LOCAL_H
#define G_GNET_LOCAL_H

#include "gdef.h"
#include "gnet.h"
#include "gaddress.h"
#include "gexception.h"

/// \namespace GNet
namespace GNet
{
	class Local ;
}

/// \class GNet::Local
/// A static class for getting information about the
/// local machine's network name and address.
///
class GNet::Local 
{
public:
	G_EXCEPTION( Error , "local domainname/hostname error" ) ;

	static std::string hostname() ;
		///< Returns the hostname. 
		///<
		///< On unix systems this ususally comes from uname().

	static Address canonicalAddress() ;
		///< Returns the canonical address associated with hostname().

	static std::string fqdn() ;
		///< Returns the fully-qualified-domain-name.
		///<
		///< This is typically implemented by doing a DNS lookup on 
		///< hostname(), but it is overridable by calling fdqn(string).

	static std::string domainname() ;
		///< Returns the fqdn()'s domainname. Throws if the fqdn() does 
		///< not contain a domain part.

	static void fqdn( const std::string & fqdn_override ) ;
		///< Sets the fqdn() (and therefore domainname()) override.

	static Address localhostAddress() ;
		///< A convenience function that returns the "127.0.0.1:0" address.

	static bool isLocal( const Address & ) ;
		///< Returns true if the given address appears to be local. 
		///< A simple implementation may compare the given address with 
		///< localhostAddress() and canonicalAddress().

	static bool isLocal( const Address & , std::string & reason ) ;
		///< An override that returns a helpful message by reference
		///< if the address not local.

private:
	static std::string m_fqdn ;
	static std::string m_fqdn_override ;
	static bool m_fqdn_override_set ;
	static Address m_canonical_address ;
	static std::string fqdnImp() ;
	static Address canonicalAddressImp() ;
	Local() ;
} ;

#endif

