//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
//
// gadminserver.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gprocess.h"
#include "geventloop.h"
#include "gsmtp.h"
#include "gadminserver.h"
#include "gmessagestore.h"
#include "gstoredmessage.h"
#include "gnullprocessor.h"
#include "gexecutableprocessor.h"
#include "glocal.h"
#include "gmonitor.h"
#include "gslot.h"
#include "gstr.h"
#include "gmemory.h"
#include <algorithm> // std::find()

GSmtp::AdminServerPeer::AdminServerPeer( GNet::Server::PeerInfo peer_info , AdminServer & server , 
	const GNet::Address & local_address , const std::string & remote_address , 
	const G::StringMap & extra_commands , bool with_terminate ) :
		BufferedServerPeer(peer_info,crlf()) ,
		m_server(server) ,
		m_local_address(local_address) ,
		m_remote_address(remote_address) ,
		m_notifying(false) ,
		m_extra_commands(extra_commands) ,
		m_with_terminate(with_terminate)
{
	G_LOG_S( "GSmtp::AdminServerPeer: admin connection from " << peer_info.m_address.displayString() ) ;
	m_client.doneSignal().connect( G::slot(*this,&AdminServerPeer::clientDone) ) ;
	// dont prompt() here -- it confuses the poke program
}

GSmtp::AdminServerPeer::~AdminServerPeer()
{
	// AdminServerPeer objects are destroyed from within the AdminServer::dtor body
	// via the GNet::Server::serverCleanup() mechanism -- this allows this
	// AdminServerPeer dtor to call the AdminServer safely

	m_server.unregister( this ) ;
	m_client.doneSignal().disconnect() ;
}

void GSmtp::AdminServerPeer::clientDone( std::string s , bool )
{
	m_client.reset() ;
	if( s.empty() )
		sendLine( "OK" ) ;
	else
		sendLine( std::string("error: ") + s ) ;
	prompt() ;
}

void GSmtp::AdminServerPeer::onSendComplete()
{
	// never gets here
}

void GSmtp::AdminServerPeer::onDelete( const std::string & reason )
{
	G_LOG_S( "GSmtp::AdminServerPeer: admin connection closed: " << reason << (reason.empty()?"":": ")
		<< peerAddress().second.displayString() ) ;
}

void GSmtp::AdminServerPeer::onSecure( const std::string & )
{
}

bool GSmtp::AdminServerPeer::onReceive( const std::string & line )
{
	if( is(line,"flush") )
	{
		if( flush() )
			prompt() ;
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
		list(spooled()) ;
		prompt() ;
	}
	else if( is(line,"failures") )
	{
		list(failures()) ;
		prompt() ;
	}
	else if( is(line,"unfail-all") )
	{
		unfailAll() ;
		prompt() ;
	}
	else if( is(line,"pid") )
	{
		pid() ;
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
			GNet::EventLoop::instance().quit("admin terminate request") ;
	}
	else if( find(line,m_extra_commands).first )
	{
		sendLine( find(line,m_extra_commands).second ) ;
		prompt() ;
	}
	else if( line.find_first_not_of(" \r\n\t") != std::string::npos )
	{
		sendLine( "error: unrecognised command" ) ;
		prompt() ;
	}
	else
	{
		prompt() ;
	}
	return true ;
}

const std::string & GSmtp::AdminServerPeer::crlf()
{
	static const std::string s( "\015\012" ) ;
	return s ;
}

bool GSmtp::AdminServerPeer::is( const std::string & line_in , const std::string & key )
{
	std::string line( line_in ) ;
	G::Str::trim( line , " \t" ) ;
	G::Str::toLower( line ) ;
	return line.find(key) == 0U ;
}

std::pair<bool,std::string> GSmtp::AdminServerPeer::find( const std::string & line , const G::StringMap & map )
{
	for( G::StringMap::const_iterator p = map.begin() ; p != map.end() ; ++p )
	{
		if( is(line,(*p).first) )
			return std::make_pair(true,(*p).second) ;
	}
	return std::make_pair(false,std::string()) ;
}

