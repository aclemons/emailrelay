//
// Copyright (C) 2001-2020 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#ifndef G_SMTP_SERVER__H
#define G_SMTP_SERVER__H

#include "gdef.h"
#include "gmultiserver.h"
#include "gsmtpclient.h"
#include "gdnsblock.h"
#include "glinebuffer.h"
#include "gverifier.h"
#include "gmessagestore.h"
#include "gsmtpserverprotocol.h"
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
	using AddressList = std::vector<GNet::Address> ;

	struct Config /// A structure containing GSmtp::Server configuration parameters.
	{
		bool allow_remote{false} ;
		G::StringArray interfaces ;
		unsigned int port{0U} ;
		std::string ident ;
		bool anonymous{false} ;
		std::string filter_address ;
		unsigned int filter_timeout{0U} ;
		std::string verifier_address ;
		unsigned int verifier_timeout{0U} ;
		GNet::ServerPeerConfig server_peer_config ;
		ServerProtocol::Config protocol_config ;
		std::string sasl_server_config ;
		std::string dnsbl_config ;

		Config() ;
		Config( bool allow_remote , const G::StringArray & interfaces , unsigned int port ,
			const std::string & ident , bool anonymous , const std::string & filter_address ,
			unsigned int filter_timeout , const std::string & verifier_adress ,
			unsigned int verifier_timeout , GNet::ServerPeerConfig server_peer_config ,
			ServerProtocol::Config protocol_config , const std::string & sasl_server_config ,
			const std::string & dnsbl_config ) ;
		Config & set_allow_remote( bool = true ) ;
		Config & set_interfaces( const G::StringArray & ) ;
		Config & set_port( unsigned int ) ;
		Config & set_ident( const std::string & ) ;
		Config & set_anonymous( bool = true ) ;
		Config & set_filter_address( const std::string & ) ;
		Config & set_filter_timeout( unsigned int ) ;
		Config & set_verifier_address( const std::string & ) ;
		Config & set_verifier_timeout( unsigned int ) ;
		Config & set_server_peer_config( const GNet::ServerPeerConfig & ) ;
		Config & set_protocol_config( const ServerProtocol::Config & ) ;
		Config & set_sasl_server_config( const std::string & ) ;
		Config & set_dnsbl_config( const std::string & ) ;
	} ;

	Server( GNet::ExceptionSink es , MessageStore & store ,
		const GAuth::Secrets & client_secrets , const GAuth::Secrets & server_secrets ,
		const Config & server_config , const std::string & forward_to ,
		const GSmtp::Client::Config & client_config ) ;
			///< Constructor. Listens on the given port number using INET_ANY
			///< if 'server_config.interfaces' is empty, or on specific
			///< interfaces otherwise.
			///<
			///< If the forward-to address is given then all messages are
			///< forwarded immediately, using the given client configuration.

	~Server() override ;
		///< Destructor.

	void report() const ;
		///< Generates helpful diagnostics after construction.

	G::Slot::Signal<const std::string&,const std::string&> & eventSignal() ;
		///< Returns a signal that indicates that something has happened.

	std::unique_ptr<ProtocolMessage> newProtocolMessage( GNet::ExceptionSink ) ;
		///< Called by GSmtp::ServerPeer to construct a ProtocolMessage.

private: // overrides
	std::unique_ptr<GNet::ServerPeer> newPeer( GNet::ExceptionSinkUnbound , GNet::ServerPeerInfo , GNet::MultiServer::ServerInfo ) override ; // Override from GNet::MultiServer.

public:
	Server( const Server & ) = delete ;
	Server( Server && ) = delete ;
	void operator=( const Server & ) = delete ;
	void operator=( Server && ) = delete ;

private:
	std::unique_ptr<Filter> newFilter( GNet::ExceptionSink ) const ;
	std::unique_ptr<ProtocolMessage> newProtocolMessageStore( std::unique_ptr<Filter> && ) ;
	std::unique_ptr<ProtocolMessage> newProtocolMessageForward( GNet::ExceptionSink , std::unique_ptr<ProtocolMessage> && ) ;
	std::unique_ptr<ServerProtocol::Text> newProtocolText( bool , const GNet::Address & ) const ;

private:
	MessageStore & m_store ;
	Config m_server_config ;
	Client::Config m_client_config ;
	const GAuth::Secrets & m_server_secrets ;
	std::string m_sasl_server_config ;
	std::string m_forward_to ;
	const GAuth::Secrets & m_client_secrets ;
	std::string m_sasl_client_config ;
	G::Slot::Signal<const std::string&,const std::string&> m_event_signal ;
} ;

