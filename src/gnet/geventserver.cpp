//
// Copyright (C) 2001-2004 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// geventserver.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "geventserver.h"
#include "gevent.h"
#include "gexception.h"
#include "glog.h"

// Class: GNet::EventServerImp
// Description: A pimple-pattern implementation class for GNet::EventServer.
//
class GNet::EventServerImp 
{
public:
	std::auto_ptr<EventLoop> m_event_loop ;

public:
	EventServerImp() ;
	void run() ;
} ;

// ===

GNet::EventServer::EventServer( unsigned int listening_port ) :
		m_imp(NULL)
{
	m_imp = new EventServerImp ;
	init( listening_port ) ; // Server::init()
}

GNet::EventServer::~EventServer()
{
	delete m_imp ;
}

void GNet::EventServer::run()
{
	m_imp->run() ;
}

// ===

GNet::EventServerImp::EventServerImp() :
	m_event_loop(EventLoop::create())
{
}

void GNet::EventServerImp::run()
{
	m_event_loop->run() ;
}

