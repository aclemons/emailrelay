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
/// \file gheapclient.h
///

#ifndef G_HEAP_CLIENT_H 
#define G_HEAP_CLIENT_H

#include "gdef.h"
#include "gnet.h"
#include "gsimpleclient.h"
#include "gtimer.h"

/// \namespace GNet
namespace GNet
{
	class HeapClient ;
}

/// \class GNet::HeapClient
/// A SimpleClient class for client objects that manage their own
/// lifetime on the heap. 
///
/// HeapClients are instantiated on the heap and should be deleted by calling
/// their doDelete() method. The doDelete() implementation starts a zero-length 
/// timer which does "delete this" when it expires, so it is safe to call 
/// doDelete() from within event callbacks.
///
/// This class automatically starts connecting after construction using a 
/// zero-length timer, so there is no need to call the base class's connect() 
/// method.
///
/// When the event loop delivers an event callback to a HeapClient and the
/// HeapClient throws a std::exception back up to the event loop the event loop
/// calls the HeapClient again via onException() causing the HeapClient to
/// self-destruct. As a result, the client code can just throw an exception 
/// to terminate the connection and delete itself.
///
class GNet::HeapClient : public GNet::SimpleClient 
{
public:
	explicit HeapClient( const ResolverInfo & remote_info ,
		const Address & local_interface = Address(0U) , bool privileged = false ,
		bool sync_dns = synchronousDnsDefault() ,
		unsigned int secure_connection_timeout = 0U ) ;
			///< Constructor. All instances must be on the heap.
			///< Initiates the connection via a zero-length timer.

	void doDelete( const std::string & reason ) ;
		///< Calls onDelete() and then does a delayed "delete this".

	virtual void onException( std::exception & ) ; 
		///< Final override from GNet::EventHandler.

	void doDeleteForExit() ;
		///< A destructor method that may be called at program 
		///< termination when the normal doDelete() mechanism has
		///< become unusable.

protected:
	virtual ~HeapClient() ;
		///< Destructor.

	virtual void onDelete( const std::string & reason , bool can_retry ) = 0 ;
		///< Called just before deletion.

	virtual void onDeleteImp( const std::string & reason , bool can_retry ) ;
		///< An alternative to onDelete() for private implementation 
		///< classes. Gets called before onDelete(). The default 
		///< implementation does nothing.

	virtual void onConnecting() ;
		///< Called just before the connection is initiated.
		///< Overridable. The default implementation does nothing.

private:
	HeapClient( const HeapClient& ) ; // not implemented
	void operator=( const HeapClient& ) ; // not implemented
	void onConnectionTimeout() ;
	void onDeletionTimeout() ;
	void doDeleteThis() ;

private:
	GNet::Timer<HeapClient> m_connect_timer ;
	GNet::Timer<HeapClient> m_delete_timer ;
} ;

#endif
