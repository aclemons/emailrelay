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
// gclient.h
//

#ifndef G_CLIENT_H 
#define G_CLIENT_H

#include "gdef.h"
#include "gnet.h"
#include "gaddress.h"
#include "gconnection.h"
#include "gsocket.h"
#include "gevent.h"
#include <string>

namespace GNet
{
	class Client ;
	class ClientImp ;
}

// Class: GNet::Client
// Description: An application-level class for making an outgoing connection
// to a remote server. The class handles address resolution and connection
// issues, and it reads incoming data. There is some support for flow-control
// issues when writing data out to the server.
//
class GNet::Client : public GNet::Connection 
{
public:
	explicit Client( bool priviledged = false , bool quit_on_disconnect = false ) ;
		// Constructor. If the 'priviledged' parameter
		// is true the the local endpoint of the
		// outgoing connection is bound to a
		// priviledged port number (ie. < 1024),
		// selected at random.
		//
		// If the 'quit' parameter is true then the client will
		// call EventLoop::quit() once it fails to connect, 
		// disconnects or looses the connection. Cleary 'quit' 
		// should only be true if this client is the only thing 
		// using the event loop.

	bool connect( std::string host, std::string service, 
		std::string *error_string = NULL ,
		bool sync_dns = synchronousDnsDefault() );
			// Initates a connection to the remote server.
			// Typically called before calling run().

	bool connected() const ;
		// Returns true if connected to the peer.

	void disconnect() ;
		// Disconnects from the peer.

	void blocked() ;
		// To be called when a Socket::write() fails
		// due to flow control. The virtual method
		// onWriteable() will then be called when
		// the connection unblocks.

	void run() ;
		// Starts the main event loop using EventLoop::run().

	static bool synchronousDnsDefault() ;
		// Returns true if DNS queries should normally be
		// synchronous on this platform.

	virtual ~Client() ;
		// Destructor.

	virtual std::pair<bool,Address> localAddress() const ;
		// Override from Connection. Returns the local address. 
		// Pair.first is false on error.

	virtual std::pair<bool,Address> peerAddress() const ;
		// Override from Connection. Returns the peer address. 
		// Pair.first is false on error.

	std::string peerName() const ;
		// Returns the peer's canonical name if available.
		// Returns the empty string if not.

protected:
	friend class ClientImp ;

	virtual void onConnect( Socket & socket ) = 0 ;
		// Called once connected. May (unfortunately) be
		// called from within connect().

	virtual void onDisconnect() = 0 ;
		// Called when disconnected by the peer.

	virtual void onData( const char * data , size_t size ) = 0 ;
		// Called on receipt of data.

	virtual void onError( const std::string &error ) = 0 ;
		// Called when an asyncronous, and fatal, error occurs.

	virtual void onWriteable() = 0 ;
		// Called when a blocked connection become writeable.

private:
	Client( const Client& ) ; // Copy constructor. Not implemented.
	void operator=( const Client& ) ; // Assignment operator. Not implemented.

private:
	ClientImp * m_imp ;
} ;

#endif
