//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file geventhandler.h
///

#ifndef G_NET_EVENT_HANDLER__H
#define G_NET_EVENT_HANDLER__H

#include "gdef.h"
#include "gdatetime.h"
#include "gexceptionhandler.h"
#include "gdescriptor.h"
#include <vector>
#include <string>

namespace GNet
{
	class EventHandler ;
}

/// \class GNet::EventHandler
/// A base class for classes that handle asynchronous events from the
/// event loop.
///
/// An event handler object has its virtual methods called when an
/// event is detected on the associated file descriptor.
///
/// If an event handler throws an exception which is caught by the
/// event loop then the event loop calls the associated ExceptionHandler,
/// if any.
///
class GNet::EventHandler
{
public:
	enum Reason
	{
		reason_closed , // fin packet, clean shutdown
		reason_down , // network down
		reason_reset , // rst packet
		reason_abort , // socket failed
		reason_other // eg. oob data
	} ;

	virtual ~EventHandler() ;
		///< Destructor.

	virtual void readEvent() ;
		///< Called for a read event. Overridable. The default
		///< implementation does nothing.

	virtual void writeEvent() ;
		///< Called for a write event. Overrideable. The default
		///< implementation does nothing.

	virtual void otherEvent( Reason ) ;
		///< Called for a socket-exception event or a socket-close
		///< event on windows. Overridable. The default
		///< implementation throws an exception.

	static std::string str( Reason ) ;
		///< Returns a printable description of the other-event
		///< reason.

private:
	void operator=( const EventHandler & ) ;
} ;

#endif
