//
// Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gsmtpserver.h
//

#ifndef G_SMTP_SERVER_H
#define G_SMTP_SERVER_H

#include "gdef.h"
#include "gsmtp.h"
#include "gnoncopyable.h"
#include "gserver.h"
#include "glinebuffer.h"
#include "gverifier.h"
#include "gmessagestore.h"
#include "gserverprotocol.h"
#include "gprotocolmessage.h"
#include "gexception.h"
#include <string>
#include <sstream>
#include <memory>
#include <list>

namespace GSmtp
{
	class Server ;
	class ServerPeer ;
	class ServerImp ;
}

// Class: GSmtp::ServerPeer
// Description: Represents a connection from an SMTP client.
// Instances are created on the heap by Server (only).
// See also: GSmtp::Server
//
class GSmtp::ServerPeer : public GNet::ServerPeer , private GSmtp::ServerProtocol::Sender 
{
public:
	ServerPeer( GNet::Server::PeerInfo , Server & server , 
		std::auto_ptr<ProtocolMessage> pmessage , 
		const std::string & ident , const Secrets & , const Verifier & verifier ) ;
			// Constructor.

private:
	ServerPeer( const ServerPeer & ) ;
	void operator=( const ServerPeer & ) ;
	virtual void protocolSend( const std::string & line ) ; // from ServerProtocol::Sender
	virtual void protocolDone() ; // from ServerProtocol::Sender
	virtual void onDelete() ; // from GNet::ServerPeer
	virtual void onData( const char * , size_t ) ; // from GNet::ServerPeer
	bool processLine( const std::string & line ) ;
	std::string thishost() const ;
	static std::string crlf() ;

private:
	Server & m_server ;
	GNet::LineBuffer m_buffer ;
	Verifier m_verifier ; // order dependency -- first
	std::auto_ptr<ProtocolMessage> m_pmessage ; // order dependency -- second
	ServerProtocol m_protocol ; // order dependency -- third
} ;

// Class: GSmtp::ServerImp
// Description: A private implementation class for GSmtp::Server.
//
class GSmtp::ServerImp : public GNet::Server 
{
public:
	explicit ServerImp( GSmtp::Server & ) ;
		// Constructor.

	virtual GNet::ServerPeer * newPeer( GNet::Server::PeerInfo ) ;
		// ServerPeer factory method.

private:
	ServerImp( const ServerImp & ) ; // not implemented
	void operator=( const ServerImp & ) ; // not implemented

private:
	GSmtp::Server & m_server ;
} ;

// Class: GSmtp::Server
// Description: An SMTP server class.
//
class GSmtp::Server : public G::noncopyable 
{
public:
	typedef std::list<GNet::Address> AddressList ;
	G_EXCEPTION( Overflow , "too many interface addresses" ) ;

	Server( MessageStore & store , 
		const Secrets & server_secrets , const Verifier & verifier ,
		const std::string & ident , bool allow_remote ,
		unsigned int port , const AddressList & interfaces ,
		const std::string & smtp_server_address ,
		unsigned int smtp_response_timeout ,
		unsigned int smtp_connection_timeout ,
		const Secrets & client_secrets ,
		const std::string & scanner_address ,
		unsigned int scanner_response_timeout ,
		unsigned int scanner_connection_timeout ) ;
			// Constructor. Listens on the given port number
			// using INET_ANY if 'interfaces' is empty, or
			// on specific interfaces otherwise. Currently
			// only three interface addresses are supported.
			//
			// If the 'downstream-server-address' parameter is
			// given then all messages are forwarded immediately,
			// using the specified client-side timeout values
			// and client-side secrets.
			//
			// If the 'downstream-server-address' parameter is
			// empty then the timeout values are ignored.
			//
			// The 'store' and 'secrets' references are kept.

	void report() const ;
		// Generates helpful diagnostics after construction.

	GNet::ServerPeer * newPeer( GNet::Server::PeerInfo ) ;
		// ServerPeer factory method used by ServerImp.

private:
	void bind( ServerImp & , GNet::Address , unsigned int ) ;
	ProtocolMessage * newProtocolMessage() ;
	ServerImp & imp( size_t n ) ;

private:
	MessageStore & m_store ;
	std::string m_ident ;
	bool m_allow_remote ;
	const Secrets & m_server_secrets ;
	Verifier m_verifier ;
	std::string m_smtp_server ;
	unsigned int m_smtp_response_timeout ;
	unsigned int m_smtp_connection_timeout ;
	std::string m_scanner_server ;
	unsigned int m_scanner_response_timeout ;
	unsigned int m_scanner_connection_timeout ;
	const Secrets & m_client_secrets ;
	ServerImp m_gnet_server_1 ;
	ServerImp m_gnet_server_2 ;
	ServerImp m_gnet_server_3 ;
} ;

#endif
