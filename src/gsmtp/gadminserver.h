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
/// \file gadminserver.h
///

#ifndef G_SMTP_ADMIN_H
#define G_SMTP_ADMIN_H

#include "gdef.h"
#include "gsmtp.h"
#include "gmultiserver.h"
#include "gexecutable.h"
#include "gstr.h"
#include "gstrings.h"
#include "glinebuffer.h"
#include "gserverprotocol.h"
#include "gclientptr.h"
#include "gsmtpclient.h"
#include "gbufferedserverpeer.h"
#include <string>
#include <list>
#include <sstream>
#include <utility>

/// \namespace GSmtp
namespace GSmtp
{
	class AdminServerPeer ;
	class AdminServer ;
}

/// \class GSmtp::AdminServerPeer
/// A derivation of ServerPeer for the administration interface.
/// \see GSmtp::AdminServer
///
class GSmtp::AdminServerPeer : public GNet::BufferedServerPeer 
{
public:
	AdminServerPeer( GNet::Server::PeerInfo , AdminServer & , const GNet::Address & local , 
		const std::string & remote , const G::StringMap & extra_commands , bool with_terminate ) ;
			///< Constructor.

	virtual ~AdminServerPeer() ;
		///< Destructor.

	void notify( const std::string & s0 , const std::string & s1 , const std::string & s2 ) ;
		///< Called when something happens.

protected:
	virtual void onSendComplete() ; 
		///< Final override from GNet::BufferedServerPeer.

	virtual bool onReceive( const std::string & ) ;
		///< Final override from GNet::BufferedServerPeer.

	virtual void onDelete( const std::string & ) ; 
		///< Final override from GNet::ServerPeer.

	virtual void onSecure( const std::string & ) ;
		///< Final override from GNet::SocketProtocolSink.

private:
	AdminServerPeer( const AdminServerPeer & ) ;
	void operator=( const AdminServerPeer & ) ;
	void clientDone( std::string , bool ) ; 
	static bool is( const std::string & , const std::string & ) ;
	static std::pair<bool,std::string> find( const std::string & line , const G::StringMap & map ) ;
	bool flush() ;
	void help() ;
	void info() ;
	MessageStore::Iterator spooled() ;
	MessageStore::Iterator failures() ;
	void list( MessageStore::Iterator ) ;
	void pid() ;
	void sendLine( std::string ) ;
	void warranty() ;
	void version() ;
	void copyright() ;
	static const std::string & crlf() ;
	void prompt() ;
	void unfailAll() ;

private:
	GNet::LineBuffer m_buffer ;
	AdminServer & m_server ;
	GNet::Address m_local_address ;
	std::string m_remote_address ;
	GNet::ClientPtr<GSmtp::Client> m_client ;
	bool m_notifying ;
	G::StringMap m_extra_commands ;
	bool m_with_terminate ;
} ;

/// \class GSmtp::AdminServer
/// A server class which implements the emailrelay administration interface.
///
class GSmtp::AdminServer : public GNet::MultiServer 
{
public:
	AdminServer( MessageStore & store , const GSmtp::Client::Config & client_config ,
		const GAuth::Secrets & client_secrets , const GNet::MultiServer::AddressList & listening_addresses , 
		bool allow_remote , const GNet::Address & local_address , const std::string & remote_address ,
		unsigned int connection_timeout , const G::StringMap & extra_commands , bool with_terminate ) ;
			///< Constructor. The 'store' and 'client-secrets' references
			///< are kept.

	virtual ~AdminServer() ;
		///< Destructor.

	void report() const ;
		///< Generates helpful diagnostics.

	MessageStore & store() ;
		///< Returns a reference to the message store, as
		///< passed in to the constructor.

	const GAuth::Secrets & secrets() const ;
		///< Returns a reference to the secrets object, as
		///< passed in to the constructor. Note that this is 
		///< a "client-side" secrets file, used to authenticate
		///< ourselves with a remote server.

	GSmtp::Client::Config clientConfig() const ;
		///< Returns the client configuration.

	unsigned int connectionTimeout() const ;
		///< Returns the connection timeout, as passed in to the
		///< constructor.

	void notify( const std::string & s0 , const std::string & s1 , const std::string & s2 ) ;
		///< Called when something happens which the admin
		///< user might be interested in.

	void unregister( AdminServerPeer * ) ;
		///< Called from the AdminServerPeer destructor.

protected:
	virtual GNet::ServerPeer * newPeer( GNet::Server::PeerInfo ) ;
		///< Final override from GNet::MultiServer.

private:
	AdminServer( const AdminServer & ) ; // not implemented
	void operator=( const AdminServer & ) ; // not implemented

private:
	typedef std::list<AdminServerPeer*> PeerList ;
	PeerList m_peers ;
	MessageStore & m_store ;
	GSmtp::Client::Config m_client_config ;
	const GAuth::Secrets & m_secrets ;
	GNet::Address m_local_address ;
	bool m_allow_remote ;
	std::string m_remote_address ;
	unsigned int m_connection_timeout ;
	G::StringMap m_extra_commands ;
	bool m_with_terminate ;
} ;

#endif
