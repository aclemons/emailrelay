//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gsmtpserver.cpp
///

#include "gdef.h"
#include "gsmtpserver.h"
#include "gsmtpservertext.h"
#include "gdnsblock.h"
#include "gprotocolmessagestore.h"
#include "gprotocolmessageforward.h"
#include "gfilterfactorybase.h"
#include "gverifierfactorybase.h"
#include "ggettext.h"
#include "gformat.h"
#include "glog.h"
#include "gassert.h"
#include <string>
#include <functional>

GSmtp::ServerPeer::ServerPeer( GNet::EventStateUnbound esu ,
	GNet::ServerPeerInfo && peer_info , Server & server , bool enabled , VerifierFactoryBase & vf ,
	const GAuth::SaslServerSecrets & server_secrets , const Server::Config & server_config ,
	std::unique_ptr<ServerProtocol::Text> ptext ) :
		GNet::ServerPeer(esbind(esu,this),std::move(peer_info),GNet::LineBuffer::Config::transparent()) ,
		m_server(server) ,
		m_server_config(server_config) ,
		m_block(std::bind(&ServerPeer::onDnsBlockResult,this,std::placeholders::_1),esbind(esu,this),server_config.dnsbl_config) ,
		m_check_timer(*this,&ServerPeer::onCheckTimeout,esbind(esu,this)) ,
		m_verifier(vf.newVerifier(esbind(esu,this),server_config.verifier_config,server_config.verifier_spec)) ,
		m_pmessage(server.newProtocolMessage(esbind(esu,this))) ,
		m_ptext(ptext.release()) ,
		m_protocol(*this,*m_verifier,*m_pmessage,server_secrets,
			*m_ptext,peerAddress(),
			server_config.protocol_config,enabled) ,
		m_input_buffer(esbind(esu,this),m_protocol,server_config.buffer_config)
{
	G_LOG_S( "GSmtp::ServerPeer: smtp connection from " << peerAddress().displayString() ) ;

	if( !server_config.protocol_config.tls_connection )
		m_check_timer.startTimer( 1U ) ;

	if( server_config.dnsbl_config.empty() )
		m_protocol.init() ;
	else
		m_block.start( peerAddress() ) ;

	m_input_buffer.flowSignal().connect( G::Slot::slot(*this,&ServerPeer::onFlow) ) ;
}

GSmtp::ServerPeer::~ServerPeer()
{
	m_input_buffer.flowSignal().disconnect() ;
}

void GSmtp::ServerPeer::onDelete( const std::string & reason )
{
	G_LOG_S( "GSmtp::ServerPeer: smtp connection closed: " << reason << (reason.empty()?"":": ")
		<< peerAddress().displayString() ) ;

	m_server.eventSignal().emit( "done" , std::string(reason) ) ;
}

void GSmtp::ServerPeer::onData( const char * data , std::size_t size )
{
	G_ASSERT( data != nullptr && size != 0U ) ;

	// this override intercepts incoming data before it is applied to the
	// base class's line buffer so that we can discard anything received
	// before we have even sent an initial greeting
	if( m_block.busy() )
		return ;

	// the base class's line buffer is configured as transparent so
	// this is effectively a direct call to onReceive() -- we go via
	// the base class only in order to kick its idle-timeout timer
	GNet::ServerPeer::onData( data , size ) ;
}

bool GSmtp::ServerPeer::onReceive( const char * data , std::size_t size , std::size_t , std::size_t , char )
{
	G_ASSERT( data != nullptr && size != 0U ) ;
	m_input_buffer.apply( data , size ) ;
	return true ;
}

void GSmtp::ServerPeer::onFlow( bool on )
{
	if( on )
		addReadHandler() ;
	else
		dropReadHandler() ;
}

void GSmtp::ServerPeer::protocolSecure()
{
	secureAccept() ;
}

void GSmtp::ServerPeer::onSecure( const std::string & certificate , const std::string & protocol ,
	const std::string & cipher )
{
	m_protocol.secure( certificate , protocol , cipher ) ;
}

void GSmtp::ServerPeer::protocolSend( const std::string & line , bool )
{
	G_ASSERT( !line.empty() ) ;
	if( m_output_blocked )
	{
		m_output_buffer.append( line ) ;
	}
	else if( !send( line ) )
	{
		m_output_blocked = true ;
		m_output_buffer.clear() ;
	}
}

void GSmtp::ServerPeer::onSendComplete()
{
	G_ASSERT( m_output_blocked ) ;
	if( send( m_output_buffer ) ) // GNet::ServerPeer::send()
		m_output_blocked = false ;
	m_output_buffer.clear() ;
}

void GSmtp::ServerPeer::protocolShutdown( int how )
{
	if( how >= 0 )
		socket().shutdown( how ) ;
}

void GSmtp::ServerPeer::protocolExpect( std::size_t n )
{
	m_input_buffer.expect( n ) ;
}

void GSmtp::ServerPeer::onDnsBlockResult( bool allow )
{
	if( allow )
		m_protocol.init() ;
	else
		throw GNet::Done() ;
}

void GSmtp::ServerPeer::onCheckTimeout()
{
	// do a better-than-nothing check for an unexpected TLS ClientHello -- false
	// positives are possible but extremely unlikely
	std::string head = m_input_buffer.head() ;
	if( head.size() > 6U && head.at(0U) == '\x16' && head.at(1U) == '\x03' &&
		( head.at(2U) == '\x03' || head.at(2U) == '\x02' || head.at(2U) == '\01' ) )
			G_WARNING( "GSmtp::ServerPeer::doCheck: received unexpected tls handshake packet from remote client: "
				"try enabling implicit tls (smtps)" ) ;
}

