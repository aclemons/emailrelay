//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#ifndef G_NET_CLIENT_PTR_H
#define G_NET_CLIENT_PTR_H

#include "gdef.h"
#include "gclient.h"
#include "gnetdone.h"
#include "gexception.h"
#include "gexceptionsource.h"
#include "geventhandler.h"
#include "gslot.h"
#include <memory>

namespace GNet
{
	class ClientPtrBase ;
	template <typename T> class ClientPtr ;
}

//| \class GNet::ClientPtrBase
/// The non-template part of GNet::ClientPtr. It is an ExcptionHandler
/// so that exceptions thrown by the Client out to the event loop can
/// be delivered back to reset the ClientPtr with the expected Client
/// onDelete() semantics (like GNet::ServerPeer).
///
class GNet::ClientPtrBase : public ExceptionHandler , public ExceptionSource
{
public:
	G::Slot::Signal<const std::string&,const std::string&,const std::string&> & eventSignal() ;
		///< See GNet::Client::eventSignal().

	G::Slot::Signal<const std::string&> & deleteSignal() ;
		///< A signal that is triggered as the client is deleted
		///< following an exception handled by this class.
		///< The parameter is normally the exception string, but it
		///< is the empty string for GNet::Done exceptions or if the
		///< client was finished().

	G::Slot::Signal<const std::string&> & deletedSignal() ;
		///< A signal that is triggered after deleteSignal() once
		///< the client has been deleted and the ClientPtr is empty.

protected:
	ClientPtrBase() ;
		///< Default constructor.

	void connectSignals( Client & ) ;
		///< Connects the given client's signals to this object's
		///< slots.

	void disconnectSignals( Client & ) noexcept ;
		///< Disconnects the given client's signals from this
		///< object's slots.

public:
	~ClientPtrBase() override = default ;
	ClientPtrBase( const ClientPtrBase & ) = delete ;
	ClientPtrBase( ClientPtrBase && ) = delete ;
	void operator=( const ClientPtrBase & ) = delete ;
	void operator=( ClientPtrBase && ) = delete ;

private:
	void eventSlot( const std::string & , const std::string & , const std::string & ) ;

private:
	G::Slot::Signal<const std::string&> m_deleted_signal ;
	G::Slot::Signal<const std::string&,const std::string&,const std::string&> m_event_signal ;
	G::Slot::Signal<const std::string&> m_delete_signal ;
} ;

//| \class GNet::ClientPtr
/// A smart pointer class for GNet::Client.
///
/// The ClientPtr is-a ExceptionHandler, so it should normally be used
/// as the Client's ExceptionSink:
/// \code
/// m_client_ptr.reset( new Client( ExceptionSink(m_client_ptr,m_es.esrc()) , ... ) ) ;
/// \endcode
///
/// If that is done then the contained Client object will get deleted as
/// the result of an exception thrown out of a network event handler
/// (including GNet::Done) with internal notification to the Client's
/// onDelete() method and external notification via the smart pointer's
/// deleteSignal(). If the Client is deleted from the smart pointer's
/// destructor then there are no notifications.
///
/// If the Client is given some higher-level object as its ExceptionSink
/// then the ClientPtr will not do any notification and the higher-level
/// object must ensure that the Client object is deleted or disconnected
/// when an exception is thrown:
/// \code
/// void Foo::fn()
/// {
///   m_client_ptr.reset( new Client( ExceptionSink(*this,&m_client_ptr) , ... ) ) ;
/// }
/// void Foo::onException( ExceptionSource * esrc , std::exception & e , bool done )
/// {
///   if( esrc == m_client_ptr.get() )
///   {
///     m_client_ptr->doOnDelete( e.what() , done ) ;
///     m_client_ptr.reset() ; // or m_client_ptr->disconnect() ;
///   }
/// }
/// \endcode
///
/// Failure to delete the client from within the higher-level object's
/// exception handler will result in bad event handling, with the event
/// loop raising events that are never cleared.
///
/// Note that the ClientPtr exception handler ignores the ExceptionSource
/// pointer, so it can be set to be the same as the higher-level object,
/// for better event logging (as shown above) (see
/// GNet::ExceptionSource::exceptionSourceId()).
///
template <typename T>
class GNet::ClientPtr : public ClientPtrBase
{
public:
	G_EXCEPTION( InvalidState , tx("invalid state of network client holder") ) ;

	explicit ClientPtr( T * p = nullptr ) ;
		///< Constructor. Takes ownership of the new-ed client.

