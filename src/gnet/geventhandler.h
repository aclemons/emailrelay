//
// Copyright (C) 2001-2004 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// geventhandler.h
//

#ifndef G_EVENT_HANDLER_H
#define G_EVENT_HANDLER_H

#include "gdef.h"
#include "gnet.h"
#include "gdatetime.h"
#include "gdescriptor.h"
#include <list>
#include <string>

namespace GNet
{
	class EventHandler ;
	class EventHandlerListItem ;
	class EventHandlerList ;
	class TimerList ;
}

// Class: GNet::EventHandler
// Description: A pseudo-abstract base class for classes
// which handle asynchronous socket events. ("Pseudo" because
// there are empty implementations for the virtual methods, 
// since you dont want to have to override all three every time.)
//
// An event handler object has its virtual methods called when
// an event is detected on the associated file descriptor.
//
// A file descriptor and its associated event handler
// are typically kept in a EventHandlerListItem
// structure, within the EventLoop singleton.
//
class GNet::EventHandler 
{
public:
	virtual ~EventHandler() ;
		// Destructor.

	virtual void readEvent() ;
		// Called for a read event. The default 
		// implementation does nothing.

	virtual void writeEvent() ;
		// Called for a write event. The default 
		// implementation does nothing.

	virtual void exceptionEvent() ;
		// Called for an exception event. The default 
		// implementation does nothing.

private:
	void operator=( const EventHandler & ) ; // not implemented
} ;

// Class: GNet::EventHandlerListItem
// Description: A private class which contains a file descriptor
// and a reference to its handler.
//
class GNet::EventHandlerListItem 
{
public:
	Descriptor m_fd ;
	EventHandler * m_handler ;
	EventHandlerListItem( Descriptor fd = Descriptor__invalid() , 
		EventHandler * handler = NULL ) ;
} ;

namespace GNet
{
	typedef std::list<EventHandlerListItem> EventHandlerListImp ;
}

// Class: GNet::EventHandlerList
// Description: A class which can be used in the implemention
// of classes derived from GNet::EventLoop.
//
class GNet::EventHandlerList 
{
public:
	typedef EventHandlerListImp List ;
	typedef List::const_iterator Iterator ;

public:
	explicit EventHandlerList( const std::string & type ) ;
		// Constructor. The type parameter (eg. "read")
		// is used only in debugging messages.

	void add( Descriptor fd , EventHandler * handler ) ;
		// Adds a file-descriptor/handler pair to
		// the list.

	void remove( Descriptor fd ) ;
		// Removes a file-descriptor from the list.

	bool contains( Descriptor fd ) const ;
		// Returns true if the list contains the
		// given file-descriptor.

	EventHandler * find( Descriptor fd ) ;
		// Finds the handler associated with the
		// given file descriptor.

	void lock() ; 
		// Called at the start of an iteration which
		// might change the list.

	void unlock() ; 
		// Called at the end of an iteration.

	Iterator begin() const ;
		// Returns a forward iterator.

	Iterator end() const ;
		// Returns an end iterator.

	static Descriptor fd( Iterator i ) ;
		// Returns the iterator's file descriptor.

	static EventHandler * handler( Iterator i ) ;
		// Returns the iterator's handler. Returns null
		// if the fd has been remove()d but the
		// list is still lock()ed.

private:
	EventHandlerList( const EventHandlerList & ) ;
	void operator=( const EventHandlerList & ) ;

private:
	std::string m_type ;
	List m_list ;
	unsigned int m_lock ;
} ;

//static 
inline
GNet::Descriptor GNet::EventHandlerList::fd( Iterator i )
{
	return (*i).m_fd ;
}

//static 
inline
GNet::EventHandler * GNet::EventHandlerList::handler( Iterator i )
{
	return (*i).m_handler ;
}

#endif

