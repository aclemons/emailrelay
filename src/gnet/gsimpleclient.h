//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gsimpleclient.h
///

#ifndef G_SIMPLE_CLIENT_H 
#define G_SIMPLE_CLIENT_H

#include "gdef.h"
#include "gnet.h"
#include "gaddress.h"
#include "gconnection.h"
#include "gexception.h"
#include "geventhandler.h"
#include "gresolver.h"
#include "gresolverinfo.h"
#include "gsocket.h"
#include "gevent.h"
#include <string>

/// \namespace GNet
namespace GNet
{
	class SimpleClient ;
	class ClientResolver ;
}

/// \class GNet::ClientResolver
/// A resolver class which calls SimpleClient::resolveCon() when done.
///
class GNet::ClientResolver : public GNet::Resolver 
{
private:
	SimpleClient & m_client ;

public:
	explicit ClientResolver( SimpleClient & ) ;
		///< Constructor.

	void resolveCon( bool success , const Address &address , std::string reason ) ;
		///< From Resolver.

private:
	ClientResolver( const ClientResolver & ) ;
	void operator=( const ClientResolver & ) ;
} ;

/// \class GNet::SimpleClient
/// A class for making an outgoing connection to a remote server. 
/// The class handles address resolution and connection issues and it reads 
/// incoming data. However, it has only minimal support for flow-control
/// when sending.
///
class GNet::SimpleClient : public GNet::EventHandler , public GNet::Connection 
{
public:
	enum ConnectStatus { Success , Failure , Retry , ImmediateSuccess } ;
    enum State { Idle , Resolving , Connecting , Connected } ;
	G_EXCEPTION( DnsError , "dns error" ) ;
	G_EXCEPTION( ConnectError , "connect failure" ) ;
	G_EXCEPTION( NotConnected , "socket not connected" ) ;
	G_EXCEPTION( ReadError , "read error: disconnected" ) ;
	typedef std::string::size_type size_type ;

	SimpleClient( const ResolverInfo & remote_info ,
		const Address & local_interface = Address(0U) , bool privileged = false ,
		bool sync_dns = synchronousDnsDefault() ) ;
			///< Constructor.
			///<
			///< The socket is bound with the given local address, but with an 
			///< arbitrary port number -- the supplied port number is ignored. 
			///< The local address defaults to the INADDR_ANY address.
			///<
			///< If the 'privileged' parameter is true then the local socket
			///< is bound with privileged port number (ie. < 1024), selected 
			///< at random.

	void connect() ;
		///< Initates a connection to the remote server. 
		///<
		///< This default implementation throws on error, and may call 
		///< onConnect() before returning.

	bool connected() const ;
		///< Returns true if connected to the peer.

	virtual std::pair<bool,Address> localAddress() const ;
		///< Override from Connection. Returns the local 
		///< address. Pair.first is false on error.
		///< Final override from GNet::Connection.

	virtual std::pair<bool,Address> peerAddress() const ;
		///< Override from Connection. Returns the peer 
		///< address. Pair.first is false on error.
		///< Final override from GNet::Connection.

	ResolverInfo resolverInfo() const ;
		///< Returns a ResolverInfo structure containing the
		///< result of host() and service() name lookup if 
		///< available.

	static bool synchronousDnsDefault() ;
		///< Returns true if DNS queries should normally be
		///< synchronous on this platform. Used to default the
		///< relevant constructor parameter.

	void updateResolverInfo( const ResolverInfo & ) ;
		///< Updates the constructor's ResolverInfo object with
		///< the given one as long as both objects have the
		///< same host and service name. This is only useful 
		///< immediately after construction and before 
		///< re-entering the event loop.

	virtual void readEvent() ; 
		///< Final override from GNet::EventHandler.

	virtual void writeEvent() ;
		///< Final override from GNet::EventHandler.

protected:
	virtual ~SimpleClient() ;
		///< Destructor.

	Socket & socket() ;
		///< Returns a reference to the socket. Throws if not connected.

	virtual void onConnect() = 0 ;
		///< Called once connected. May (unfortunately) be
		///< called from within connect().

	virtual void onConnectImp() ;
		///< An alternative to onConnect() for private implementation 
		///< classes. The default implementation does nothing.

	virtual void onData( const char * data , size_type size ) = 0 ;
		///< Called on receipt of data.

	virtual void onWriteable() = 0 ;
		///< Called when the socket becomes writable
		///< after flow control is released.

	static bool canRetry( const std::string & reason ) ;
		///< Parses the given failure reason and returns
		///< true if the client can reasonably retry
		///< at some later time.

	std::string logId() const ;
		///< Returns a identification string for logging purposes.
		///< Not guaranteed to stay the same during the lifetime
		///< of the object.

private:
	friend class ClientResolver ;
	void resolveCon( bool , const Address & , std::string ) ;

private:
	SimpleClient( const SimpleClient& ) ; // not implemented
	void operator=( const SimpleClient& ) ; // not implemented
	void close() ;
	static int getRandomPort() ;
	StreamSocket & s() ;
	const StreamSocket & s() const ;
	bool startConnecting() ;
	bool localBind( Address ) ;
	ConnectStatus connectCore( Address remote_address , std::string *error_p ) ;
	void setState( State ) ;
	void immediateConnection() ;

private:
	ClientResolver m_resolver ;
	StreamSocket * m_s ;
	ResolverInfo m_remote ;
	Address m_local_address ;
	bool m_privileged ;
	State m_state ;
	bool m_sync_dns ;
} ;

#endif
