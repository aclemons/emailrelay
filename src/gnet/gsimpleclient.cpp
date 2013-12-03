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
//
// gsimpleclient.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gaddress.h"
#include "gsocket.h"
#include "gdatetime.h"
#include "gexception.h"
#include "gresolver.h"
#include "groot.h"
#include "gmonitor.h"
#include "gsimpleclient.h"
#include "gassert.h"
#include "gtest.h"
#include "gdebug.h"
#include "glog.h"
#include <cstdlib>

namespace
{
	const int c_retries = 10 ; // number of retries when using a privileged local port number
	const int c_port_start = 512 ;
	const int c_port_end = 1024 ;
	const char * c_cannot_connect_to = "cannot connect to " ;
}

// ==

GNet::SimpleClient::SimpleClient( const ResolverInfo & remote ,
	const Address & local_address , bool privileged , bool sync_dns ,
	unsigned int secure_connection_timeout ) :
		m_remote(remote) ,
		m_local_address(local_address) ,
		m_privileged(privileged) ,
		m_state(Idle) ,
		m_sync_dns(sync_dns) ,
		m_secure_connection_timeout(secure_connection_timeout)
{
	G_DEBUG( "SimpleClient::ctor" ) ;
	if( Monitor::instance() ) Monitor::instance()->addClient( *this ) ;
}

GNet::SimpleClient::~SimpleClient()
{
	if( Monitor::instance() ) Monitor::instance()->removeClient( *this ) ;
	close() ;
}

std::string GNet::SimpleClient::logId() const
{
	std::string s = m_remote.displayString(true) ;
	if( m_s.get() != NULL )
		s.append( std::string() + "@" + m_s->asString() ) ; // cf. ServerPeer::logId()
	return s ;
}

GNet::ResolverInfo GNet::SimpleClient::resolverInfo() const
{
	return m_remote ;
}

void GNet::SimpleClient::updateResolverInfo( const ResolverInfo & update )
{
	if( m_remote.host() == update.host() && m_remote.service() == update.service() && update.hasAddress() )
	{
		G_DEBUG( "GNet::SimpleClient::updateResolverInfo: reusing dns lookup for " << update.displayString() ) ;
		m_remote = update ;
	}
}

GNet::StreamSocket & GNet::SimpleClient::socket()
{
	if( m_s.get() == NULL )
		throw NotConnected() ;
	return *m_s.get() ;
}

const GNet::StreamSocket & GNet::SimpleClient::socket() const
{
	if( m_s.get() == NULL )
		throw NotConnected() ;
	return *m_s.get() ;
}

void GNet::SimpleClient::connect()
{
	G_DEBUG( "GNet::SimpleClient::connect: [" << m_remote.str() << "]" ) ;
	if( m_state != Idle )
	{
		G_WARNING( "SimpleClient::connect: invalid state" ) ;
		return ;
	}

	if( m_remote.hasAddress() )
	{
		bool immediate = startConnecting() ;
		if( immediate )
		{
			immediateConnection() ; // calls onConnect()
		}
		else
		{
			setState( Connecting ) ;
		}
	}
	else if( m_sync_dns )
	{
		std::string error = Resolver::resolve( m_remote ) ;
		if( !error.empty() )
		{
			throw DnsError( error ) ;
		}
		bool immediate = startConnecting() ;
		if( immediate )
		{
			immediateConnection() ; // calls onConnect()
		}
		else
		{
			setState( Connecting ) ;
		}
	}
	else
	{
		if( m_resolver.get() == NULL )
			m_resolver <<= new ClientResolver( *this ) ;
		if( !m_resolver->resolveReq( m_remote.str() ) )
			throw DnsError( m_remote.str() ) ;
		setState( Resolving ) ;
	}
}

void GNet::SimpleClient::immediateConnection()
{
	G_DEBUG( "GNet::SimpleClient::connect: immediate connection" ) ;
	socket().addReadHandler( *this ) ;
	socket().addExceptionHandler( *this ) ;
	setState( Connected ) ;
	onConnectImp() ; // from within connect()
	onConnect() ; // from within connect()
}

bool GNet::SimpleClient::canRetry( const std::string & error )
{
	return error.find( c_cannot_connect_to ) == 0U ;
}

unsigned int GNet::SimpleClient::getRandomPort() 
{
	static bool first = true ;
	if( first )
	{
		std::srand( static_cast<unsigned int>(G::DateTime::now()) ) ;
		first = false ;
	}

	int r = std::rand() ;
	if( r < 0 ) r = -r ;
	r = c_port_start + ( r % (c_port_end - c_port_start) ) ;
	G_ASSERT( r > 0 ) ;
	return static_cast<unsigned int>(r) ;
}

