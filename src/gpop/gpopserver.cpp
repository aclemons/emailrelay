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
// gpopserver.cpp
//

#include "gdef.h"
#include "gpopserver.h"
#include "gsocketprotocol.h"
#include "glocal.h"
#include "glog.h"
#include <string>

GPop::ServerPeer::ServerPeer( GNet::ExceptionSinkUnbound esu , GNet::ServerPeerInfo peer_info , Store & store ,
	const GAuth::SaslServerSecrets & server_secrets , const std::string & sasl_server_config ,
	unique_ptr<ServerProtocol::Text> ptext , const ServerProtocol::Config & protocol_config ) :
		GNet::ServerPeer(esu.bind(this),peer_info,GNet::LineBufferConfig::pop()) ,
		m_ptext(ptext.release()) ,
		m_protocol(*this,*this,store,server_secrets,sasl_server_config,*m_ptext.get(),peer_info.m_address,protocol_config)
{
	G_LOG_S( "GPop::ServerPeer: pop connection from " << peer_info.m_address.displayString() ) ;
	m_protocol.init() ;
}

void GPop::ServerPeer::onDelete( const std::string & reason )
{
	G_LOG_S( "GPop::ServerPeer: pop connection closed: " << reason << (reason.empty()?"":": ")
		<< peerAddress().second.displayString() ) ;
}

bool GPop::ServerPeer::onReceive( const char * line_data , size_t line_size , size_t , size_t , char )
{
	processLine( std::string(line_data,line_size) ) ;
	return true ;
}

void GPop::ServerPeer::processLine( const std::string & line )
{
	m_protocol.apply( line ) ;
}

bool GPop::ServerPeer::protocolSend( const std::string & line , size_t offset )
{
	return send( line , offset ) ; // ServerPeer::send()
}

void GPop::ServerPeer::onSendComplete()
{
	m_protocol.resume() ; // calls back to protocolSend()
}

bool GPop::ServerPeer::securityEnabled() const
{
	// require a tls server certificate -- see GSsl::Library::addProfile()
	bool enabled = GNet::SocketProtocol::secureAcceptCapable() ;
	G_DEBUG( "ServerPeer::securityEnabled: tls library " << (enabled?"enabled":"disabled") ) ;
	return enabled ;
}

void GPop::ServerPeer::securityStart()
{
	secureAccept() ; // base class
}

void GPop::ServerPeer::onSecure( const std::string & /*certificate*/ , const std::string & )
{
	m_protocol.secure() ;
}

// ===

GPop::Server::Server( GNet::ExceptionSink es , Store & store , const GAuth::SaslServerSecrets & secrets , const Config & config ) :
	GNet::MultiServer(es,GNet::MultiServer::addressList(config.addresses,config.port),config.server_peer_config) ,
	m_config(config) ,
	m_store(store) ,
	m_secrets(secrets)
{
}

GPop::Server::~Server()
{
	serverCleanup() ; // base class early cleanup
}

void GPop::Server::report() const
{
	serverReport( "pop" ) ; // base class implementation
	G_LOG_S( "GPop::Server: pop authentication secrets from \"" << m_secrets.source() << "\"" ) ;
}

unique_ptr<GNet::ServerPeer> GPop::Server::newPeer( GNet::ExceptionSinkUnbound esu , GNet::ServerPeerInfo peer_info , GNet::MultiServer::ServerInfo )
{
	unique_ptr<GNet::ServerPeer> ptr ;
	try
	{
		std::string reason ;
		if( ! m_config.allow_remote && ! GNet::Local::isLocal(peer_info.m_address,reason) )
		{
			G_WARNING( "GPop::Server: configured to reject non-local pop connection: " << reason ) ;
		}
		else
		{
			ptr.reset( new ServerPeer( esu , peer_info , m_store , m_secrets , m_config.sasl_server_config ,
				newProtocolText(peer_info.m_address) , ServerProtocol::Config() ) ) ;
		}
	}
	catch( std::exception & e ) // newPeer()
	{
		G_WARNING( "GPop::Server: new connection error: " << e.what() ) ;
	}
	return ptr ;
}

unique_ptr<GPop::ServerProtocol::Text> GPop::Server::newProtocolText( GNet::Address peer_address ) const
{
	return unique_ptr<ServerProtocol::Text>( new ServerProtocolText(peer_address) ) ;
}

// ===

GPop::Server::Config::Config() :
	allow_remote(false) ,
	port(110) ,
	server_peer_config(0U)
{
}

GPop::Server::Config::Config( bool allow_remote_ , unsigned int port_ , const G::StringArray & addresses_ ,
	const GNet::ServerPeerConfig & server_peer_config_ , const std::string & sasl_server_config_ ) :
		allow_remote(allow_remote_) ,
		port(port_) ,
		addresses(addresses_) ,
		server_peer_config(server_peer_config_) ,
		sasl_server_config(sasl_server_config_)
{
}

/// \file gpopserver.cpp
