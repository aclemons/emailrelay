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
//
// gsimpleclient.cpp
//

#include "gdef.h"
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
	const char * c_cannot_connect_to = "cannot connect to " ;
}

// ==

GNet::SimpleClient::SimpleClient( ExceptionHandler & eh ,
	const Location & remote , bool bind_local_address , const Address & local_address ,
	bool sync_dns , unsigned int secure_connection_timeout ) :
		m_eh(eh) ,
		m_resolver(*this,eh) ,
		m_remote_location(remote) ,
		m_bind_local_address(bind_local_address) ,
		m_local_address(local_address) ,
		m_state(Idle) ,
		m_sync_dns(sync_dns) ,
		m_secure_connection_timeout(secure_connection_timeout) ,
		m_on_connect_timer(*this,&GNet::SimpleClient::onConnectTimer,eh)
{
	G_DEBUG( "SimpleClient::ctor" ) ;
	if( Monitor::instance() ) Monitor::instance()->addClient( *this ) ;
}

GNet::SimpleClient::~SimpleClient()
{
	if( Monitor::instance() ) Monitor::instance()->removeClient( *this ) ;
	m_sp.reset() ;
	m_socket.reset() ;
}

std::string GNet::SimpleClient::logId() const
{
	std::string s = m_remote_location.displayString() ;
	if( m_socket.get() != nullptr )
		s.append( std::string() + "@" + m_socket->asString() ) ; // cf. ServerPeer::logId()
	return s ;
}

GNet::Location GNet::SimpleClient::remoteLocation() const
{
	return m_remote_location ;
}

void GNet::SimpleClient::updateLocation( const Location & update )
{
	if( m_remote_location.host() == update.host() && m_remote_location.service() == update.service() && update.resolved() )
	{
		G_DEBUG( "GNet::SimpleClient::updateLocation: reusing dns lookup for " << update.displayString() ) ;
		m_remote_location = update ;
	}
}

GNet::StreamSocket & GNet::SimpleClient::socket()
{
	if( m_socket.get() == nullptr )
		throw NotConnected() ;
	return *m_socket.get() ;
}

const GNet::StreamSocket & GNet::SimpleClient::socket() const
{
	if( m_socket.get() == nullptr )
		throw NotConnected() ;
	return *m_socket.get() ;
}

void GNet::SimpleClient::connect()
{
	G_DEBUG( "GNet::SimpleClient::connect: [" << m_remote_location.displayString() << "]" ) ;
	if( m_state != Idle )
		throw ConnectError( "wrong state" ) ;

	m_remote_location.resolveTrivially() ; // if host:service is already address:port
	if( m_remote_location.resolved() )
	{
		setState( Connecting ) ;
		startConnecting() ;
	}
	else if( m_sync_dns || !Resolver::async() )
	{
		std::string error = Resolver::resolve( m_remote_location ) ;
		if( !error.empty() )
			throw DnsError( error ) ;

		setState( Connecting ) ;
		startConnecting() ;
	}
	else
	{
		setState( Resolving ) ;
		m_resolver.start( m_remote_location ) ;
	}
}

void GNet::SimpleClient::onResolved( std::string error , Location location )
{
	if( !error.empty() )
		throw DnsError( error ) ;

	G_DEBUG( "GNet::SimpleClient::onResolved: " << location.displayString() ) ;
	m_remote_location.update( location.address() , location.name() ) ;
	setState( Connecting ) ;
	startConnecting() ;
}

void GNet::SimpleClient::startConnecting()
{
	G_DEBUG( "GNet::SimpleClient::startConnecting: local: " << m_local_address.displayString() ) ;
	G_DEBUG( "GNet::SimpleClient::startConnecting: remote: " << m_remote_location.displayString() ) ;
	if( G::Test::enabled("client-slow-connect") )
		setState( Testing ) ;

	// create and open a socket
	//
	m_socket.reset( new StreamSocket(m_remote_location.address().domain()) ) ;
	socket().addWriteHandler( *this , m_eh ) ;

	// create a socket protocol object
	//
	m_sp.reset( new SocketProtocol(*this,m_eh,*this,*m_socket.get(),m_secure_connection_timeout) ) ;

	// bind a local address to the socket (throws on failure)
	//
	if( m_bind_local_address )
		bindLocalAddress( m_local_address ) ;

	// start connecting
	//
	bool immediate = false ;
	if( !socket().connect( m_remote_location.address() , &immediate ) )
		throw ConnectError( c_cannot_connect_to + m_remote_location.address().displayString() ) ;

	// deal with immediate connection (typically if connecting locally)
	//
	if( immediate )
	{
		socket().dropWriteHandler() ;
		m_on_connect_timer.startTimer( 0U ) ; // -> onConnectTimer()
	}
}

void GNet::SimpleClient::onConnectTimer()
{
	G_DEBUG( "GNet::SimpleClient::onConnectTimer: immediate connection" ) ;
	onWriteable() ;
}

void GNet::SimpleClient::writeEvent()
{
	G_DEBUG( "GNet::SimpleClient::writeEvent" ) ;
	onWriteable() ;
}