void GNet::SimpleClient::close()
{
	m_sp <<= 0 ;
	m_s <<= 0 ;
}

bool GNet::SimpleClient::connected() const
{
	return m_state == Connected ;
}

void GNet::SimpleClient::resolveCon( bool success , const Address & address , std::string name_or_reason )
{
	if( success )
	{
		G_DEBUG( "GNet::SimpleClient::resolveCon: " << address.displayString() ) ;
		std::string peer_name = name_or_reason ;
		m_remote.update( address , peer_name ) ;
		bool immediate = startConnecting() ;
		setState( immediate ? Connected : Connecting ) ;
	}
	else
	{
		throw DnsError( name_or_reason ) ;
	}
}

bool GNet::SimpleClient::startConnecting()
{
	G_DEBUG( "GNet::SimpleClient::startConnecting: " << m_remote.displayString() ) ;

	// create and open a socket
	//
	m_s <<= new StreamSocket( m_remote.address() ) ;
	if( !socket().valid() )
		throw ConnectError( "cannot open socket" ) ;

	// create a socket protocol object
	//
	m_sp <<= new SocketProtocol( *this , *this , *m_s.get() , m_secure_connection_timeout ) ;

	// specifiy this as a 'write' event handler for the socket
	// (before the connect() in case it is reentrant)
	//
	socket().addWriteHandler( *this ) ;

	// bind a local address to the socket and connect
	//
	ConnectStatus status = Failure ;
	std::string error ;
	if( m_privileged )
	{
		for( int i = 0 ; i < c_retries ; i++ )
		{
			unsigned int port = getRandomPort() ;
			m_local_address.setPort( port ) ;
			G_DEBUG( "GNet::SimpleClient::startConnecting: trying to bind " << m_local_address.displayString() ) ;
			status = localBind( m_local_address ) ? Success : Retry ;
			if( status == Retry )
				continue ;

			status = connectCore( m_remote.address() , &error ) ;
			if( status != Retry )
				break ;
		}
	}
	else if( m_local_address == Address(0U) )
	{
		status = connectCore( m_remote.address() , &error ) ;
	}
	else
	{
		if( localBind( m_local_address ) )
			status = connectCore( m_remote.address() , &error ) ;
	}

	// deal with immediate connection (typically if connecting locally)
	//
	bool immediate = status == ImmediateSuccess ;
	if( status != Success )
	{
		socket().dropWriteHandler() ;
	}
	if( immediate && m_remote.socks() )
	{
		immediate = false ;
		socket().addReadHandler( *this ) ;
		socket().addExceptionHandler( *this ) ;
		setState( Socksing ) ;
		sendSocksRequest() ;
	}

	return immediate ;
}

bool GNet::SimpleClient::localBind( Address local_address )
{
	G::Root claim_root ;
	bool bound = socket().bind(local_address) ;
	if( bound )
	{
		G_DEBUG( "GNet::SimpleClient::bind: bound local address " << local_address.displayString() ) ;
	}
	return bound ;
}

GNet::SimpleClient::ConnectStatus GNet::SimpleClient::connectCore( Address remote_address , std::string *error_p )
{
	G_ASSERT( error_p != NULL ) ;
	std::string & error = *error_p ;

	// initiate the connection
	//
	bool immediate = false ;
	if( !socket().connect( remote_address , &immediate ) )
	{
		G_DEBUG( "GNet::SimpleClient::connectCore: immediate failure" ) ;
		error = c_cannot_connect_to + remote_address.displayString() ; // see canRetry()

		// we should return Failure here, but Microsoft's stack
		// will happily bind the same local address more than once,
		// so it is the connect that fails, not the bind, if
		// the port was already in use
		//
		return Retry ;
	}
	else 
	{
		return immediate ? ImmediateSuccess : Success ;
	}
}

void GNet::SimpleClient::writeEvent()
{
	G_DEBUG( "GNet::SimpleClient::writeEvent" ) ;

	if( m_state == Connected )
	{
		if( m_sp->writeEvent() )
			onSendComplete() ;
	}
	else if( m_state == Connecting && socket().hasPeer() )
	{
		socket().addReadHandler( *this ) ;
		socket().addExceptionHandler( *this ) ;
		socket().dropWriteHandler() ;

		if( m_remote.socks() )
		{
			setState( Socksing ) ;
			sendSocksRequest() ;
		}
		else
		{
			setState( Connected ) ;
			onConnectImp() ;
			onConnect() ;
		}
	}
	else if( m_state == Connecting )
	{
		throw G::Exception( c_cannot_connect_to + m_remote.address().displayString() ) ; // see canRetry()
	}
}

