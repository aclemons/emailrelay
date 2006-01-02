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
// gresolver.h
//

#ifndef G_RESOLVER_H
#define G_RESOLVER_H

#include "gdef.h"
#include "gnet.h"
#include "gaddress.h"

namespace GNet
{
	class Resolver ;
	class ResolverImp ;
}

// Class: GNet::Resolver
// Description: A class for asynchronous TCP name-to-address 
// resolution. (The motivation for a fully asynchronous 
// interface is so that GUIs and single-threaded servers
// are not blocked during DNS lookup. However, simple
// clients, especially those without a GUI, can reasonably
// use synchronous lookup.)
//
class GNet::Resolver 
{
public:
	Resolver();
		// Default constructor. 
		// Postcondition: state == idle

	virtual ~Resolver() ;
		// Virtual destructor.

	bool resolveReq( std::string name , bool udp = false ) ;
		// Initiates a name-to-address resolution. Returns
		// false if a confirmation will not be generated.
		//
		// The name should be in the format:
		//
		//	{<host-name>|<host-address>}:{<service-name>|<port-number>}
		//	host-address := <n-1>.<n-2>.<n-3>.<n-4>
		//
		// (Clearly if either part is in its numeric format then the
		// resolver has no name-to-address conversion work to do
		// for that part.)
		//	
		// Postcondition: state == resolving (returns true)
		// Postcondition: state == idle (returns false)

	bool resolveReq( std::string host_name, std::string service_name , bool udp = false ) ;
		// Alternative form of ResolveReq(std::string,bool) with 
		// separate hostname and service name parameters.
		// A zero-length host_name defaults to "0.0.0.0". A 
		// zero-length service name defaults to "0".

	virtual void resolveCon( bool success, const Address & address ,
		std::string fqdn_or_failure_reason ) ;
			// Called when the resolution process is complete.
			// This function is never called from within
			// resolveReq().
			//
			// Precondition: state == resolving
			// Postcondition: state == idle

	bool busy() const ;
		// Returns true if there is a pending resolve request.
		//
		// Postcondition: state == resolving <= returns true
		// Postcondition: state == idle <= returns false

	void cancelReq() ;
		// Cancels an outstanding resolve request.
		//
		// Precondition: state == resolving
		// Postcondition: state == idle

	struct HostInfo // Holds the results of a name resolution (address + full-name).
	{
		Address address ;
		std::string canonical_name ;
		HostInfo() ;
	} ;

	typedef std::pair<HostInfo,std::string> HostInfoPair ;

	static HostInfoPair resolve( const std::string & host_name , const std::string & service_name , bool udp = false ) ;
		// Does syncronous name resolution. The returned
		// error string ('pair.second') is zero length 
		// on success. Not implemented on all platforms.

private:
	void operator=( const Resolver & ) ; // not implemented
	Resolver( const Resolver & ) ; // not implemented
	static bool resolveHost( const std::string & host_name , HostInfo & ) ;
	static unsigned int resolveService( const std::string & host_name , bool , std::string & ) ;

private:
	ResolverImp *m_imp ;
} ;

#endif
