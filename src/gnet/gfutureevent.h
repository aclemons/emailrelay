//
// Copyright (C) 2001-2020 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#ifndef G_NET_FUTURE_EVENT__H
#define G_NET_FUTURE_EVENT__H

#include "gdef.h"
#include "gexceptionsink.h"
#include "geventhandler.h"

namespace GNet
{
	class FutureEvent ;
	class FutureEventHandler ;
	class FutureEventImp ;
}

/// \class GNet::FutureEvent
/// An object that hooks into the event loop and calls back to the client
/// code. The thread-safe trigger function send() is typically called from
/// a "future/promise" worker thread just before it finishes.
///
/// Eg:
/// \code
/// struct Foo : private FutureEventHandler , private ExceptionHandler
/// {
///  Foo() ;
///  void onFutureEvent() ;
///  void run( FutureEvent::handle_type ) ;
///  FutureEvent m_future_event ;
///  std::thread m_thread ;
///  int m_result ;
/// }
/// Foo::Foo() :
///    m_future_event(*this,*this) ,
///    m_thread(&Foo::run,this,m_future_event.handle())
/// {
/// }
/// void Foo::run( FutureEvent::handle_type h )
/// {
///   m_result = ... ; // do blocking work in worker thread
///   FutureEvent::send( h ) ; // raise 'work complete' event
/// }
/// \endcode
///
class GNet::FutureEvent
{
public:
	G_EXCEPTION( Error , "FutureEvent error" ) ;
	using handle_type = HANDLE ;

	FutureEvent( FutureEventHandler & , ExceptionSink ) ;
		///< Constructor. Installs itself in the event loop.

	~FutureEvent() ;
		///< Destructor.

	handle_type handle() noexcept ;
		///< Extracts a handle that can be passed between threads
		///< and used in send(). This should be called once,
		///< typically as the worker thread is created.

	static bool send( handle_type handle , bool close = true ) noexcept ;
		///< Pokes an event into the main event loop so that the
		///< FutureEventHandler callback is called asynchronously.
		///<
		///< Should be called exactly once with 'close' true
		///< if handle() has been called, typically just before
		///< the worker thread finishes.
		///<
		///< This is safe even if the FutureEvent object has been
		///< deleted. Returns true on success.

public:
	FutureEvent( const FutureEvent & ) = delete ;
	FutureEvent( FutureEvent && ) = delete ;
	void operator=( const FutureEvent & ) = delete ;
	void operator=( FutureEvent && ) = delete ;

private:
	std::unique_ptr<FutureEventImp> m_imp ;
} ;

/// \class GNet::FutureEventHandler
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