void GNet::SimpleClient::readEvent()
{
	G_ASSERT( m_sp.get() != NULL ) ;
	if( m_state == Socksing )
	{
		bool complete = readSocksResponse() ;
		if( complete )
		{
			setState( Connected ) ;
			onConnectImp() ;
			onConnect() ;
		}
	}
	else
	{
		if( m_sp.get() != NULL )
			m_sp->readEvent() ;
	}
}

void GNet::SimpleClient::setState( State new_state )
{
	m_state = new_state ;
}

std::pair<bool,GNet::Address> GNet::SimpleClient::localAddress() const
{
	return 
		m_s.get() != NULL ?
			socket().getLocalAddress() :
			std::make_pair(false,GNet::Address::invalidAddress()) ;
}

std::pair<bool,GNet::Address> GNet::SimpleClient::peerAddress() const
{
	return 
		m_s.get() != NULL ?
			socket().getPeerAddress() :
			std::make_pair(false,GNet::Address::invalidAddress()) ;
}

std::string GNet::SimpleClient::peerCertificate() const
{
	return m_sp->peerCertificate() ;
}

void GNet::SimpleClient::sslConnect()
{
	if( m_sp.get() == NULL )
		throw NotConnected( "for ssl-connect" ) ;
	m_sp->sslConnect() ;
}

void GNet::SimpleClient::onConnectImp()
{
}

bool GNet::SimpleClient::send( const std::string & data , std::string::size_type offset )
{
	bool rc = m_sp->send( data , offset ) ;
	onSendImp() ; // allow derived classes to implement a response timeout
	return rc ;
}

void GNet::SimpleClient::onSendImp()
{
}

void GNet::SimpleClient::sendSocksRequest()
{
	unsigned int far_port = m_remote.socksFarPort() ;
	if( !Address::validPort(far_port) ) throw SocksError("invalid port") ;
	g_port_t far_port_n = htons( static_cast<g_port_t>(far_port) ) ;
	g_port_t far_port_lo = far_port_n & 0xffU ;
	g_port_t far_port_hi = (far_port_n>>8) & 0xffU ;

	std::string userid ; // TODO - socks userid
	std::string data ;
	data.append( 1U , 4 ) ; // version 4
	data.append( 1U , 1 ) ; // connect request
	data.append( 1U , static_cast<char>(far_port_lo) ) ;
	data.append( 1U , static_cast<char>(far_port_hi) ) ;
	data.append( 1U , 0 ) ;
	data.append( 1U , 0 ) ;
	data.append( 1U , 0 ) ;
	data.append( 1U , 1 ) ;
	data.append( userid ) ;
	data.append( 1U , 0 ) ; // NUL
	data.append( m_remote.socksFarHost() ) ;
	data.append( 1U , 0 ) ; // NUL
	GNet::Socket::ssize_type n = socket().write( data.data() , data.size() ) ;
	if( static_cast<std::string::size_type>(n) != data.size() ) // TODO - socks flow control
		throw SocksError( "request not sent" ) ;
}

bool GNet::SimpleClient::readSocksResponse()
{
	char buffer[8] ;
	GNet::Socket::ssize_type rc = socket().read( buffer , sizeof(buffer) ) ;
	if( rc == 0 || ( rc == -1 && !socket().eWouldBlock() ) ) throw SocksError( "read error" ) ;
	else if( rc == -1 ) return false ; // go again
	if( rc != 8 ) throw SocksError( "incomplete response" ) ; // TODO - socks response reassembly
	if( buffer[0] != 0 ) throw SocksError( "invalid response" ) ;
	if( buffer[1] != 'Z' ) throw SocksError( "request rejected" ) ;
	G_LOG( "GNet::SimpleClient::readSocksResponse: " << logId() << ": socks connection completed" ) ;
	return true ;
}

// ===

GNet::ClientResolver::ClientResolver( SimpleClient & client ) :
	Resolver(client) ,
	m_client(client)
{
}

void GNet::ClientResolver::resolveCon( bool success , const Address &address , 
	std::string reason )
{
	m_client.resolveCon( success , address , reason ) ;
}

/// \file gsimpleclient.cpp
