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
// gadminserver.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "geventloop.h"
#include "gsmtp.h"
#include "gadminserver.h"
#include "gmessagestore.h"
#include "gstoredmessage.h"
#include "gmonitor.h"
#include "gslot.h"
#include "gstr.h"
#include "gmemory.h"
#include <algorithm> // std::find()

GSmtp::AdminPeer::AdminPeer( GNet::Server::PeerInfo peer_info , AdminServer & server , 
	const std::string & server_address , bool with_terminate ) :
		GNet::ServerPeer(peer_info) ,
		m_buffer(crlf()) ,
		m_server(server) ,
		m_server_address(server_address) ,
		m_notifying(false) ,
		m_with_terminate(with_terminate)
{
	G_LOG_S( "GSmtp::AdminPeer: admin connection from " << peer_info.m_address.displayString() ) ;
	// dont prompt() here -- it confuses the poke program
}

GSmtp::AdminPeer::~AdminPeer()
{
	// only safe because AdminServer::dtor calls serverCleanup() -- otherwise
	// the derived part of the server may already be destroyed
	m_server.unregister( this ) ;
        if( m_client.get() ) m_client->doneSignal().disconnect() ;
}

void GSmtp::AdminPeer::clientDone( std::string s )
{
	if( s.empty() )
		send( "OK" ) ;
	else
		send( std::string("error: ") + s ) ;

	prompt() ;
}

void GSmtp::AdminPeer::onDelete()
{
	G_LOG_S( "GSmtp::AdminPeer: admin connection closed: " << peerAddress().second.displayString() ) ;
}

void GSmtp::AdminPeer::onData( const char * data , size_t n )
{
	m_buffer.add( std::string(data,n) ) ;
	while( m_buffer.more() )
	{
		if( ! processLine( m_buffer.line() ) )
			return ;
	}
}

bool GSmtp::AdminPeer::processLine( const std::string & line )
{
	if( is(line,"FLUSH") )
	{
		flush( m_server_address ) ;
	}
	else if( is(line,"HELP") )
	{
		help() ;
		prompt() ;
	}
	else if( is(line,"INFO") )
	{
		info() ;
		prompt() ;
	}
	else if( is(line,"NOTIFY") )
	{
		m_notifying = true ;
		prompt() ;
	}
	else if( is(line,"LIST") )
	{
		list() ;
		prompt() ;
	}
	else if( is(line,"QUIT") )
	{
		doDelete() ;
		return false ;
	}
	else if( is(line,"TERMINATE") && m_with_terminate )
	{
		if( GNet::EventLoop::exists() )
			GNet::EventLoop::instance().quit() ;
	}
	else if( line.find_first_not_of(" \r\n\t") != std::string::npos )
	{
		send( "error: unrecognised command" ) ;
		prompt() ;
	}
	else
	{
		prompt() ;
	}
	return true ;
}

//static
std::string GSmtp::AdminPeer::crlf()
{
	return "\015\012" ;
}

//static
bool GSmtp::AdminPeer::is( const std::string & line_in , const char * key )
{
	std::string line( line_in ) ;
	G::Str::trim( line , " \t" ) ;
	G::Str::toUpper( line ) ;
	return line.find(key) == 0U ;
}

void GSmtp::AdminPeer::help()
{
	send( "commands: flush, help, info, list, notify, quit" ) ;
}

void GSmtp::AdminPeer::flush( const std::string & address )
{
	G_DEBUG( "GSmtp::AdminPeer: flush: \"" << address << "\"" ) ;

	if( m_client.get() != NULL && m_client->busy() )
	{
		send( "error: still working" ) ;
	}
	else if( address.empty() )
	{
		send( "error: no remote server configured: use --forward-to" ) ;
	}
	else
	{
		const bool quit_on_disconnect = false ;
		m_client <<= new Client( m_server.store() , m_server.secrets() ,
			quit_on_disconnect , m_server.responseTimeout() ) ;
		m_client->doneSignal().connect( G::slot(*this,&AdminPeer::clientDone) ) ;
		std::string rc = m_client->startSending( address , m_server.connectionTimeout() ) ;
		if( rc.length() != 0U )
		{
			send( std::string("error: ") + rc ) ;
		}
	}
}

