//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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

GNet::EventEmitter::EventEmitter( EventHandler * handler , ExceptionSink es ) noexcept :
	m_handler(handler) ,
	m_es(es)
{
}

void GNet::EventEmitter::raiseReadEvent( Descriptor fd )
{
	raiseEvent( &EventHandler::readEvent , fd ) ;
}

void GNet::EventEmitter::raiseWriteEvent( Descriptor fd )
{
	raiseEvent( &EventHandler::writeEvent , fd ) ;
}

void GNet::EventEmitter::raiseOtherEvent( Descriptor fd , EventHandler::Reason reason )
{
	raiseEvent( &EventHandler::otherEvent , fd , reason ) ;
}

void GNet::EventEmitter::raiseEvent( void (EventHandler::*method)(Descriptor) ,
	Descriptor fd )
{
	// see also: std::make_exception_ptr, std::rethrow_exception

	EventLoggingContext set_logging_context( m_handler && m_es.set() ? m_es.esrc() : nullptr ) ;
	try
	{
		if( m_handler != nullptr )
			(m_handler->*method)( fd ) ; // EventHandler::readEvent()/writeEvent()
	}
	catch( GNet::Done & e )
	{
		if( m_es.set() )
			m_es.call( e , true ) ; // ExceptionHandler::onException()
		else
			throw ;
	}
	catch( std::exception & e )
	{
		if( m_es.set() )
			m_es.call( e , false ) ; // ExceptionHandler::onException()
		else
			throw ;
	}
}

void GNet::EventEmitter::raiseEvent( void (EventHandler::*method)(Descriptor,EventHandler::Reason) ,
	Descriptor fd , EventHandler::Reason reason )
{
	EventLoggingContext set_logging_context( m_handler && m_es.set() ? m_es.esrc() : nullptr ) ;
	try
	{
		if( m_handler != nullptr )
			(m_handler->*method)( fd , reason ) ; // EventHandler::otherEvent()
	}
	catch( GNet::Done & e )
	{
		if( m_es.set() )
			m_es.call( e , true ) ; // ExceptionHandler::onException()
		else
			throw ;
	}
	catch( std::exception & e )
	{
		if( m_es.set() )
			m_es.call( e , false ) ; // ExceptionHandler::onException()
		else
			throw ;
	}
}

