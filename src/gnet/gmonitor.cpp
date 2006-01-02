//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gmonitor.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gmonitor.h"
#include "gassert.h"
#include <set>

// Class: GNet::MontiorImp
// Description: A pimple pattern implementation class for GNet::Monitor.
//
class GNet::MonitorImp 
{
public:
	typedef const Client * C_p ;
	typedef const ServerPeer * S_p ;
	typedef std::set<C_p> Clients ;
	typedef std::pair<Clients::iterator,bool> ClientInsertion ;
	typedef std::set<S_p> ServerPeers ;
	typedef std::pair<ServerPeers::iterator,bool> ServerPeerInsertion ;

	explicit MonitorImp( Monitor & monitor ) ;

	Monitor & m_monitor ;
	Clients m_clients ;
	ServerPeers m_server_peers ;
	unsigned long m_client_adds ;
	unsigned long m_client_removes ;
	unsigned long m_server_peer_adds ;
	unsigned long m_server_peer_removes ;
} ;

GNet::MonitorImp::MonitorImp( Monitor & monitor ) :
	m_monitor(monitor) ,
	m_client_adds(0UL) ,
	m_client_removes(0UL) ,
	m_server_peer_adds(0UL) ,
	m_server_peer_removes(0UL)
{
}

// ===

GNet::Monitor * GNet::Monitor::m_this = NULL ;

GNet::Monitor::Monitor() :
	m_imp( new MonitorImp(*this) )
{
	G_ASSERT( m_this == NULL ) ;
	m_this = this ;
}

GNet::Monitor::~Monitor()
{
	delete m_imp ;
	m_this = NULL ;
}

GNet::Monitor * GNet::Monitor::instance()
{
	return m_this ;
}

void GNet::Monitor::add( const Client & client )
{
	MonitorImp::ClientInsertion rc = m_imp->m_clients.insert( &client ) ;
	if( rc.second )
		m_imp->m_client_adds++ ;
	m_signal.emit( "out" , "start" ) ;
}

void GNet::Monitor::remove( const Client & client )
{
	if( m_imp->m_clients.erase( &client ) )
		m_imp->m_client_removes++ ;
	m_signal.emit( "out" , "end" ) ;
}

void GNet::Monitor::add( const ServerPeer & peer )
{
	MonitorImp::ServerPeerInsertion rc = m_imp->m_server_peers.insert( & peer ) ;
	if( rc.second )
		m_imp->m_server_peer_adds++ ;
	m_signal.emit( "in" , "start" ) ;
}

void GNet::Monitor::remove( const ServerPeer & peer )
{
	if( m_imp->m_server_peers.erase( & peer ) )
		m_imp->m_server_peer_removes++ ;
	m_signal.emit( "in" , "end" ) ;
}

void GNet::Monitor::report( std::ostream & s , const std::string & px , const std::string & eol )
{
	s << px << "OUT started: " << m_imp->m_client_adds << eol ;
	s << px << "OUT finished: " << m_imp->m_client_removes << eol ;
	{
		for( MonitorImp::Clients::const_iterator p = m_imp->m_clients.begin() ;
			p != m_imp->m_clients.end() ; ++p )
		{
			s << px
				<< "OUT: "
				<< (*p)->localAddress().second.displayString() << " -> " 
				<< (*p)->peerAddress().second.displayString() << eol ;
		}
	}

	s << px << "IN started: " << m_imp->m_server_peer_adds << eol ;
	s << px << "IN finished: " << m_imp->m_server_peer_removes << eol ;
	{
		for( MonitorImp::ServerPeers::const_iterator p = m_imp->m_server_peers.begin() ; 
			p != m_imp->m_server_peers.end() ; ++p )
		{
			s << px
				<< "IN: "
				<< (*p)->localAddress().second.displayString() << " <- " 
				<< (*p)->peerAddress().second.displayString() << eol ;
		}
	}
}

G::Signal2<std::string,std::string> & GNet::Monitor::signal()
{
	return m_signal ;
}

