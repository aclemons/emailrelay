//
// Copyright (C) 2001-2005 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gpopserver.h
//

#ifndef G_POP_SERVER_H
#define G_POP_SERVER_H

#include "gdef.h"
#include "gpop.h"
#include "gserver.h"
#include "glinebuffer.h"
#include "gpopsecrets.h"
#include "gpopserverprotocol.h"
#include "gexception.h"
#include <string>
#include <sstream>
#include <memory>
#include <list>

namespace GPop
{
	class Server ;
	class ServerPeer ;
}

// Class: GPop::ServerPeer
// Description: Represents a connection from a POP client.
// Instances are created on the heap by Server (only).
// See also: GPop::Server
//
class GPop::ServerPeer : public GNet::ServerPeer , private GPop::ServerProtocol::Sender 
{
public:
	G_EXCEPTION( SendError , "network send error" ) ;
 
	ServerPeer( GNet::Server::PeerInfo , Server & , Store & , const Secrets & , 
		std::auto_ptr<ServerProtocol::Text> ptext , ServerProtocol::Config ) ;
			// Constructor.

private:
	ServerPeer( const ServerPeer & ) ;
	void operator=( const ServerPeer & ) ;
	virtual bool protocolSend( const std::string & line , size_t ) ; // from ServerProtocol::Sender
	virtual void onDelete() ; // from GNet::ServerPeer
	virtual void onData( const char * , size_t ) ; // from GNet::ServerPeer
	virtual void writeEvent() ; // from GNet::EventHandler
	void processLine( const std::string & line ) ;
	static std::string crlf() ;

private:
	Server & m_server ;
	GNet::LineBuffer m_buffer_in ;
	std::string m_buffer_out ;
	std::auto_ptr<ServerProtocol::Text> m_ptext ; // order dependency
	ServerProtocol m_protocol ; // order dependency -- last
} ;

// Class: GPop::Server
// Description: A POP server class.
//
class GPop::Server : public GNet::Server 
{
public:
	struct Config // A structure containing GPop::Server configuration parameters.
	{
		bool allow_remote ;
		unsigned int port ;
		std::string address ;
		Config() ;
		Config( bool , unsigned int , const std::string & address ) ;
	} ;

	Server( Store & store , const Secrets & , Config ) ;
		// Constructor. The 'secrets' reference is kept.

	virtual GNet::ServerPeer * newPeer( GNet::Server::PeerInfo ) ;
		// From GNet::Server.

	void report() const ;
		// Generates helpful diagnostics after construction.

private:
	ServerProtocol::Text * newProtocolText( GNet::Address ) const ;

private:
	bool m_allow_remote ;
	Store & m_store ;
	const Secrets & m_secrets ;
} ;

#endif
