//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gpopserver.cpp
//

#include "gdef.h"
#include "gpop.h"
#include "gpopserver.h"
#include "gssl.h"
#include "gmemory.h"
#include "glocal.h"
#include "glog.h"
#include "gdebug.h"
#include "gassert.h"
#include <string>

GPop::ServerPeer::ServerPeer( GNet::Server::PeerInfo peer_info , Server & server , Store & store , 
	const Secrets & secrets , std::auto_ptr<ServerProtocol::Text> ptext ,
	ServerProtocol::Config protocol_config ) :
		GNet::BufferedServerPeer(peer_info,crlf()) ,
		m_server(server) ,
		m_ptext(ptext) ,
		m_protocol(*this,*this,store,secrets,*m_ptext.get(),peer_info.m_address,protocol_config)
{
	G_LOG_S( "GPop::ServerPeer: pop connection from " << peer_info.m_address.displayString() ) ;
	m_protocol.init() ;
}

const std::string & GPop::ServerPeer::crlf()
{
	static const std::string s( "\015\012" ) ;
	return s ;
}

void GPop::ServerPeer::onDelete( const std::string & reason )
{
	G_LOG_S( "GPop::ServerPeer: pop connection closed: " << reason << (reason.empty()?"":": ")
		<< peerAddress().second.displayString() ) ;
}

bool GPop::ServerPeer::onReceive( const std::string & line )
{
	processLine( line ) ;
	return true ;
}

void GPop::ServerPeer::processLine( const std::string & line )
{
	m_protocol.apply( line ) ;
}

bool GPop::ServerPeer::protocolSend( const std::string & line , size_t offset )
{
	return send( line , offset ) ; // BufferedServerPeer::send() -- see also GNet::Sender
}

void GPop::ServerPeer::onSendComplete()
{
	m_protocol.resume() ; // calls back to protocolSend()
}

bool GPop::ServerPeer::securityEnabled() const
{
	GSsl::Library * ssl = GSsl::Library::instance() ;
	bool enabled = ssl != NULL && ssl->enabled(true) ;
	G_DEBUG( "ServerPeer::securityEnabled: ssl library " << (enabled?"enabled":"disabled") ) ;
	return enabled ;
}

void GPop::ServerPeer::securityStart()
{
	sslAccept() ; // base class
}

void GPop::ServerPeer::onSecure( const std::string & certificate )
{
	m_protocol.secure() ;
}

// ===

GPop::Server::Server( Store & store , const Secrets & secrets , Config config ) :
	GNet::MultiServer( GNet::MultiServer::addressList(config.interfaces,config.port) , false ) ,
	m_allow_remote(config.allow_remote) ,
	m_store(store) ,
	m_secrets(secrets)
{
}

GPop::Server::~Server()
{
	// early cleanup -- not really required
	serverCleanup() ; // base class
}

void GPop::Server::report() const
{
	serverReport( "pop" ) ; // base class implementation
	G_LOG_S( "GPop::Server: pop authentication secrets from \"" << m_secrets.path() << "\"" ) ;
}

GNet::ServerPeer * GPop::Server::newPeer( GNet::Server::PeerInfo peer_info )
{
	try
	{
		std::string reason ;
		if( ! m_allow_remote && ! GNet::Local::isLocal(peer_info.m_address,reason) )
		{
			G_WARNING( "GPop::Server: configured to reject non-local pop connection: " << reason ) ;
			return NULL ;
		}

		std::auto_ptr<ServerProtocol::Text> ptext( newProtocolText(peer_info.m_address) ) ;
		return new ServerPeer( peer_info , *this , m_store , m_secrets , 
			ptext , ServerProtocol::Config() ) ;
	}
	catch( std::exception & e ) // newPeer()
	{
		G_WARNING( "GPop::Server: exception from new connection: " << e.what() ) ;
		return NULL ;
	}
}

GPop::ServerProtocol::Text * GPop::Server::newProtocolText( GNet::Address peer_address ) const
{
	return new ServerProtocolText( peer_address ) ;
}

// ===

GPop::Server::Config::Config() :
	allow_remote(false) ,
	port(110)
{
}

GPop::Server::Config::Config( bool allow_remote_ , unsigned int port_ , const G::Strings & interfaces_ ) :
	allow_remote(allow_remote_) ,
	port(port_) ,
	interfaces(interfaces_)
{
}

/// \file gpopserver.cpp
