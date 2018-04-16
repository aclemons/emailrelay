//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gmultiserver.h
///

#ifndef G_NET_MULTI_SERVER__H
#define G_NET_MULTI_SERVER__H

#include "gdef.h"
#include "gserver.h"
#include <vector>
#include <utility> // std::pair<>

namespace GNet
{
	class MultiServer ;
	class MultiServerImp ;
	class MultiServerPtr ;
}

/// \class GNet::MultiServerImp
/// A private implementation class used by GNet::MultiServer.
///
class GNet::MultiServerImp : public GNet::Server
{
public:
	MultiServerImp( MultiServer & , ExceptionHandler & , const Address & ) ;
		///< Constructor.

	void cleanup() ;
		///< Does cleanup.

private:
	virtual ServerPeer * newPeer( PeerInfo ) override ;

private:
	MultiServer & m_ms ;
} ;

/// \class GNet::MultiServerPtr
/// A private implementation class used by GNet::MultiServer.
/// The implementation is unusual in that it only has proper value semantics
/// if the contained pointer is null.
///
class GNet::MultiServerPtr
{
public:
	typedef GNet::MultiServerImp ServerImp ;

	explicit MultiServerPtr( ServerImp * = nullptr ) ;
		///< Constructor.

	~MultiServerPtr() ;
		///< Destructor.

	void swap( MultiServerPtr & ) ;
		///< Swaps internals with the other.

	MultiServerImp * get() ;
		///< Returns the raw pointer.

	const MultiServerImp * get() const ;
		///< Returns the raw const pointer.

	MultiServerPtr( const MultiServerPtr & ) ;
		///< Copy constructor.

	void operator=( const MultiServerPtr & ) ;
		///< Assignment operator.

private:
	MultiServerImp * m_p ;
} ;

/// \class GNet::MultiServer
/// A server that listens on more than one interface using a facade
/// pattern to multiple GNet::Server instances.
///
class GNet::MultiServer
{
public:
	typedef std::vector<Address> AddressList ;
	typedef Server::PeerInfo PeerInfo ;

	struct ServerInfo /// A structure used in GNet::MultiServer::newPeer().
	{
		Address m_address ; ///< The server address that the peer connected to.
		ServerInfo() ;
	} ;

	MultiServer( ExceptionHandler & , const AddressList & address_list ) ;
		///< Constructor. The server listens on on the
		///< specific (local) interfaces.
		///<
		///< Precondition: ! address_list.empty()

	explicit MultiServer( ExceptionHandler & ) ;
		///< Default constructor. Initialise with init().

	void init( const AddressList & address_list ) ;
		///< Initilisation after default construction.
		///<
		///< Precondition: ! address_list.empty()

	virtual ~MultiServer() ;
		///< Destructor.

	std::pair<bool,Address> firstAddress() const ;
		///< Returns the first listening address. The boolean
		///< value is false if none.

	virtual ServerPeer * newPeer( PeerInfo , ServerInfo ) = 0 ;
		///< A factory method which new()s a ServerPeer-derived
		///< object. See Server for the details.

	static bool canBind( const AddressList & listening_address_list , bool do_throw ) ;
		///< Checks that the specified addresses can be
		///< bound. Throws CannotBind if an address cannot
		///< be bound and 'do_throw' is true.

	static AddressList addressList( const Address & ) ;
		///< A trivial convenience fuction that returns the given
		///< address as a single-element list.

	static AddressList addressList( const AddressList & , unsigned int port ) ;
		///< Returns the given list of addresses with the port set
		///< correctly. If the given list is empty then a single
		///< 'any' address is returned.

	static AddressList addressList( const G::StringArray & , unsigned int port ) ;
		///< A convenience function that returns a list of
		///< listening addresses given a list of listening
		///< interfaces and a port number. If the list of
		///< interfaces is empty then a single 'any' address
		///< is returned.

protected:
	void serverCleanup() ;
		///< Should be called from all derived classes' destructors
		///< so that peer objects can use their Server objects
		///< safely during their own destruction.

	void serverReport( const std::string & server_type ) const ;
		///< Writes to the system log a summary of the underlying server objects
		///< and their addresses.

private:
	MultiServer( const MultiServer & ) ; // not implemented
	void operator=( const MultiServer & ) ; // not implemented
	void init( const Address & ) ;

private:
	typedef std::list<MultiServerPtr> List ;
	ExceptionHandler & m_eh ;
	List m_server_list ;
} ;

#endif
