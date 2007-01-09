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
// gmultiserver.h
//

#ifndef G_MULTI_SERVER_H 
#define G_MULTI_SERVER_H

#include "gdef.h"
#include "gnet.h"
#include "gserver.h"
#include <list>
#include <utility> // std::pair<>

namespace GNet
{
	class MultiServer ;
}

// Class: GNet::MultiServer
// Description: A server that listens on multiple interfaces using
// a facade pattern to Server instances.
//
class GNet::MultiServer 
{
public:
	typedef std::list<Address> AddressList ;
	typedef Server::PeerInfo PeerInfo ;

	static bool canBind( const AddressList & listening_address_list , bool do_throw ) ;
		// Checks that the specified addresses can be
		// bound. Throws CannotBind if an address cannot 
		// be bound and 'do_throw' is true.

	static AddressList addressList( const Address & ) ;
		// A trivial convenience fuction that returns the given 
		// addresses as a list.

	static AddressList addressList( const AddressList & , unsigned int port ) ;
		// Returns the given list of addresses with the port set
		// correctly. If the given list is empty then a single
		// 'any' address is returned.

	static AddressList addressList( const G::Strings & , unsigned int port ) ;
		// A convenience function that returns a list of
		// listening addresses given a list of listening
		// interfaces and a port number. If the list of
		// interfaces is empty then a single 'any' address
		// is returned.

	explicit MultiServer( const AddressList & address_list ) ;
		// Constructor. The server listens on on the
		// specific (local) interfaces.
		// 
		// Precondition: ! address_list.empty()

	MultiServer() ;
		// Default constructor. Initialise with init().

	void init( const AddressList & address_list ) ;
		// Initilisation after default construction.
		// 
		// Precondition: ! address_list.empty()

	virtual ~MultiServer() ;
		// Destructor.

	std::pair<bool,Address> firstAddress() const ;
		// Returns the first listening address. The boolean
		// value is false if none.

protected:
	virtual ServerPeer * newPeer( PeerInfo ) = 0 ;
		// A factory method which new()s a ServerPeer-derived 
		// object. See Server for the details.

	void serverCleanup() ;
		// Should be called from all derived classes' destructors
		// so that peer objects can use their Server objects
		// safely during their own destruction.

	void serverReport( const std::string & server_type ) const ;
		// Writes to the system log a summary of the underlying server objects
		// and their addresses.

private:
	struct ServerImp : public Server
	{
		ServerImp( MultiServer & ms , const Address & ) ;
		virtual ServerPeer * newPeer( PeerInfo ) ;
		void cleanup() ;
		MultiServer & m_ms ;
	} ;
	struct ServerPtr
	{
		typedef GNet::MultiServer::ServerImp ServerImp ;
		explicit ServerPtr( ServerImp * = NULL ) ;
		~ServerPtr() ;
		void swap( ServerPtr & ) ;
		ServerImp * get() ;
		const ServerImp * get() const ;
		ServerImp * m_p ;
	} ;
	typedef std::list<ServerPtr> List ;

private:
	MultiServer( const MultiServer & ) ; // not implemented
	void operator=( const MultiServer & ) ; // not implemented
	void init( const Address & ) ;

private:
	List m_server_list ;
} ;

#endif
