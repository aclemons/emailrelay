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
/// \file unit.cpp
///

#include "gdef.h"
#include "unit.h"
#include "run.h"
#include "legal.h"
#include "filterfactory.h"
#include "verifierfactory.h"
#include "gssl.h"
#include "gpop.h"
#include "glog.h"
#include "gassert.h"
#include "gformat.h"
#include <functional>

Main::Unit::Unit( Run & run , unsigned int unit_id , const std::string & version_number ) :
	m_run(run) ,
	m_configuration(run.configuration(unit_id)) ,
	m_version_number(version_number) ,
	m_unit_id(unit_id)
{
	G_ASSERT( GSsl::Library::instance() != nullptr ) ;
	using G::format ;
	using G::txt ;

	// connect signals so that we know what the ClientPtr is doing
	//
	m_client_ptr.deletedSignal().connect( G::Slot::slot(*this,&Unit::onClientDone) ) ;
	m_client_ptr.eventSignal().connect( G::Slot::slot(*this,&Unit::onClientEvent) ) ;

	// connect the signal that allows the AdminServer to trigger our forwarding
	//
	m_forward_request_signal.connect( G::Slot::slot(*this,&Unit::onForwardRequest) ) ;

	// cache the forwarding address's address family
	//
	m_resolver_family = resolverFamily() ;

	// early check that the forward-to address can be resolved
	//
	if( m_configuration.log() && !m_configuration.serverAddress().empty() && !m_configuration.forwardOnStartup() &&
		!GNet::Address::isFamilyLocal( m_configuration.serverAddress() ) )
	{
		GNet::Location location( m_configuration.serverAddress() , m_resolver_family ) ;
		std::string error = GNet::Resolver::resolve( location ) ;
		if( !error.empty() )
			G_WARNING( "Main::Unit::ctor: " << format(txt("dns lookup of forward-to address failed: %1%")) % error ) ;
		else
			G_LOG( "Main::Unit::ctor: " << format(txt("forwarding address %1%")) % location.displayString() ) ;
	}

	// early check on the DNSBL configuration string
	//
	if( !m_configuration.dnsbl().empty() )
	{
		GNet::Dnsbl::checkConfig( m_configuration.dnsbl() ) ;
	}

	// create the TLS server profile
	//
	if( m_configuration.serverTls() || m_configuration.serverTlsConnection() )
		GSsl::Library::instance()->addProfile( serverTlsProfile() , true ,
			m_configuration.serverTlsPrivateKey().str() ,
			m_configuration.serverTlsCertificate().str() ,
			m_configuration.serverTlsCaList().str() ) ;

	// create the TLS client profile
	//
	if( m_configuration.clientTls() || m_configuration.clientOverTls() )
		GSsl::Library::instance()->addProfile( clientTlsProfile() , false ,
			m_configuration.clientTlsPrivateKey().str() ,
			m_configuration.clientTlsCertificate().str() ,
			m_configuration.clientTlsCaList().str() ,
			m_configuration.clientTlsPeerCertificateName() ,
			m_configuration.clientTlsPeerHostName() ) ;

	// do forwarding via a timer
	//
	m_forwarding_timer = std::make_unique<GNet::Timer<Unit>>( *this ,
		&Unit::onRequestForwardingTimeout , GNet::ExceptionSink::logOnly() ) ;

	// create the polling timer
	//
	m_poll_timer = std::make_unique<GNet::Timer<Unit>>( *this ,
		&Unit::onPollTimeout , GNet::ExceptionSink::logOnly() ) ;

	// figure out what we're doing
	//
	bool do_smtp = m_configuration.doServing() && m_configuration.doSmtp() ;
	bool do_pop = m_configuration.doServing() && GPop::enabled() && m_configuration.doPop() ;
	bool do_admin = m_configuration.doServing() && m_configuration.doAdmin() ;
	m_serving = do_smtp || do_pop || do_admin ;
	bool admin_forwarding = do_admin && !m_configuration.serverAddress().empty() ;
	m_forwarding = m_configuration.forwardOnStartup() || m_configuration.doPolling() || admin_forwarding ;
	m_quit_when_sent =
		!m_serving &&
		m_configuration.forwardOnStartup() &&
		!m_configuration.doPolling() &&
		!admin_forwarding ;

	// create message store stuff
	//
	m_file_store = std::make_unique<GStore::FileStore>( m_configuration.spoolDir() , m_configuration.fileStoreConfig() ) ;
	m_filter_factory = std::make_unique<Main::FilterFactory>( m_run , *this , *m_file_store ) ;
	m_verifier_factory = std::make_unique<Main::VerifierFactory>( m_run , *this ) ;
	store().messageStoreRescanSignal().connect( G::Slot::slot(*this,&Unit::onStoreRescanEvent) ) ;
	if( do_pop )
	{
		m_pop_store = GPop::newStore( m_configuration.spoolDir() ,
			m_configuration.popByName() , !m_configuration.popNoDelete() ) ;
	}

	// prepare authentication secrets
	//
	GAuth::Secrets::check( m_configuration.clientSecretsFile().str() , m_configuration.serverSecretsFile().str() ,
		do_pop ? m_configuration.popSecretsFile().str() : std::string() ) ;
	m_client_secrets = std::make_unique<GAuth::Secrets>( m_configuration.clientSecretsFile().str() , "client" ) ;
	m_server_secrets = std::make_unique<GAuth::Secrets>( m_configuration.serverSecretsFile().str() , "server" ) ;
	if( do_pop )
		m_pop_secrets = GPop::newSecrets( m_configuration.popSecretsFile().str() ) ;

	// create the smtp server
	//
	if( do_smtp )
	{
		if( m_configuration.immediate() )
			G_WARNING( "Unit::ctor: " << txt("using --immediate can result in client timeout errors: "
				"try --forward-on-disconnect instead") ) ;

		G_ASSERT( m_file_store != nullptr ) ;
		G_ASSERT( m_client_secrets != nullptr ) ;
		G_ASSERT( m_server_secrets != nullptr ) ;
		m_smtp_server = std::make_unique<GSmtp::Server>(
			GNet::ExceptionSink() ,
			*m_file_store ,
			*m_filter_factory ,
			*m_verifier_factory ,
			*m_client_secrets ,
			*m_server_secrets ,
			m_configuration.smtpServerConfig( ident() , m_server_secrets->valid() , serverTlsProfile() , std::bind(&Run::defaultDomain,&run) ) ,
			m_configuration.immediate() ? m_configuration.serverAddress() : std::string() ,
			m_resolver_family ,
			m_configuration.smtpClientConfig( clientTlsProfile() ) ) ;

		m_smtp_server->eventSignal().connect( G::Slot::slot(*this,&Main::Unit::onServerEvent) ) ;
	}

	// create the pop server
	//
	if( do_pop )
	{
		G_ASSERT( m_pop_store != nullptr ) ;
		G_ASSERT( m_pop_secrets != nullptr ) ;
		m_pop_server = GPop::newServer(
			GNet::ExceptionSink() ,
			*m_pop_store ,
			*m_pop_secrets ,
			m_configuration.popServerConfig( serverTlsProfile() , std::bind(&Run::defaultDomain,&run) ) ) ;
	}

	// create the admin server
	//
	if( do_admin )
	{
		G_ASSERT( m_file_store != nullptr ) ;
		G_ASSERT( m_client_secrets != nullptr ) ;

		G::StringMap info_map ;
		info_map["version"] = version_number ;
		info_map["warranty"] = Legal::warranty("","\n") ;
		info_map["credit"] = GSsl::Library::credit("","\n","") ;
		info_map["copyright"] = Legal::copyright() ;

		G::StringMap config_map ;
		//config_map["forward-to"] = cfg.serverAddress() ;
		//config_map["spool-dir"] = cfg.spoolDir().str() ;

		m_admin_server = std::make_unique<GSmtp::AdminServer>(
			GNet::ExceptionSink() ,
			*m_file_store ,
			*m_filter_factory ,
			m_forward_request_signal ,
			*m_client_secrets ,
			m_configuration.listeningNames("admin") ,
			m_configuration.adminServerConfig( info_map , config_map , clientTlsProfile() ) ) ;
	}
}

