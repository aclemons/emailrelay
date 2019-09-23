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
// gsmtpserver.cpp
//

#include "gdef.h"
#include "gsmtpserver.h"
#include "gresolver.h"
#include "gdnsblock.h"
#include "gprotocolmessagestore.h"
#include "gprotocolmessageforward.h"
#include "gfilterfactory.h"
#include "gverifierfactory.h"
#include "glocal.h"
#include "glog.h"
#include "gassert.h"
#include "gtest.h"
#include <string>
#include <functional>

namespace
{
	struct AnonymousText : public GSmtp::ServerProtocol::Text
	{
		explicit AnonymousText( const std::string & = std::string() ) ;
		virtual std::string greeting() const override ;
		virtual std::string hello( const std::string & peer_name ) const override ;
		virtual std::string received( const std::string & smtp_peer_name ,
			bool auth , bool secure , const std::string & cipher ) const override ;
		std::string m_thishost ;
	} ;
}

AnonymousText::AnonymousText( const std::string & thishost ) :
	m_thishost(thishost)
{
	if( m_thishost.empty() )
		m_thishost = "smtp" ;
}

std::string AnonymousText::greeting() const
{
	return "ready" ;
}

std::string AnonymousText::hello( const std::string & ) const
{
	return m_thishost + " says hello" ;
}

std::string AnonymousText::received( const std::string & , bool , bool , const std::string & ) const
{
	return std::string() ; // no Received line
}

// ===

GSmtp::ServerPeer::ServerPeer( GNet::ExceptionSinkUnbound esu , GNet::ServerPeerInfo peer_info , Server & server ,
	const GAuth::Secrets & server_secrets , const Server::Config & server_config ,
	unique_ptr<ServerProtocol::Text> ptext ) :
		GNet::ServerPeer(esu.bind(this),peer_info,GNet::LineBufferConfig::transparent()) ,
		m_server(server) ,
		m_block(*this,esu.bind(this),server_config.dnsbl_config) ,
		m_verifier(VerifierFactory::newVerifier(esu.bind(this),server_config.verifier_address,server_config.verifier_timeout)) ,
		m_pmessage(server.newProtocolMessage(esu.bind(this))) ,
		m_ptext(ptext.release()) ,
		m_line_buffer(GNet::LineBufferConfig::smtp()) ,
		m_protocol(esu.bind(this),*this,*m_verifier.get(),*m_pmessage.get(),server_secrets,server_config.sasl_server_config,
			*m_ptext.get(),peer_info.m_address,server_config.protocol_config)
{
	G_LOG_S( "GSmtp::ServerPeer: smtp connection from " << peer_info.m_address.displayString() ) ;
	if( server_config.dnsbl_config.empty() )
		m_protocol.init() ;
	else
		m_block.start( peer_info.m_address ) ;
}

void GSmtp::ServerPeer::onDelete( const std::string & reason )
{
	G_LOG_S( "GSmtp::ServerPeer: smtp connection closed: " << reason << (reason.empty()?"":": ")
		<< peerAddress().second.displayString() ) ;

	m_server.eventSignal().emit( "done" , reason ) ;
}

void GSmtp::ServerPeer::onSendComplete()
{
	// never gets here -- see protocolSend()
}

void GSmtp::ServerPeer::onData( const char * data , size_t size )
{
	if( !m_block.busy() ) // DoS prevention
		GNet::ServerPeer::onData( data , size ) ;
}

bool GSmtp::ServerPeer::onReceive( const char * data , size_t size , size_t , size_t , char )
{
	G_ASSERT( size != 0U ) ; if( size == 0U ) return true ;
	m_line_buffer.apply( &m_protocol , &ServerProtocol::apply , data , size , &ServerProtocol::inDataState ) ;
	return true ;
}

void GSmtp::ServerPeer::onSecure( const std::string & certificate , const std::string & cipher )
{
	m_protocol.secure( certificate , cipher ) ;
}

void GSmtp::ServerPeer::protocolSend( const std::string & line , bool go_secure )
{
	if( !send( line , 0U ) ) // GNet::ServerPeer::send()
		throw SendError() ; // we only send short half-duplex responses, so treat flow control as fatal

	if( go_secure )
		secureAccept() ;
}

