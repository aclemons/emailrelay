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
/// \file geventloop.h
///

#ifndef G_NET_EVENT_LOOP__H
#define G_NET_EVENT_LOOP__H

#include "gdef.h"
#include "geventhandler.h"
#include "gexceptionsink.h"
#include "gexception.h"
#include "gdescriptor.h"
#include "gsignalsafe.h"
#include <list>
#include <string>

namespace GNet
{
	class EventLoop ;
}

/// \class GNet::EventLoop
/// An abstract base class for a singleton that keeps track of open sockets
/// and their associated handlers. Derived classes are used to implement
/// different event loops, such as select() or WaitForMultipleObjects().
///
/// In practice sockets should be added and removed from the class by
/// calling GNet::Socket::addReadHandler() and friends rather than by calling
/// EventLoop::addRead() etc. so that the event handle is passed correctly
/// when running on windows.
///
/// The class has a static member for finding an instance, but instances
/// are not created automatically.
///
class GNet::EventLoop
{
public:
	G_EXCEPTION( Error , "failed to initialise the event loop" ) ;
	G_EXCEPTION( NoInstance , "no event loop instance" ) ;
	G_EXCEPTION( Overflow , "event loop overflow" ) ;

protected:
	EventLoop() ;
		///< Constructor.

	struct Running /// RAII class that sets a boolean, for implementations of run()/running().
	{
		explicit Running( bool & bref ) ;
		~Running() ;
		bool & m_bref ;
	} ;

public:
	static EventLoop * create() ;
		///< A factory method which creates an instance of a derived
		///< class on the heap. Throws on error.

	static EventLoop & instance() ;
		///< Returns a reference to an instance of the class,
		///< if any. Throws if none. Does not do any instantiation
		///< itself.

	static bool exists() ;
		///< Returns true if an instance exists.

	static void stop( const G::SignalSafe & ) ;
		///< Calls quit() on instance().

	virtual ~EventLoop() ;
		///< Destructor.

	virtual std::string run() = 0 ;
		///< Runs the main event loop. Returns a quit() reason,
		///< if any.

	virtual bool running() const = 0 ;
		///< Returns true if called from within run().

	virtual void quit( const std::string & reason ) = 0 ;
		///< Causes run() to return (once the call stack has
		///< unwound). If there are multiple quit()s before
		///< run() returns then the latest reason is used.

	virtual void quit( const G::SignalSafe & ) = 0 ;
		///< A signal-safe overload to quit() the event loop.

	virtual void addRead( Descriptor fd , EventHandler & , ExceptionSink ) = 0 ;
		///< Adds the given event source descriptor and associated
		///< handler to the read list.
		///< See also Socket::addReadHandler().

	virtual void addWrite( Descriptor fd , EventHandler & , ExceptionSink ) = 0 ;
		///< Adds the given event source descriptor and associated
		///< handler to the write list.
		///< See also Socket::addWriteHandler().

	virtual void addOther( Descriptor fd , EventHandler & , ExceptionSink ) = 0 ;
		///< Adds the given event source descriptor and associated
		///< handler to the exception list.
		///< See also Socket::addOtherHandler().

	virtual void dropRead( Descriptor fd ) = 0 ;
		///< Removes the given event source descriptor from the
		///< list of read sources.
		///< See also Socket::dropReadHandler().

	virtual void dropWrite( Descriptor fd ) = 0 ;
		///< Removes the given event source descriptor from the
		///< list of write sources.
		///< See also Socket::dropWriteHandler().

	virtual void dropOther( Descriptor fd ) = 0 ;
		///< Removes the given event source descriptor from the
		///< list of exception sources.
		///< See also Socket::dropOtherHandler().

	virtual std::string report() const = 0 ;
		///< Returns a line of text reporting the status of the event loop.
		///< Used in debugging and diagnostics.

	virtual void disarm( ExceptionHandler * ) = 0 ;
		///< Used to prevent the given interface from being used,
		///< typically called from the ExceptionHandler
		///< destructor.

private:
	EventLoop( const EventLoop & ) g__eq_delete ;
	void operator=( const EventLoop & ) g__eq_delete ;

private:
	static EventLoop * m_this ;
} ;

inline
GNet::EventLoop::Running::Running( bool & bref ) :
	m_bref(bref)
{
	m_bref = true ;
}

inline
GNet::EventLoop::Running::~Running()
{
	m_bref = false ;
}

#endif
