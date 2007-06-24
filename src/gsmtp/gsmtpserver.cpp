//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// gsmtpserver.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gsmtpserver.h"
#include "gprotocolmessagestore.h"
#include "gprotocolmessageforward.h"
#include "gprotocolmessagescanner.h"
#include "gmemory.h"
#include "glocal.h"
#include "glog.h"
#include "gdebug.h"
#include "gassert.h"
#include <string>

namespace
{
	struct AnonymousText : public GSmtp::ServerProtocol::Text
	{
		AnonymousText( const std::string & thishost , const GNet::Address & peer_address ) ;
		virtual std::string greeting() const ;
		virtual std::string hello( const std::string & peer_name ) const ;
		virtual std::string received( const std::string & peer_name ) const ;
		std::string m_thishost ;
		GNet::Address m_peer_address ;
	} ;
}

AnonymousText::AnonymousText( const std::string & thishost , const GNet::Address & peer_address ) :
	m_thishost(thishost) ,
	m_peer_address(peer_address)
{
}

std::string AnonymousText::greeting() const 
{ 
	return "ready" ; 
}

std::string AnonymousText::hello( const std::string & ) const 
{ 
	return "hello" ; 
}

std::string AnonymousText::received( const std::string & peer_name ) const
{ 
	return GSmtp::ServerProtocolText::receivedLine( peer_name , 
		m_peer_address.displayString(false) , m_thishost ) ;
}

// ===

GSmtp::ServerPeer::ServerPeer( GNet::Server::PeerInfo peer_info ,
	Server & server , std::auto_ptr<ProtocolMessage> pmessage ,
	const Secrets & server_secrets , const Verifier & verifier , 
	std::auto_ptr<ServerProtocol::Text> ptext ,
	ServerProtocol::Config protocol_config ) :
		GNet::BufferedServerPeer( peer_info , crlf() , true ) , // <= throw-on-flow-control
		m_server( server ) ,
		m_verifier( verifier ) ,
		m_pmessage( pmessage ) ,
		m_ptext( ptext ) ,
		m_protocol( *this , m_verifier , *m_pmessage.get() , server_secrets , *m_ptext.get() ,
			peer_info.m_address , protocol_config )
{
	G_LOG_S( "GSmtp::ServerPeer: smtp connection from " << peer_info.m_address.displayString() ) ;
	m_protocol.init() ;
}

std::string GSmtp::ServerPeer::crlf()
{
	return std::string("\015\012") ;
}

void GSmtp::ServerPeer::onDelete()
{
	G_LOG_S( "GSmtp::ServerPeer: smtp connection closed: " << peerAddress().second.displayString() ) ;
}

void GSmtp::ServerPeer::onSendComplete()
{
	// never gets here -- see GNet::Sender ctor
}

bool GSmtp::ServerPeer::onReceive( const std::string & line )
{
	// apply the line to the protocol
	m_protocol.apply( line ) ;
	return true ;
}

void GSmtp::ServerPeer::protocolSend( const std::string & line )
{
	send( line , 0U ) ; // GNet::Sender -- may throw SendError
}

// ===

GSmtp::Server::Server( MessageStore & store , const Secrets & client_secrets , const Secrets & server_secrets ,
	const Verifier & verifier , Config config ,
	std::string smtp_server_address , unsigned int smtp_connection_timeout ,
	GSmtp::Client::Config client_config ) :
		GNet::MultiServer( GNet::MultiServer::addressList(config.interfaces,config.port) ) ,
		m_store(store) ,
		m_newfile_preprocessor(config.newfile_preprocessor) ,
		m_client_config(client_config) ,
		m_ident(config.ident) ,
		m_allow_remote(config.allow_remote) ,
		m_server_secrets(server_secrets) ,
		m_verifier(verifier) ,
		m_smtp_server(smtp_server_address) ,
		m_smtp_connection_timeout(smtp_connection_timeout) ,
		m_scanner_server(config.scanner_server) ,
		m_scanner_response_timeout(config.scanner_response_timeout) ,
		m_scanner_connection_timeout(config.scanner_connection_timeout) ,
		m_client_secrets(client_secrets) ,
		m_anonymous(config.anonymous) ,
		m_preprocessor_timeout(config.preprocessor_timeout)
{
}

