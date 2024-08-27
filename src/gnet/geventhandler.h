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
/// \file geventhandler.h
///

#ifndef G_NET_EVENT_HANDLER_H
#define G_NET_EVENT_HANDLER_H

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

//| \class GNet::EventHandler
/// A base class for classes that have a file descriptor and handle
/// asynchronous events from the event loop.
///
/// The event handler virtual methods are called when an event is
/// detected on the associated file descriptor.
///
/// The EventEmitter class ensures that if an exception is thrown out
/// of an event handler it is caught and delivered to an associated
/// ExceptionHandler interface (if any).
///
class GNet::EventHandler
{
public:
	enum class Reason
	{
		failed , // connection failed
		closed , // fin packet, clean shutdown
		down , // network down
		reset , // rst packet
		abort , // socket failed
		other // eg. oob data
	} ;

	EventHandler() ;
		///< Constructor.

	virtual ~EventHandler() ;
		///< Destructor.

	virtual void readEvent() ;
		///< Called for a read event. Overridable. The default
		///< implementation does nothing. The descriptor might
		///< not be valid() if a non-socket event on windows.

	virtual void writeEvent() ;
		///< Called for a write event. Overrideable. The default
		///< implementation does nothing.

	virtual void otherEvent( Reason ) ;
		///< Called for a socket-exception event, or a socket-close
		///< event on windows. Overridable. The default
		///< implementation throws an exception.

	static std::string str( Reason ) ;
		///< Returns a printable description of the other-event
		///< reason.

	void setDescriptor( Descriptor ) noexcept ;
		///< File descriptor setter. Used by the event loop.

	Descriptor descriptor() const noexcept ;
		///< File descriptor getter. Used by the event loop.

public:
	EventHandler( const EventHandler & ) = delete ;
	EventHandler( EventHandler && ) = delete ;
	EventHandler & operator=( const EventHandler & ) = delete ;
	EventHandler & operator=( EventHandler && ) = delete ;

private:
	Descriptor m_fd ;
} ;

inline
void GNet::EventHandler::setDescriptor( Descriptor fd ) noexcept
{
	m_fd = fd ;
}

inline
GNet::Descriptor GNet::EventHandler::descriptor() const noexcept
{
	return m_fd ;
}

#endif
