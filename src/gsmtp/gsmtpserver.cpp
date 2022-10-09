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
#include "gresolver.h"
#include "gprotocolmessagestore.h"
#include "gprotocolmessageforward.h"
#include "gfilterfactory.h"
#include "gverifierfactory.h"
#include "glocal.h"
#include "ggettext.h"
#include "gformat.h"
#include "glog.h"
#include "gassert.h"
#include "gtest.h"
#include <string>
#include <utility>
#include <functional>

namespace GSmtp
{
	struct AnonymousText : public ServerProtocol::Text /// Provides anodyne SMTP protocol text.
	{
		AnonymousText( bool with_received_line , const std::string & this_host , const GNet::Address & peer_address ) ;
		std::string greeting() const override ;
		std::string hello( const std::string & peer_name ) const override ;
		std::string received( const std::string & smtp_peer_name ,
			bool auth , bool secure , const std::string & protocol ,
			const std::string & cipher ) const override ;

		bool m_with_received_line ;
		std::string m_thishost ;
		GNet::Address m_peer_address ;
	} ;
}

GSmtp::AnonymousText::AnonymousText( bool with_received_line , const std::string & thishost , const GNet::Address & peer_address ) :
	m_with_received_line(with_received_line) ,
	m_thishost(thishost) ,
	m_peer_address(peer_address)
{
}

std::string GSmtp::AnonymousText::greeting() const
{
	return "ready" ;
}

std::string GSmtp::AnonymousText::hello( const std::string & ) const
{
	return "smtp says hello" ;
}

std::string GSmtp::AnonymousText::received( const std::string & smtp_peer_name ,
	bool authenticated , bool secure , const std::string & protocol , const std::string & cipher ) const
{
	if( m_with_received_line )
		return ServerProtocolText::receivedLine( smtp_peer_name , m_peer_address.hostPartString() , m_thishost ,
			authenticated , secure , protocol , cipher ) ;
	else
		return {} ;
}

// ===

GSmtp::ServerPeer::ServerPeer( GNet::ExceptionSinkUnbound esu ,
	GNet::ServerPeerInfo && peer_info , Server & server ,
	const GAuth::SaslServerSecrets & server_secrets , const Server::Config & server_config ,
	std::unique_ptr<ServerProtocol::Text> ptext ) :
		GNet::ServerPeer(esu.bind(this),std::move(peer_info),GNet::LineBufferConfig::transparent()) ,
		m_server(server) ,
		m_block(std::bind(&ServerPeer::onDnsBlockResult,this,std::placeholders::_1),esu.bind(this),server_config.dnsbl_config) ,
		m_flush_timer(*this,&ServerPeer::onFlushTimeout,esu.bind(this)) ,
		m_check_timer(*this,&ServerPeer::onCheckTimeout,esu.bind(this)) ,
		m_verifier(VerifierFactory::newVerifier(esu.bind(this),
			server_config.verifier_spec,server_config.verifier_timeout)) ,
		m_pmessage(server.newProtocolMessage(esu.bind(this))) ,
		m_ptext(ptext.release()) ,
		m_line_buffer(GNet::LineBufferConfig::smtp()) ,
		m_protocol(*this,*m_verifier,*m_pmessage,server_secrets,server_config.sasl_server_config,
			*m_ptext,peerAddress(),server_config.protocol_config)
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

void GSmtp::ServerPeer::onSendComplete()
{
	// never gets here -- see protocolSend()
}

void GSmtp::ServerPeer::onData( const char * data , std::size_t size )
{
	// discard anything received before we have even sent an initial greeting
	if( m_block.busy() )
		return ;

	// just buffer up anything received in a half-duplex busy state
	if( m_protocol.halfDuplexBusy(data,size) )
	{
		m_line_buffer.add( data , size ) ;
		return ;
	}

	GNet::ServerPeer::onData( data , size ) ;
}

bool GSmtp::ServerPeer::onReceive( const char * data , std::size_t size , std::size_t , std::size_t , char )
{
	G_ASSERT( size != 0U ) ; if( size == 0U ) return true ;
	m_line_buffer.apply( &m_protocol , &ServerProtocol::apply , data , size , &ServerProtocol::inDataState ) ;

	// bad clients can send multiple commands at once, so if we have any
	// residue in the line buffer we have to deal with it once the protocol
	// has finished with the current one -- see protocolSend()
	if( m_protocol.halfDuplexBusy() && !m_line_buffer.state().empty() )
	{
		G_WARNING( "GSmtp::ServerPeer::onReceive: smtp client protocol violation: pipelining detected" ) ;
		m_flush_timer.startTimer( G::TimeInterval::limit() ) ; // heat-death timeout
	}

	return true ;
}

