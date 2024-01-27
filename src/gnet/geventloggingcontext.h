//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file geventloggingcontext.h
///

#ifndef G_NET_EVENT_LOGGING_CONTEXT_H
#define G_NET_EVENT_LOGGING_CONTEXT_H

#include "gdef.h"
#include "gexceptionsource.h"

namespace GNet
{
	class EventLoggingContext ;
}

//| \class GNet::EventLoggingContext
/// A class that sets the G::LogOuput::context() while in scope.
///
class GNet::EventLoggingContext
{
public:
	explicit EventLoggingContext( ExceptionSource * esrc ) ;
		///< Constructor that sets the logging context to
		///< whatever ExceptionSource::exceptionSourceId()
		///< returns.

	explicit EventLoggingContext( const std::string & ) ;
		///< Constructor that sets the logging context to
		///< the given string.

	~EventLoggingContext() noexcept ;
		///< Destructor. Restores the logging context.

public:
	EventLoggingContext( const EventLoggingContext & ) = delete ;
	EventLoggingContext( EventLoggingContext && ) = delete ;
	EventLoggingContext & operator=( const EventLoggingContext & ) = delete ;
	EventLoggingContext & operator=( EventLoggingContext && ) = delete ;

private:
	static std::string fn( void * ) ;
	std::string str() ;

private:
	static EventLoggingContext * m_inner ;
	EventLoggingContext * m_outer ;
	ExceptionSource * m_esrc ;
	std::string m_s ;
} ;

#endif
