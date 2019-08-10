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
// gsmtpserver.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gsmtpserver.h"
#include "gresolver.h"
#include "gprotocolmessagestore.h"
#include "gprotocolmessageforward.h"
#include "gfilterfactory.h"
#include "gverifierfactory.h"
#include "glocal.h"
#include "glog.h"
#include "gdebug.h"
#include "gassert.h"
#include "gtest.h"
#include <string>
#include <functional>

namespace
{
	struct AnonymousText : public GSmtp::ServerProtocol::Text
	{
		explicit AnonymousText( const std::string & = std::string() ) ;
		virtual std::string greeting() const ;
		virtual std::string hello( const std::string & peer_name ) const ;
		virtual std::string received( const std::string & smtp_peer_name , bool , bool ) const ;
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

std::string AnonymousText::received( const std::string & smtp_peer_name , bool authenticated , bool secure ) const
{
	return std::string() ; // no Received line
}

// ===

GSmtp::ServerPeer::ServerPeer( GNet::Server::PeerInfo peer_info , Server & server ,
	const GAuth::Secrets & server_secrets , const Server::Config & server_config ,
	unique_ptr<ServerProtocol::Text> ptext ) :
		GNet::ServerPeer(peer_info) ,
		m_server(server) ,
		m_verifier(VerifierFactory::newVerifier(*this,server_config.verifier_address,server_config.verifier_timeout,server_config.verifier_compatibility)) ,
		m_pmessage(server.newProtocolMessage(*this)) ,
		m_ptext(ptext.release()) ,
		m_line_buffer(GNet::LineBufferConfig::smtp()) ,
		m_protocol(*this,*this,*m_verifier.get(),*m_pmessage.get(),server_secrets,*m_ptext.get(),
			peer_info.m_address,server_config.protocol_config)
{
	G_LOG_S( "GSmtp::ServerPeer: smtp connection from " << peer_info.m_address.displayString() ) ;
	m_protocol.init() ;
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

namespace
{
	struct Adaptor
	{
		GSmtp::ServerProtocol & m_protocol ;
		Adaptor( GSmtp::ServerProtocol & protocol ) : m_protocol(protocol) {}
		bool operator()( const char * p , size_t n , size_t e )
		{
			return m_protocol.apply( p , n , e ) ;
		}
	} ;
}

void GSmtp::ServerPeer::onData( const char * line_data , size_t line_size )
{
	G_ASSERT( line_size != 0U ) ; if( line_size == 0U ) return ;
	Adaptor a( m_protocol ) ;
	m_line_buffer.apply( line_data , line_size , a ) ;
}

void GSmtp::ServerPeer::onSecure( const std::string & certificate )
{
	m_protocol.secure( certificate ) ;
}

void GSmtp::ServerPeer::protocolSend( const std::string & line , bool go_secure )
{
	if( !send( line , 0U ) ) // GNet::ServerPeer::send()
		throw SendError() ; // we only send short responses, so treat flow control as fatal

	if( go_secure )
		secureAccept() ;
}

void GSmtp::ServerPeer::protocolShutdown()
{
	socket().shutdown() ; // fwiw
}

// ===

GSmtp::Server::Server( GNet::ExceptionHandler & eh , MessageStore & store ,
	const GAuth::Secrets & client_secrets , const GAuth::Secrets & server_secrets ,
	Config server_config , std::string forward_to , GSmtp::Client::Config client_config ) :
		GNet::MultiServer( eh , GNet::MultiServer::addressList(server_config.interfaces,server_config.port) ) ,
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
	// early cleanup -- not really required
	serverCleanup() ; // base class
}

G::Slot::Signal2<std::string,std::string> & GSmtp::Server::eventSignal()
{
	return m_event_signal ;
}

void GSmtp::Server::report() const
{
	serverReport( "smtp" ) ; // base class
}

GNet::ServerPeer * GSmtp::Server::newPeer( GNet::Server::PeerInfo peer_info , GNet::MultiServer::ServerInfo )
{
	try
	{
		std::string reason ;
		if( ! m_server_config.allow_remote && ! GNet::Local::isLocal(peer_info.m_address,reason) )
		{
			G_WARNING( "GSmtp::Server: configured to reject non-local smtp connection: " << reason ) ;
			return nullptr ;
		}

		return new ServerPeer( peer_info , *this ,
			m_server_secrets , m_server_config ,
			unique_ptr<ServerProtocol::Text>(newProtocolText(m_server_config.anonymous,peer_info.m_address)) ) ;
	}
	catch( std::exception & e ) // newPeer()
	{
		G_WARNING( "GSmtp::Server: exception from new connection: " << e.what() ) ;
		return nullptr ;
	}
}

unique_ptr<GSmtp::ServerProtocol::Text> GSmtp::Server::newProtocolText( bool anonymous , const GNet::Address & peer_address ) const
{
	if( anonymous )
		return unique_ptr<ServerProtocol::Text>( new AnonymousText ) ;
	else
		return unique_ptr<ServerProtocol::Text>( new ServerProtocolText( m_server_config.ident , GNet::Local::canonicalName() , peer_address ) ) ;
}

unique_ptr<GSmtp::Filter> GSmtp::Server::newFilter( ServerPeer & server_peer )
{
	return FilterFactory::newFilter( server_peer , true , m_server_config.filter_address , m_server_config.filter_timeout ) ;
}

unique_ptr<GSmtp::ProtocolMessage> GSmtp::Server::newProtocolMessageStore( unique_ptr<Filter> filter )
{
	return unique_ptr<ProtocolMessage>( new ProtocolMessageStore( m_store , unique_ptr<Filter>(filter.release()) ) ) ;
}

unique_ptr<GSmtp::ProtocolMessage> GSmtp::Server::newProtocolMessageForward( unique_ptr<ProtocolMessage> pm )
{
	// wrap the given 'store' object in a 'forward' one
	return unique_ptr<ProtocolMessage>( new ProtocolMessageForward( m_store , unique_ptr<ProtocolMessage>(pm.release()) ,
		m_client_config , m_client_secrets , m_forward_to ) ) ;
}

unique_ptr<GSmtp::ProtocolMessage> GSmtp::Server::newProtocolMessage( ServerPeer & server_peer )
{
	const bool do_forward = ! m_forward_to.empty() ;
	return do_forward ?
		newProtocolMessageForward( newProtocolMessageStore(newFilter(server_peer)) ) :
		newProtocolMessageStore( newFilter(server_peer) ) ;
}

// ===

GSmtp::Server::Config::Config( bool allow_remote_ , unsigned int port_ , const AddressList & interfaces_ ,
	const std::string & ident_ , bool anonymous_ ,
	const std::string & filter_address_ ,
	unsigned int filter_timeout_ ,
	const std::string & verifier_address_ ,
	unsigned int verifier_timeout_ ,
	bool verifier_compatibility_ ,
	ServerProtocol::Config protocol_config_ ) :
		allow_remote(allow_remote_) ,
		port(port_) ,
		interfaces(interfaces_) ,
		ident(ident_) ,
		anonymous(anonymous_) ,
		filter_address(filter_address_) ,
		filter_timeout(filter_timeout_) ,
		verifier_address(verifier_address_) ,
		verifier_timeout(verifier_timeout_) ,
		verifier_compatibility(verifier_compatibility_) ,
		protocol_config(protocol_config_)
{
}

/// \file gsmtpserver.cpp
