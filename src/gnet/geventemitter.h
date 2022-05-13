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

	void reset() noexcept ;
		///< Resets the EventHandler so that the raise methods
		///< do nothing and the exception sink is not used.
		///< Postcondition: !handler()

	void disarm() noexcept ;
		///< Resets the ExceptionSink so that exceptions thrown
		///< out of event handlers are rethrown, typically out
		///< of the event loop and back to main().

private:
	void raiseEvent( void (EventHandler::*method)(Descriptor) , Descriptor ) ;
	void raiseEvent( void (EventHandler::*method)(Descriptor,EventHandler::Reason) , Descriptor , EventHandler::Reason ) ;

private:
	EventHandler * m_handler ; // handler for the event
	ExceptionSink m_es ; // handler for any thrown exception
} ;

inline
GNet::EventEmitter::EventEmitter() noexcept :
	m_handler(nullptr)
{
}

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

inline
void GNet::EventEmitter::reset() noexcept
{
	m_handler = nullptr ;
}

inline
void GNet::EventEmitter::disarm() noexcept
{
	m_es.reset() ;
}

#endif
