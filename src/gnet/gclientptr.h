//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "geventlogging.h"
#include "gexceptionsource.h"
#include "geventhandler.h"
#include "gscope.h"
#include "gslot.h"
#include "glog.h"
#include <memory>
#include <type_traits>

namespace GNet
{
	class ClientPtrBase ;
	template <typename T> class ClientPtr ;
}

namespace GNet
{
	namespace ClientPtrImp /// An implementation namespace for GNet::ClientPtr.
	{
		template <typename T>
		bool hasConnected( T * p ,
			typename std::enable_if<std::is_convertible<T*,Client*>::value>::type * = nullptr )
		{
			return p ? static_cast<Client*>(p)->hasConnected() : false ;
		}
		template <typename T>
		bool hasConnected( T * ,
			typename std::enable_if<!std::is_convertible<T*,Client*>::value>::type * = nullptr )
		{
			return false ;
		}
	}
}

//| \class GNet::ClientPtrBase
/// The non-template part of GNet::ClientPtr.
///
class GNet::ClientPtrBase
{
public:
	G::Slot::Signal<const std::string&,const std::string&,const std::string&> & eventSignal() noexcept ;
		///< A signal that is linked to the contained client's
		///< eventSignal().

	G::Slot::Signal<const std::string&> & deleteSignal() noexcept ;
		///< A signal that is triggered as the client is deleted
		///< following an exception handled by this class.
		///< The parameter is normally the exception string, but it
		///< is the empty string for GNet::Done exceptions or if the
		///< client was finished().

	G::Slot::Signal<const std::string&> & deletedSignal() noexcept ;
		///< A signal that is triggered after deleteSignal() once
		///< the client has been deleted and the ClientPtr is empty.

protected:
	ClientPtrBase() ;
		///< Default constructor.

	void eventSlot( const std::string & , const std::string & , const std::string & ) ;
		///< Emits an eventSignal().

public:
	virtual ~ClientPtrBase() = default ;
	ClientPtrBase( const ClientPtrBase & ) = delete ;
	ClientPtrBase( ClientPtrBase && ) = delete ;
	ClientPtrBase & operator=( const ClientPtrBase & ) = delete ;
	ClientPtrBase & operator=( ClientPtrBase && ) = delete ;

private:
	G::Slot::Signal<const std::string&> m_deleted_signal ;
	G::Slot::Signal<const std::string&,const std::string&,const std::string&> m_event_signal ;
	G::Slot::Signal<const std::string&> m_delete_signal ;
} ;

//| \class GNet::ClientPtr
/// A smart pointer class for GNet::Client or similar.
///
/// The ClientPtr is-a ExceptionHandler, so it should be the ExceptionHandler
/// part of the Client's EventState:
/// \code
/// m_client_ptr.reset( new Client( m_es.eh(m_client_ptr) , ... ) ) ;
/// \endcode
///
/// If that is done then the contained Client object will get deleted as
/// the result of an exception thrown out of a network event handler
/// (including GNet::Done) with internal notification to the Client's
/// onDelete() method and external notification via the smart pointer's
/// deleteSignal(). If the Client is deleted from the smart pointer's
/// destructor then there are no notifications.
///
/// If the Client is given some higher-level object as its ExceptionHandler
/// then the ClientPtr will not do any notification and the higher-level
/// object must ensure that the Client object is deleted or disconnected
/// when an exception is thrown:
/// \code
/// void Foo::fn()
/// {
///   m_client_ptr.reset( new Client( m_es.eh(this,&m_client_ptr) , ... ) ) ;
/// }
/// void Foo::onException( ExceptionSource * esrc , std::exception & e , bool done )
/// {
///   if( esrc == &m_client_ptr )
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
template <typename T>
class GNet::ClientPtr : public ClientPtrBase , public ExceptionHandler , public ExceptionSource
{
public:
	G_EXCEPTION( InvalidState , tx("invalid state of network client holder") )

	explicit ClientPtr( T * p = nullptr ) ;
		///< Constructor. Takes ownership of the new-ed client.

	~ClientPtr() override ;
		///< Destructor.

	bool busy() const ;
		///< Returns true if the pointer is not nullptr.

	void reset( T * p ) ;
		///< Resets the pointer. There is no call to onDelete()
		///< and no emitted signals.

	void reset( std::unique_ptr<T> p ) ;
		///< Resets the pointer. There is no call to onDelete()
		///< and no emitted signals.

	void reset() noexcept ;
		///< Resets the pointer. There is no call to onDelete()
		///< and no emitted signals.

	T * get() noexcept ;
		///< Returns the pointer, or nullptr if deleted.

	const T * get() const noexcept ;
		///< Returns the pointer, or nullptr if deleted.

	T * operator->() ;
		///< Returns the pointer. Throws if deleted.

	const T * operator->() const ;
		///< Returns the pointer. Throws if deleted.

	bool hasConnected() const noexcept ;
		///< Returns true if any Client owned by this smart pointer
		///< has ever successfully connected. Returns false if T
		///< is-not-a Client.

private: // overrides
	void onException( ExceptionSource * , std::exception & , bool ) override ; // GNet::ExceptionHandler

public:
	ClientPtr( const ClientPtr & ) = delete ;
	ClientPtr( ClientPtr && ) noexcept ;
	ClientPtr & operator=( const ClientPtr & ) = delete ;
	ClientPtr & operator=( ClientPtr && ) noexcept ;

private:
	T * set( T * ) ;
	T * set( std::nullptr_t ) noexcept ;
	T * release() noexcept ;
	void connectSignals( T & ) ;
	void disconnectSignals( T & ) ;

private:
	T * m_p ;
	bool m_has_connected {false} ;
} ;

template <typename T>
GNet::ClientPtr<T>::ClientPtr( T * p ) :
	m_p(p)
{
	if( m_p != nullptr )
		connectSignals( *m_p ) ;
}

namespace GNet
{
	template <typename T>
	ClientPtr<T>::~ClientPtr()
	{
		delete release() ;
	}
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
			G::ScopeExit _( [this](){this->reset();} ) ; // (was release())
			m_p->doOnDelete( e.what() , done ) ; // first
			deleteSignal().emit( reason ) ; // second -- m_p still set
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
		if( ClientPtrImp::hasConnected(m_p) ) m_has_connected = true ;
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
		if( ClientPtrImp::hasConnected(m_p) ) m_has_connected = true ;
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
void GNet::ClientPtr<T>::reset( T * p )
{
	delete set( p ) ;
}

template <typename T>
void GNet::ClientPtr<T>::reset( std::unique_ptr<T> p )
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
const T * GNet::ClientPtr<T>::get() const noexcept
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
void GNet::ClientPtr<T>::connectSignals( T & t )
{
	t.eventSignal().connect( G::Slot::slot(*static_cast<ClientPtrBase*>(this),&ClientPtr<T>::eventSlot) ) ;
}

template <typename T>
void GNet::ClientPtr<T>::disconnectSignals( T & t )
{
	t.eventSignal().disconnect() ;
}

#endif
