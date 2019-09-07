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
///
/// \file gpopserver.h
///

#ifndef G_POP_SERVER_H
#define G_POP_SERVER_H

#include "gdef.h"
#include "gmultiserver.h"
#include "glinebuffer.h"
#include "gpopserverprotocol.h"
#include "gsecrets.h"
#include "gexception.h"
#include "gstrings.h"
#include <string>
#include <sstream>
#include <memory>
#include <list>

namespace GPop
{
	class Server ;
	class ServerPeer ;
}

/// \class GPop::ServerPeer
/// Represents a connection from a POP client. Instances are created
/// on the heap by GPop::Server.
/// \see GPop::Server
///
class GPop::ServerPeer : public GNet::ServerPeer , private ServerProtocol::Sender , private ServerProtocol::Security
{
public:
	G_EXCEPTION( SendError , "network send error" ) ;

	ServerPeer( GNet::ExceptionSinkUnbound , GNet::ServerPeerInfo , Store & ,
		const GAuth::SaslServerSecrets & , const std::string & sasl_server_config ,
		unique_ptr<ServerProtocol::Text> ptext , const ServerProtocol::Config & ) ;
			///< Constructor.

private: // overrides
	virtual bool protocolSend( const std::string & line , size_t ) override ; // Override from GPop::ServerProtocol::Sender.
	virtual void onDelete( const std::string & ) override ; // Override from GNet::ServerPeer.
	virtual bool onReceive( const char * , size_t , size_t , size_t , char ) override ; // Override from GNet::ServerPeer.
	virtual void onSecure( const std::string & , const std::string & ) override ; // Override from GNet::SocketProtocolSink.
	virtual void onSendComplete() override ; // Override from GNet::ServerPeer.
	virtual bool securityEnabled() const override ; // Override from GPop::ServerProtocol::Security.
	virtual void securityStart() override ; // Override from GPop::ServerProtocol::Security.

private:
	ServerPeer( const ServerPeer & ) g__eq_delete ;
	void operator=( const ServerPeer & ) g__eq_delete ;
	void processLine( const std::string & line ) ;

private:
	unique_ptr<ServerProtocol::Text> m_ptext ; // order dependency
	ServerProtocol m_protocol ; // order dependency -- last
} ;

/// \class GPop::Server
/// A POP server class.
///
class GPop::Server : public GNet::MultiServer
{
public:
	G_EXCEPTION( Overflow , "too many interface addresses" ) ;
	struct Config /// A structure containing GPop::Server configuration parameters.
	{
		bool allow_remote ;
		unsigned int port ;
		G::StringArray addresses ;
		GNet::ServerPeerConfig server_peer_config ;
		std::string sasl_server_config ;
		Config() ;
		Config( bool , unsigned int port , const G::StringArray & addresses ,
			const GNet::ServerPeerConfig & , const std::string & sasl_server_config ) ;
	} ;

	Server( GNet::ExceptionSink , Store & store , const GAuth::SaslServerSecrets & , const Config & ) ;
		///< Constructor. The 'secrets' reference is kept.

	virtual ~Server() ;
		///< Destructor.

	void report() const ;
		///< Generates helpful diagnostics after construction.

private: // overrides
	virtual unique_ptr<GNet::ServerPeer> newPeer( GNet::ExceptionSinkUnbound , GNet::ServerPeerInfo , GNet::MultiServer::ServerInfo ) override ;

private:
	Server( const Server & ) g__eq_delete ;
	void operator=( const Server & ) g__eq_delete ;
	unique_ptr<ServerProtocol::Text> newProtocolText( GNet::Address ) const ;

private:
	Config m_config ;
	Store & m_store ;
	const GAuth::SaslServerSecrets & m_secrets ;
} ;

#endif
