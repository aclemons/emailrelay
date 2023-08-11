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
/// \file geventemitter.h
///

#ifndef G_NET_EVENT_EMITTER_H
#define G_NET_EVENT_EMITTER_H

#include "gdef.h"
#include "geventhandler.h"
#include "gexceptionsink.h"

namespace GNet
{
	class EventEmitter ;
}

//| \class GNet::EventEmitter
/// An EventHandler and ExceptionSink tuple, with methods to raise an
/// event and handle any exceptions. Used in EventLoop implementations.
/// Also sets an appropriate G::LogOutput context while events are
/// being handled.
///
/// The event loop should normally instantiate a read emitter and
/// a write emitter for each new file descriptor; any existing
/// emitters should be update()d rather than destructed and
/// constructed, with garbage collection once all the events
/// have been handled.
///
class GNet::EventEmitter
{
public:
	EventEmitter() noexcept ;
		///< Default constructor. The raise methods do nothing and
		///< consequently the exception sink is not used.
		///< Postcondition: !handler()

	EventEmitter( EventHandler * , ExceptionSink ) noexcept ;
		///< Constructor.

	void raiseReadEvent( Descriptor ) ;
		///< Calls the EventHandler readEvent() method.

	void raiseWriteEvent( Descriptor ) ;
		///< Calls the EventHandler writeEvent() method.

	void raiseOtherEvent( Descriptor , EventHandler::Reason ) ;
		///< Calls the EventHandler otherEvent() method.

	EventHandler * handler() const ;
		///< Returns the handler, as passed to the ctor.

	ExceptionSink es() const ;
		///< Returns the exception sink, as passed to the ctor.

	void update( EventHandler * , ExceptionSink ) noexcept ;
		///< Sets the event handler and the exception sink.

	void reset() noexcept ;
		///< Resets the EventHandler so that the raise methods
		///< do nothing.
		///< Postcondition: !handler()

	void disarm( ExceptionHandler * ) noexcept ;
		///< If the exception handler matches then reset it
		///< so that it is not called. Any exceptions will be
		///< thrown out of of the event loop and back to
		///< main().

private:
	void raiseEvent( void (EventHandler::*method)() , Descriptor ) ;
	void raiseEvent( void (EventHandler::*method)(EventHandler::Reason) , Descriptor , EventHandler::Reason ) ;

private:
	EventHandler * m_handler {nullptr} ; // handler for the event
	ExceptionSink m_es ; // handler for any thrown exception
	ExceptionSink m_es_saved ; // in case disarm()ed but still needed
} ;

inline
GNet::EventHandler * GNet::EventEmitter::handler() const
{
	return m_handler ;
}

inline
GNet::ExceptionSink GNet::EventEmitter::es() const
{
	return m_es ;
}

#endif