void GNet::SimpleClient::onWriteable()
{
	if( m_state == Connected )
	{
		if( m_sp->writeEvent() )
			onSendComplete() ;
	}
	else if( m_state == Testing )
	{
		socket().dropWriteHandler() ;
		setState( Connecting ) ;
		m_on_connect_timer.startTimer( 2U , 100000U ) ; // -> onConnectTimer()
	}
	else if( m_state == Connecting && socket().hasPeer() && m_remote_location.socks() )
	{
		setState( Socksing ) ;
		m_socks.reset( new Socks(m_remote_location) ) ;
		if( m_socks->send( socket() ) )
		{
			socket().addOtherHandler( *this , m_eh ) ;
			socket().dropWriteHandler() ;
			socket().addReadHandler( *this , m_eh ) ; // wait for the socks response
		}
		else
		{
			socket().addOtherHandler( *this , m_eh ) ;
			socket().addWriteHandler( *this , m_eh ) ;
			socket().dropReadHandler() ;
		}
	}
	else if( m_state == Connecting && socket().hasPeer() )
	{
		socket().dropWriteHandler() ;
		socket().addReadHandler( *this , m_eh ) ;
		socket().addOtherHandler( *this , m_eh ) ;

		setState( Connected ) ;
		onConnectImp() ;
		onConnect() ;
	}
	else if( m_state == Connecting )
	{
		socket().dropWriteHandler() ;
		throw ConnectError( c_cannot_connect_to + m_remote_location.address().displayString() ) ;
	}
	else if( m_state == Socksing )
	{
		G_ASSERT( m_socks.get() != nullptr ) ;
		if( m_socks->send( socket() ) )
		{
			socket().dropWriteHandler() ;
			socket().addReadHandler( *this , m_eh ) ;

			setState( Connected ) ;
			onConnectImp() ;
			onConnect() ;
		}
	}
}

void GNet::SimpleClient::otherEvent( EventHandler::Reason reason )
{
	if( m_state == Socksing || m_sp.get() == nullptr )
		EventHandler::otherEvent( reason ) ; // default implementation
	else
		m_sp->otherEvent( reason ) ;
}

void GNet::SimpleClient::readEvent()
{
	G_ASSERT( m_sp.get() != nullptr ) ;
	if( m_state == Socksing )
	{
		G_ASSERT( m_socks.get() != nullptr ) ;
		bool complete = m_socks->read( socket() ) ;
		if( complete )
		{
			setState( Connected ) ;
			onConnectImp() ;
			onConnect() ;
		}
	}
	else
	{
		if( m_sp.get() != nullptr )
			m_sp->readEvent() ;
	}
}

bool GNet::SimpleClient::connectError( const std::string & error )
{
	return error.find( c_cannot_connect_to ) == 0U ;
}

void GNet::SimpleClient::close()
{
	m_sp.reset() ;
	m_socket.reset() ;
}

bool GNet::SimpleClient::connected() const
{
	return m_state == Connected ;
}

void GNet::SimpleClient::bindLocalAddress( const Address & local_address )
{
	{
		G::Root claim_root ;
		socket().bind( local_address ) ;
	}

	if( local_address.isLoopback() && !m_remote_location.address().isLoopback() )
		G_WARNING_ONCE( "GNet::SimpleClient::bindLocalAddress: binding the loopback address for "
			"outgoing connections may result in connection failures" ) ;
}

void GNet::SimpleClient::setState( State new_state )
{
	m_state = new_state ;
}

std::pair<bool,GNet::Address> GNet::SimpleClient::localAddress() const
{
	return
		m_socket.get() != nullptr ?
			socket().getLocalAddress() :
			std::make_pair(false,GNet::Address::defaultAddress()) ;
}

std::pair<bool,GNet::Address> GNet::SimpleClient::peerAddress() const
{
	return
		m_socket.get() != nullptr ?
			socket().getPeerAddress() :
			std::make_pair(false,GNet::Address::defaultAddress()) ;
}

std::string GNet::SimpleClient::connectionState() const
{
	std::pair<bool,Address> pair =
		m_socket.get() != nullptr ?
			socket().getPeerAddress() :
			std::make_pair(false,GNet::Address::defaultAddress()) ;

	return
		pair.first ?
			pair.second.displayString() :
			("("+m_remote_location.displayString()+")") ;
}

std::string GNet::SimpleClient::peerCertificate() const
{
	return m_sp->peerCertificate() ;
}

void GNet::SimpleClient::secureConnect()
{
	if( m_sp.get() == nullptr )
		throw NotConnected( "for secure-connect" ) ;
	m_sp->secureConnect() ;
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

bool GNet::SimpleClient::synchronousDnsDefault()
{
	if( G::Test::enabled("client-dns-asynchronous") ) return false ;
	if( G::Test::enabled("client-dns-synchronous") ) return true ;
	return false ; // default to async, but G::thread might run synchronously as a build-time option
}

/// \file gsimpleclient.cpp
