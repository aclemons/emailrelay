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
/// \file gpopserverprotocol.h
///

#ifndef G_POP_SERVER_PROTOCOL_H
#define G_POP_SERVER_PROTOCOL_H

#include "gdef.h"
#include "gaddress.h"
#include "gstatemachine.h"
#include "gsaslserversecrets.h"
#include "gpopstore.h"
#include "gsaslserver.h"
#include "gtimer.h"
#include "gexception.h"
#include <memory>

namespace GPop
{
	class ServerProtocol ;
	class ServerProtocolText ;
}

//| \class GPop::ServerProtocol
/// Implements the POP server-side protocol.
///
/// Uses the ServerProtocol::Sender as its "sideways"
/// interface to talk back to the client.
///
/// \see RFC-1939
///
class GPop::ServerProtocol
{
public:
	G_EXCEPTION( ProtocolDone , tx("pop protocol done") ) ;

	class Sender /// An interface used by ServerProtocol to send protocol replies.
	{
	public:
		virtual bool protocolSend( G::string_view , std::size_t offset ) = 0 ;
		virtual ~Sender() = default ;
	} ;

	class Text /// An interface used by ServerProtocol to provide response text strings.
	{
	public:
		virtual std::string greeting() const = 0 ;
		virtual std::string quit() const = 0 ;
		virtual std::string capa() const = 0 ;
		virtual std::string user( const std::string & id ) const = 0 ;
		virtual ~Text() = default ;
	} ;

	struct Config /// A structure containing configuration parameters for ServerProtocol.
	{
		Config() ;
		bool crlf_only {true} ; // (RFC-2821 2.3.7 does not apply to POP)
		std::string sasl_server_challenge_domain ;
		Config & set_crlf_only( bool = true ) ;
		Config & set_sasl_server_challenge_domain( const std::string & ) ;
	} ;

	class Security /// An interface used by ServerProtocol to enable TLS.
	{
	public:
		virtual bool securityEnabled() const = 0 ;
		virtual void securityStart() = 0 ;
		virtual ~Security() = default ;
	} ;

	ServerProtocol( Sender & sender , Security & security , Store & store ,
		const GAuth::SaslServerSecrets & server_secrets , const std::string & sasl_server_config ,
		const Text & text , const GNet::Address & peer_address , const Config & config ) ;
			///< Constructor.
			///<
			///< The Sender interface is used to send protocol
			///< replies back to the client.
			///<
			///< The Text interface is used to get informational text
			///< for returning to the client.
			///<
			///< All references are kept.

	void init() ;
		///< Starts the protocol.

	void apply( const std::string & line ) ;
		///< Called on receipt of a string from the client.
		///< The string is expected to be CR-LF terminated.
		///< Throws ProtocolDone if done.

	void resume() ;
		///< Called when the Sender can send again. The Sender returns
		///< false from Sender::protocolSend() when blocked, and calls
		///< resume() when unblocked.

