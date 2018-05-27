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
/// \file gheapclient.h
///

#ifndef G_NET_HEAP_CLIENT__H
#define G_NET_HEAP_CLIENT__H

#include "gdef.h"
#include "gsimpleclient.h"
#include "gtimer.h"

namespace GNet
{
	class HeapClient ;
}

/// \class GNet::HeapClient
/// A SimpleClient class for client objects that manage their own lifetime on
/// the heap.
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
/// calls the HeapClient again via onException(). The implementation of
/// onException() in HeapClient causes the HeapClient to self-destruct.
/// As a result, the client code can just throw an exception to terminate
/// the connection and delete itself.
///
class GNet::HeapClient : protected ExceptionHandler , public SimpleClient
{
public:
	explicit HeapClient( const Location & remote_info ,
		bool bind_local_address = false ,
		const Address & local_address = Address::defaultAddress() ,
		bool sync_dns = synchronousDnsDefault() ,
		unsigned int secure_connection_timeout = 0U ) ;
			///< Constructor. All instances must be on the heap.
			///< Initiates the connection via a zero-length timer.

	void doDelete( const std::string & reason ) ;
		///< Calls onDelete() and then does a delayed "delete this".

protected:
	virtual ~HeapClient() ;
		///< Destructor.

	void finish() ;
		///< Indicates that the last data has been sent and the client
		///< is expecting a peer disconnect. The subsequent onDelete()
		///< callback will have an empty reason string. The caller
		///< should also consider using Socket::shutdown().

	virtual void onDelete( const std::string & reason ) = 0 ;
		///< Called just before deletion.

	virtual void onDeleteImp( const std::string & reason ) ;
		///< An alternative to onDelete() for derived classes in the
		///< GNet namespace (in practice GNet::Client). Gets called
		///< before onDelete(). The default implementation does nothing.

	virtual void onConnecting() ;
		///< Called just before the connection is initiated.
		///< Overridable. The default implementation does nothing.

	virtual void onException( std::exception & ) g__final override ;
		///< Override from GNet::ExceptionHandler.
		///< Calls doDelete(). Note that the exception
		///< text is available via onDelete().

private:
	HeapClient( const HeapClient & ) ;
	void operator=( const HeapClient & ) ;
	void onConnectionTimeout() ;
	void onDeletionTimeout() ;
	void doDeleteThis() ;

private:
	GNet::Timer<HeapClient> m_connect_timer ;
	GNet::Timer<HeapClient> m_delete_timer ;
	bool m_finished ;
} ;

#endif
