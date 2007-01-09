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
	const GNet::Address & local_address , const std::string & remote_address , 
	const G::StringMap & extra_commands , bool with_terminate ) :
		GNet::ServerPeer(peer_info) ,
		m_buffer(std::string(1U,'\n')) ,
		m_server(server) ,
		m_local_address(local_address) ,
		m_remote_address(remote_address) ,
		m_notifying(false) ,
		m_extra_commands(extra_commands) ,
		m_with_terminate(with_terminate)
{
	G_LOG_S( "GSmtp::AdminPeer: admin connection from " << peer_info.m_address.displayString() ) ;
	// dont prompt() here -- it confuses the poke program
}

GSmtp::AdminPeer::~AdminPeer()
{
	// AdminPeer objects are destroyed from within the AdminServer::dtor body
	// via the GNet::Server::serverCleanup() mechanism -- this allows this
	// AdminPeer dtor to call the AdminServer safely

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
	if( is(line,"flush") )
	{
		flush() ;
	}
	else if( is(line,"help") )
	{
		help() ;
		prompt() ;
	}
	else if( is(line,"info") )
	{
		info() ;
		prompt() ;
	}
	else if( is(line,"notify") )
	{
		m_notifying = true ;
		prompt() ;
	}
	else if( is(line,"list") )
	{
		list() ;
		prompt() ;
	}
	else if( is(line,"quit") )
	{
		doDelete() ;
		return false ;
	}
	else if( is(line,"terminate") && m_with_terminate )
	{
		if( GNet::EventLoop::exists() )
			GNet::EventLoop::instance().quit() ;
	}
	else if( find(line,m_extra_commands).first )
	{
		send( find(line,m_extra_commands).second ) ;
		prompt() ;
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
bool GSmtp::AdminPeer::is( const std::string & line_in , const std::string & key )
{
	std::string line( line_in ) ;
	G::Str::trim( line , " \t" ) ;
	G::Str::toLower( line ) ;
	return line.find(key) == 0U ;
}

//static
std::pair<bool,std::string> GSmtp::AdminPeer::find( const std::string & line , const G::StringMap & map )
{
	for( G::StringMap::const_iterator p = map.begin() ; p != map.end() ; ++p )
	{
		if( is(line,(*p).first) )
			return std::make_pair(true,(*p).second) ;
	}
	return std::make_pair(false,std::string()) ;
}

void GSmtp::AdminPeer::help()
{
	G::Strings commands ;
	commands.push_back( "flush" ) ;
	commands.push_back( "help" ) ;
	commands.push_back( "info" ) ;
	commands.push_back( "list" ) ;
	commands.push_back( "notify" ) ;
	commands.push_back( "quit" ) ;
	if( m_with_terminate ) commands.push_back( "terminate" ) ;
	G::Strings extras = G::Str::keys( m_extra_commands ) ;
	commands.splice( commands.end() , extras ) ;
	commands.sort() ;
	send( std::string("commands: ") + G::Str::join(commands,", ") ) ;
}

void GSmtp::AdminPeer::flush()
{
	G_DEBUG( "GSmtp::AdminPeer: flush: \"" << m_remote_address << "\"" ) ;

	if( m_client.get() != NULL && m_client->busy() )
	{
		send( "error: still working" ) ;
	}
	else if( m_remote_address.empty() )
	{
		send( "error: no remote server configured: use --forward-to" ) ;
	}
	else
	{
		const bool quit_on_disconnect = false ;
		m_client <<= new Client( m_server.store() , m_server.secrets() , 
			m_server.clientConfig() , quit_on_disconnect ) ;
		m_client->doneSignal().connect( G::slot(*this,&AdminPeer::clientDone) ) ;
		std::string rc = m_client->startSending( m_remote_address , m_server.connectionTimeout() ) ;
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
	line.append( "\n" ) ;
	G::Str::replaceAll( line , "\n" , crlf() ) ;
	ssize_t rc = socket().write( line.data() , line.length() ) ;
	if( rc < 0 || static_cast<size_t>(rc) < line.length() )
		doDelete() ; // onDelete() and "delete this"
}

void GSmtp::AdminPeer::notify( const std::string & s0 , const std::string & s1 , const std::string & s2 )
{
	if( m_notifying )
		send( std::string(1U,'\n') + "EVENT: " + s0 + ": " + s1 + ": " + s2 ) ;
}

void GSmtp::AdminPeer::info()
{
	std::ostringstream ss ;
	if( GNet::Monitor::instance() )
	{
		GNet::Monitor::instance()->report( ss , "" , std::string(1U,'\n') ) ;
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
		ss << message->name() << "\n" ;
	}

	std::string result = ss.str() ;
	if( result.size() == 0U )
		send( "<none>" ) ;
	else
		send( ss.str() ) ;
}

// ===

GSmtp::AdminServer::AdminServer( MessageStore & store , const GSmtp::Client::Config & client_config ,
	const Secrets & secrets , const GNet::Address & listening_address , bool allow_remote , 
	const GNet::Address & local_address , const std::string & remote_address , 
	unsigned int connection_timeout , const G::StringMap & extra_commands , bool with_terminate ) :
		GNet::MultiServer( GNet::MultiServer::addressList(listening_address) ) ,
		m_store(store) ,
		m_client_config(client_config) ,
		m_secrets(secrets) ,
		m_local_address(local_address) ,
		m_allow_remote(allow_remote) ,
		m_remote_address(remote_address) ,
		m_connection_timeout(connection_timeout) ,
		m_extra_commands(extra_commands) ,
		m_with_terminate(with_terminate)
{
	G_DEBUG( "GSmtp::AdminServer: administrative interface listening on " << listening_address.displayString() ) ;
}

GSmtp::AdminServer::~AdminServer()
{
	// early cleanup so peers can call unregister() safely
	serverCleanup() ; // GNet::MultiServer
}

GNet::ServerPeer * GSmtp::AdminServer::newPeer( GNet::Server::PeerInfo peer_info )
{
	try
	{
		AdminPeer * peer = new AdminPeer( peer_info , *this , m_local_address , m_remote_address , 
			m_extra_commands , m_with_terminate ) ;
		m_peers.push_back( peer ) ;
		return peer ;
	}
	catch( std::exception & e )
	{
		G_WARNING( "GSmtp::AdminServer: exception from new connection: " << e.what() ) ;
		return NULL ;
	}
}

void GSmtp::AdminServer::report() const
{
	serverReport( "admin" ) ;
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

unsigned int GSmtp::AdminServer::connectionTimeout() const
{
	return m_connection_timeout ;
}

GSmtp::Client::Config GSmtp::AdminServer::clientConfig() const
{
	return m_client_config ;
}