void GSmtp::ServerPeer::protocolShutdown()
{
	socket().shutdown() ; // fwiw
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

// ===

GSmtp::Server::Server( GNet::ExceptionSink es , MessageStore & store ,
	const GAuth::Secrets & client_secrets , const GAuth::Secrets & server_secrets ,
	Config server_config , std::string forward_to , GSmtp::Client::Config client_config ) :
		GNet::MultiServer( es , GNet::MultiServer::addressList(server_config.interfaces,server_config.port) , server_config.server_peer_config ) ,
		m_store(store) ,
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

G::Slot::Signal2<std::string,std::string> & GSmtp::Server::eventSignal()
{
	return m_event_signal ;
}

void GSmtp::Server::report() const
{
	serverReport( "smtp" ) ; // base class
}

unique_ptr<GNet::ServerPeer> GSmtp::Server::newPeer( GNet::ExceptionSinkUnbound esu , GNet::ServerPeerInfo peer_info , GNet::MultiServer::ServerInfo )
{
	unique_ptr<GNet::ServerPeer> ptr ;
	try
	{
		std::string reason ;
		if( ! m_server_config.allow_remote && ! GNet::Local::isLocal(peer_info.m_address,reason) )
		{
			G_WARNING( "GSmtp::Server: configured to reject non-local smtp connection: " << reason ) ;
		}
		else
		{
			ptr.reset( new ServerPeer( esu , peer_info , *this ,
				m_server_secrets , m_server_config ,
				newProtocolText(m_server_config.anonymous,peer_info.m_address) ) ) ;
		}
	}
	catch( std::exception & e ) // newPeer()
	{
		G_WARNING( "GSmtp::Server: new connection error: " << e.what() ) ;
	}
	return ptr ;
}

unique_ptr<GSmtp::ServerProtocol::Text> GSmtp::Server::newProtocolText( bool anonymous , const GNet::Address & peer_address ) const
{
	if( anonymous )
		return unique_ptr<ServerProtocol::Text>( new AnonymousText ) ;
	else
		return unique_ptr<ServerProtocol::Text>( new ServerProtocolText( m_server_config.ident , GNet::Local::canonicalName() , peer_address ) ) ;
}

unique_ptr<GSmtp::Filter> GSmtp::Server::newFilter( GNet::ExceptionSink es )
{
	return FilterFactory::newFilter( es , true , m_server_config.filter_address , m_server_config.filter_timeout ) ;
}

unique_ptr<GSmtp::ProtocolMessage> GSmtp::Server::newProtocolMessageStore( unique_ptr<Filter> filter )
{
	return unique_ptr<ProtocolMessage>( new ProtocolMessageStore( m_store , unique_ptr<Filter>(filter.release()) ) ) ;
}

unique_ptr<GSmtp::ProtocolMessage> GSmtp::Server::newProtocolMessageForward( GNet::ExceptionSink es , unique_ptr<ProtocolMessage> pm )
{
	// wrap the given 'store' object in a 'forward' one
	return unique_ptr<ProtocolMessage>( new ProtocolMessageForward( es , m_store , unique_ptr<ProtocolMessage>(pm.release()) ,
		m_client_config , m_client_secrets , m_forward_to ) ) ;
}

unique_ptr<GSmtp::ProtocolMessage> GSmtp::Server::newProtocolMessage( GNet::ExceptionSink es )
{
	const bool do_forward = ! m_forward_to.empty() ;
	return do_forward ?
		newProtocolMessageForward( es , newProtocolMessageStore(newFilter(es)) ) :
		newProtocolMessageStore( newFilter(es) ) ;
}

// ===

GSmtp::Server::Config::Config( bool allow_remote_ , unsigned int port_ , const AddressList & interfaces_ ,
	const std::string & ident_ , bool anonymous_ ,
	const std::string & filter_address_ ,
	unsigned int filter_timeout_ ,
	const std::string & verifier_address_ ,
	unsigned int verifier_timeout_ ,
	GNet::ServerPeerConfig server_peer_config_ ,
	ServerProtocol::Config protocol_config_ ,
	const std::string & sasl_server_config_ ,
	const std::string & dnsbl_config_ ) :
		allow_remote(allow_remote_) ,
		port(port_) ,
		interfaces(interfaces_) ,
		ident(ident_) ,
		anonymous(anonymous_) ,
		filter_address(filter_address_) ,
		filter_timeout(filter_timeout_) ,
		verifier_address(verifier_address_) ,
		verifier_timeout(verifier_timeout_) ,
		server_peer_config(server_peer_config_) ,
		protocol_config(protocol_config_) ,
		sasl_server_config(sasl_server_config_) ,
		dnsbl_config(dnsbl_config_)
{
}

/// \file gsmtpserver.cpp
