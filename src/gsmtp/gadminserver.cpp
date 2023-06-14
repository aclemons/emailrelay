//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gadminserver.cpp
///

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
#include "gstringtoken.h"
#include "gstr.h"
#include "gstringarray.h"
#include <utility>
#include <limits>

GSmtp::AdminServerPeer::AdminServerPeer( GNet::ExceptionSinkUnbound esu , GNet::ServerPeerInfo && peer_info ,
	AdminServer & server , const std::string & remote_address ,
	const G::StringMap & info_commands ,
	bool with_terminate ) :
		GNet::ServerPeer(esu.bind(this),std::move(peer_info),GNet::LineBufferConfig::autodetect()),
		m_es(esu.bind(this)) ,
		m_server(server) ,
		m_prompt("E-MailRelay> ") ,
		m_blocked(false) ,
		m_remote_address(remote_address) ,
		m_notifying(false) ,
		m_info_commands(info_commands) ,
		m_with_terminate(with_terminate) ,
		m_error_limit(30U) ,
		m_error_count(0U)
{
	G_LOG_S( "GSmtp::AdminServerPeer: admin connection from " << peerAddress().displayString() ) ;
	m_client_ptr.deletedSignal().connect( G::Slot::slot(*this,&AdminServerPeer::clientDone) ) ;
	// dont prompt here -- it confuses some clients
}

GSmtp::AdminServerPeer::~AdminServerPeer()
{
	m_client_ptr.deletedSignal().disconnect() ;
}

void GSmtp::AdminServerPeer::clientDone( const std::string & s )
{
	if( s.empty() )
		sendLine( "OK" ) ;
	else
		sendLine( std::move(std::string("error: ").append(s)) ) ;
}

void GSmtp::AdminServerPeer::onDelete( const std::string & reason )
{
	G_LOG_S( "GSmtp::AdminServerPeer: admin connection closed: " << reason << (reason.empty()?"":": ")
		<< peerAddress().displayString() ) ;
}

void GSmtp::AdminServerPeer::onSecure( const std::string & , const std::string & , const std::string & )
{
}

bool GSmtp::AdminServerPeer::onReceive( const char * line_data , std::size_t line_size , std::size_t ,
	std::size_t , char )
{
	G::string_view line( line_data , line_size ) ;
	G::StringTokenView t( line , G::Str::ws() ) ;
	if( is(t(),"flush") )
	{
		flush() ;
	}
	else if( is(t(),"forward") )
	{
		forward() ;
	}
	else if( is(t(),"help") )
	{
		help() ;
	}
	else if( is(t(),"status") )
	{
		status() ;
	}
	else if( is(t(),"notify") )
	{
		m_notifying = true ;
		setIdleTimeout( 0U ) ; // GNet::ServerPeer
	}
	else if( is(t(),"list") )
	{
		sendMessageIds( m_server.store().ids() ) ;
	}
	else if( is(t(),"failures") )
	{
		sendMessageIds( m_server.store().failures() ) ;
	}
	else if( is(t(),"unfail-all") )
	{
		m_server.store().unfailAll() ;
		sendLine( std::string() ) ;
	}
	else if( is(t(),"pid") )
	{
		sendLine( G::Process::Id().str() ) ;
	}
	else if( is(t(),"quit") )
	{
		throw GNet::Done() ;
	}
	else if( is(t(),"terminate") && m_with_terminate )
	{
		G_LOG_S( "GSmtp::AdminServerPeer::onReceive: received a terminate command from "
			<< peerAddress().displayString() ) ;
		if( GNet::EventLoop::exists() )
			GNet::EventLoop::instance().quit("") ;
	}
	else if( is(t(),"info") && !m_info_commands.empty() )
	{
		G::string_view arg = (++t)() ;
		if( arg.empty() || !find(arg,m_info_commands).first )
			sendLine( std::move(std::string("usage: info {").append(G::Str::join("|",G::Str::keys(m_info_commands))).append(1U,'}')) ) ;
		else
			sendLine( find(arg,m_info_commands).second ) ;
	}
	else if( is(t(),"dnsbl") )
	{
		G::string_view action = (++t)() ;
		G::string_view arg = (++t)() ;
		bool start = G::Str::imatch( action , "start" ) ;
		if( ( start && arg.empty() ) || ( G::Str::imatch(action,"stop") && ( arg.empty() || G::Str::isUInt(arg) ) ) )
		{
			sendLine( "OK" ) ;
			m_server.emitCommand( AdminServer::Command::dnsbl , start ? 0U :
				( arg.empty() ? std::numeric_limits<unsigned int>::max() : G::Str::toUInt(arg,"0") ) ) ;
		}
		else
		{
			sendLine( "usage: dnsbl {start|stop <timeout>}" ) ;
		}
	}
	else if( line.find_first_not_of(" \r\n\t") != std::string::npos )
	{
		sendLine( "error: unrecognised command" ) ;
		m_error_count++ ;
		if( m_error_limit && m_error_count >= m_error_limit )
			throw G::Exception( "too many errors" ) ;
	}
	else
	{
		sendLine( std::string() ) ;
	}
	return true ;
}

