//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file geventhandlerlist.h
///

#ifndef G_NET_EVENT_HANDLER_LIST__H
#define G_NET_EVENT_HANDLER_LIST__H

#include "gdef.h"
#include "geventhandler.h"
#include "gexceptionsink.h"
#include "gdescriptor.h"
#include <vector>
#include <string>

namespace GNet
{
	class EventHandlerList ;
}

/// \class GNet::EventHandlerList
/// A class that maps from a file descriptor to an event handler and
/// exception handler, used in the implemention of classes derived
/// from GNet::EventLoop.
///
/// If an event handler is removed from the list while the list is
/// being iterated over then the relevant pointer is reset without
/// affecting the iteration. The unlock() method does garbage collection
/// once the iteration is complete.
///
/// If an event handler is added to the list while the list is being
/// iterated over then it is added to a pending-list. The pending
/// list is added to the main list by unlock() once the iteration
/// is complete.
///
/// Each event handler has an associated exception handler, typically a
/// more long-lived object that has the event handler as a sub-object.
/// If the event hander throws an exception then the event
/// handler list catches it and invokes the exception handler -- and
/// if that throws then the exception escapes the event loop. This is
/// safe even if the exception handler object is destroyed by the
/// original exception because the exception handler base-class
/// destructor uses the list's disarm() mechanism. This is the same
/// behaviour as in the TimerList.
///
/// Exception handlers are combined with an additional 'source' pointer
/// in an ExceptionSink tuple. The source pointer can be used to
/// provide additional information to the exception handler, typically
/// as a pointer to the event handler object.
///
/// Note that the Descriptor class is actually in two parts for Windows:
/// a socket handle (file descriptor) and an event-object handle. The
/// event handler list is keyed by the full Descriptor object rather than
/// just the socket handle and this allows it to contain event-handling
/// objects that are not sockets. See GNet::FutureEvent.
///
class GNet::EventHandlerList
{
public:
	struct Value /// A tuple for GNet::EventHandlerList.
	{
		Descriptor m_fd ;
		EventHandler * m_event_handler ; // handler for the event, source of any exception
		ExceptionSink m_es ; // handler for any thrown exception
		explicit Value( Descriptor fd ) ;
		Value( Descriptor fd , EventHandler * , ExceptionSink ) ;
	} ;
	struct Iterator /// An iterator for GNet::EventHandlerList.
	{
		typedef EventHandlerList::Value Value ;
		Iterator( const std::vector<Value> & , bool ) ;
		Iterator( std::vector<Value>::const_iterator , std::vector<Value>::const_iterator ) ;
		Iterator & operator++() ;
		const Value & operator*() const ;
		bool operator==( const Iterator & ) const ;
		bool operator!=( const Iterator & ) const ;
		Descriptor fd() const ;
		EventHandler * handler() ;
		ExceptionSink es() ;
		void raiseEvent( void (EventHandler::*method)() ) ;
		void raiseEvent( void (EventHandler::*method)(EventHandler::Reason) , EventHandler::Reason ) ;
		std::vector<Value>::const_iterator m_p ;
		std::vector<Value>::const_iterator m_end ;
	} ;
	struct Lock /// A raii class to lock and unlock GNet::EventHandlerList.
	{
		explicit Lock( EventHandlerList & , bool * invalid_p = nullptr ) ;
		~Lock() ;
		EventHandlerList & m_list ;
		bool * m_invalid_p ;
	} ;

public:
	explicit EventHandlerList( const std::string & type ) ;
		///< Constructor. The type parameter (eg. "read") is used only in
		///< debugging messages.

	void add( Descriptor fd , EventHandler * handler , ExceptionSink ) ;
		///< Adds a file-descriptor/handler tuple to the list.

	void remove( Descriptor fd ) ;
		///< Removes a file-descriptor from the list. Does nothing if the
		///< file-descriptor is not in the list.

	Iterator find( Descriptor fd ) const ;
		///< Finds an entry in the list. Returns end() if not found.
		///< This ignores any pending changes, ie. descriptors add()ed
		///< or remove()d while lock()ed.

