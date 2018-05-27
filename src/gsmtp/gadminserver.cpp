//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "geventloop.h"
#include "gsmtp.h"
#include "gadminserver.h"
#include "gmessagestore.h"
#include "gstoredmessage.h"
#include "gprocess.h"
#include "glocal.h"
#include "gmonitor.h"
#include "gslot.h"
#include "gstr.h"
#include <algorithm> // std::find()

GSmtp::AdminServerPeer::AdminServerPeer( GNet::Server::PeerInfo peer_info ,
	AdminServer & server , const std::string & remote_address ,
	const G::StringMap & info_commands , const G::StringMap & config_commands ,
	bool with_terminate ) :
		GNet::BufferedServerPeer(peer_info,GNet::LineBufferConfig::autodetect()) ,
		m_server(server) ,
		m_prompt("E-MailRelay> ") ,
		m_blocked(false) ,
		m_remote_address(remote_address) ,
		m_notifying(false) ,
		m_info_commands(info_commands) ,
		m_config_commands(config_commands) ,
		m_with_terminate(with_terminate)
{
	G_LOG_S( "GSmtp::AdminServerPeer: admin connection from " << peer_info.m_address.displayString() ) ;
	m_client.doneSignal().connect( G::Slot::slot(*this,&AdminServerPeer::clientDone) ) ;
	// dont prompt here -- it confuses some clients
}

GSmtp::AdminServerPeer::~AdminServerPeer()
{
	// AdminServerPeer objects are destroyed from within the AdminServer::dtor body
	// via the GNet::Server::serverCleanup() mechanism -- this allows this
	// AdminServerPeer dtor to call the AdminServer safely

	m_server.unregister( this ) ;
	m_client.doneSignal().disconnect() ;
}

void GSmtp::AdminServerPeer::clientDone( std::string s )
{
	m_client.reset() ;
	if( s.empty() )
		sendLine( "OK" ) ;
	else
		sendLine( std::string("error: ") + s ) ;
}

void GSmtp::AdminServerPeer::onDelete( const std::string & reason )
{
	G_LOG_S( "GSmtp::AdminServerPeer: admin connection closed: " << reason << (reason.empty()?"":": ")
		<< peerAddress().second.displayString() ) ;
}

void GSmtp::AdminServerPeer::onSecure( const std::string & )
{
}

bool GSmtp::AdminServerPeer::onReceive( const char * line_data , size_t line_size , size_t )
{
	std::string line( line_data , line_size ) ;
	if( is(line,"flush") )
	{
		flush() ;
	}
	else if( is(line,"help") )
	{
		help() ;
	}
	else if( is(line,"status") )
	{
		status() ;
	}
	else if( is(line,"notify") )
	{
		m_notifying = true ;
	}
	else if( is(line,"list") )
	{
		sendList( spooled() ) ;
	}
	else if( is(line,"failures") )
	{
		sendList( failures() ) ;
	}
	else if( is(line,"unfail-all") )
	{
		unfailAll() ;
		sendLine( "" ) ;
	}
	else if( is(line,"pid") )
	{
		sendLine( G::Process::Id().str() ) ;
	}
	else if( is(line,"quit") )
	{
		doDelete() ;
		return false ;
	}
	else if( is(line,"terminate") && m_with_terminate )
	{
		if( GNet::EventLoop::exists() )
			GNet::EventLoop::instance().quit("") ;
	}
	else if( is(line,"info") && !m_info_commands.empty() )
	{
		std::string arg = argument( line ) ;
		if( arg.empty() || !find(arg,m_info_commands).first )
			sendLine( "usage: info {" + G::Str::join("|",G::Str::keySet(m_info_commands)) + "}" ) ;
		else
			sendLine( find(arg,m_info_commands).second ) ;
	}
	else if( is(line,"config") && !m_config_commands.empty() )
	{
		std::string arg = argument( line ) ;
		if( arg.empty() )
			sendLine( G::Str::join( eol() , m_config_commands , "=[" , "]" ) ) ;
		else if( !find(arg,m_config_commands).first )
			sendLine( "usage: config [{" + G::Str::join("|",G::Str::keySet(m_config_commands)) + "}]" ) ;
		else
			sendLine( find(arg,m_config_commands).second ) ;
	}
	else if( line.find_first_not_of(" \r\n\t") != std::string::npos )
	{
		sendLine( "error: unrecognised command" ) ;
	}
	else
	{
		sendLine( "" ) ;
	}
	return true ;
}

std::string GSmtp::AdminServerPeer::eol() const
{
	return lineBufferEndOfLine().empty() ? std::string("\r\n") : lineBufferEndOfLine() ;
}

bool GSmtp::AdminServerPeer::is( const std::string & line_in , const std::string & key )
{
	G::StringArray parts ;
	G::Str::splitIntoTokens( line_in , parts , G::Str::ws() ) ;
	return parts.size() && G::Str::imatch( parts.at(0) , key ) ;
}

std::string GSmtp::AdminServerPeer::argument( const std::string & line_in )
{
	G::StringArray parts ;
	G::Str::splitIntoTokens( line_in , parts , G::Str::ws() ) ;
	return parts.size() > 1U ? parts.at(1U) : std::string() ;
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
	std::set<std::string> commands ;
	commands.insert( "flush" ) ;
	commands.insert( "help" ) ;
	commands.insert( "status" ) ;
	commands.insert( "list" ) ;
	commands.insert( "failures" ) ;
	commands.insert( "unfail-all" ) ;
	commands.insert( "notify" ) ;
	commands.insert( "pid" ) ;
	commands.insert( "quit" ) ;
	if( !m_info_commands.empty() ) commands.insert( "info" ) ;
	if( !m_config_commands.empty() ) commands.insert( "config" ) ;
	if( m_with_terminate ) commands.insert( "terminate" ) ;
	sendLine( std::string("commands: ") + G::Str::join(", ",commands) ) ;
}

