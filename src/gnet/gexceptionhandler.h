//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gexceptionhandler.h
///

#ifndef G_NET_EXCEPTION_HANDLER_H
#define G_NET_EXCEPTION_HANDLER_H

#include "gdef.h"
#include "gexceptionsource.h"
#include <exception>

namespace GNet
{
	class ExceptionHandler ;
	class EventHandler ;
}

//| \class GNet::ExceptionHandler
/// An abstract interface for handling exceptions thrown out of
/// event-loop callbacks (socket/future events and timer events).
/// If the handler just rethrows then the event loop will terminate.
///
/// The ExceptionHandler destructor calls disarm() on the EventHandlerList
/// and TimerList so that an onException() callback is not delivered
/// if the target object has been destroyed.
///
class GNet::ExceptionHandler
{
public:
	virtual ~ExceptionHandler() ;
		///< Destructor. Matching entries in the EventHandlerList and
		///< TimerList are disarm()ed.

	virtual void onException( ExceptionSource * source , std::exception & e , bool done ) = 0 ;
		///< Called by the event loop when an exception is thrown out
		///< of an event loop callback. The exception is still active
		///< so it can be rethrown with "throw" or captured with
		///< std::current_exception().
		///<
		///< The source parameter can be used to point to the object
		///< that received the original event loop callback. This
		///< requires the appropriate exception source pointer is
		///< defined when the event source is first registered with
		///< the event loop, otherwise it defaults to a null pointer.
		///< (The ExceptionSinkUnbound class is used where necessary
		///< to encourage the definition of a valid exception source
		///< pointer.)
		///<
		///< The 'done' parameter indicates whether the exception
		///< was of type GNet::Done.

public:
	ExceptionHandler() = default ;
	ExceptionHandler( const ExceptionHandler & ) = delete ;
	ExceptionHandler( ExceptionHandler && ) = default ;
	ExceptionHandler & operator=( const ExceptionHandler & ) = delete ;
	ExceptionHandler & operator=( ExceptionHandler && ) = default ;
} ;

#endif
