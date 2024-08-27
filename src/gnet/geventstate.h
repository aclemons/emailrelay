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
/// \file geventstate.h
///

#ifndef G_NET_EVENT_STATE_H
#define G_NET_EVENT_STATE_H

#include "gdef.h"
#include "gexceptionhandler.h"
#include "gexceptionsource.h"
#include "geventlogging.h"
#include <new>
#include <exception>
#include <type_traits>

namespace GNet
{
	class EventState ;
	class EventStateUnbound ;
	class EventLogging ;
	class TimerList ;
	class EventLoopImp ;
}

//| \class GNet::EventState
/// A lightweight object containing an ExceptionHandler pointer,
/// optional ExceptionSource pointer and optional EventLogging
/// pointer. Instances are used in the event loop and timer
/// list and they are also copied throughout the containment
/// hierarchy of network and timer objects: the parent object's
/// EventState object is passed to the constructor of all the
/// child objects that it contains. When an object regiters with
/// the event loop it passes its EventState object for the event
/// loop to use when it calls back with an event. If an object
/// can outlast its container (eg. GNet::TaskImp) then it must
/// create() a fresh EventState object, independent of its container.
///
/// An ExceptionHandler implementation normally just rethrows the
/// exception to terminate the event loop, but sometimes the
/// exception can be handled less drastically, perhaps by
/// deleting the object identified as the exception source.
///
/// For example, Server objects create and contain ServerPeer
/// objects:
///
/// \code
/// class ServerPeer : public ExceptionSource { ... } ;
/// class Server : private ExceptionHandler
/// {
///    void init() { m_peer = newServerPeer(...) ; }
///    void onException( ExceptionSource * source ... ) override
///    {
///        assert( source == m_peer ) ;
///        delete source ;
///    }
///    ServerPeer * m_peer ;
/// } ;
/// \endcode
///
/// The EventStateUnbound class is used as a device to force
/// factory methods to plumb-in an ExceptionSource pointer into
/// the newly-created object as soon as its address is available,
/// before the EventState propagates into base  classes and
/// sub-objects.
///
/// \code
/// class FooServer
/// {
/// public:
///     void accept()
///     {
///         std::make_unique<FooServerPeer>( m_es.unbound() , ... ) ;
///     }
///     EventState m_es ;
/// } ;
/// class FooServerPeer : public FooBase , public ExceptionSource
/// {
/// public:
///     FooServerPeer( EventStateUnbound esu , ... ) :
///        FooBase(esbind(esu,this),...) ,
///        m_es(esbind(esu,this)) ,
///        m_sub_object(m_es)
///     {
///     }
///     EventState m_es ; // first
///     FooItem m_sub_object ; // eg. timer
/// } ;
/// \endcode
///
/// To automatically set a G::LogOutput logging context during event
/// processing certain key classes in the containment tree should
/// override GNet::EventLogging::eventLoggingString() and set the
/// logging interface pointer in their EventState:
///
/// \code
/// struct Foo : EventLogging , FooBase
/// {
///   Foo( EventState es , ... ) :
///     EventLogging(es.logging()) ,
///     FooBase(es.logging(this),...) ,
///     m_es(es.logging(this)) ,
///     m_event_logging_string("foo: ")
///   {
///   }
///   std::string_view eventLoggingString() const override
///   {
///     return m_event_logging_string ;
///   }
///   EventState m_es ;
///   std::string m_event_logging_string ;
/// } ;
/// \endcode
///
class GNet::EventState
{
public:
	struct Private /// Overload discriminator for GNet::EventState.
	{
		private:
		Private() = default ;
		friend class GNet::EventState ;
		friend class GNet::EventStateUnbound ;
		friend class GNet::TimerList ;
		friend class GNet::EventLoopImp ;
		friend class GNet::EventStateUnbound ;
	} ;

	EventState( Private , ExceptionHandler * eh , ExceptionSource * source ) noexcept ;
		///< Constructor used by event loops etc. The ExceptionHandler
		///< pointer must remain valid as the EventState is copied around.

	EventState esrc( Private , ExceptionSource * ) const noexcept ;
		///< Returns a copy of this object with the ExceptionSource
		///< pointer set. Used by EventStateUnbound.

	EventState eh( ExceptionHandler * , ExceptionSource * = nullptr ) const noexcept ;
		///< Returns a copy of this object with the ExceptionHandler
		///< and ExceptionSource set.