unsigned int Main::Unit::id() const
{
	return m_unit_id ;
}

void Main::Unit::onClientEvent( const std::string & p1 , const std::string & p2 , const std::string & p3 )
{
	// p1: connecting, resolving, connected, sending, sent
	m_client_event_signal.emit( m_unit_id , p1 , p2 , p3 ) ;
}

void Main::Unit::onClientDone( const std::string & reason )
{
	if( m_forwarding_pending )
	{
		m_forwarding_pending = false ;
		using G::format ;
		using G::txt ;
		G_LOG( "Main::Unit::onClientDone: "
			<< format(txt("forwarding: queued request [%1%]")) % m_forwarding_reason ) ;
		requestForwarding() ;
	}

	m_client_event_signal.emit( m_unit_id , "forward" , "end" , reason ) ;
	m_client_done_signal.emit( m_unit_id , reason , m_quit_when_sent ) ;
}

void Main::Unit::onPollTimeout()
{
	G_DEBUG( "Main::Unit::onPollTimeout" ) ;
	m_poll_timer->startTimer( m_configuration.pollingTimeout() ) ;
	requestForwarding( "poll" ) ;
}

void Main::Unit::onForwardRequest( const std::string & reason )
{
	requestForwarding( reason ) ;
}

void Main::Unit::requestForwarding( const std::string & reason )
{
	G_ASSERT( m_forwarding_timer != nullptr ) ;
	if( !reason.empty() )
		m_forwarding_reason = reason ;
	m_forwarding_timer->startTimer( 0U ) ;
}

