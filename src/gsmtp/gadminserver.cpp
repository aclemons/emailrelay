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

GSmtp::AdminServerPeer::AdminServerPeer( GNet::ExceptionSinkUnbound esu , GNet::ServerPeerInfo && peer_info ,
	AdminServer & server , const std::string & remote_address ,
	const G::StringMap & info_commands , const G::StringMap & config_commands ,
	bool with_terminate ) :
		GNet::ServerPeer(esu.bind(this),std::move(peer_info),GNet::LineBufferConfig::autodetect()),
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
	G_LOG_S( "GSmtp::AdminServerPeer: admin connection from " << peerAddress().displayString() ) ;
	m_client_ptr.deletedSignal().connect( G::Slot::slot(*this,&AdminServerPeer::clientDone) ) ;
	// dont prompt here -- it confuses some clients
}

GSmtp::AdminServerPeer::~AdminServerPeer()
{
	m_client_ptr.deletedSignal().disconnect() ; // fwiw
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
	if( is(line,"flush"_sv) )
	{
		flush() ;
	}
	else if( is(line,"forward"_sv) )
	{
		forward() ;
	}
	else if( is(line,"help"_sv) )
	{
		help() ;
	}
	else if( is(line,"status"_sv) )
	{
		status() ;
	}
	else if( is(line,"notify"_sv) )
	{
		m_notifying = true ;
		setIdleTimeout( 0U ) ; // GNet::ServerPeer
	}
	else if( is(line,"list"_sv) )
	{
		sendList( spooled() ) ;
	}
	else if( is(line,"failures"_sv) )
	{
		sendList( failures() ) ;
	}
	else if( is(line,"unfail-all"_sv) )
	{
		unfailAll() ;
		sendLine( std::string() ) ;
	}
	else if( is(line,"pid"_sv) )
	{
		sendLine( G::Process::Id().str() ) ;
	}
	else if( is(line,"quit"_sv) )
	{
		throw GNet::Done() ;
	}
	else if( is(line,"terminate"_sv) && m_with_terminate )
	{
		G_LOG_S( "GSmtp::AdminServerPeer::onReceive: received a terminate command from "
			<< peerAddress().displayString() ) ;
		if( GNet::EventLoop::exists() )
			GNet::EventLoop::instance().quit("") ;
	}
	else if( is(line,"info"_sv) && !m_info_commands.empty() )
	{
		G::string_view arg = argument( line ) ;
		if( arg.empty() || !find(arg,m_info_commands).first )
			sendLine( std::move(std::string("usage: info {").append(G::Str::join("|",G::Str::keys(m_info_commands))).append(1U,'}')) ) ;
		else
			sendLine( find(arg,m_info_commands).second ) ;
	}
	else if( is(line,"config"_sv) && !m_config_commands.empty() )
	{
		G::string_view arg = argument( line ) ;
		if( arg.empty() )
			sendLine( G::Str::join( eol() , m_config_commands , "=[" , "]" ) ) ;
		else if( !find(arg,m_config_commands).first )
			sendLine( std::move(std::string("usage: config [{").append(G::Str::join("|",G::Str::keys(m_config_commands))).append("}]",2U)) ) ;
		else
			sendLine( find(arg,m_config_commands).second ) ;
	}
	else if( line.find_first_not_of(" \r\n\t") != std::string::npos )
	{
		sendLine( "error: unrecognised command" ) ;
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

bool GSmtp::AdminServerPeer::is( G::string_view line_in , G::string_view key )
{
	G::StringTokenView t( line_in , G::Str::ws() ) ;
	return t.valid() && G::Str::imatch( t() , key ) ;
}

G::string_view GSmtp::AdminServerPeer::argument( G::string_view line_in )
{
	G::StringTokenView t( line_in , G::Str::ws() ) ;
	++t ;
	return t.valid() ? t() : ""_sv ;
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
		.append( "config, " , m_config_commands.empty() ? 0U : 8U )
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
		m_server.forward() ;
		sendLine( "OK" ) ;
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

std::shared_ptr<GStore::MessageStore::Iterator> GSmtp::AdminServerPeer::spooled()
{
	return m_server.store().iterator(false) ;
}

std::shared_ptr<GStore::MessageStore::Iterator> GSmtp::AdminServerPeer::failures()
{
	return m_server.store().failures() ;
}

void GSmtp::AdminServerPeer::sendList( std::shared_ptr<GStore::MessageStore::Iterator> iter )
{
	std::ostringstream ss ;
	for( bool first = true ;; first = false )
	{
		std::unique_ptr<GStore::StoredMessage> message( ++iter ) ;
		if( message == nullptr ) break ;
		if( !first ) ss << eol() ;
		ss << message->id().str() ;
	}

	std::string result = ss.str() ;
	if( result.empty() )
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

GSmtp::AdminServer::AdminServer( GNet::ExceptionSink es , GStore::MessageStore & store ,
	FilterFactoryBase & ff , G::Slot::Signal<const std::string&> & forward_request ,
	const GAuth::SaslClientSecrets & client_secrets ,
	const G::StringArray & interfaces , const Config & config ) :
		GNet::MultiServer(es,interfaces,config.port,"admin",config.net_server_peer_config,config.net_server_config) ,
		m_forward_timer(*this,&AdminServer::onForwardTimeout,es) ,
		m_store(store) ,
		m_ff(ff) ,
		m_forward_request(forward_request) ,
		m_client_secrets(client_secrets) ,
		m_config(config)
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
				m_config.config_commands , m_config.with_terminate ) ; // up-cast
		}
	}
	catch( std::exception & e ) // newPeer()
	{
		G_WARNING( "GSmtp::AdminServer: new connection error: " << e.what() ) ;
	}
	return ptr ;
}

void GSmtp::AdminServer::forward()
{
	// asychronous emit() for safety
	m_forward_timer.startTimer( 0 ) ;
}

void GSmtp::AdminServer::onForwardTimeout()
{
	try
	{
		m_forward_request.emit( std::string("admin") ) ;
	}
	catch( std::exception & e )
	{
		G_WARNING( "GSmtp::AdminServer: exception: " << e.what() ) ;
	}
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
		using List = std::vector<std::weak_ptr<GNet::ServerPeer> > ;
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
		using List = std::vector<std::weak_ptr<GNet::ServerPeer> > ;
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

