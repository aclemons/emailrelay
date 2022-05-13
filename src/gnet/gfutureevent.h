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
/// \file gfutureevent.h
///

#ifndef G_NET_FUTURE_EVENT_H
#define G_NET_FUTURE_EVENT_H

#include "gdef.h"
#include "gexceptionsink.h"
#include "geventhandler.h"
#include "gexception.h"
#include <memory>

namespace GNet
{
	class FutureEvent ;
	class FutureEventHandler ;
	class FutureEventImp ;
}

//| \class GNet::FutureEvent
/// A FutureEvent object can be used to send a one-shot event between
/// threads via the event loop, resulting in a call to the relevant
/// event handler. This is used in the implementation of multi-threaded
/// asynchronous task classes such as GNet::Task and GNet::Resolver.
///
/// The thread-safe trigger function send() is typically called from
/// a worker thread just before it finishes.
///
/// Eg:
/// \code
/// struct Foo : private FutureEventHandler , private ExceptionHandler
/// {
///  Foo() ;
///  void onFutureEvent() ;
///  void run( HANDLE ) ;
///  FutureEvent m_future_event ;
///  std::thread m_thread ;
///  int m_result ;
/// }
/// Foo::Foo() :
///    m_future_event(*this,*this) ,
///    m_thread(&Foo::run,this,m_future_event.handle())
/// {
/// }
/// void Foo::run( HANDLE h )
/// {
///   m_result = ... ; // do blocking work in worker thread
///   FutureEvent::send( h ) ; // raise 'work complete' event
/// }
/// \endcode
///
/// The typical implementation uses a socketpair, with the read socket's
/// file descriptor registered with the event loop in the normal way
/// and the socket event handler delegating to the future-event
/// handler.
///
class GNet::FutureEvent
{
public:
	G_EXCEPTION( Error , tx("FutureEvent error") ) ;

	FutureEvent( FutureEventHandler & , ExceptionSink ) ;
		///< Constructor. Installs itself in the event loop.

	~FutureEvent() ;
		///< Destructor.

	HANDLE handle() noexcept ;
		///< Extracts a handle that can be passed between threads
		///< and used in send(). This should be called once,
		///< typically as the worker thread is created.

	static bool send( HANDLE handle , bool close = true ) noexcept ;
		///< Pokes an event into the main event loop so that the
		///< FutureEventHandler callback is called asynchronously.
		///<
		///< Should be called exactly once with 'close' true
		///< if handle() has been called, typically just before
		///< the worker thread finishes.
		///<
		///< This is safe even if the FutureEvent object has been
		///< deleted. Returns true on success.

	static HANDLE createHandle() ;
		///< Used by some event loop implementations to create the
		///< underlying synchronisation object.

public:
	FutureEvent( const FutureEvent & ) = delete ;
	FutureEvent( FutureEvent && ) = delete ;
	void operator=( const FutureEvent & ) = delete ;
	void operator=( FutureEvent && ) = delete ;

private:
	std::unique_ptr<FutureEventImp> m_imp ;
} ;

//| \class GNet::FutureEventHandler
/// A callback interface for GNet::FutureEvent.
///
class GNet::FutureEventHandler
{
public:
	virtual ~FutureEventHandler() = default ;
		///< Destructor.

	virtual void onFutureEvent() = 0 ;
		///< Callback function that delivers the future event.
} ;

#endif