void Main::Unit::onRequestForwardingTimeout()
{
	using G::format ;
	using G::txt ;
	if( m_client_ptr.busy() )
	{
		G_LOG( "Main::Unit::onRequestForwardingTimeout: "
			<< format(txt("forwarding: [%1%]: still busy from last time")) % m_forwarding_reason ) ;
		m_forwarding_pending = true ;
	}
	else
	{
		if( logForwarding() )
		{
			G_LOG( "Main::Unit::onRequestForwardingTimeout: "
				<< format(txt("forwarding: [%1%]")) % m_forwarding_reason ) ;
		}

		std::string error = startForwarding() ;

		if( error.empty() )
			m_client_event_signal.emit( m_unit_id , "forward" , "start" , m_forwarding_reason ) ;
		else
			m_client_event_signal.emit( m_unit_id , "forward" , "end" , error ) ;
	}
}

bool Main::Unit::logForwarding() const
{
	return m_forwarding_reason != "poll" || m_configuration.pollingLog() ||
		( G::LogOutput::instance() && G::LogOutput::instance()->at( G::Log::Severity::Debug ) ) ;
}

std::string Main::Unit::startForwarding()
{
	using G::txt ;
	try
	{
		G_ASSERT( m_file_store != nullptr ) ;
		if( m_file_store && store().empty() )
		{
			if( logForwarding() )
				G_LOG( "Main::Unit::startForwarding: " << txt("forwarding: no messages to send") ) ;
			return "no messages" ;
		}
		else
		{
			G_ASSERT( m_client_secrets != nullptr ) ;
			m_client_ptr.reset( std::make_unique<GSmtp::Client>(
				GNet::ExceptionSink(m_client_ptr,nullptr) ,
				*m_file_store ,
				*m_filter_factory ,
				GNet::Location(m_configuration.serverAddress(),m_resolver_family) ,
				*m_client_secrets ,
				m_configuration.smtpClientConfig( clientTlsProfile() ) ) ) ;
			return std::string() ;
		}
	}
	catch( std::exception & e )
	{
		G_ERROR( "Main::Unit::startForwarding: " << txt("forwarding failure") << ": " << e.what() ) ;
		return e.what() ;
	}
}

