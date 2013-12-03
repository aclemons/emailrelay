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
/// \file gsmtpserver.h
///

#ifndef G_SMTP_SERVER_H
#define G_SMTP_SERVER_H

#include "gdef.h"
#include "gsmtp.h"
#include "gnoncopyable.h"
#include "gexecutable.h"
#include "gbufferedserverpeer.h"
#include "gmultiserver.h"
#include "gsmtpclient.h"
#include "gverifier.h"
#include "gmessagestore.h"
#include "gserverprotocol.h"
#include "gprotocolmessage.h"
#include "gconnectionlookup.h"
#include "gexception.h"
#include <string>
#include <sstream>
#include <memory>
#include <list>

/// \namespace GSmtp
namespace GSmtp
{
	class Server ;
	class ServerPeer ;
}

/// \class GSmtp::ServerPeer
/// Represents a connection from an SMTP client.
/// Instances are created on the heap by Server (only).
/// \see GSmtp::Server
///
class GSmtp::ServerPeer : public GNet::BufferedServerPeer , private GSmtp::ServerProtocol::Sender 
{
public:
	ServerPeer( GNet::Server::PeerInfo , Server & server , std::auto_ptr<ProtocolMessage> pmessage , 
		const GAuth::Secrets & , const std::string & verifier_address , unsigned int verifier_timeout ,
		std::auto_ptr<ServerProtocol::Text> ptext , ServerProtocol::Config ) ;
			///< Constructor.

protected:
	virtual void onSendComplete() ; 
		///< Final override from GNet::BufferedServerPeer.

	virtual void onDelete( const std::string & reason ) ; 
		///< Final override from GNet::ServerPeer.

	virtual bool onReceive( const std::string & line ) ; 
		///< Final override from GNet::BufferedServerPeer.

	virtual void onSecure( const std::string & ) ;
		///< Final override from GNet::SocketProtocolSink.

private:
	ServerPeer( const ServerPeer & ) ;
	void operator=( const ServerPeer & ) ;
	virtual void protocolSend( const std::string & line , bool ) ; // override from private base class
	static const std::string & crlf() ;

private:
	Server & m_server ;
	std::auto_ptr<Verifier> m_verifier ;
	std::auto_ptr<ProtocolMessage> m_pmessage ;
	std::auto_ptr<ServerProtocol::Text> m_ptext ;
	ServerProtocol m_protocol ; // order dependency -- last
} ;

/// \class GSmtp::Server
/// An SMTP server class.
///
class GSmtp::Server : public GNet::MultiServer 
{
public:
	typedef std::list<GNet::Address> AddressList ;
	G_EXCEPTION( Overflow , "too many interface addresses" ) ;

	/// A structure containing GSmtp::Server configuration parameters.
	struct Config 
	{
		bool allow_remote ;
		unsigned int port ;
		AddressList interfaces ;
		///<
		std::string ident ;
		bool anonymous ;
		///<
		std::string processor_address ;
		unsigned int processor_timeout ;
		///<
		std::string verifier_address ;
		unsigned int verifier_timeout ;
		///<
		bool use_connection_lookup ;
		///<
		Config( bool , unsigned int , const AddressList & , const std::string & , bool ,
			const std::string & , unsigned int , const std::string & , unsigned int , bool ) ;
	} ;

	Server( MessageStore & store , const GAuth::Secrets & client_secrets , const GAuth::Secrets & server_secrets ,
		Config server_config , std::string smtp_server_address , unsigned int smtp_connection_timeout ,
		GSmtp::Client::Config client_config ) ;
			///< Constructor. Listens on the given port number using 
			///< INET_ANY if 'interfaces' is empty, or on specific 
			///< interfaces otherwise.
			///<
			///< If the 'smtp-server-address' parameter is given then all 
			///< messages are forwarded immediately, using the specified 
			///< client-side timeout values and client-side secrets.
			///<
			///< If the 'smtp-server-address' parameter is empty then 
			///< the timeout values are ignored.
			///<
			///< The references are kept.

	virtual ~Server() ;
		///< Destructor.

	void report() const ;
		///< Generates helpful diagnostics after construction.

	GNet::ServerPeer * newPeer( GNet::Server::PeerInfo ) ;
		///< From MultiServer.

	G::Signal2<std::string,std::string> & eventSignal() ;
		///< Returns a signal that indicates that something has happened.

private:
	ProtocolMessage * newProtocolMessage() ;
	ProtocolMessage * newProtocolMessageStore( std::auto_ptr<Processor> ) ;
	ProtocolMessage * newProtocolMessageScanner( std::auto_ptr<ProtocolMessage> ) ;
	ProtocolMessage * newProtocolMessageForward( std::auto_ptr<ProtocolMessage> ) ;
	ServerProtocol::Text * newProtocolText( bool , GNet::Address , const std::string & ) const ;

private:
	MessageStore & m_store ;
	std::string m_processor_address ;
	unsigned int m_processor_timeout ;
	GSmtp::Client::Config m_client_config ;
	std::string m_ident ;
	bool m_allow_remote ;
	const GAuth::Secrets & m_server_secrets ;
	std::string m_smtp_server ;
	unsigned int m_smtp_connection_timeout ;
	const GAuth::Secrets & m_client_secrets ;
	std::string m_verifier_address ;
	unsigned int m_verifier_timeout ;
	bool m_anonymous ;
	std::auto_ptr<ServerProtocol::Text> m_protocol_text ;
	G::Signal2<std::string,std::string> m_event_signal ;
	std::auto_ptr<GNet::ConnectionLookup> m_connection_lookup ;
} ;

#endif