void GSmtp::AdminServerPeer::flush()
{
	G_DEBUG( "GSmtp::AdminServerPeer: flush: \"" << m_remote_address << "\"" ) ;
	if( m_client.busy() )
	{
		sendLine( "error: still working" ) ;
	}
	else if( m_remote_address.empty() )
	{
		sendLine( "error: no remote server configured: use --forward-to" ) ;
	}
	else if( m_server.store().empty() )
	{
		sendLine( "error: no messages to send" ) ;
	}
	else
	{
		m_client.reset( new Client(GNet::Location(m_remote_address),m_server.secrets(),m_server.clientConfig()) ) ;
		m_client->sendMessagesFrom( m_server.store() ) ; // once connected
		// no sendLine() -- sends "OK" or "error:..." when complete
	}
}

void GSmtp::AdminServerPeer::sendLine( std::string line , bool with_prompt )
{
	if( !line.empty() )
		line.append( "\n" ) ;
	G::Str::replaceAll( line , "\n" , eol() ) ;
	if( with_prompt )
		line.append( m_prompt ) ;
	send_( line ) ;
}

void GSmtp::AdminServerPeer::notify( const std::string & s0 , const std::string & s1 , const std::string & s2 )
{
	if( m_notifying )
		send_( eol() + "EVENT: " + G::Str::printable(G::Str::join(": ",s0,s1,s2)) ) ;
}

void GSmtp::AdminServerPeer::send_( const std::string & s )
{
	if( m_blocked )
	{
		G_DEBUG( "GSmtp::AdminServerPeer::send: flow control asserted: cannot send" ) ;
		// could do better
	}
	else
	{
		m_blocked = ! send( s ) ; // GNet::SocketProtocol
	}
}

void GSmtp::AdminServerPeer::onSendComplete()
{
	m_blocked = false ;
}

void GSmtp::AdminServerPeer::status()
{
	std::ostringstream ss ;
	if( GNet::Monitor::instance() )
	{
		GNet::Monitor::instance()->report( ss , "" , eol() ) ;
		std::string report = ss.str() ;
		G::Str::trimRight( report , eol() ) ;
		sendLine( report ) ;
	}
	else
	{
		sendLine( "no info" ) ;
	}
}

GSmtp::MessageStore::Iterator GSmtp::AdminServerPeer::spooled()
{
	return m_server.store().iterator(false) ;
}

GSmtp::MessageStore::Iterator GSmtp::AdminServerPeer::failures()
{
	return m_server.store().failures() ;
}

void GSmtp::AdminServerPeer::sendList( MessageStore::Iterator iter )
{
	std::ostringstream ss ;
	for( bool first = true ;; first = false )
	{
		unique_ptr<StoredMessage> message( iter.next() ) ;
		if( message.get() == nullptr ) break ;
		if( !first ) ss << eol() ;
		ss << message->name() ;
	}

	std::string result = ss.str() ;
	if( result.size() == 0U )
		sendLine( "<none>" ) ;
	else
		sendLine( ss.str() ) ;
}

void GSmtp::AdminServerPeer::unfailAll()
{
	return m_server.store().unfailAll() ;
}

// ===

GSmtp::AdminServer::AdminServer( GNet::ExceptionHandler & eh , MessageStore & store ,
	const GSmtp::Client::Config & client_config , const GAuth::Secrets & secrets ,
	const GNet::MultiServer::AddressList & listening_addresses , bool allow_remote ,
	const std::string & remote_address , unsigned int connection_timeout ,
	const G::StringMap & info_commands , const G::StringMap & config_commands ,
	bool with_terminate ) :
		GNet::MultiServer(eh,listening_addresses) ,
		m_store(store) ,
		m_client_config(client_config) ,
		m_secrets(secrets) ,
		m_allow_remote(allow_remote) ,
		m_remote_address(remote_address) ,
		m_connection_timeout(connection_timeout) ,
		m_info_commands(info_commands) ,
		m_config_commands(config_commands) ,
		m_with_terminate(with_terminate)
{
}

GSmtp::AdminServer::~AdminServer()
{
	// early cleanup so peers can call unregister() safely
	serverCleanup() ; // GNet::MultiServer
}

GNet::ServerPeer * GSmtp::AdminServer::newPeer( GNet::Server::PeerInfo peer_info , GNet::MultiServer::ServerInfo )
{
	try
	{
		std::string reason ;
		if( ! m_allow_remote && ! GNet::Local::isLocal(peer_info.m_address,reason) )
		{
			G_WARNING( "GSmtp::Server: configured to reject non-local admin connection: " << reason ) ;
			return nullptr ;
		}

		AdminServerPeer * peer = new AdminServerPeer( peer_info , *this , m_remote_address ,
			m_info_commands , m_config_commands , m_with_terminate ) ;
		m_peers.push_back( peer ) ;
		return peer ;
	}
	catch( std::exception & e ) // newPeer()
	{
		G_WARNING( "GSmtp::AdminServer: exception from new connection: " << e.what() ) ;
		return nullptr ;
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
