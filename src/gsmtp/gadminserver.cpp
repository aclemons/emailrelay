//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gnetdone.h"
#include "gadminserver.h"
#include "gmessagestore.h"
#include "gstoredmessage.h"
#include "gprocess.h"
#include "glocal.h"
#include "gmonitor.h"
#include "gslot.h"
#include "gstr.h"
#include <utility>

GSmtp::AdminServerPeer::AdminServerPeer( GNet::ExceptionSinkUnbound esu , GNet::ServerPeerInfo peer_info ,
	AdminServer & server , const std::string & remote_address ,
	const G::StringMap & info_commands , const G::StringMap & config_commands ,
	bool with_terminate ) :
		GNet::ServerPeer(esu.bind(this),peer_info,GNet::LineBufferConfig::autodetect()) ,
		m_es(esu.bind(this)) ,
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
	m_client_ptr.deletedSignal().connect( G::Slot::slot(*this,&AdminServerPeer::clientDone) ) ;
	// dont prompt here -- it confuses some clients
}

GSmtp::AdminServerPeer::~AdminServerPeer()
{
}

void GSmtp::AdminServerPeer::clientDone( std::string s )
{
	if( s.empty() )
		sendLine( "OK" ) ;
	else
		sendLine( "error: " + s ) ;
}

void GSmtp::AdminServerPeer::onDelete( const std::string & reason )
{
	G_LOG_S( "GSmtp::AdminServerPeer: admin connection closed: " << reason << (reason.empty()?"":": ")
		<< peerAddress().second.displayString() ) ;
}

void GSmtp::AdminServerPeer::onSecure( const std::string & , const std::string & )
{
}

bool GSmtp::AdminServerPeer::onReceive( const char * line_data , size_t line_size , size_t , size_t , char )
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
		throw GNet::Done() ;
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
	std::string eol = lineBuffer().eol() ;
	return eol.empty() ? std::string("\r\n") : eol ;
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
	if( m_client_ptr.busy() )
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
		m_client_ptr.reset( new GSmtp::Client( GNet::ExceptionSink(m_client_ptr,nullptr) ,
			GNet::Location(m_remote_address) , m_server.clientSecrets() , m_server.clientConfig() ) ) ;

		m_client_ptr->sendMessagesFrom( m_server.store() ) ; // once connected
		// no sendLine() -- sends "OK" or "error:" when complete -- see AdminServerPeer::clientDone()
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

void GSmtp::AdminServerPeer::notify( const std::string & s0 , const std::string & s1 ,
	const std::string & s2 , const std::string &s3 )
{
	if( m_notifying )
	{
		std::string s = eol() + "EVENT: " + G::Str::printable(G::Str::join(": ",s0,s1,s2,s3)) ;
		G::Str::unique( s , ' ' , ' ' ) ;
		s.append( 2U , ' ' ) ;
		send_( s ) ;
	}
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

bool GSmtp::AdminServerPeer::notifying() const
{
	return m_notifying ;
}

// ===

GSmtp::AdminServer::AdminServer( GNet::ExceptionSink es , MessageStore & store ,
	const GNet::ServerPeerConfig & server_peer_config ,
	const GSmtp::Client::Config & client_config , const GAuth::Secrets & client_secrets ,
	const GNet::MultiServer::AddressList & listening_addresses , bool allow_remote ,
	const std::string & remote_address , unsigned int connection_timeout ,
	const G::StringMap & info_commands , const G::StringMap & config_commands ,
	bool with_terminate ) :
		GNet::MultiServer(es,listening_addresses,server_peer_config) ,
		m_store(store) ,
		m_client_config(client_config) ,
		m_client_secrets(client_secrets) ,
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
	serverCleanup() ; // base class early cleanup
}

unique_ptr<GNet::ServerPeer> GSmtp::AdminServer::newPeer( GNet::ExceptionSinkUnbound esu , GNet::ServerPeerInfo peer_info , GNet::MultiServer::ServerInfo )
{
	unique_ptr<GNet::ServerPeer> ptr ;
	try
	{
		std::string reason ;
		if( ! m_allow_remote && ! GNet::Local::isLocal(peer_info.m_address,reason) )
		{
			G_WARNING( "GSmtp::Server: configured to reject non-local admin connection: " << reason ) ;
		}
		else
		{
			ptr.reset( new AdminServerPeer( esu , peer_info , *this , m_remote_address ,
				m_info_commands , m_config_commands , m_with_terminate ) ) ;
		}
	}
	catch( std::exception & e ) // newPeer()
	{
		G_WARNING( "GSmtp::AdminServer: new connection error: " << e.what() ) ;
	}
	return ptr ;
}

void GSmtp::AdminServer::report() const
{
	serverReport( "admin" ) ;
}

void GSmtp::AdminServer::notify( const std::string & s0 , const std::string & s1 ,
	const std::string & s2 , const std::string & s3 )
{
	if( hasPeers() )
	{
		typedef std::vector<weak_ptr<GNet::ServerPeer> > List ;
		List list = peers() ;
		for( List::iterator list_p = list.begin() ; list_p != list.end() ; ++list_p )
		{
			if( (*list_p).expired() ) continue ;
			shared_ptr<GNet::ServerPeer> ptr = (*list_p).lock() ;
			AdminServerPeer * peer = static_cast<AdminServerPeer*>( ptr.get() ) ; // downcast
			G_DEBUG( "GSmtp::AdminServer::notify: " << peer << ": " << s0 << ": " << s1 ) ;
			peer->notify( s0 , s1 , s2 , s3 ) ;
		}
	}
}

GSmtp::MessageStore & GSmtp::AdminServer::store()
{
	return m_store ;
}

const GAuth::Secrets & GSmtp::AdminServer::clientSecrets() const
{
	return m_client_secrets ;
}

unsigned int GSmtp::AdminServer::connectionTimeout() const
{
	return m_connection_timeout ;
}

GSmtp::Client::Config GSmtp::AdminServer::clientConfig() const
{
	return m_client_config ;
}

bool GSmtp::AdminServer::notifying() const
{
	bool result = false ;
	if( hasPeers() )
	{
		typedef std::vector<weak_ptr<GNet::ServerPeer> > List ;
		List list = const_cast<AdminServer*>(this)->peers() ;
		for( List::iterator list_p = list.begin() ; list_p != list.end() ; ++list_p )
		{
			if( (*list_p).expired() ) continue ;
			shared_ptr<GNet::ServerPeer> ptr = (*list_p).lock() ;
			AdminServerPeer * peer = static_cast<AdminServerPeer*>( ptr.get() ) ; // downcast
			if( peer->notifying() )
			{
				result = true ;
				break ;
			}
		}
	}
	return result ;
}

/// \file gadminserver.cpp
