//
// Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// geventserver.h
//

#ifndef G_EVENT_SERVER_H
#define G_EVENT_SERVER_H

#include "gdef.h"
#include "gnet.h"
#include "gserver.h"

namespace GNet
{
	class EventServer ;
	class EventServerImp ;
}

// Class: GNet::EventServer
// Description: A simple derivation from Server which
// simply adds an event loop. Only one instance
// may be created.
// See also: GNet::EventLoop
//
class GNet::EventServer : public GNet::Server 
{
public:
	explicit EventServer( unsigned int listening_port ) ;
		// Constructor. Throws exceptions on
		// error.

	virtual ~EventServer() ;
		// Destructor.

	void run() ;
		// Runs the event loop.

private:
	EventServer( const EventServer & ) ;
	void operator=( const EventServer & ) ;

private:
	EventServerImp * m_imp ;
} ;

#endif

