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
/// \file gexceptionhandler.h
///

#ifndef G_NET_EXCEPTION_HANDLER__H
#define G_NET_EXCEPTION_HANDLER__H

#include "gdef.h"
#include <exception>

namespace GNet
{
	class ExceptionHandler ;
}

/// \class GNet::ExceptionHandler
/// An abstract interface for handling exceptions thrown out of event-loop
/// callbacks (socket events, future events and timer events). If the handler
/// just rethrows then the event loop will terminate.
///
/// Many GNet classes require a ExceptionHandler reference to be passed
/// around, but only classes that manage their own lifetimes on the heap will
/// use this as a base; these classes override onException() to absorb the
/// exception and schedule their own deletion.
///
/// The event loop itself derives from this interface as a convenience.
///
class GNet::ExceptionHandler
{
public:
	virtual void onException( std::exception & ) = 0 ;
		///< Called by the event loop when an exception is thrown out
		///< of an event loop callback.
		///<
		///< The implementation may just do a "throw" to rethrow the
		///< current exception out of the event loop, or schedule
		///< a "delete this" for objects that manage themselves
		///< on the heap.

protected:
	virtual ~ExceptionHandler() ;
		///< Destructor.

private:
	void operator=( const ExceptionHandler & ) ;
} ;

#endif