GSmtp::Server::~Server()
{
	// early cleanup -- not really required
	serverCleanup() ; // base class
}

void GSmtp::Server::report() const
{
	serverReport( "smtp" ) ; // base class
}

GNet::ServerPeer * GSmtp::Server::newPeer( GNet::Server::PeerInfo peer_info )
{
	try
	{
		std::string reason ;
		if( ! m_allow_remote && ! GNet::Local::isLocal(peer_info.m_address,reason) )
		{
			G_WARNING( "GSmtp::Server: configured to reject non-local connection: " << reason ) ;
			return NULL ;
		}

		std::auto_ptr<ServerProtocol::Text> ptext( newProtocolText(m_anonymous,peer_info.m_address) ) ;
		std::auto_ptr<ProtocolMessage> pmessage( newProtocolMessage() ) ;
		return new ServerPeer( peer_info , *this , pmessage , m_server_secrets , 
			m_verifier , ptext , ServerProtocol::Config(!m_anonymous,m_preprocessor_timeout) ) ;
	}
	catch( std::exception & e )
	{
		G_WARNING( "GSmtp::Server: exception from new connection: " << e.what() ) ;
		return NULL ;
	}
}

GSmtp::ServerProtocol::Text * GSmtp::Server::newProtocolText( bool anonymous , GNet::Address peer_address ) const
{
	if( anonymous )
		return new AnonymousText( GNet::Local::fqdn() , peer_address ) ;
	else
		return new ServerProtocolText( m_ident , GNet::Local::fqdn() , peer_address ) ;
}

GSmtp::ProtocolMessage * GSmtp::Server::newProtocolMessage()
{
	const bool immediate = ! m_smtp_server.empty() ;
	const bool scan = ! m_scanner_server.empty() ;
	if( immediate && scan )
	{
		G_DEBUG( "GSmtp::Server::newProtocolMessage: new ProtocolMessageScanner" ) ;
		return new ProtocolMessageScanner(m_store,m_newfile_preprocessor,m_client_config,
			m_client_secrets,m_smtp_server,m_smtp_connection_timeout,
			m_scanner_server,m_scanner_response_timeout,m_scanner_connection_timeout) ;
	}
	else if( immediate )
	{
		G_DEBUG( "GSmtp::Server::newProtocolMessage: new ProtocolMessageForward" ) ;
		return new ProtocolMessageForward(m_store,m_newfile_preprocessor,m_client_config,
			m_client_secrets,m_smtp_server,m_smtp_connection_timeout) ;
	}
	else
	{
		G_DEBUG( "GSmtp::Server::newProtocolMessage: new ProtocolMessageStore" ) ;
		return new ProtocolMessageStore(m_store,m_newfile_preprocessor) ;
	}
}

// ===

GSmtp::Server::Config::Config( bool allow_remote_ , unsigned int port_ , const AddressList & interfaces_ , 
	const std::string & ident_ , bool anonymous_ ,
	const std::string & scanner_server_ , 
	unsigned int scanner_response_timeout_ , 
	unsigned int scanner_connection_timeout_ , 
	const G::Executable & newfile_preprocessor_ , unsigned int preprocessor_timeout_ ) :
		allow_remote(allow_remote_) ,
		port(port_) ,
		interfaces(interfaces_) ,
		ident(ident_) ,
		anonymous(anonymous_) ,
		scanner_server(scanner_server_) ,
		scanner_response_timeout(scanner_response_timeout_) ,
		scanner_connection_timeout(scanner_connection_timeout_) ,
		newfile_preprocessor(newfile_preprocessor_) ,
		preprocessor_timeout(preprocessor_timeout_)
{
}


/// \file gsmtpserver.cpp
