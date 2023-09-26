//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gmultiserver.h"
#include "gsmtpclient.h"
#include "gdnsbl.h"
#include "glinebuffer.h"
#include "gverifier.h"
#include "gmessagestore.h"
#include "gfilterfactorybase.h"
#include "gverifierfactorybase.h"
#include "gsmtpserverprotocol.h"
#include "gsmtpserversender.h"
#include "gsmtpserverbufferin.h"
#include "gprotocolmessage.h"
#include "glimits.h"
#include "gexception.h"
#include <algorithm>
#include <string>
#include <sstream>
#include <memory>
#include <list>

namespace GSmtp
{
	class Server ;
	class ServerPeer ;
}

//| \class GSmtp::Server
/// An SMTP server class.
///
class GSmtp::Server : public GNet::MultiServer
{
public:
	using AddressList = std::vector<GNet::Address> ;

	struct Config /// A configuration structure for GSmtp::Server.
	{
		bool allow_remote {false} ;
		G::StringArray interfaces ;
		unsigned int port {0U} ;
		std::string ident ;
		bool anonymous_smtp {false} ;
		bool anonymous_content {false} ;
		Filter::Config filter_config ;
		FilterFactoryBase::Spec filter_spec ;
		Verifier::Config verifier_config ;
		VerifierFactoryBase::Spec verifier_spec ;
		GNet::ServerPeer::Config net_server_peer_config ;
		GNet::Server::Config net_server_config ;
		ServerProtocol::Config protocol_config ;
		std::string dnsbl_config ;
		ServerBufferIn::Config buffer_config ;
		std::string domain ;

		Config & set_allow_remote( bool = true ) noexcept ;
		Config & set_interfaces( const G::StringArray & ) ;
		Config & set_port( unsigned int ) noexcept ;
		Config & set_ident( const std::string & ) ;
		Config & set_anonymous( bool = true ) noexcept ;
		Config & set_anonymous_smtp( bool = true ) noexcept ;
		Config & set_anonymous_content( bool = true ) noexcept ;
		Config & set_filter_config( const Filter::Config & ) ;
		Config & set_filter_spec( const FilterFactoryBase::Spec & ) ;
		Config & set_verifier_config( const Verifier::Config & ) ;
		Config & set_verifier_spec( const VerifierFactoryBase::Spec & ) ;
		Config & set_net_server_peer_config( const GNet::ServerPeer::Config & ) ;
		Config & set_net_server_config( const GNet::Server::Config & ) ;
		Config & set_protocol_config( const ServerProtocol::Config & ) ;
		Config & set_dnsbl_config( const std::string & ) ;
		Config & set_buffer_config( const ServerBufferIn::Config & ) ;
		Config & set_domain( const std::string & ) ;
	} ;

	Server( GNet::ExceptionSink es , GStore::MessageStore & ,
		FilterFactoryBase & , VerifierFactoryBase & , const GAuth::SaslClientSecrets & ,
		const GAuth::SaslServerSecrets & , const Config & server_config ,
		const std::string & forward_to , int forward_to_family ,
		const GSmtp::Client::Config & client_config ) ;
			///< Constructor. Listens on the given port number using INET_ANY
			///< if 'server_config.interfaces' is empty, or on specific
			///< interfaces otherwise.
			///<
			///< If the forward-to address is given then all messages are
			///< forwarded as soon as they are received using the
			///< GSmtp::ProtocolMessageForward class and the given client
			///< configuration.
			///<
			///< The forward-to-family is used if the forward-to address
			///< is a DNS name that needs to be resolved.

	~Server() override ;
		///< Destructor.

	void report( const std::string & group = {} ) const ;
		///< Generates helpful diagnostics after construction.

	G::Slot::Signal<const std::string&,const std::string&> & eventSignal() noexcept ;
		///< Returns a signal that indicates that something has happened.

	std::unique_ptr<ProtocolMessage> newProtocolMessage( GNet::ExceptionSink ) ;
		///< Called by GSmtp::ServerPeer to construct a ProtocolMessage.

	Config & config() ;
		///< Exposes the configuration sub-object.

	void nodnsbl( unsigned int seconds ) ;
		///< Clears the DNSBL configuration string for a period of time.

	void enable( bool ) ;
		///< Disables or re-enables new SMTP sessions. When disabled new
		///< network connections are allowed but the SMTP protocol is
		///< disabled in some way.

private: // overrides
	std::unique_ptr<GNet::ServerPeer> newPeer( GNet::ExceptionSinkUnbound ,
		GNet::ServerPeerInfo && , GNet::MultiServer::ServerInfo ) override ; // Override from GNet::MultiServer.

public:
	Server( const Server & ) = delete ;
	Server( Server && ) = delete ;
	Server & operator=( const Server & ) = delete ;
	Server & operator=( Server && ) = delete ;

private:
	std::unique_ptr<Filter> newFilter( GNet::ExceptionSink ) const ;
	std::unique_ptr<ProtocolMessage> newProtocolMessageStore( std::unique_ptr<Filter> ) ;
	std::unique_ptr<ProtocolMessage> newProtocolMessageForward( GNet::ExceptionSink , std::unique_ptr<ProtocolMessage> ) ;
	std::unique_ptr<ServerProtocol::Text> newProtocolText( bool , bool , const GNet::Address & , const std::string & domain ) const ;
	Config serverConfig() const ;

private:
	GStore::MessageStore & m_store ;
	FilterFactoryBase & m_ff ;
	VerifierFactoryBase & m_vf ;
	Config m_server_config ;
	Client::Config m_client_config ;
	const GAuth::SaslServerSecrets & m_server_secrets ;
	std::string m_sasl_server_config ;
	std::string m_forward_to ;
	int m_forward_to_family ;
	const GAuth::SaslClientSecrets & m_client_secrets ;
	std::string m_sasl_client_config ;
	G::Slot::Signal<const std::string&,const std::string&> m_event_signal ;
	G::TimerTime m_dnsbl_suspend_time ;
	bool m_enabled ;
} ;

