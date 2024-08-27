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
/// \file geventlogging.h
///

#ifndef GNET_EVENT_LOGGING_H
#define GNET_EVENT_LOGGING_H

#include "gdef.h"
#include "gstringview.h"
#include "gassert.h"
#include <string>

namespace GNet
{
	class EventLogging ;
}

//| \class GNet::EventLogging
/// An interface for GNet classes that define a logging context
/// string.
///
/// The EventLogging interface pointer should be installed in an
/// EventState object; then various GNet classes collaborate so
/// that the G::LogOuput context is set appropriately when events
/// are delivered to any objects that inherit copies of that
/// EventState.
///
/// \see GNet::EventState, GNet::EventLoggingContext
///
class GNet::EventLogging
{
public:
	explicit EventLogging( const EventLogging * ) ;
		///< Constructor. Sets the next() pointer.

	virtual ~EventLogging() ;
		///< Destructor.

	virtual std::string_view eventLoggingString() const ;
		///< Returns a string containing logging information
		///< for the object. The string-view should refer to
		///< a string data member or be a nullptr string-view
		///< if there is no logging information.

	const EventLogging * next() const noexcept ;
		///< Returns the link pointer.

public:
	EventLogging( const EventLogging & ) = delete ;
	EventLogging( EventLogging && ) = delete ;
	EventLogging & operator=( const EventLogging & ) = delete ;
	EventLogging & operator=( EventLogging && ) = delete ;

private:
	const EventLogging * m_next {nullptr} ;
} ;

#endif