bool Main::Unit::needsTls( const Configuration & configuration )
{
	return
		configuration.clientTls() ||
		configuration.clientOverTls() ||
		configuration.serverTls() ||
		configuration.serverTlsConnection() ;
}

bool Main::Unit::prefersTls( const Configuration & configuration )
{
	return
		!configuration.clientSecretsFile().empty() ||
		!configuration.serverSecretsFile().empty() ||
		!configuration.popSecretsFile().empty() ;
}

bool Main::Unit::nothingToDo() const
{
	return !m_serving && !m_forwarding ;
}

bool Main::Unit::nothingToSend() const
{
	return m_file_store && store().empty() ;
}

void Main::Unit::start()
{
	// report stuff
	if( m_smtp_server ) m_smtp_server->report() ;
	if( m_admin_server ) m_admin_server->report() ;
	if( m_pop_server ) GPop::report( m_pop_server.get() ) ;

	// kick off some forwarding
	//
	if( m_configuration.forwardOnStartup() )
		requestForwarding( "startup" ) ;

	// kick off the polling cycle
	//
	if( m_configuration.doPolling() )
		m_poll_timer->startTimer( m_configuration.pollingTimeout() ) ;
}

void Main::Unit::onServerEvent( const std::string & s1 , const std::string & )
{
	if( s1 == "done" && m_configuration.forwardOnDisconnect() )
		requestForwarding( "client disconnect" ) ;
}

void Main::Unit::onStoreRescanEvent()
{
	requestForwarding( "rescan" ) ;
}

bool Main::Unit::adminNotification() const
{
	return m_admin_server && m_admin_server->notifying() ;
}

void Main::Unit::adminNotify( std::string s0 , std::string s1 , std::string s2 , std::string s3 )
{
	if( m_admin_server )
		m_admin_server->notify( s0 , s1 , s2 , s3 ) ;
}

int Main::Unit::resolverFamily() const
{
	// choose an address family for the DNS lookup based on the "--client-interface" address
	std::string client_bind_address = m_configuration.clientBindAddress() ;
	if( client_bind_address.empty() )
		return AF_UNSPEC ;

	GNet::Address address =
		GNet::Address::validString( client_bind_address , GNet::Address::NotLocal() ) ?
			GNet::Address::parse( client_bind_address , GNet::Address::NotLocal() ) :
			GNet::Address::parse( client_bind_address , 0U ) ;

	return ( address.af() == AF_INET || address.af() == AF_INET6 ) ? address.af() : AF_UNSPEC ;
}

bool Main::Unit::quitWhenSent() const
{
	return m_quit_when_sent ;
}

GStore::MessageStore & Main::Unit::store()
{
	G_ASSERT( m_file_store.get() != nullptr ) ;
	return *(static_cast<GStore::MessageStore*>(m_file_store.get())) ;
}

const GStore::MessageStore & Main::Unit::store() const
{
	G_ASSERT( m_file_store.get() != nullptr ) ;
	return *(static_cast<const GStore::MessageStore*>(m_file_store.get())) ;
}

G::Slot::Signal<unsigned,std::string,bool> & Main::Unit::clientDoneSignal()
{
	return m_client_done_signal ;
}

G::Slot::Signal<unsigned,std::string,std::string,std::string> & Main::Unit::clientEventSignal()
{
	return m_client_event_signal ;
}

std::string Main::Unit::serverTlsProfile() const
{
	return std::string("server-").append(G::Str::fromUInt(m_unit_id)) ;
}

std::string Main::Unit::clientTlsProfile() const
{
	return std::string("client-").append(G::Str::fromUInt(m_unit_id)) ;
}

std::string Main::Unit::ident() const
{
	return std::string("E-MailRelay V").append(m_version_number) ;
}