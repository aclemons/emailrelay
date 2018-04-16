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
#include "gexception.h"
#include <string>
#include <sstream>
#include <memory>
#include <list>

namespace GSmtp
{
	class Server ;
	class ServerPeer ;
}

/// \class GSmtp::Server
/// An SMTP server class.
///
class GSmtp::Server : public GNet::MultiServer
{
public:
	typedef std::vector<GNet::Address> AddressList ;
	G_EXCEPTION( Overflow , "too many interface addresses" ) ;

	struct Config /// A structure containing GSmtp::Server configuration parameters.
	{
		bool allow_remote ;
		unsigned int port ;
		AddressList interfaces ;
		std::string ident ;
		bool anonymous ;
		std::string filter_address ;
		unsigned int filter_timeout ;
		std::string verifier_address ;
		unsigned int verifier_timeout ;
		bool verifier_compatibility ;
		ServerProtocol::Config protocol_config ;

		Config( bool allow_remote , unsigned int port , const AddressList & , const std::string & ident ,
			bool anonymous , const std::string & filter_address , unsigned int filter_timeout ,
			const std::string & verifier_adress , unsigned int verifier_timeout , bool verifier_compatibility ,
			ServerProtocol::Config protocol_config ) ;
	} ;

	Server( GNet::ExceptionHandler & eh , MessageStore & store , const GAuth::Secrets & client_secrets ,
		const GAuth::Secrets & server_secrets , Config server_config , std::string forward_to ,
		GSmtp::Client::Config client_config ) ;
			///< Constructor. Listens on the given port number using
			///< INET_ANY if 'interfaces' is empty, or on specific
			///< interfaces otherwise.
			///<
			///< If the forward-to address is given then all messages are
			///< forwarded immediately, using the given client configuration.
			///<
			///< The references are kept.

	virtual ~Server() ;
		///< Destructor.

	void report() const ;
		///< Generates helpful diagnostics after construction.

	G::Slot::Signal2<std::string,std::string> & eventSignal() ;
		///< Returns a signal that indicates that something has happened.

	unique_ptr<ProtocolMessage> newProtocolMessage( ServerPeer & ) ;
		///< Called by GSmtp::ServerPeer to construct a ProtocolMessage.

private:
	unique_ptr<Filter> newFilter( ServerPeer & ) ;
	unique_ptr<ProtocolMessage> newProtocolMessageStore( unique_ptr<Filter> ) ;
	unique_ptr<ProtocolMessage> newProtocolMessageScanner( unique_ptr<ProtocolMessage> ) ;
	unique_ptr<ProtocolMessage> newProtocolMessageForward( unique_ptr<ProtocolMessage> ) ;
	unique_ptr<ServerProtocol::Text> newProtocolText( bool , const GNet::Address & ) const ;
	virtual GNet::ServerPeer * newPeer( GNet::Server::PeerInfo , GNet::MultiServer::ServerInfo ) override ; // GNet::MultiServer

private:
	MessageStore & m_store ;
	Config m_server_config ;
	Client::Config m_client_config ;
	const GAuth::Secrets & m_server_secrets ;
	std::string m_forward_to ;
	const GAuth::Secrets & m_client_secrets ;
	G::Slot::Signal2<std::string,std::string> m_event_signal ;
} ;

/// \class GSmtp::ServerPeer
/// Represents a connection from an SMTP client.
/// \see GSmtp::Server
///
class GSmtp::ServerPeer : public GNet::BufferedServerPeer , private ServerProtocol::Sender
{
public:
	ServerPeer( GNet::Server::PeerInfo peer_info , Server & server ,
		const GAuth::Secrets & server_secrets , const Server::Config & server_config ,
		unique_ptr<ServerProtocol::Text> ptext ) ;
			///< Constructor.

protected:
	virtual void onSendComplete() override ;
		///< Override from GNet::BufferedServerPeer.

	virtual void onDelete( const std::string & reason ) override ;
		///< Override from GNet::ServerPeer.

	virtual bool onReceive( const std::string & line ) override ;
		///< Override from GNet::BufferedServerPeer.

	virtual void onSecure( const std::string & ) override ;
		///< Override from GNet::SocketProtocolSink.

private:
	ServerPeer( const ServerPeer & ) ;
	void operator=( const ServerPeer & ) ;
	virtual void protocolSend( const std::string & line , bool ) override ; // override from private base class

private:
	Server & m_server ;
	bool m_first_line ;
	unique_ptr<Verifier> m_verifier ;
	unique_ptr<ProtocolMessage> m_pmessage ;
	unique_ptr<ServerProtocol::Text> m_ptext ;
	ServerProtocol m_protocol ; // order dependency -- last
} ;

#endif