void GSmtp::ServerPeer::onFlushTimeout()
{
	m_line_buffer.apply( &m_protocol , &ServerProtocol::apply , nullptr ,
		std::size_t(0U) , &ServerProtocol::inDataState ) ;
}

void GSmtp::ServerPeer::onSecure( const std::string & certificate , const std::string & protocol ,
	const std::string & cipher )
{
	m_protocol.secure( certificate , protocol , cipher ) ;
}

void GSmtp::ServerPeer::protocolSend( const std::string & line , bool go_secure )
{
	if( !send( line ) ) // GNet::ServerPeer::send()
		throw SendError() ; // we only send short half-duplex responses, so treat flow control as fatal

	if( go_secure )
		secureAccept() ;

	if( m_flush_timer.active() )
	{
		G_DEBUG( "GSmtp::ServerPeer::protocolSend: pipeline released (" << m_line_buffer.state().size() << ")" ) ;
		m_flush_timer.startTimer( 0U ) ;
	}
}

void GSmtp::ServerPeer::protocolShutdown()
{
	m_flush_timer.cancelTimer() ;
	socket().shutdown() ; // fwiw
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
	std::string head = m_line_buffer.state().head() ;
	if( head.size() > 6U && head.at(0U) == '\x16' && head.at(1U) == '\x03' &&
		( head.at(2U) == '\x03' || head.at(2U) == '\x02' || head.at(2U) == '\01' ) )
			G_WARNING( "GSmtp::ServerPeer::doCheck: received unexpected tls handshake packet from remote client: "
				"try enabling implicit tls (smtps)" ) ;
}

// ===

GSmtp::Server::Server( GNet::ExceptionSink es , MessageStore & store , FilterFactory & ff ,
	const GAuth::SaslClientSecrets & client_secrets , const GAuth::SaslServerSecrets & server_secrets ,
	const Config & server_config , const std::string & forward_to , int forward_to_family ,
	const GSmtp::Client::Config & client_config ) :
		GNet::MultiServer(es,server_config.interfaces,server_config.port,"smtp",server_config.server_peer_config,server_config.server_config) ,
		m_store(store) ,
		m_ff(ff) ,
		m_server_config(server_config) ,
		m_client_config(client_config) ,
		m_server_secrets(server_secrets) ,
		m_forward_to(forward_to) ,
		m_forward_to_family(forward_to_family) ,
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
				newProtocolText(m_server_config.anonymous_smtp,m_server_config.anonymous_content,peer_address) ) ;
		}
	}
	catch( std::exception & e ) // newPeer()
	{
		G_WARNING( "GSmtp::Server: new connection error: " << e.what() ) ;
	}
	return std::unique_ptr<GNet::ServerPeer>( ptr.release() ) ; // up-cast
}

std::unique_ptr<GSmtp::ServerProtocol::Text> GSmtp::Server::newProtocolText( bool anonymous_smtp ,
	bool anonymous_content , const GNet::Address & peer_address ) const
{
	if( anonymous_smtp )
		return std::make_unique<AnonymousText>( !anonymous_content ,
			GNet::Local::canonicalName() , peer_address ) ;
	else
		return std::make_unique<ServerProtocolText>( m_server_config.ident ,
			!anonymous_content , GNet::Local::canonicalName() , peer_address ) ;
}

std::unique_ptr<GSmtp::Filter> GSmtp::Server::newFilter( GNet::ExceptionSink es ) const
{
	return m_ff.newFilter( es , true , m_server_config.filter_spec , m_server_config.filter_timeout ) ;
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
		m_client_secrets , m_forward_to , m_forward_to_family ) ; // up-cast
}

std::unique_ptr<GSmtp::ProtocolMessage> GSmtp::Server::newProtocolMessage( GNet::ExceptionSink es )
{
	const bool do_forward = ! m_forward_to.empty() ;
	return do_forward ?
		newProtocolMessageForward( es , newProtocolMessageStore(newFilter(es)) ) :
		newProtocolMessageStore( newFilter(es) ) ;
}