std::string GSmtp::AdminServerPeer::eol() const
{
	std::string eol = lineBuffer().eol() ;
	return eol.empty() ? std::string("\r\n") : eol ;
}

bool GSmtp::AdminServerPeer::is( G::string_view token , G::string_view key )
{
	return G::Str::imatch( token , key ) ;
}

std::pair<bool,std::string> GSmtp::AdminServerPeer::find( G::string_view line , const G::StringMap & map )
{
	for( const auto & item : map )
	{
		if( is(line,item.first) )
			return { true , item.second } ;
	}
	return { false , {} } ;
}

void GSmtp::AdminServerPeer::help()
{
	sendLine( std::move(std::string("commands: ")
		.append( "dnsbl, " )
		.append( "failures, " )
		.append( "flush, " )
		.append( "forward, " )
		.append( "help, " )
		.append( "info, " , m_info_commands.empty() ? 0U : 6U )
		.append( "list, " )
		.append( "notify, " )
		.append( "pid, " )
		.append( "quit, " )
		.append( "status, " )
		.append( "terminate, " , m_with_terminate ? 11U : 0U )
		.append( "unfail-all" )) ) ;
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
		m_client_ptr.reset( std::make_unique<GSmtp::Client>( GNet::ExceptionSink(m_client_ptr,m_es.esrc()) ,
			m_server.store() , m_server.ff() , GNet::Location(m_remote_address) ,
			m_server.clientSecrets() , m_server.clientConfig() ) ) ;
		// no sendLine() -- sends "OK" or "error:" when complete -- see AdminServerPeer::clientDone()
	}
}

void GSmtp::AdminServerPeer::forward()
{
	if( m_remote_address.empty() )
	{
		sendLine( "error: no remote server configured: use --forward-to" ) ;
	}
	else
	{
		sendLine( "OK" ) ;
		m_server.emitCommand( AdminServer::Command::forward , 0U ) ;
	}
}

void GSmtp::AdminServerPeer::sendLine( std::string && line )
{
	if( !line.empty() )
		line.append( "\n" ) ;
	G::Str::replaceAll( line , "\n" , eol() ) ;
	line.append( m_prompt ) ;
	sendImp( line ) ;
}

void GSmtp::AdminServerPeer::notify( const std::string & s0 , const std::string & s1 ,
	const std::string & s2 , const std::string &s3 )
{
	if( m_notifying )
	{
		std::string s = eol().append("EVENT: ").append(G::Str::printable(G::Str::join(": ",s0,s1,s2,s3))) ;
		G::Str::unique( s , ' ' , ' ' ) ;
		s.append( 2U , ' ' ) ;
		sendImp( s ) ;
	}
}

void GSmtp::AdminServerPeer::sendImp( const std::string & s )
{
	if( m_blocked )
	{
		G_DEBUG( "GSmtp::AdminServerPeer::send: flow control asserted: cannot send" ) ;
		// could do better
	}
	else
	{
		m_blocked = !send( s ) ; // GNet::ServerPeer::send()
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
		const std::string eolstr = eol() ;
		GNet::Monitor::instance()->report( ss , "" , eolstr ) ;
		std::string report = ss.str() ;
		G::Str::trimRight( report , eolstr ) ;
		sendLine( std::move(report) ) ;
	}
	else
	{
		sendLine( "no info" ) ;
	}
}

void GSmtp::AdminServerPeer::sendMessageIds( const std::vector<GStore::MessageId> & ids )
{
	std::ostringstream ss ;
	bool first = true ;
	for( const auto & id : ids )
	{
		if( !first ) ss << eol() ;
		ss << id.str() ;
		first = false ;
	}

	std::string result = ss.str() ;
	if( result.empty() )
		sendLine( "<none>" ) ;
	else
		sendLine( ss.str() ) ;
}