void GSmtp::AdminServerPeer::help()
{
	G::Strings commands ;
	commands.push_back( "flush" ) ;
	commands.push_back( "help" ) ;
	commands.push_back( "info" ) ;
	commands.push_back( "list" ) ;
	commands.push_back( "failures" ) ;
	commands.push_back( "unfail-all" ) ;
	commands.push_back( "notify" ) ;
	commands.push_back( "pid" ) ;
	commands.push_back( "quit" ) ;
	if( m_with_terminate ) commands.push_back( "terminate" ) ;
	G::Strings extras = G::Str::keys( m_extra_commands ) ;
	commands.splice( commands.end() , extras ) ;
	commands.sort() ;
	sendLine( std::string("commands: ") + G::Str::join(commands,", ") ) ;
}

bool GSmtp::AdminServerPeer::flush()
{
	G_DEBUG( "GSmtp::AdminServerPeer: flush: \"" << m_remote_address << "\"" ) ;

	bool do_prompt = false ;
	if( m_client.busy() )
	{
		sendLine( "error: still working" ) ;
	}
	else if( m_remote_address.empty() )
	{
		sendLine( "error: no remote server configured: use --forward-to" ) ;
		do_prompt = true ;
	}
	else if( m_server.store().empty() )
	{
		sendLine( "error: no messages to send" ) ;
		do_prompt = true ;
	}
	else
	{
		m_client.reset( new Client(GNet::ResolverInfo(m_remote_address),m_server.secrets(),m_server.clientConfig()) ) ;
		m_client->sendMessagesFrom( m_server.store() ) ; // once connected
	}
	return do_prompt ;
}

void GSmtp::AdminServerPeer::prompt()
{
	send( "E-MailRelay> " ) ;
}

void GSmtp::AdminServerPeer::sendLine( std::string line )
{
	line.append( "\n" ) ;
	G::Str::replaceAll( line , "\n" , crlf() ) ;
	send( line ) ;
}

void GSmtp::AdminServerPeer::notify( const std::string & s0 , const std::string & s1 , const std::string & s2 )
{
	if( m_notifying )
		sendLine( std::string(1U,'\n') + "EVENT: " + s0 + ": " + s1 + ": " + s2 ) ;
}

void GSmtp::AdminServerPeer::info()
{
	std::ostringstream ss ;
	if( GNet::Monitor::instance() )
	{
		GNet::Monitor::instance()->report( ss , "" , std::string(1U,'\n') ) ;
		send( ss.str() ) ;
	}
	else
	{
		sendLine( "no info" ) ;
	}
}

void GSmtp::AdminServerPeer::pid()
{
	sendLine( G::Process::Id().str() ) ;
}

GSmtp::MessageStore::Iterator GSmtp::AdminServerPeer::spooled()
{
	return m_server.store().iterator(false) ;
}

GSmtp::MessageStore::Iterator GSmtp::AdminServerPeer::failures()
{
	return m_server.store().failures() ;
}

void GSmtp::AdminServerPeer::list( MessageStore::Iterator iter )
{
	std::ostringstream ss ;
	for(;;)
	{
		std::auto_ptr<StoredMessage> message( iter.next() ) ;
		if( message.get() == NULL ) break ;
		ss << message->name() << "\n" ;
	}

	std::string result = ss.str() ;
	if( result.size() == 0U )
		sendLine( "<none>" ) ;
	else
		send( ss.str() ) ;
}

void GSmtp::AdminServerPeer::unfailAll()
{
	return m_server.store().unfailAll() ;
}

// ===

GSmtp::AdminServer::AdminServer( MessageStore & store , const GSmtp::Client::Config & client_config ,
	const GAuth::Secrets & secrets , const GNet::MultiServer::AddressList & listening_addresses , 
	bool allow_remote , const GNet::Address & local_address , const std::string & remote_address , 
	unsigned int connection_timeout , const G::StringMap & extra_commands , bool with_terminate ) :
		GNet::MultiServer( listening_addresses , false ) ,
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
		std::string reason ;
		if( ! m_allow_remote && ! GNet::Local::isLocal(peer_info.m_address,reason) )
		{
			G_WARNING( "GSmtp::Server: configured to reject non-local admin connection: " << reason ) ;
			return NULL ;
		}

		AdminServerPeer * peer = new AdminServerPeer( peer_info , *this , m_local_address , m_remote_address , 
			m_extra_commands , m_with_terminate ) ;
		m_peers.push_back( peer ) ;
		return peer ;
	}
	catch( std::exception & e ) // newPeer()
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

void GSmtp::AdminServer::unregister( AdminServerPeer * peer )
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

const GAuth::Secrets & GSmtp::AdminServer::secrets() const
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

/// \file gadminserver.cpp