// ===

GSmtp::Server::Server( GNet::EventState es , GStore::MessageStore & store , FilterFactoryBase & ff ,
	VerifierFactoryBase & vf , const GAuth::SaslClientSecrets & client_secrets ,
	const GAuth::SaslServerSecrets & server_secrets , const Config & server_config ,
	const std::string & forward_to , int forward_to_family ,
	const GSmtp::Client::Config & client_config ) :
		GNet::MultiServer(es,server_config.interfaces,server_config.port,"smtp",
			server_config.net_server_peer_config,
			server_config.net_server_config) ,
		m_store(store) ,
		m_ff(ff) ,
		m_vf(vf) ,
		m_server_config(server_config) ,
		m_client_config(client_config) ,
		m_server_secrets(server_secrets) ,
		m_forward_to(forward_to) ,
		m_forward_to_family(forward_to_family) ,
		m_client_secrets(client_secrets) ,
		m_dnsbl_suspend_time(G::TimerTime::zero())
{
}

GSmtp::Server::~Server()
{
	serverCleanup() ; // base class early cleanup
}

#ifndef G_LIB_SMALL
GSmtp::Server::Config & GSmtp::Server::config()
{
	return m_server_config ;
}
#endif

G::Slot::Signal<const std::string&,const std::string&> & GSmtp::Server::eventSignal() noexcept
{
	return m_event_signal ;
}

void GSmtp::Server::report( const std::string & group ) const
{
	serverReport( group ) ; // base class
}

std::unique_ptr<GNet::ServerPeer> GSmtp::Server::newPeer( GNet::EventStateUnbound esu ,
	GNet::ServerPeerInfo && peer_info , GNet::MultiServer::ServerInfo )
{
	using G::format ;
	using G::txt ;
	std::unique_ptr<ServerPeer> ptr ;
	try
	{
		std::string reason ;
		if( ! m_server_config.allow_remote && !peer_info.m_address.isLocal(reason) )
		{
			G_WARNING( "GSmtp::Server: "
				<< format(txt("configured to reject non-local smtp connection: %1%")) % reason ) ;
		}
		else
		{
			GNet::Address peer_address = peer_info.m_address ;
			ptr = std::make_unique<ServerPeer>( esu , std::move(peer_info) , *this ,
				m_enabled , m_vf , m_server_secrets , serverConfig() ,
				newProtocolText(m_server_config.anonymous_smtp,m_server_config.anonymous_content,peer_address,m_server_config.domain) ) ;
		}
	}
	catch( std::exception & e ) // newPeer()
	{
		G_WARNING( "GSmtp::Server: new connection error: " << e.what() ) ;
	}
	return ptr ;
}

GSmtp::Server::Config GSmtp::Server::serverConfig() const
{
	if( !m_dnsbl_suspend_time.isZero() && G::TimerTime::now() < m_dnsbl_suspend_time )
		return Config(m_server_config).set_dnsbl_config({}) ;
	return m_server_config ;
}

void GSmtp::Server::nodnsbl( unsigned int s )
{
	G_LOG( "GSmtp::Server::nodnsbl: dnsbl " << (s?"disabled":"enabled") << (s?(" for "+G::Str::fromUInt(s).append(1U,'s')):"") ) ;
	m_dnsbl_suspend_time = G::TimerTime::now() + G::TimeInterval(s) ;
}

void GSmtp::Server::enable( bool b )
{
	m_enabled = b ;
}

std::unique_ptr<GSmtp::ServerProtocol::Text> GSmtp::Server::newProtocolText( bool anonymous_smtp ,
	bool anonymous_content , const GNet::Address & peer_address , const std::string & domain ) const
{
	const bool with_received_line = !anonymous_content ;
	return std::make_unique<ServerText>( m_server_config.ident , anonymous_smtp , with_received_line ,
		domain , peer_address ) ;
}

std::unique_ptr<GSmtp::Filter> GSmtp::Server::newFilter( GNet::EventState es ) const
{
	return m_ff.newFilter( es , Filter::Type::server , m_server_config.filter_config ,
		m_server_config.filter_spec ) ;
}

std::unique_ptr<GSmtp::ProtocolMessage> GSmtp::Server::newProtocolMessageStore( std::unique_ptr<Filter> filter )
{
	return std::make_unique<ProtocolMessageStore>( m_store , std::move(filter) ) ;
}

std::unique_ptr<GSmtp::ProtocolMessage> GSmtp::Server::newProtocolMessageForward( GNet::EventState es ,
	std::unique_ptr<ProtocolMessage> pm )
{
	// wrap the given 'store' object in a 'forward' one
	return std::make_unique<ProtocolMessageForward>( es , m_store , m_ff , std::move(pm) , m_client_config ,
		m_client_secrets , m_forward_to , m_forward_to_family ) ; // up-cast
}

std::unique_ptr<GSmtp::ProtocolMessage> GSmtp::Server::newProtocolMessage( GNet::EventState es )
{
	const bool do_forward = ! m_forward_to.empty() ;
	return do_forward ?
		newProtocolMessageForward( es , newProtocolMessageStore(newFilter(es)) ) :
		newProtocolMessageStore( newFilter(es) ) ;
}

