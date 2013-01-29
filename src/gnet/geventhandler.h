//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#ifndef G_EVENT_HANDLER_H
#define G_EVENT_HANDLER_H

#include "gdef.h"
#include "gnet.h"
#include "gdatetime.h"
#include "gdescriptor.h"
#include <map>
#include <string>

/// \namespace GNet
namespace GNet
{
	class EventHandler ;
	class EventHandlerList ;
	class TimerList ;
}

/// \class GNet::EventHandler
/// A base class for classes that handle asynchronous 
/// socket events.
///
/// An event handler object has its virtual methods called when
/// an event is detected on the associated file descriptor.
///
/// If an event handler throws an exception which is caught
/// by the event loop then the event loop calls the handler's
/// onException() method.
///
/// A file descriptors and associated event handlers are
/// typically kept in a EventHandlerList container within 
/// the EventLoop singleton.
///
class GNet::EventHandler 
{
public:
	virtual ~EventHandler() ;
		///< Destructor.

	virtual void readEvent() ;
		///< Called for a read event. Overridable.
		///< The default implementation does nothing.

	virtual void writeEvent() ;
		///< Called for a write event. Overrideable.
		///< The default implementation does nothing.

	virtual void exceptionEvent() ;
		///< Called for an exception event. Overridable.
		///< The default implementation throws an exception 
		///< resulting in a call to onException().

	virtual void onException( std::exception & ) = 0 ;
		///< Called when an exception is thrown out of
		///< readEvent(), writeEvent() or exceptionEvent().
		///<
		///< The implementation may just do a "throw" to throw
		///< the current exception out of the event loop, 
		///< or a "delete this" for objects that manage
		///< themselves on the heap.
		///<
		///< EventHandler objects or timer objects that are 
		///< sub-objects of other EventHandler objects will 
		///< normally have their implementation of 
		///< onException() or onTimeoutException() delgate
		///< to the outer object's onException().

private:
	void operator=( const EventHandler & ) ; // not implemented
} ;

/// \class GNet::EventHandlerList
/// A class which can be used in the implemention
/// of classes derived from GNet::EventLoop.
///
class GNet::EventHandlerList 
{
public:
	typedef std::map<Descriptor,EventHandler*> Map ;
	typedef Map::const_iterator Iterator ;

public:
	explicit EventHandlerList( const std::string & type ) ;
		///< Constructor. The type parameter (eg. "read")
		///< is used only in debugging messages.

	void add( Descriptor fd , EventHandler * handler ) ;
		///< Adds a file-descriptor/handler pair to
		///< the list.

	void remove( Descriptor fd ) ;
		///< Removes a file-descriptor from the list.

	bool contains( Descriptor fd ) const ;
		///< Returns true if the list contains the
		///< given file-descriptor.

	EventHandler * find( Descriptor fd ) ;
		///< Finds the handler associated with the
		///< given file descriptor.

	void lock() ; 
		///< Called at the start of an iteration which
		///< might change the list.

	void unlock() ; 
		///< Called at the end of an iteration.

	Iterator begin() const ;
		///< Returns a forward iterator.

	Iterator end() const ;
		///< Returns an end iterator.

	static Descriptor fd( Iterator i ) ;
		///< Returns the iterator's file descriptor.

	static EventHandler * handler( Iterator i ) ;
		///< Returns the iterator's handler. Returns null
		///< if the fd has been remove()d but the
		///< list is still lock()ed.

private:
	EventHandlerList( const EventHandlerList & ) ;
	void operator=( const EventHandlerList & ) ;
	void collectGarbage() ;

private:
	std::string m_type ;
	Map m_map ;
	unsigned int m_lock ;
	bool m_has_garbage ;
} ;

inline
GNet::Descriptor GNet::EventHandlerList::fd( Iterator i )
{
	return (*i).first ;
}

inline
GNet::EventHandler * GNet::EventHandlerList::handler( Iterator i )
{
	return (*i).second ;
}

#endif

