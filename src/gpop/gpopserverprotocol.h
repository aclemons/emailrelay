//
// Copyright (C) 2001-2010 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gpop.h"
#include "gaddress.h"
#include "gstatemachine.h"
#include "gpopsecrets.h"
#include "gpopstore.h"
#include "gpopauth.h"
#include "gmemory.h"
#include "gtimer.h"
#include "gexception.h"

/// \namespace GPop
namespace GPop
{
	class ServerProtocol ;
	class ServerProtocolText ;
}

/// \class GPop::ServerProtocol
/// Implements the POP server-side protocol.
///
/// Uses the ServerProtocol::Sender as its "sideways"
/// interface to talk back to the client.
/// 
/// \see RFC1939
///
class GPop::ServerProtocol 
{
public:
	G_EXCEPTION( ProtocolDone , "pop protocol done" ) ;
	/// An interface used by ServerProtocol to send protocol replies.
	class Sender 
	{
		public: virtual bool protocolSend( const std::string & s , size_t offset ) = 0 ;
		public: virtual ~Sender() ;
		private: void operator=( const Sender & ) ; // not implemented
	} ;
	/// An interface used by ServerProtocol to provide response text strings.
	class Text 
	{
		public: virtual std::string greeting() const = 0 ;
		public: virtual std::string quit() const = 0 ;
		public: virtual std::string capa() const = 0 ;
		public: virtual std::string user( const std::string & id ) const = 0 ;
		public: virtual ~Text() ;
		private: void operator=( const Text & ) ; // not implemented
	} ;
	/// A structure containing configuration parameters for ServerProtocol. NOT USED.
	struct Config 
	{
		bool dummy ;
		Config() ;
	} ;
	/// An interface used by ServerProtocol to enable TLS.
	class Security 
	{
		public: virtual bool securityEnabled() const = 0 ;
		public: virtual void securityStart() = 0 ;
		public: virtual ~Security() ;
		private: void operator=( const Security & ) ; // not implemented
	} ;

	ServerProtocol( Sender & sender , Security & security , Store & store , const Secrets & secrets , 
		const Text & text , GNet::Address peer_address , Config config ) ;
			///< Constructor. 
			///<
			///< The Sender interface is used to send protocol
			///< replies back to the client.
			///<
			///< The Text interface is used to get informational text
			///< for returning to the client.
			///<
			///< All references are kept.

	virtual ~ServerProtocol() ;
		///< Destructor.

	void init() ;
		///< Starts the protocol.

	void apply( const std::string & line ) ;
		///< Called on receipt of a string from the client.
		///< The string is expected to be CR-LF terminated.
		///< Throws ProtocolDone if done.

	void resume() ;
		///< Called when the Sender can send again. The Sender returns
		///< false from protocolSend() when blocked, and calls
		///< resume() when unblocked.

	void secure() ;
		///< Called when the server connection becomes secure.

private:
	enum Event
	{
		eApop ,
		eAuth ,
		eAuthData ,
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
	enum State
	{
		sStart ,
		sEnd ,
		sActive ,
		sData ,
		sAuth ,
		s_Any ,
		s_Same
	} ;
	typedef G::StateMachine<ServerProtocol,State,Event> Fsm ;

private:
	ServerProtocol( const ServerProtocol & ) ; // not implemented
	void operator=( const ServerProtocol & ) ; // not implemented
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
	void doUidl( const std::string & line , bool & ) ;
	void sendInit() ;
	void sendError() ;
	void sendError( const std::string & ) ;
	void sendOk() ;
	static const std::string & crlf() ;
	Event commandEvent( const std::string & ) const ;
	int commandNumber( const std::string & , int , size_t index = 1U ) const ;
	void sendList( const std::string & , bool ) ;
	std::string commandWord( const std::string & ) const ;
	std::string commandParameter( const std::string & , size_t index = 1U ) const ;
	std::string commandPart( const std::string & , size_t index ) const ;
	void sendContent() ;
	bool sendContentLine( std::string & , bool & ) ;
	void send( std::string ) ;
	void lockStore() ;
	std::string mechanismsWithoutLogin() const ;
	bool mechanismsIncludePlain() const ;

private:
	const Text & m_text ;
	Sender & m_sender ;
	Security & m_security ;
	Store & m_store ;
	StoreLock m_store_lock ;
	const Secrets & m_secrets ;
	Auth m_auth ;
	GNet::Address m_peer_address ;
	Fsm m_fsm ;
	std::string m_user ;
	std::auto_ptr<std::istream> m_content ;
	long m_body_limit ;
	bool m_in_body ;
	bool m_secure ;
} ;

/// \class GPop::ServerProtocolText
/// A default implementation for the 
/// ServerProtocol::Text interface.
///
class GPop::ServerProtocolText : public GPop::ServerProtocol::Text 
{
public:
	explicit ServerProtocolText( GNet::Address peer ) ;
		///< Constructor.

	virtual std::string greeting() const ;
		///< Final override from GPop::ServerProtocol::Text.

	virtual std::string quit() const ;
		///< Final override from GPop::ServerProtocol::Text.

	virtual std::string capa() const ;
		///< Final override from GPop::ServerProtocol::Text.

	virtual std::string user( const std::string & id ) const ;
		///< Final override from GPop::ServerProtocol::Text.
} ;

#endif