//| \class GSmtp::ServerPeer
/// Handles a connection from a remote SMTP client.
/// \see GSmtp::Server
///
class GSmtp::ServerPeer : public GNet::ServerPeer , private ServerSender
{
public:
	G_EXCEPTION( Error , tx("smtp server error") ) ;
	G_EXCEPTION( SendError , tx("failed to send smtp response") ) ;

	ServerPeer( GNet::ExceptionSinkUnbound , GNet::ServerPeerInfo && peer_info , Server & server ,
		bool enabled , VerifierFactoryBase & vf , const GAuth::SaslServerSecrets & server_secrets ,
		const Server::Config & server_config , std::unique_ptr<ServerProtocol::Text> ptext ) ;
			///< Constructor.

	~ServerPeer() override ;
		///< Destructor.

private: // overrides
	void onSendComplete() override ; // GNet::ServerPeer
	void onDelete( const std::string & reason ) override ; // GNet::ServerPeer
	bool onReceive( const char * , std::size_t , std::size_t , std::size_t , char ) override ; // GNet::ServerPeer
	void onSecure( const std::string & , const std::string & , const std::string & ) override ; // GNet::SocketProtocolSink
	void protocolSecure() override ; // GSmtp::ServerSender
	void protocolSend( const std::string & line , bool ) override ; // GSmtp::ServerSender
	void protocolShutdown( int how ) override ; // GSmtp::ServerSender
	void protocolExpect( std::size_t ) override ; // GSmtp::ServerSender
	void onData( const char * , std::size_t ) override ; // GNet::ServerPeer

public:
	ServerPeer( const ServerPeer & ) = delete ;
	ServerPeer( ServerPeer && ) = delete ;
	ServerPeer & operator=( const ServerPeer & ) = delete ;
	ServerPeer & operator=( ServerPeer && ) = delete ;

private:
	void onDnsBlockResult( bool ) ; // GNet::Dnsbl callback
	void onCheckTimeout() ;
	void onFlow( bool ) ;

private:
	Server & m_server ;
	Server::Config m_server_config ;
	GNet::Dnsbl m_block ;
	GNet::Timer<ServerPeer> m_check_timer ;
	std::unique_ptr<Verifier> m_verifier ;
	std::unique_ptr<ProtocolMessage> m_pmessage ;
	std::unique_ptr<ServerProtocol::Text> m_ptext ;
	ServerProtocol m_protocol ;
	ServerBufferIn m_input_buffer ;
	std::string m_output_buffer ;
	bool m_output_blocked ;
} ;

inline GSmtp::Server::Config & GSmtp::Server::Config::set_allow_remote( bool b ) noexcept { allow_remote = b ; return *this ; }
inline GSmtp::Server::Config & GSmtp::Server::Config::set_interfaces( const G::StringArray & a ) { interfaces = a ; return *this ; }
inline GSmtp::Server::Config & GSmtp::Server::Config::set_port( unsigned int n ) noexcept { port = n ; return *this ; }
inline GSmtp::Server::Config & GSmtp::Server::Config::set_ident( const std::string & s ) { ident = s ; return *this ; }
inline GSmtp::Server::Config & GSmtp::Server::Config::set_anonymous( bool b ) noexcept { anonymous_smtp = anonymous_content = b ; return *this ; }
inline GSmtp::Server::Config & GSmtp::Server::Config::set_anonymous_smtp( bool b ) noexcept { anonymous_smtp = b ; return *this ; }
inline GSmtp::Server::Config & GSmtp::Server::Config::set_anonymous_content( bool b ) noexcept { anonymous_content = b ; return *this ; }
inline GSmtp::Server::Config & GSmtp::Server::Config::set_filter_config( const Filter::Config & c ) { filter_config = c ; return *this ; }
inline GSmtp::Server::Config & GSmtp::Server::Config::set_filter_spec( const FilterFactoryBase::Spec & r ) { filter_spec = r ; return *this ; }
inline GSmtp::Server::Config & GSmtp::Server::Config::set_verifier_config( const Verifier::Config & c ) { verifier_config = c ; return *this ; }
inline GSmtp::Server::Config & GSmtp::Server::Config::set_verifier_spec( const VerifierFactoryBase::Spec & r ) { verifier_spec = r ; return *this ; }
inline GSmtp::Server::Config & GSmtp::Server::Config::set_net_server_peer_config( const GNet::ServerPeer::Config & c ) { net_server_peer_config = c ; return *this ; }
inline GSmtp::Server::Config & GSmtp::Server::Config::set_net_server_config( const GNet::Server::Config & c ) { net_server_config = c ; return *this ; }
inline GSmtp::Server::Config & GSmtp::Server::Config::set_protocol_config( const ServerProtocol::Config & c ) { protocol_config = c ; return *this ; }
inline GSmtp::Server::Config & GSmtp::Server::Config::set_dnsbl_config( const std::string & s ) { dnsbl_config = s ; return *this ; }
inline GSmtp::Server::Config & GSmtp::Server::Config::set_buffer_config( const ServerBufferIn::Config & c ) { buffer_config = c ; return *this ; }
inline GSmtp::Server::Config & GSmtp::Server::Config::set_domain( const std::string & s ) { domain = s ; return *this ; }

#endif