	bool contains( Descriptor fd ) const ;
		///< Returns true if the list, taking account of any pending
		///< changes, contains the given descriptor.

	void lock() ;
		///< To be called at the start of an begin()/end() iteration if the
		///< list might change during the iteration. Must be paired with
		///< unlock().

	bool unlock() ;
		///< Called at the end of a begin()/end() iteration to match a call
		///< to lock(). Does garbage collection. Returns true if anything
		///< might have changed.

	Iterator begin() const ;
		///< Returns a forward iterator. If the list is lock()ed then the
		///< iterator will skip over descriptors that have bee remove()d
		///< or add()ed while locked.

	Iterator end() const ;
		///< Returns an end iterator.

	void disarm( ExceptionHandler * ) ;
		///< Resets any matching ExceptionHandler pointers, so exceptions
		///< thrown out of the relevant file descriptors' event handlers
		///< are rethrown by this class rather than being delivered to
		///< the exception handler interface.

	void getHandles( std::vector<HANDLE> & ) const ;
		///< Adds unique, non-zero Descriptor handles to the given
		///< sorted list, including any add()ed while lock()ed.

private:
	typedef std::vector<Value> List ;
	EventHandlerList( const EventHandlerList & ) g__eq_delete ;
	void operator=( const EventHandlerList & ) g__eq_delete ;
	static void addImp( List & list , Descriptor fd , EventHandler * , ExceptionSink ) ;
	static bool disable( List & list , Descriptor fd ) ;
	static bool remove( List & list , Descriptor fd ) ;
	static void getHandles( const List & , std::vector<HANDLE> & ) ;
	void disarm( List & , ExceptionHandler * ) ;
	void commitPending() ;
	void collectGarbage() ;

private:
	std::string m_type ;
	List m_list ;
	List m_pending_list ;
	unsigned int m_lock ;
	bool m_has_garbage ;
} ;

inline
GNet::EventHandlerList::Value::Value( Descriptor fd , EventHandler * event_handler , ExceptionSink es ) :
	m_fd(fd),
	m_event_handler(event_handler),
	m_es(es)
{
}

inline
GNet::EventHandlerList::Value::Value( Descriptor fd ) :
	m_fd(fd),
	m_event_handler(nullptr),
	m_es(ExceptionSink(ExceptionSink::Type::Null))
{
}

inline
GNet::EventHandlerList::Iterator GNet::EventHandlerList::begin() const
{
	return Iterator( m_list , false ) ;
}

inline
GNet::EventHandlerList::Iterator GNet::EventHandlerList::end() const
{
	return Iterator( m_list , true ) ;
}

inline
GNet::EventHandlerList::Iterator::Iterator( std::vector<Value>::const_iterator p , std::vector<Value>::const_iterator end ) :
	m_p(p) ,
	m_end(end)
{
}

inline
GNet::EventHandlerList::Iterator::Iterator( const std::vector<Value> & v , bool end ) :
	m_p(end?v.end():v.begin()) ,
	m_end(v.end())
{
	while( m_p != m_end && (*m_p).m_event_handler == nullptr )
		++m_p ;
}

inline
GNet::EventHandlerList::Iterator & GNet::EventHandlerList::Iterator::operator++()
{
	++m_p ;
	while( m_p != m_end && (*m_p).m_event_handler == nullptr )
		++m_p ;
	return *this ;
}

inline
const GNet::EventHandlerList::Value & GNet::EventHandlerList::Iterator::operator*() const
{
	return *m_p ;
}

inline
GNet::Descriptor GNet::EventHandlerList::Iterator::fd() const
{
	return (*m_p).m_fd ;
}

inline
GNet::EventHandler * GNet::EventHandlerList::Iterator::handler()
{
	return (*m_p).m_event_handler ;
}

inline
GNet::ExceptionSink GNet::EventHandlerList::Iterator::es()
{
	return (*m_p).m_es ;
}

inline
bool GNet::EventHandlerList::Iterator::operator==( const Iterator & other ) const
{
	return m_p == other.m_p ;
}

inline
bool GNet::EventHandlerList::Iterator::operator!=( const Iterator & other ) const
{
	return !(*this == other) ;
}

#endif