bool GSmtp::AdminServerPeer::notifying() const
{
	return m_notifying ;
}

// ===

GSmtp::AdminServer::AdminServer( GNet::ExceptionSink es , GStore::MessageStore & store ,
	FilterFactoryBase & ff , const GAuth::SaslClientSecrets & client_secrets ,
	const G::StringArray & interfaces , const Config & config ) :
		GNet::MultiServer(es,interfaces,config.port,"admin",config.net_server_peer_config,config.net_server_config) ,
		m_store(store) ,
		m_ff(ff) ,
		m_client_secrets(client_secrets) ,
		m_config(config) ,
		m_command_timer(*this,&AdminServer::onCommandTimeout,es) ,
		m_command(Command::forward) ,
		m_command_arg(0U)
{
}

GSmtp::AdminServer::~AdminServer()
{
	serverCleanup() ; // base class early cleanup
}

std::unique_ptr<GNet::ServerPeer> GSmtp::AdminServer::newPeer( GNet::ExceptionSinkUnbound esu ,
	GNet::ServerPeerInfo && peer_info , GNet::MultiServer::ServerInfo )
{
	std::unique_ptr<GNet::ServerPeer> ptr ;
	try
	{
		std::string reason ;
		if( !m_config.allow_remote && !peer_info.m_address.isLocal(reason) )
		{
			G_WARNING( "GSmtp::Server: configured to reject non-local admin connection: " << reason ) ;
		}
		else
		{
			ptr = std::make_unique<AdminServerPeer>( esu , std::move(peer_info) , *this ,
				m_config.remote_address , m_config.info_commands ,
				m_config.with_terminate ) ;
		}
	}
	catch( std::exception & e ) // newPeer()
	{
		G_WARNING( "GSmtp::AdminServer: new connection error: " << e.what() ) ;
	}
	return ptr ;
}

void GSmtp::AdminServer::emitCommand( Command command , unsigned int arg )
{
	m_command = command ;
	m_command_arg = arg ;
	m_command_timer.startTimer( 0 ) ;
}

void GSmtp::AdminServer::onCommandTimeout()
{
	try
	{
		m_command_signal.emit( m_command , m_command_arg ) ;
	}
	catch( std::exception & e )
	{
		G_WARNING( "GSmtp::AdminServer: exception: " << e.what() ) ;
	}
}

G::Slot::Signal<GSmtp::AdminServer::Command,unsigned int> & GSmtp::AdminServer::commandSignal() noexcept
{
	return m_command_signal ;
}

void GSmtp::AdminServer::report( const std::string & group ) const
{
	serverReport( group ) ;
}

void GSmtp::AdminServer::notify( const std::string & s0 , const std::string & s1 ,
	const std::string & s2 , const std::string & s3 )
{
	if( hasPeers() )
	{
		using List = std::vector<std::weak_ptr<GNet::ServerPeer>> ;
		List list = peers() ;
		for( auto & wptr : list )
		{
			if( wptr.expired() ) continue ;
			std::shared_ptr<GNet::ServerPeer> ptr = wptr.lock() ;
			AdminServerPeer * peer = static_cast<AdminServerPeer*>( ptr.get() ) ; // downcast
			G_DEBUG( "GSmtp::AdminServer::notify: " << peer << ": " << s0 << ": " << s1 ) ;
			peer->notify( s0 , s1 , s2 , s3 ) ;
		}
	}
}

GStore::MessageStore & GSmtp::AdminServer::store()
{
	return m_store ;
}

GSmtp::FilterFactoryBase & GSmtp::AdminServer::ff()
{
	return m_ff ;
}

const GAuth::SaslClientSecrets & GSmtp::AdminServer::clientSecrets() const
{
	return m_client_secrets ;
}

GSmtp::Client::Config GSmtp::AdminServer::clientConfig() const
{
	return m_config.smtp_client_config ;
}

bool GSmtp::AdminServer::notifying() const
{
	bool result = false ;
	if( hasPeers() )
	{
		using List = std::vector<std::weak_ptr<GNet::ServerPeer>> ;
		List list = const_cast<AdminServer*>(this)->peers() ;
		for( auto & wptr : list )
		{
			if( wptr.expired() ) continue ;
			std::shared_ptr<GNet::ServerPeer> ptr = wptr.lock() ;
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

