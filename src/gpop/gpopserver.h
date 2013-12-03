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
///
/// \file gpopserver.h
///

#ifndef G_POP_SERVER_H
#define G_POP_SERVER_H

#include "gdef.h"
#include "gpop.h"
#include "gmultiserver.h"
#include "gbufferedserverpeer.h"
#include "glinebuffer.h"
#include "gpopsecrets.h"
#include "gpopserverprotocol.h"
#include "gexception.h"
#include "gstrings.h"
#include <string>
#include <sstream>
#include <memory>
#include <list>

/// \namespace GPop
namespace GPop
{
	class Server ;
	class ServerPeer ;
}

/// \class GPop::ServerPeer
/// Represents a connection from a POP client.
/// Instances are created on the heap by Server (only).
/// \see GPop::Server
///
class GPop::ServerPeer : public GNet::BufferedServerPeer , private GPop::ServerProtocol::Sender , private GPop::ServerProtocol::Security 
{
public:
	G_EXCEPTION( SendError , "network send error" ) ;
 
	ServerPeer( GNet::Server::PeerInfo , Server & , Store & , const Secrets & , 
		std::auto_ptr<ServerProtocol::Text> ptext , ServerProtocol::Config ) ;
			///< Constructor.

	virtual bool protocolSend( const std::string & line , size_t ) ; 
		///< Final override from GPop::ServerProtocol::Sender.

protected:
	virtual void onDelete( const std::string & ) ; 
		///< Final override from GNet::ServerPeer.

	virtual bool onReceive( const std::string & ) ; 
		///< Final override from GNet::BufferedServerPeer.

	virtual void onSecure( const std::string & ) ;
		///< Final override from GNet::SocketProtocolSink.

	virtual void onSendComplete() ; 
		///< Final override from GNet::BufferedServerPeer.

	virtual bool securityEnabled() const ;
		///< Final override from GPop::ServerProtocol::Security.

	virtual void securityStart() ;
		///< Final override from GPop::ServerProtocol::Security.

private:
	ServerPeer( const ServerPeer & ) ;
	void operator=( const ServerPeer & ) ;
	void processLine( const std::string & line ) ;
	static const std::string & crlf() ;

private:
	Server & m_server ;
	std::auto_ptr<ServerProtocol::Text> m_ptext ; // order dependency
	ServerProtocol m_protocol ; // order dependency -- last
} ;

/// \class GPop::Server
/// A POP server class.
///
class GPop::Server : public GNet::MultiServer 
{
public:
	G_EXCEPTION( Overflow , "too many interface addresses" ) ;
	/// A structure containing GPop::Server configuration parameters.
	struct Config 
	{
		bool allow_remote ;
		unsigned int port ;
		G::Strings interfaces ;
		Config() ;
		Config( bool , unsigned int , const G::Strings & interfaces ) ;
	} ;

	Server( Store & store , const Secrets & , Config ) ;
		///< Constructor. The 'secrets' reference is kept.

	virtual ~Server() ;
		///< Destructor.

	GNet::ServerPeer * newPeer( GNet::Server::PeerInfo ) ;
		///< From MultiServer.

	void report() const ;
		///< Generates helpful diagnostics after construction.

private:
	Server( const Server & ) ; // not implemented
	void operator=( const Server & ) ; // not implemented
	ServerProtocol::Text * newProtocolText( GNet::Address ) const ;

private:
	bool m_allow_remote ;
	Store & m_store ;
	const Secrets & m_secrets ;
} ;

#endif