void GSmtp::AdminPeer::prompt()
{
	std::string p( "E-MailRelay> " ) ;
	ssize_t rc = socket().write( p.data() , p.length() ) ;
	if( rc < 0 || static_cast<size_t>(rc) < p.length() )
		doDelete() ; // onDelete() and "delete this"
}

void GSmtp::AdminPeer::send( std::string line )
{
	line.append( crlf() ) ;
	ssize_t rc = socket().write( line.data() , line.length() ) ;
	if( rc < 0 || static_cast<size_t>(rc) < line.length() )
		doDelete() ; // onDelete() and "delete this"
}

void GSmtp::AdminPeer::notify( const std::string & s0 , const std::string & s1 , const std::string & s2 )
{
	if( m_notifying )
		send( crlf() + "EVENT: " + s0 + ": " + s1 + ": " + s2 ) ;
}

void GSmtp::AdminPeer::info()
{
	std::ostringstream ss ;
	if( GNet::Monitor::instance() )
	{
		GNet::Monitor::instance()->report( ss , "" , crlf() ) ;
		send( ss.str() ) ;
	}
	else
	{
		send( "no info" ) ;
	}
}

void GSmtp::AdminPeer::list()
{
	std::ostringstream ss ;
	MessageStore::Iterator iter( m_server.store().iterator(false) ) ;
	for(;;)
	{
		std::auto_ptr<StoredMessage> message( iter.next() ) ;
		if( message.get() == NULL ) break ;
		ss << message->name() << crlf() ;
	}

	std::string result = ss.str() ;
	if( result.size() == 0U )
		send( "<none>" ) ;
	else
		send( ss.str() ) ;
}

// ===

GSmtp::AdminServer::AdminServer( MessageStore & store , const Secrets & secrets , 
	const GNet::Address & listening_address , bool allow_remote , 
	const std::string & address , unsigned int response_timeout ,
	unsigned int connection_timeout , bool with_terminate ) :
		GNet::Server( listening_address ) ,
		m_store( store ) ,
		m_secrets( secrets ) ,
		m_allow_remote( allow_remote ) ,
		m_server_address( address ) ,
		m_response_timeout( response_timeout ) ,
		m_connection_timeout( connection_timeout ) ,
		m_with_terminate( with_terminate )
{
	G_DEBUG( "GSmtp::AdminServer: administrative interface listening on " << listening_address.displayString() ) ;
}

GSmtp::AdminServer::~AdminServer()
{
	// early cleanup so peers can call unregister() safely
	serverCleanup() ; // GNet::Server
}

GNet::ServerPeer * GSmtp::AdminServer::newPeer( GNet::Server::PeerInfo peer_info )
{
	AdminPeer * peer = new AdminPeer( peer_info , *this , m_server_address , m_with_terminate ) ;
	m_peers.push_back( peer ) ;
	return peer ;
}

void GSmtp::AdminServer::report() const
{
	// no-op
}

void GSmtp::AdminServer::notify( const std::string & s0 , const std::string & s1 , const std::string & s2 )
{
	for( PeerList::iterator p = m_peers.begin() ; p != m_peers.end() ; ++p )
	{
		G_DEBUG( "GSmtp::AdminServer::notify: " << (*p) << ": " << s0 << ": " << s1 ) ;
		(*p)->notify( s0 , s1 , s2 ) ;
	}
}

void GSmtp::AdminServer::unregister( AdminPeer * peer )
{
	G_DEBUG( "GSmtp::AdminServer::unregister: server=" << this << ": peer=" << peer ) ;
	PeerList::iterator p = std::find( m_peers.begin() , m_peers.end() , peer ) ;
	if( p != m_peers.end() )
		m_peers.erase( p ) ;
}

GSmtp::MessageStore & GSmtp::AdminServer::store()
{
	return m_store ;
}

const GSmtp::Secrets & GSmtp::AdminServer::secrets() const
{
	return m_secrets ;
}

unsigned int GSmtp::AdminServer::responseTimeout() const
{
	return m_response_timeout ;
}

unsigned int GSmtp::AdminServer::connectionTimeout() const
{
	return m_connection_timeout ;
}

