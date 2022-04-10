//
// Copyright (C) 2001-2021 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "glog.h"

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
	// this event is mostly relevant to windows -- the default action
	// is to throw an exception -- for 'reason_closed' (ie. a clean
	// shutdown()) it would also be reasonable to read the socket
	// until it returns an error or zero, and/or set a close timer

	throw G::Exception( "socket disconnect event" , str(reason) ) ;
}

std::string GNet::EventHandler::str( EventHandler::Reason reason )
{
	if( reason == EventHandler::Reason::closed ) return "closed" ;
	if( reason == EventHandler::Reason::down ) return "network down" ;
	if( reason == EventHandler::Reason::reset ) return "connection reset by peer" ;
	if( reason == EventHandler::Reason::abort ) return "connection aborted" ;
	return std::string() ;
}

