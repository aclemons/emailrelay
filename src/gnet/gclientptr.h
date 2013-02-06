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
/// \file gclientptr.h
///

#ifndef G_SMTP_CLIENT_PTR_H
#define G_SMTP_CLIENT_PTR_H

#include "gdef.h"
#include "gnet.h"
#include "gclient.h"
#include "gexception.h"
#include "gassert.h"
#include "gslot.h"
#include <memory>

/// \namespace GNet
namespace GNet
{

/// \class ClientPtr
/// A smart pointer class for GNet::HeapClient.
/// Keeps track of when the contained instance deletes itself and 
/// also does some signal forwarding so that the user of the
/// smart pointer can work with the smart pointers signals
/// and not worry about the client objects' signals. 
///
/// When the smart pointer goes out of scope while it is
/// managing a client object then the client object is
/// told to delete itself asynchronously using the HeapClient
/// class's doDelete() mechanism.
///
/// The smart pointer also optionally maintains an internal 
/// ResolverInfo object so that name resolution results can be 
/// retained from one contained client object to the next. If 
/// the smart pointer is loaded with a new client that has the 
/// same host and service name in its ResolverInfo as the previous
/// client then the resolved address is propagated into the 
/// new client so that name lookup is short-circuited within
/// the SimpleClient code. The danger with this is that the
/// name lookup becomes stale, so it is probably only a good
/// idea for local addresses and/or short-lived objects.
///
template <typename TClient>
class ClientPtr 
{
public:
	G_EXCEPTION( InvalidState , "invalid state of smtp client" ) ;

	explicit ClientPtr( TClient * p = NULL , bool preserve_resolver_info = false ) ;
		///< Constructor.

	~ClientPtr() ;
		///< Destructor.

	bool busy() const ;
		///< Returns true if the pointer is not NULL.

	void reset( TClient * p = NULL ) ;
		///< Resets the pointer.

	TClient * get() ;
		///< Returns the pointer, or NULL if deleted.

	TClient * operator->() ;
		///< Returns the pointer. Throws if deleted.

	const TClient * operator->() const ;
		///< Returns the pointer. Throws if deleted.

	G::Signal2<std::string,bool> & doneSignal() ;
		///< Returns a signal which indicates that client processing
		///< is complete and the client instance has deleted
		///< itself. The first signal parameter is the failure
		///< reason and the second is a 'can-retry' flag.

	G::Signal2<std::string,std::string> & eventSignal() ;
		///< Returns a signal which indicates something interesting.

	G::Signal0 & connectedSignal() ;
		///< Returns a signal which indicates that the 
		///< connection has been established successfully.

	ResolverInfo resolverInfo() const ;
		///< Returns the current or last client's ResolverInfo.

	void releaseForExit() ;
		///< Can be called on program termination when there may
		///< be no TimerList or EventLoop instances.
		///< The client object leaks.

