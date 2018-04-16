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
/// \file gclientptr.h
///

#ifndef G_NET_CLIENT_PTR__H
#define G_NET_CLIENT_PTR__H

#include "gdef.h"
#include "gclient.h"
#include "gexception.h"
#include "gassert.h"
#include "gslot.h"
#include <memory>

namespace GNet
{
	template <typename T> class ClientPtr ;
}

/// \class GNet::ClientPtr
/// A smart pointer class for GNet::HeapClient that keeps track of when the
/// contained instance deletes itself. When the smart pointer goes out of
/// scope the HeapClient object is told to delete itself asynchronously
/// using its doDelete() mechanism. The GNet::Client's slots-and-signals
/// are managed automatically so that ClientPtr users do not have to
/// disconnect and reconnect them.
///
template <typename T>
class GNet::ClientPtr
{
public:
	G_EXCEPTION( InvalidState , "invalid state of network client handle" ) ;

	explicit ClientPtr( T * p = nullptr ) ;
		///< Constructor. Takes ownership of the new-ed client.

	~ClientPtr() ;
		///< Destructor.

	bool busy() const ;
		///< Returns true if the pointer is not nullptr.

	void reset( T * p = nullptr ) ;
		///< Resets the pointer. The old client object, if any, is
		///< told to delete itself asynchronously and it will be
		///< released from this smart pointer while it does so,
		///< with its signals going nowhere.

	void resetForExit() ;
		///< Resets the pointer. The old client object, if any, is
		///< deleted synchronously. This should only be used from
		///< outside of EventLoop::run(), just as the TimerList
		///< is going away.

	T * release() ;
		///< Releases the pointer so ownership is transfered to the
		///< caller. The released object's signals are disconnected.

	T * get() ;
		///< Returns the pointer, or nullptr if deleted.

	T * operator->() ;
		///< Returns the pointer. Throws if deleted.

	const T * operator->() const ;
		///< Returns the pointer. Throws if deleted.

	G::Slot::Signal1<std::string> & doneSignal() ;
		///< Returns a signal which indicates that client processing
		///< is complete and the client instance has deleted
		///< itself. The signal parameter is the failure
		///< reason.

	G::Slot::Signal2<std::string,std::string> & eventSignal() ;
		///< Returns a signal which indicates something interesting.

	G::Slot::Signal0 & connectedSignal() ;
		///< Returns a signal which indicates that the
		///< connection has been established successfully.

private:
	ClientPtr( const ClientPtr & ) ; // not implemented
	void operator=( const ClientPtr & ) ; // not implemented
	void doneSlot( std::string ) ;
	void eventSlot( std::string , std::string ) ;
	void connectedSlot() ;
	void connectSignalsToSlots() ;
	void disconnectSignals() ;

private:
	T * m_p ;
	G::Slot::Signal1<std::string> m_done_signal ;
	G::Slot::Signal2<std::string,std::string> m_event_signal ;
	G::Slot::Signal0 m_connected_signal ;
} ;

template <typename T>
GNet::ClientPtr<T>::ClientPtr( T * p ) :
	m_p(p)
{
	try
	{
		connectSignalsToSlots() ;
	}
	catch(...)
	{
		// should p->doDelete() here
		throw ;
	}
}

template <typename T>
GNet::ClientPtr<T>::~ClientPtr()
{
	if( m_p != nullptr )
	{
		disconnectSignals() ;
		m_p->doDelete( std::string() ) ;
	}
}

template <typename T>
T * GNet::ClientPtr<T>::release()
{
	T * result = nullptr ;
	if( m_p != nullptr )
	{
		disconnectSignals() ;
		result = m_p ;
		m_p = nullptr ;
	}
	return result ;
}

template <typename T>
void GNet::ClientPtr<T>::connectSignalsToSlots()
{
	if( m_p != nullptr )
	{
		m_p->doneSignal().connect( G::Slot::slot(*this,&ClientPtr::doneSlot) ) ;
		m_p->eventSignal().connect( G::Slot::slot(*this,&ClientPtr::eventSlot) ) ;
		m_p->connectedSignal().connect( G::Slot::slot(*this,&ClientPtr::connectedSlot) ) ;
	}
}

template <typename T>
void GNet::ClientPtr<T>::reset( T * p )
{
	disconnectSignals() ;
	T * old = m_p ;
	m_p = p ;
	if( old != nullptr )
	{
		old->doDelete( std::string() ) ;
	}
	connectSignalsToSlots() ;
}

template <typename T>
void GNet::ClientPtr<T>::resetForExit()
{
	disconnectSignals() ;
	T * p = m_p ;
	m_p = nullptr ;
	if( p != nullptr )
		p->doDelete( std::string() ) ;
	delete p ;
}

template <typename T>
G::Slot::Signal1<std::string> & GNet::ClientPtr<T>::doneSignal()
{
	return m_done_signal ;
}

template <typename T>
G::Slot::Signal2<std::string,std::string> & GNet::ClientPtr<T>::eventSignal()
{
	return m_event_signal ;
}

template <typename T>
G::Slot::Signal0 & GNet::ClientPtr<T>::connectedSignal()
{
	return m_connected_signal ;
}

template <typename T>
void GNet::ClientPtr<T>::doneSlot( std::string reason )
{
	G_ASSERT( m_p != nullptr ) ;
	disconnectSignals() ;
	m_p = nullptr ;
	m_done_signal.emit( reason ) ;
}

template <typename T>
void GNet::ClientPtr<T>::disconnectSignals()
{
	if( m_p != nullptr )
	{
		m_p->doneSignal().disconnect() ;
		m_p->eventSignal().disconnect() ;
		m_p->connectedSignal().disconnect() ;
	}
}

template <typename T>
void GNet::ClientPtr<T>::connectedSlot()
{
	G_ASSERT( m_p != nullptr ) ;
	m_connected_signal.emit() ;
}

template <typename T>
void GNet::ClientPtr<T>::eventSlot( std::string s1 , std::string s2 )
{
	m_event_signal.emit( s1 , s2 ) ;
}

template <typename T>
T * GNet::ClientPtr<T>::get()
{
	return m_p ;
}

template <typename T>
bool GNet::ClientPtr<T>::busy() const
{
	return m_p != nullptr ;
}

template <typename T>
T * GNet::ClientPtr<T>::operator->()
{
	if( m_p == nullptr )
		throw InvalidState() ;
	return m_p ;
}

template <typename T>
const T * GNet::ClientPtr<T>::operator->() const
{
	if( m_p == nullptr )
		throw InvalidState() ;
	return m_p ;
}

#endif