/// \class GSmtp::ServerPeer
/// Handles a connection from a remote SMTP client.
/// \see GSmtp::Server
///
class GSmtp::ServerPeer : public GNet::ServerPeer , private ServerProtocol::Sender , private GNet::DnsBlockCallback
{
public:
	G_EXCEPTION( SendError , "failed to send smtp response" ) ;

	ServerPeer( GNet::ExceptionSinkUnbound , const GNet::ServerPeerInfo & peer_info , Server & server ,
		const GAuth::Secrets & server_secrets , const Server::Config & server_config ,
		std::unique_ptr<ServerProtocol::Text> ptext ) ;
			///< Constructor.

private: // overrides
	void onSendComplete() override ; // Override from GNet::ServerPeer.
	void onDelete( const std::string & reason ) override ; // Override from GNet::ServerPeer.
	bool onReceive( const char * , std::size_t , std::size_t , std::size_t , char ) override ; // Override from GNet::ServerPeer.
	void onSecure( const std::string & , const std::string & ) override ; // Override from GNet::SocketProtocolSink.
	void protocolSend( const std::string & line , bool ) override ; // Override from ServerProtocol::Sender.
	void protocolShutdown() override ; // Override from ServerProtocol::Sender.
	void onDnsBlockResult( const GNet::DnsBlockResult & ) override ; // Override from GNet::DnsBlockCallback.
	void onData( const char * , std::size_t ) override ; // Override from GNet::ServerPeer.

public:
	~ServerPeer() override = default ;
	ServerPeer( const ServerPeer & ) = delete ;
	ServerPeer( ServerPeer && ) = delete ;
	void operator=( const ServerPeer & ) = delete ;
	void operator=( ServerPeer && ) = delete ;

private:
	void onCheckTimeout() ;
	void onFlushTimeout() ;

private:
	Server & m_server ;
	GNet::DnsBlock m_block ;
	GNet::Timer<ServerPeer> m_flush_timer ;
	GNet::Timer<ServerPeer> m_check_timer ;
	std::unique_ptr<Verifier> m_verifier ;
	std::unique_ptr<ProtocolMessage> m_pmessage ;
	std::unique_ptr<ServerProtocol::Text> m_ptext ;
	GNet::LineBuffer m_line_buffer ;
	ServerProtocol m_protocol ; // order dependency -- last
} ;

inline GSmtp::Server::Config & GSmtp::Server::Config::set_allow_remote( bool b ) { allow_remote = b ; return *this ; }
inline GSmtp::Server::Config & GSmtp::Server::Config::set_interfaces( const G::StringArray & a ) { interfaces = a ; return *this ; }
inline GSmtp::Server::Config & GSmtp::Server::Config::set_port( unsigned int n ) { port = n ; return *this ; }
inline GSmtp::Server::Config & GSmtp::Server::Config::set_ident( const std::string & s ) { ident = s ; return *this ; }
inline GSmtp::Server::Config & GSmtp::Server::Config::set_anonymous( bool b ) { anonymous = b ; return *this ; }
inline GSmtp::Server::Config & GSmtp::Server::Config::set_filter_address( const std::string & s ) { filter_address = s ; return *this ; }
inline GSmtp::Server::Config & GSmtp::Server::Config::set_filter_timeout( unsigned int t ) { filter_timeout = t ; return *this ; }
inline GSmtp::Server::Config & GSmtp::Server::Config::set_verifier_address( const std::string & s ) { verifier_address = s ; return *this ; }
inline GSmtp::Server::Config & GSmtp::Server::Config::set_verifier_timeout( unsigned int t ) { verifier_timeout = t ; return *this ; }
inline GSmtp::Server::Config & GSmtp::Server::Config::set_server_peer_config( const GNet::ServerPeerConfig & c ) { server_peer_config = c ; return *this ; }
inline GSmtp::Server::Config & GSmtp::Server::Config::set_protocol_config( const ServerProtocol::Config & c ) { protocol_config = c ; return *this ; }
inline GSmtp::Server::Config & GSmtp::Server::Config::set_sasl_server_config( const std::string & s ) { sasl_server_config = s ; return *this ; }
inline GSmtp::Server::Config & GSmtp::Server::Config::set_dnsbl_config( const std::string & s ) { dnsbl_config = s ; return *this ; }

#endif
