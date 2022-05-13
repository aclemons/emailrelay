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
/// \file gsmtpserver.cpp
///

#include "gdef.h"
#include "gsmtpserver.h"
#include "gsmtpservertext.h"
#include "gdnsblock.h"
#include "gprotocolmessagestore.h"
#include "gprotocolmessageforward.h"
#include "gfilterfactory.h"
#include "gverifierfactory.h"
#include "glocal.h"
#include "ggettext.h"
#include "gformat.h"
#include "glog.h"
#include "gassert.h"
#include <string>
#include <functional>

namespace GSmtp
{
	namespace SmtpServerImp
	{
		struct AnonymousText : public ServerProtocol::Text
		{
			explicit AnonymousText( const std::string & = std::string() ) ;
			std::string greeting() const override ;
			std::string hello( const std::string & peer_name ) const override ;
			std::string received( const std::string & smtp_peer_name ,
				bool auth , bool secure , const std::string & protocol ,
				const std::string & cipher ) const override ;
			std::string m_thishost ;
		} ;
	}
}

GSmtp::SmtpServerImp::AnonymousText::AnonymousText( const std::string & thishost ) :
	m_thishost(thishost)
{
	if( m_thishost.empty() )
		m_thishost = "smtp" ;
}

std::string GSmtp::SmtpServerImp::AnonymousText::greeting() const
{
	return "ready" ;
}

std::string GSmtp::SmtpServerImp::AnonymousText::hello( const std::string & ) const
{
	return m_thishost + " says hello" ;
}

std::string GSmtp::SmtpServerImp::AnonymousText::received( const std::string & , bool , bool ,
	const std::string & , const std::string & ) const
{
	return std::string() ; // no Received line
}

// ===

GSmtp::ServerPeer::ServerPeer( GNet::ExceptionSinkUnbound esu ,
	GNet::ServerPeerInfo && peer_info , Server & server ,
	const GAuth::SaslServerSecrets & server_secrets , const Server::Config & server_config ,
	std::unique_ptr<ServerProtocol::Text> ptext ) :
		GNet::ServerPeer(esu.bind(this),std::move(peer_info),GNet::LineBufferConfig::transparent()) ,
		m_server(server) ,
		m_server_config(server_config) ,
		m_block(*this,esu.bind(this),server_config.dnsbl_config) ,
		m_check_timer(*this,&ServerPeer::onCheckTimeout,esu.bind(this)) ,
		m_verifier(VerifierFactory::newVerifier(esu.bind(this),
			server_config.verifier_address,server_config.verifier_timeout)) ,
		m_pmessage(server.newProtocolMessage(esu.bind(this))) ,
		m_ptext(ptext.release()) ,
		m_protocol(*this,*m_verifier,*m_pmessage,server_secrets,
			server_config.sasl_server_config,
			*m_ptext,peerAddress(),
			server_config.protocol_config) ,
		m_protocol_buffer(esu.bind(this),m_protocol,*this,
			server_config.line_buffer_limit,
			server_config.pipelining_buffer_limit,
			server_config.protocol_config.advertise_pipelining)
{
	G_LOG_S( "GSmtp::ServerPeer: smtp connection from " << peerAddress().displayString() ) ;
	if( server_config.dnsbl_config.empty() )
		m_protocol.init() ;
	else
		m_block.start( peerAddress() ) ;

	if( !server_config.protocol_config.tls_connection )
		m_check_timer.startTimer( 1U ) ;
}

void GSmtp::ServerPeer::onDelete( const std::string & reason )
{
	G_LOG_S( "GSmtp::ServerPeer: smtp connection closed: " << reason << (reason.empty()?"":": ")
		<< peerAddress().displayString() ) ;

	m_server.eventSignal().emit( "done" , std::string(reason) ) ;
}

