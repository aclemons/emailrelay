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
/// \file geventhandler.cpp
///

#include "gdef.h"
#include "geventhandler.h"
#include "gexception.h"
#include "geventloop.h"
#include "glog.h"

GNet::EventHandler::EventHandler()
= default ;

GNet::EventHandler::~EventHandler()
{
	static_assert( noexcept(EventLoop::ptr()) , "" ) ;
	static_assert( noexcept(EventLoop::ptr()->drop(GNet::Descriptor())) , "" ) ;
	EventLoop * event_loop = EventLoop::ptr() ;
	if( event_loop != nullptr )
		event_loop->drop( m_fd ) ;
}

void GNet::EventHandler::readEvent()
{
	G_DEBUG( "GNet::EventHandler::readEvent: no override" ) ;
}

void GNet::EventHandler::writeEvent()
{
	G_DEBUG( "GNet::EventHandler::writeEvent: no override" ) ;
}

void GNet::EventHandler::otherEvent( EventHandler::Reason reason )
{
	throw G::Exception( "socket disconnect event" , str(reason) ) ;
}

std::string GNet::EventHandler::str( EventHandler::Reason reason )
{
	if( reason == EventHandler::Reason::failed ) return "connection failed" ;
	if( reason == EventHandler::Reason::closed ) return "closed" ;
	if( reason == EventHandler::Reason::down ) return "network down" ;
	if( reason == EventHandler::Reason::reset ) return "connection reset by peer" ;
	if( reason == EventHandler::Reason::abort ) return "connection aborted" ;
	return {} ;
}

