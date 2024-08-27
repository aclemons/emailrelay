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
/// \file geventemitter.cpp
///

#include "gdef.h"
#include "geventemitter.h"
#include "gnetdone.h"
#include "geventloggingcontext.h"
#include "glog.h"
#include "gassert.h"
#include <functional>

namespace GNet
{
	namespace EventEmitterImp
	{
		template <typename T> void raiseEvent( T handler , EventState & es ) ;

		// for cleaner stack traces compared to std::bind ...
		struct Binder1
		{
			using Method = void (GNet::EventHandler::*)() ;
			Method m_method ;
			EventHandler * m_handler ;
			void operator()() { (m_handler->*m_method)() ; }
		} ;
		struct Binder2
		{
			using Method = void (GNet::EventHandler::*)( EventHandler::Reason ) ;
			Method m_method ;
			EventHandler * m_handler ;
			EventHandler::Reason m_reason ;
			void operator()() { (m_handler->*m_method)( m_reason ) ; }
		} ;
		Binder1 bind( Binder1::Method method , EventHandler * handler ) { return {method,handler} ; }
		Binder2 bind( Binder2::Method method , EventHandler * handler , EventHandler::Reason reason ) { return {method,handler,reason} ; }
	}
}

template <typename T>
void GNet::EventEmitterImp::raiseEvent( T handler , EventState & es )
{
	// see also: std::make_exception_ptr, std::rethrow_exception

	EventLoggingContext set_logging_context( es ) ;
	try
	{
		handler() ; // eg. EventHandler::readEvent()
	}
	catch( GNet::Done & e )
	{
		if( es.hasExceptionHandler() )
			es.doOnException( e , true ) ;
		else
			throw ;
	}
	catch( std::exception & e )
	{
		if( es.hasExceptionHandler() )
			es.doOnException( e , false ) ;
		else
			throw ;
	}
}

// --

void GNet::EventEmitter::raiseReadEvent( EventHandler * handler , EventState & es )
{
	namespace imp = EventEmitterImp ;
	if( handler )
		imp::raiseEvent( imp::bind(&EventHandler::readEvent,handler) , es ) ;
}

void GNet::EventEmitter::raiseWriteEvent( EventHandler * handler , EventState & es )
{
	namespace imp = EventEmitterImp ;
	if( handler )
		imp::raiseEvent( imp::bind(&EventHandler::writeEvent,handler) , es ) ;
}

void GNet::EventEmitter::raiseOtherEvent( EventHandler * handler , EventState & es , EventHandler::Reason reason )
{
	namespace imp = EventEmitterImp ;
	if( handler )
		imp::raiseEvent( imp::bind(&EventHandler::otherEvent,handler,reason) , es ) ;
}

