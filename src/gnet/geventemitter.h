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
/// \file geventemitter.h
///

#ifndef G_NET_EVENT_EMITTER_H
#define G_NET_EVENT_EMITTER_H

#include "gdef.h"
#include "geventhandler.h"
#include "geventstate.h"

namespace GNet
{
	class EventEmitter ;
}

//| \class GNet::EventEmitter
/// Provides static methods to raise an EventHandler event, as used
/// by the various event loop implementations.
///
/// Any exceptions thrown by an event handler are caught and delivered
/// to the associated exception handler.
///
/// Event loop implementations are required to keep the EventState
/// object valid when using this interface, even if the event handler
/// deletes the target object(s) (see EventLoop::disarm()).
///
class GNet::EventEmitter
{
public:
	static void raiseReadEvent( EventHandler * , EventState & ) ;
		///< Calls readEvent() on the event handler and catches any
		///< exceptions and delivers them to the EventState exception
		///< handler.

	static void raiseWriteEvent( EventHandler * , EventState & ) ;
		///< Calls writeEvent() on the event handler and catches any
		///< exceptions and delivers them to the EventState exception
		///< handler.

	static void raiseOtherEvent( EventHandler * , EventState & , EventHandler::Reason ) ;
		///< Calls otherEvent() on the event handler and catches any
		///< exceptions and delivers them to the EventState exception
		///< handler.

public:
	EventEmitter() = delete ;
} ;

#endif
