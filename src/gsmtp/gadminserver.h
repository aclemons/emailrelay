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
// gadminserver.h
//

#ifndef G_SMTP_ADMIN_H
#define G_SMTP_ADMIN_H

#include "gdef.h"
#include "gsmtp.h"
#include "gserver.h"
#include "gstr.h"
#include "glinebuffer.h"
#include "gserverprotocol.h"
#include "gsmtpclient.h"
#include <string>
#include <list>
#include <sstream>
#include <utility>

namespace GSmtp
{
	class AdminPeer ;
	class AdminServer ;
}

// Class: GSmtp::AdminPeer
// Description: A derivation of ServerPeer for the administration interface.
// See also: AdminServer
//
class GSmtp::AdminPeer : public GNet::ServerPeer 
{
public:
	AdminPeer( GNet::Server::PeerInfo , AdminServer & , const GNet::Address & local , 
		const std::string & remote , const G::StringMap & extra_commands , bool with_terminate ) ;
			// Constructor.

	virtual ~AdminPeer() ;
		// Destructor.

	void notify( const std::string & s0 , const std::string & s1 , const std::string & s2 ) ;
		// Called when something happens.

private:
	AdminPeer( const AdminPeer & ) ;
	void operator=( const AdminPeer & ) ;
	virtual void onDelete() ; // from GNet::ServerPeer
	virtual void onData( const char * , size_t ) ; // from GNet::ServerPeer
	virtual void clientDone( std::string ) ; // Client::doneSignal()
	bool processLine( const std::string & line ) ;
	static bool is( const std::string & , const std::string & ) ;
	static std::pair<bool,std::string> find( const std::string & line , const G::StringMap & map ) ;
	void flush() ;
	void help() ;
	void info() ;
	void list() ;
	void send( std::string ) ;
	void warranty() ;
	void version() ;
	void copyright() ;
	static std::string crlf() ;
	void prompt() ;

private:
	GNet::LineBuffer m_buffer ;
	AdminServer & m_server ;
	GNet::Address m_local_address ;
	std::string m_remote_address ;
	std::auto_ptr<GSmtp::Client> m_client ;
	bool m_notifying ;
	G::StringMap m_extra_commands ;
	bool m_with_terminate ;
} ;

// Class: GSmtp::AdminServer
// Description: A server class which implements the emailrelay administration interface.
//
class GSmtp::AdminServer : public GNet::Server 
{
public:
	AdminServer( MessageStore & store , const Secrets & client_secrets , 
		const GNet::Address & listening_address , bool allow_remote , 
		const GNet::Address & local_address , const std::string & remote_address ,
		unsigned int response_timeout , unsigned int connection_timeout , 
		const G::StringMap & extra_commands , bool with_terminate ) ;
			// Constructor. The 'store' and 'client-secrets' references
			// are kept.

	virtual ~AdminServer() ;
		// Destructor.

	void report() const ;
		// Generates helpful diagnostics.

	MessageStore & store() ;
		// Returns a reference to the message store, as
		// passed in to the constructor.

	const Secrets & secrets() const ;
		// Returns a reference to the secrets object, as
		// passed in to the constructor. Note that this is 
		// a "client-side" secrets file, used to authenticate
		// ourselves with a remote server.

	unsigned int responseTimeout() const ;
		// Returns the response timeout, as passed in to the
		// constructor.

	unsigned int connectionTimeout() const ;
		// Returns the connection timeout, as passed in to the
		// constructor.

	void notify( const std::string & s0 , const std::string & s1 , const std::string & s2 ) ;
		// Called when something happens which the admin
		// user might be interested in.

	void unregister( AdminPeer * ) ;
		// Called from the AdminPeer destructor.

private:
	virtual GNet::ServerPeer * newPeer( GNet::Server::PeerInfo ) ;
	AdminServer( const AdminServer & ) ;
	void operator=( const AdminServer & ) ;

private:
	typedef std::list<AdminPeer*> PeerList ;
	PeerList m_peers ;
	MessageStore & m_store ;
	const Secrets & m_secrets ;
	GNet::Address m_local_address ;
	bool m_allow_remote ;
	std::string m_remote_address ;
	unsigned int m_response_timeout ;
	unsigned int m_connection_timeout ;
	G::StringMap m_extra_commands ;
	bool m_with_terminate ;
} ;

#endif