	EventState eh( ExceptionHandler & , ExceptionSource * = nullptr ) const noexcept ;
		///< Returns a copy of this object with the ExceptionHandler
		///< and ExceptionSource set.

	EventState logging( EventLogging * ) const noexcept ;
		///< Returns a copy of this object with the ExceptionLogging
		///< pointer set to the given value.

	EventStateUnbound unbound() const noexcept ;
		///< Returns a copy of this object as type EventStateUnbound
		///< with a null ExceptionSource.

	static EventState create() ;
		///< A factory function for an exception handler that rethrows.

	static EventState create( std::nothrow_t ) ;
		///< A factory function for an exception handler that logs the
		///< exception as an error but does not re-throw. This can
		///< be a convenient alternative to a try/catch block for
		///< code that might throw but should not terminate a
		///< long-running server process.

	ExceptionHandler * eh() const noexcept ;
		///< Returns the exception handler pointer.

	ExceptionSource * esrc() const noexcept ;
		///< Returns the exception source pointer.

	EventLogging * logging() const noexcept ;
		///< Returns the event logging pointer.

	bool hasExceptionHandler() const noexcept ;
		///< Returns true if eh() is not null.

	void doOnException( std::exception & e , bool done ) ;
		///< Calls the exception handler's onException() method.
		///< Used by EventEmitter and TimerList when handling
		///< an exception thrown from an event handler.
		///< Precondition: hasExceptionHandler()

	void disarm() noexcept ;
		///< Resets the exception handler.
		///< Postcondition: !hasExceptionHandler()

public:
	void eh( std::nullptr_t , ExceptionSource * ) const = delete ;
	void logging( std::nullptr_t ) const = delete ;

private:
	ExceptionHandler * m_eh {nullptr} ;
	ExceptionSource * m_esrc {nullptr} ;
	EventLogging * m_logging {nullptr} ;
} ;

//| \class GNet::EventStateUnbound
/// The EventStateUnbound class is used as a device to force
/// factory methods to plumb-in an ExceptionSource pointer
/// into the newly-created object as soon as its address is
/// available, before the EventState propagates into base
/// classes and sub-objects.
///
/// The free function GNet::esbind() can be used to bind() the
/// new EventState in a way that takes account of the type
/// of the constructed object (see GNet::EventLogging).
///
/// Eg:
/// \code
/// Foo::Foo( EventStateUnbound ebu , ... )
///   FooBase(esbind(ebu,this),...) ,
///   m_sub_object(esbind(ebu,this),...)
/// {
/// }
/// \endcode
///
class GNet::EventStateUnbound
{
public:
	explicit EventStateUnbound( EventState ) noexcept ;
		///< Constructor. See also EventState::unbound().

private:
	friend EventState esbindfriend( EventStateUnbound , ExceptionSource * ) noexcept ;
	EventState bind( ExceptionSource * source ) const noexcept ;

private:
	EventState m_es ;
} ;

inline
GNet::EventState GNet::EventState::eh( ExceptionHandler * eh , ExceptionSource * esrc ) const noexcept
{
	EventState copy( *this ) ;
	copy.m_eh = eh ;
	copy.m_esrc = esrc ;
	return copy ;
}

inline
GNet::EventState GNet::EventState::eh( ExceptionHandler & eh , ExceptionSource * esrc ) const noexcept
{
	EventState copy( *this ) ;
	copy.m_eh = &eh ;
	copy.m_esrc = esrc ;
	return copy ;
}

inline
GNet::ExceptionHandler * GNet::EventState::eh() const noexcept
{
	return m_eh ;
}

inline
GNet::ExceptionSource * GNet::EventState::esrc() const noexcept
{
	return m_esrc ;
}

inline
GNet::EventStateUnbound GNet::EventState::unbound() const noexcept
{
	return EventStateUnbound( *this ) ;
}

inline
GNet::EventLogging * GNet::EventState::logging() const noexcept
{
	return m_logging ;
}

inline
bool GNet::EventState::hasExceptionHandler() const noexcept
{
	return m_eh != nullptr ;
}

namespace GNet
{
	inline EventState esbindfriend( EventStateUnbound esu ,
		ExceptionSource * esrc ) noexcept
	{
		return esu.bind( esrc ) ;
	}

	template <typename T>
	EventState esbind( EventStateUnbound esu , T * p )
	{
		return esbindfriend( esu , p ) ;
	}
}

#endif
