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
/// \file geventhandler.cpp
///

#include "gdef.h"
#include "geventhandler.h"
#include "gexception.h"
#include "glog.h"

void GNet::EventHandler::readEvent( Descriptor )
{
	G_DEBUG( "GNet::EventHandler::readEvent: no override" ) ;
}

void GNet::EventHandler::writeEvent( Descriptor )
{
	G_DEBUG( "GNet::EventHandler::writeEvent: no override" ) ;
}

void GNet::EventHandler::otherEvent( Descriptor , EventHandler::Reason reason )
{
	// this event is mostly relevant to windows -- the default action here
	// is to throw an exception, but overrides can check for Reason::closed
	// (a clean shutdown()) and read residual data out of the socket buffers
	// before either throwing an exception or setting a zero-length timer
	// to close the socket -- (note that the Client::otherEvent() and
	// ServerPeer::otherEvent() overrides call SocketProtocol::otherEvent()
	// to read residual data and then throw)
	//
	throw G::Exception( "socket disconnect event" , str(reason) ) ;
}

std::string GNet::EventHandler::str( EventHandler::Reason reason )
{
	if( reason == EventHandler::Reason::closed ) return "closed" ;
	if( reason == EventHandler::Reason::down ) return "network down" ;
	if( reason == EventHandler::Reason::reset ) return "connection reset by peer" ;
	if( reason == EventHandler::Reason::abort ) return "connection aborted" ;
	return {} ;
}