void GSmtp::ServerPeer::onData( const char * data , std::size_t size )
{
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
	G_ASSERT( size != 0U ) ;
	m_protocol_buffer.apply( data , size ) ;
	return true ;
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

bool GSmtp::ServerPeer::protocolSend( const std::string & line , bool )
{
	return send( line ) ; // GNet::ServerPeer::send()
}

void GSmtp::ServerPeer::onSendComplete()
{
	m_protocol_buffer.sendComplete() ;
}

void GSmtp::ServerPeer::protocolShutdown( int how )
{
	if( how >= 0 )
		socket().shutdown( how ) ;
}

void GSmtp::ServerPeer::protocolExpect( std::size_t )
{
	// never gets here -- see ServerProtocolBuffer
}

void GSmtp::ServerPeer::onDnsBlockResult( const GNet::DnsBlockResult & result )
{
	result.log() ;
	result.warn() ;
	if( result.allow() )
		m_protocol.init() ;
	else
		throw GNet::Done() ;
}

void GSmtp::ServerPeer::onCheckTimeout()
{
	// do a better-than-nothing check for an unexpected TLS ClientHello -- false
	// positives are possible but extremely unlikely
	std::string head = m_protocol_buffer.head() ;
	if( head.size() > 6U && head.at(0U) == '\x16' && head.at(1U) == '\x03' &&
		( head.at(2U) == '\x03' || head.at(2U) == '\x02' || head.at(2U) == '\01' ) )
			G_WARNING( "GSmtp::ServerPeer::doCheck: received unexpected tls handshake packet from remote client: "
				"try enabling implicit tls (smtps)" ) ;
}

// ===

GSmtp::Server::Server( GNet::ExceptionSink es , MessageStore & store , FilterFactory & ff ,
	const GAuth::SaslClientSecrets & client_secrets , const GAuth::SaslServerSecrets & server_secrets ,
	const Config & server_config , const std::string & forward_to ,
	const GSmtp::Client::Config & client_config ) :
		GNet::MultiServer(es,server_config.interfaces,server_config.port,"smtp",
			server_config.net_server_peer_config,
			server_config.net_server_config) ,
		m_store(store) ,
		m_ff(ff) ,
		m_server_config(server_config) ,
		m_client_config(client_config) ,
		m_server_secrets(server_secrets) ,
		m_forward_to(forward_to) ,
		m_client_secrets(client_secrets)
{
}

GSmtp::Server::~Server()
{
	serverCleanup() ; // base class early cleanup
}

G::Slot::Signal<const std::string&,const std::string&> & GSmtp::Server::eventSignal()
{
	return m_event_signal ;
}

void GSmtp::Server::report() const
{
	serverReport() ; // base class
}

std::unique_ptr<GNet::ServerPeer> GSmtp::Server::newPeer( GNet::ExceptionSinkUnbound esu ,
	GNet::ServerPeerInfo && peer_info , GNet::MultiServer::ServerInfo )
{
	using G::format ;
	using G::txt ;
	std::unique_ptr<ServerPeer> ptr ;
	try
	{
		std::string reason ;
		if( ! m_server_config.allow_remote && ! GNet::Local::isLocal(peer_info.m_address,reason) )
		{
			G_WARNING( "GSmtp::Server: "
				<< format(txt("configured to reject non-local smtp connection: %1%")) % reason ) ;
		}
		else
		{
			GNet::Address peer_address = peer_info.m_address ;
			ptr = std::make_unique<ServerPeer>( esu , std::move(peer_info) , *this ,
				m_server_secrets , m_server_config ,
				newProtocolText(m_server_config.anonymous,peer_address) ) ;
		}
	}
	catch( std::exception & e ) // newPeer()
	{
		G_WARNING( "GSmtp::Server: new connection error: " << e.what() ) ;
	}
	return std::unique_ptr<GNet::ServerPeer>( ptr.release() ) ; // up-cast
}

std::unique_ptr<GSmtp::ServerProtocol::Text> GSmtp::Server::newProtocolText( bool anonymous ,
	const GNet::Address & peer_address ) const
{
	if( anonymous )
		return std::make_unique<SmtpServerImp::AnonymousText>() ; // up-cast
	else
		return std::make_unique<ServerText>( m_server_config.ident ,
			GNet::Local::canonicalName() , peer_address ) ; // up-cast
}

std::unique_ptr<GSmtp::Filter> GSmtp::Server::newFilter( GNet::ExceptionSink es ) const
{
	return m_ff.newFilter( es , true , m_server_config.filter_address , m_server_config.filter_timeout ) ;
}

std::unique_ptr<GSmtp::ProtocolMessage> GSmtp::Server::newProtocolMessageStore( std::unique_ptr<Filter> filter )
{
	return std::make_unique<ProtocolMessageStore>( m_store , std::move(filter) ) ; // up-cast
}

std::unique_ptr<GSmtp::ProtocolMessage> GSmtp::Server::newProtocolMessageForward( GNet::ExceptionSink es ,
	std::unique_ptr<ProtocolMessage> pm )
{
	// wrap the given 'store' object in a 'forward' one
	return std::make_unique<ProtocolMessageForward>( es , m_store , m_ff , std::move(pm) , m_client_config ,
		m_client_secrets , m_forward_to ) ; // up-cast
}

std::unique_ptr<GSmtp::ProtocolMessage> GSmtp::Server::newProtocolMessage( GNet::ExceptionSink es )
{
	const bool do_forward = ! m_forward_to.empty() ;
	return do_forward ?
		newProtocolMessageForward( es , newProtocolMessageStore(newFilter(es)) ) :
		newProtocolMessageStore( newFilter(es) ) ;
}