	void secure() ;
		///< Called when the server connection becomes secure.

private:
	enum class Event
	{
		eApop ,
		eAuth ,
		eAuthData ,
		eAuthComplete ,
		eCapa ,
		eDele ,
		eList ,
		eNoop ,
		ePass ,
		eQuit ,
		eRetr ,
		eRset ,
		eSent ,
		eStat ,
		eTop ,
		eUidl ,
		eUser ,
		eStls ,
		eSecure ,
		eUnknown
	} ;
	enum class State
	{
		sStart ,
		sEnd ,
		sActive ,
		sData ,
		sAuth ,
		s_Any ,
		s_Same
	} ;
	using EventData = const std::string & ;
	using Fsm = G::StateMachine<ServerProtocol,State,Event,EventData> ;

public:
	~ServerProtocol() = default ;
	ServerProtocol( const ServerProtocol & ) = delete ;
	ServerProtocol( ServerProtocol && ) = delete ;
	ServerProtocol & operator=( const ServerProtocol & ) = delete ;
	ServerProtocol & operator=( ServerProtocol && ) = delete ;

private:
	void doQuit( const std::string & line , bool & ) ;
	void doQuitEarly( const std::string & line , bool & ) ;
	void doStat( const std::string & line , bool & ) ;
	void doList( const std::string & line , bool & ) ;
	void doRetr( const std::string & line , bool & ) ;
	void doDele( const std::string & line , bool & ) ;
	void doRset( const std::string & line , bool & ) ;
	void doUser( const std::string & line , bool & ) ;
	void doPass( const std::string & line , bool & ) ;
	void doNoop( const std::string & line , bool & ) ;
	void doNothing( const std::string & line , bool & ) ;
	void doApop( const std::string & line , bool & ) ;
	void doTop( const std::string & line , bool & ) ;
	void doCapa( const std::string & line , bool & ) ;
	void doStls( const std::string & line , bool & ) ;
	void doAuth( const std::string & line , bool & ) ;
	void doAuthData( const std::string & line , bool & ) ;
	void doAuthComplete( const std::string & line , bool & ) ;
	void doUidl( const std::string & line , bool & ) ;
	void sendInit() ;
	void sendError() ;
	void sendError( const std::string & ) ;
	void sendOk() ;
	Event commandEvent( G::string_view ) const ;
	int commandNumber( const std::string & , int , std::size_t index = 1U ) const ;
	void sendList( const std::string & , bool ) ;
	std::string commandWord( const std::string & ) const ;
	std::string commandParameter( const std::string & , std::size_t index = 1U ) const ;
	std::string commandPart( const std::string & , std::size_t index ) const ;
	void sendContent() ;
	bool sendContentLine( std::string & , bool & ) ;
	void sendLine( G::string_view , bool has_crlf = false ) ;
	void sendLine( std::string && ) ;
	void sendLines( std::ostringstream & ) ;
	void readStore( const std::string & ) ;
	std::string mechanisms() const ;
	bool mechanismsIncludePlain() const ;

private:
	const Text & m_text ;
	Sender & m_sender ;
	Security & m_security ;
	Store & m_store ;
	Config m_config ;
	std::unique_ptr<StoreUser> m_store_user ;
	StoreList m_store_list ;
	std::unique_ptr<GAuth::SaslServer> m_sasl ;
	GNet::Address m_peer_address ;
	Fsm m_fsm ;
	std::string m_user ;
	std::unique_ptr<std::istream> m_content ;
	long m_body_limit ;
	bool m_in_body ;
	bool m_secure ;
	bool m_sasl_init_apop ;
} ;

//| \class GPop::ServerProtocolText
/// A default implementation for the ServerProtocol::Text interface.
///
class GPop::ServerProtocolText : public ServerProtocol::Text
{
public:
	explicit ServerProtocolText( const GNet::Address & peer ) ;
		///< Constructor.

public:
	~ServerProtocolText() override = default ;
	ServerProtocolText( const ServerProtocolText & ) = delete ;
	ServerProtocolText( ServerProtocolText && ) = delete ;
	ServerProtocolText & operator=( const ServerProtocolText & ) = delete ;
	ServerProtocolText & operator=( ServerProtocolText && ) = delete ;

private: // overrides
	std::string greeting() const override ; // Override from GPop::ServerProtocol::Text.
	std::string quit() const override ; // Override from GPop::ServerProtocol::Text.
	std::string capa() const override ; // Override from GPop::ServerProtocol::Text.
	std::string user( const std::string & id ) const override ; // Override from GPop::ServerProtocol::Text.
} ;

inline GPop::ServerProtocol::Config & GPop::ServerProtocol::Config::set_crlf_only( bool b ) { crlf_only = b ; return *this ; }
inline GPop::ServerProtocol::Config & GPop::ServerProtocol::Config::set_sasl_server_challenge_domain( const std::string & s ) { sasl_server_challenge_domain = s ; return *this ; }

#endif
