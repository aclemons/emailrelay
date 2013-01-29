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
/// \file gresolver.h
///

#ifndef G_RESOLVER_H
#define G_RESOLVER_H

#include "gdef.h"
#include "gnet.h"
#include "gresolverinfo.h"
#include "geventhandler.h"
#include "gaddress.h"

/// \namespace GNet
namespace GNet
{
	class Resolver ;
	class ResolverImp ;
}

/// \class GNet::Resolver
/// A class for asynchronous TCP name-to-address 
/// resolution. (The motivation for a fully asynchronous 
/// interface is so that GUIs and single-threaded servers
/// are not blocked during DNS lookup. However, simple
/// clients, especially those without a GUI, can reasonably
/// use synchronous lookup.)
///
class GNet::Resolver 
{
public:
	explicit Resolver( EventHandler & ) ;
		///< Constructor taking an event handler reference.
		///< The supplied event handler's onException() method
		///< is called if an exception is thrown out of (eg.)
		///< resolveCon().

	virtual ~Resolver() ;
		///< Virtual destructor.

	static bool parse( const std::string & in , std::string & host_or_address , std::string & service_or_port ) ;
		///< Parses a string that contains a hostname or ip address plus a 
		///< server name or port number. Returns false if not valid.
		///<
		///< The input format should be:
		///<	{<host-name>|<host-address>}:{<service-name>|<port-number>}
		///<	where host-address := <n-1>.<n-2>.<n-3>.<n-4> for ipv4

	bool resolveReq( std::string name , bool udp = false ) ;
		///< Initiates a name-to-address resolution. Returns
		///< false on error, in which case a confirmation will 
		///< not be generated.
		///<
		///< Postcondition: state == resolving (returns true)
		///< Postcondition: state == idle (returns false)

	bool resolveReq( std::string host_name, std::string service_name , bool udp = false ) ;
		///< Alternative form of ResolveReq(std::string,bool) with 
		///< separate hostname and service name parameters.
		///< A zero-length host_name defaults to "0.0.0.0". A 
		///< zero-length service name defaults to "0".

	virtual void resolveCon( bool success, const Address & address ,
		std::string fqdn_or_failure_reason ) ;
			///< Called when the resolution process is complete.
			///< Overridable. This default implementation does nothing.
			///< This function is never called from within resolveReq().
			///<
			///< Precondition: state == resolving
			///< Postcondition: state == idle

	bool busy() const ;
		///< Returns true if there is a pending resolve request.
		///<
		///< Postcondition: state == resolving <= returns true
		///< Postcondition: state == idle <= returns false

	static std::string resolve( ResolverInfo & host_and_service , bool udp = false ) ;
		///< Does syncronous name resolution. Fills in the
		///< name and address fields of the supplied ResolverInfo 
		///< structure. The returned error string is zero length 
		///< on success. Not implemented on all platforms.

private:
	void operator=( const Resolver & ) ; // not implemented
	Resolver( const Resolver & ) ; // not implemented
	static unsigned int resolveService( const std::string & , bool , std::string & ) ;
	static std::string resolveHost( const std::string & host_name , unsigned int , ResolverInfo & ) ;

private:
	ResolverImp *m_imp ;
} ;

#endif