	void cleanupForExit() ;
		///< Can be called on program termination when there may
		///< be no TimerList or EventLoop instances. The client
		///< is destructed so all relevant destructors should 
		///< avoid doing anything with timers or the network 
		///< (possibly even just closing sockets).

private:
	ClientPtr( const ClientPtr & ) ; // not implemented
	void operator=( const ClientPtr & ) ; // not implemented
	void doneSlot( std::string , bool ) ;
	void eventSlot( std::string , std::string ) ;
	void connectedSlot() ;
	void connectSignalsToSlots() ;
	void disconnectSignals() ;

private:
	TClient * m_p ;
	bool m_update ;
	ResolverInfo m_resolver_info ;
	G::Signal2<std::string,bool> m_done_signal ;
	G::Signal2<std::string,std::string> m_event_signal ;
	G::Signal0 m_connected_signal ;
} ;

template <typename TClient>
ClientPtr<TClient>::ClientPtr( TClient * p , bool update ) :
	m_p(p) ,
	m_update(update) ,
	m_resolver_info(std::string(),std::string())
{
	try
	{
		if( m_p != NULL )
		{
			m_resolver_info = m_p->resolverInfo() ;
		}
		connectSignalsToSlots() ;
	}
	catch(...)
	{
		///< should p->doDelete() here
		throw ;
	}
}

template <typename TClient>
ClientPtr<TClient>::~ClientPtr()
{
	if( m_p != NULL )
	{
		disconnectSignals() ;
		m_p->doDelete(std::string()) ;
	}
}

template <typename TClient>
void ClientPtr<TClient>::releaseForExit()
{
	if( m_p != NULL )
	{
		disconnectSignals() ;
		m_p = NULL ;
	}
}

template <typename TClient>
void ClientPtr<TClient>::cleanupForExit()
{
	if( m_p != NULL )
	{
		disconnectSignals() ;
		TClient * p = m_p ;
		m_p = NULL ;
		p->doDeleteForExit() ;
	}
}

template <typename TClient>
void ClientPtr<TClient>::connectSignalsToSlots()
{
	if( m_p != NULL )
	{
		m_p->doneSignal().connect( G::slot(*this,&ClientPtr::doneSlot) ) ;
		m_p->eventSignal().connect( G::slot(*this,&ClientPtr::eventSlot) ) ;
		m_p->connectedSignal().connect( G::slot(*this,&ClientPtr::connectedSlot) ) ;
	}
}

template <typename TClient>
void ClientPtr<TClient>::reset( TClient * p )
{
	disconnectSignals() ;

	TClient * old = m_p ;
	m_p = p ;
	if( old != NULL )
	{
		old->doDelete(std::string()) ;
	}

	if( m_p != NULL && m_update )
	{
		m_p->updateResolverInfo( m_resolver_info ) ; // if host() and service() match
	}

	connectSignalsToSlots() ;
}

template <typename TClient>
G::Signal2<std::string,bool> & ClientPtr<TClient>::doneSignal()
{
	return m_done_signal ;
}

template <typename TClient>
G::Signal2<std::string,std::string> & ClientPtr<TClient>::eventSignal()
{
	return m_event_signal ;
}

template <typename TClient>
G::Signal0 & ClientPtr<TClient>::connectedSignal()
{
	return m_connected_signal ;
}

template <typename TClient>
void ClientPtr<TClient>::doneSlot( std::string reason , bool retry )
{
	G_ASSERT( m_p != NULL ) ;
	disconnectSignals() ;
	m_p = NULL ;
	m_done_signal.emit( reason , retry ) ;
}

template <typename TClient>
void ClientPtr<TClient>::disconnectSignals()
{
	if( m_p != NULL )
	{
		m_p->doneSignal().disconnect() ;
		m_p->eventSignal().disconnect() ;
		m_p->connectedSignal().disconnect() ;
	}
}

template <typename TClient>
void ClientPtr<TClient>::connectedSlot()
{
	G_ASSERT( m_p != NULL ) ;
	m_resolver_info = m_p->resolverInfo() ;
	m_connected_signal.emit() ;
}

template <typename TClient>
void ClientPtr<TClient>::eventSlot( std::string s1 , std::string s2 )
{
	m_event_signal.emit( s1 , s2 ) ;
}

template <typename TClient>
TClient * ClientPtr<TClient>::get()
{
	return m_p ;
}

template <typename TClient>
bool ClientPtr<TClient>::busy() const
{
	return m_p != NULL ;
}

template <typename TClient>
TClient * ClientPtr<TClient>::operator->()
{
	if( m_p == NULL )
		throw InvalidState() ;
	return m_p ;
}

template <typename TClient>
const TClient * ClientPtr<TClient>::operator->() const
{
	if( m_p == NULL )
		throw InvalidState() ;
	return m_p ;
}

template <typename TClient>
ResolverInfo ClientPtr<TClient>::resolverInfo() const
{
	return m_resolver_info ;
}

}

#endif
