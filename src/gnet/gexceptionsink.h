//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gexceptionsink.h
///

#ifndef G_NET_EXCEPTION_SINK__H
#define G_NET_EXCEPTION_SINK__H

#include "gdef.h"
#include "gexceptionhandler.h"
#include "gexceptionsource.h"
#include <exception>
#include <typeinfo>

namespace GNet
{
	class ExceptionSink ;
	class ExceptionSinkUnbound ;
}

/// \class GNet::ExceptionSink
/// A tuple containing an ExceptionHandler interface pointer and
/// a bound 'exception source' pointer.
///
/// The EventHandlerList and TimerList classes associate an
/// event handler and ExceptionSink with each event source
/// (file descriptor or timer). If the event handler throws
/// an exception then the associated ExceptionHandler's
/// onException() method is called, via ExceptionSink::call().
///
/// An onException() implementation normally just rethrows the
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
/// The ExceptionSinkUnbound class is used as syntactic sugar
/// to force factory methods to supply an ExceptionSource
/// pointer.
///
/// \code
/// class FooServerPeer : public ServerPeer
/// {
/// public:
///     FooServerPeer * newServerPeer( ExceptionSinkUnbound esu , ... ) override
///     {
///         return new FooServerPeer( esu , ... ) ;
///     }
/// private:
///     FooServerPeer( ExceptionSinkUnbound esu , ... ) :
///        ServerPeer( esu.bind(this) , ... )
///     {
///     }
/// } ;
/// \endcode
///
/// So then the ServerPeer constructor has a bound ExceptionSink that
/// it can pass to its timers etc. and to the event-handler list:
///
/// \code
/// ServerPeer::ServerPeer( ExceptionSink es , ... ) :
///    m_timer(..., es)
/// {
///   EventHandlerList::instance().add( es , ... ) ;
/// }
/// \endcode
///
class GNet::ExceptionSink
{
public:
	g__enum(Type)
	{
		Null , // eh() is nullptr, call() does nothing
		Rethrow , // rethrows
		Log // logs an error with G_ERROR
	} ; g__enum_end(Type)

	explicit ExceptionSink( Type = Type::Rethrow , ExceptionSource * source = nullptr ) ;
		///< Constructor.

	ExceptionSink( ExceptionHandler & eh , ExceptionSource * source ) ;
		///< Constructor. The ExceptionHandler reference must
		///< remain valid as the ExceptionSink is copied around.

	ExceptionSink( ExceptionHandler * eh , ExceptionSource * source ) ;
		///< Constructor. The ExceptionHandler pointer must
		///< remain valid as the ExceptionSink is copied around.

	ExceptionHandler * eh() const ;
		///< Returns the exception handler pointer.

	ExceptionSource * esrc() const ;
		///< Returns the exception source pointer.

	void call( std::exception & e , bool done ) ;
		///< Calls the exception handler's onException() method.
		///< Used by EventHandlerList and TimerList. Exceptions
		///< thrown out of the onException() implementation are
		///< allowed to propagate.

	void reset() ;
		///< Resets the pointers.

	bool set() const ;
		///< Returns true if eh() is not null.

private:
	ExceptionHandler * m_eh ;
	ExceptionSource * m_esrc ;
} ;

/// \class GNet::ExceptionSinkUnbound
/// A potential ExceptionSink that is realised by bind()ing an
/// exception source pointer. This is used in factory functions
/// such as GNet::Server::newPeer() where the container that
/// uses the factory function to create a containee needs to
/// know which containee a subsequent exception came from.
///
class GNet::ExceptionSinkUnbound
{
public:
	explicit ExceptionSinkUnbound( ExceptionHandler * eh ) ;
		///< Constructor.

	explicit ExceptionSinkUnbound( ExceptionHandler & eh ) ;
		///< Constructor.

	ExceptionSink bind( ExceptionSource * source ) const ;
		///< Returns a sink object with the source pointer set.

private:
	ExceptionHandler * m_eh ;
} ;

#endif