	~ClientPtr() override ;
		///< Destructor.

	bool busy() const ;
		///< Returns true if the pointer is not nullptr.

	void reset( T * p ) noexcept ;
		///< Resets the pointer. There is no call to onDelete()
		///< and no emitted signals.

	void reset( std::unique_ptr<T> p ) noexcept ;
		///< Resets the pointer. There is no call to onDelete()
		///< and no emitted signals.

	void reset() noexcept ;
		///< Resets the pointer. There is no call to onDelete()
		///< and no emitted signals.

	T * get() noexcept ;
		///< Returns the pointer, or nullptr if deleted.

	T * operator->() ;
		///< Returns the pointer. Throws if deleted.

	const T * operator->() const ;
		///< Returns the pointer. Throws if deleted.

	bool hasConnected() const noexcept ;
		///< Returns true if any Client owned by this smart pointer
		///< has ever successfully connected.

private: // overrides
	void onException( ExceptionSource * , std::exception & , bool ) override ; // Override from GNet::ExceptionHandler.
	std::string exceptionSourceId() const override ; // Override from GNet::ExceptionSource.

public:
	ClientPtr( const ClientPtr & ) = delete ;
	ClientPtr( ClientPtr && ) = delete ;
	void operator=( const ClientPtr & ) = delete ;
	void operator=( ClientPtr && ) = delete ;

private:
	T * set( T * ) ;
	T * set( std::nullptr_t ) noexcept ;
	T * release() noexcept ;

private:
	T * m_p ;
	bool m_has_connected{false} ;
} ;

template <typename T>
GNet::ClientPtr<T>::ClientPtr( T * p ) :
	m_p(p)
{
	if( m_p != nullptr )
		connectSignals( *m_p ) ;
}

template <typename T>
GNet::ClientPtr<T>::~ClientPtr()
{
	delete release() ;
}

template <typename T>
void GNet::ClientPtr<T>::onException( ExceptionSource * , std::exception & e , bool done )
{
	if( m_p == nullptr )
	{
		G_WARNING( "GNet::ClientPtr::onException: unhandled exception: " << e.what() ) ;
		throw ; // should never get here -- rethrow just in case
	}
	else
	{
		std::string reason = ( done || m_p->finished() ) ? std::string() : std::string(e.what()) ;
		{
			std::unique_ptr<T> ptr( release() ) ;
			ptr->doOnDelete( e.what() , done ) ; // first
			deleteSignal().emit( reason ) ; // second
			///< T client deleted here
		}
		deletedSignal().emit( reason ) ;
	}
}

template <typename T>
T * GNet::ClientPtr<T>::set( T * p )
{
	if( m_p != nullptr )
	{
		if( m_p->hasConnected() ) m_has_connected = true ;
		disconnectSignals( *m_p ) ;
	}
	if( p != nullptr )
	{
		connectSignals( *p ) ; // may throw AlreadyConnected
	}
	std::swap( p , m_p ) ;
	return p ; // return old m_p for deletion
}

template <typename T>
T * GNet::ClientPtr<T>::set( std::nullptr_t ) noexcept
{
	if( m_p != nullptr )
	{
		if( m_p->hasConnected() ) m_has_connected = true ;
		disconnectSignals( *m_p ) ;
	}
	T * old_p = m_p ;
	m_p = nullptr ;
	return old_p ;
}

template <typename T>
T * GNet::ClientPtr<T>::release() noexcept
{
	return set( nullptr ) ;
}

template <typename T>
void GNet::ClientPtr<T>::reset( T * p ) noexcept
{
	delete set( p ) ;
}

template <typename T>
void GNet::ClientPtr<T>::reset( std::unique_ptr<T> p ) noexcept
{
	delete set( p.release() ) ;
}

template <typename T>
void GNet::ClientPtr<T>::reset() noexcept
{
	delete set( nullptr ) ;
}

template <typename T>
T * GNet::ClientPtr<T>::get() noexcept
{
	return m_p ;
}

template <typename T>
bool GNet::ClientPtr<T>::busy() const
{
	return m_p != nullptr ;
}

template <typename T>
bool GNet::ClientPtr<T>::hasConnected() const noexcept
{
	return m_has_connected ;
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

template <typename T>
std::string GNet::ClientPtr<T>::exceptionSourceId() const
{
	const ExceptionSource * base_p = m_p ;
	return base_p ? base_p->exceptionSourceId() : std::string() ;
}

#endif
