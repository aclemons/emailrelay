//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gclient.cpp
//

#include "gdef.h"
#include "gaddress.h"
#include "gsocket.h"
#include "gdatetime.h"
#include "gexception.h"
#include "gresolver.h"
#include "groot.h"
#include "gmonitor.h"
#include "gclient.h"
#include "gassert.h"
#include "gtest.h"
#include "glog.h"
#include <sstream>
#include <cstdlib>

GNet::Client::Client( ExceptionSink es , const Location & remote , Config config ) :
	m_es(es) ,
	m_line_buffer(config.line_buffer_config) ,
	m_remote_location(remote) ,
	m_bind_local_address(config.bind_local_address) ,
	m_local_address(config.local_address) ,
	m_sync_dns(config.sync_dns) ,
	m_secure_connection_timeout(config.secure_connection_timeout) ,
	m_connection_timeout(config.connection_timeout) ,
	m_response_timeout(config.response_timeout) ,
	m_idle_timeout(config.idle_timeout) ,
	m_state(State::Idle) ,
	m_finished(false) ,
	m_has_connected(false) ,
	m_start_timer(*this,&GNet::Client::onStartTimeout,es) ,
	m_connect_timer(*this,&GNet::Client::onConnectTimeout,es) ,
	m_connected_timer(*this,&GNet::Client::onConnectedTimeout,es) ,
	m_response_timer(*this,&GNet::Client::onResponseTimeout,es) ,
	m_idle_timer(*this,&GNet::Client::onIdleTimeout,es)
{
	G_DEBUG( "Client::ctor" ) ;
	if( config.auto_start )
		m_start_timer.startTimer( 0U ) ;
	Monitor::addClient( *this ) ;
}

GNet::Client::~Client()
{
	Monitor::removeClient( *this ) ;
	m_sp.reset() ;
}

void GNet::Client::disconnect()
{
	G_DEBUG( "GNet::Client::disconnect" ) ;

	m_start_timer.cancelTimer() ;
	m_connect_timer.cancelTimer() ;
	m_connected_timer.cancelTimer() ;
	m_response_timer.cancelTimer() ;
	m_idle_timer.cancelTimer() ;

	m_state = State::Disconnected ;
	m_finished = true ;

	m_sp.reset() ;
	m_socket.reset() ;
	m_resolver.reset() ;
}

G::Slot::Signal3<std::string,std::string,std::string> & GNet::Client::eventSignal()
{
	return m_event_signal ;
}

GNet::Location GNet::Client::remoteLocation() const
{
	return m_remote_location ;
}

GNet::StreamSocket & GNet::Client::socket()
{
	if( m_socket.get() == nullptr )
		throw NotConnected() ;
	return *m_socket.get() ;
}

const GNet::StreamSocket & GNet::Client::socket() const
{
	if( m_socket.get() == nullptr )
		throw NotConnected() ;
	return *m_socket.get() ;
}

void GNet::Client::clearInput()
{
	m_line_buffer.clear() ;
	m_response_timer.cancelTimer() ;
}

void GNet::Client::onStartTimeout()
{
	G_DEBUG( "GNet::Client::onStartTimeout: auto-start connecting" ) ;
	connect() ;
}

void GNet::Client::connect()
{
	G_DEBUG( "GNet::Client::connect: [" << m_remote_location.displayString() << "] (" << static_cast<int>(m_state) << ")" ) ;
	if( m_state != State::Idle )
		throw ConnectError( "wrong state" ) ;

	// (one timer covers dns resolution and socket connection)
	if( m_connection_timeout )
		m_connect_timer.startTimer( m_connection_timeout ) ;

	m_remote_location.resolveTrivially() ; // if host:service is already address:port
	if( m_remote_location.resolved() )
	{
		setState( State::Connecting ) ;
		startConnecting() ;
	}
	else if( m_sync_dns || !Resolver::async() )
	{
		std::string error = Resolver::resolve( m_remote_location ) ;
		if( !error.empty() )
			throw DnsError( error ) ;

		setState( State::Connecting ) ;
		startConnecting() ;
	}
	else
	{
		setState( State::Resolving ) ;
		if( m_resolver.get() == nullptr )
			m_resolver.reset( new Resolver(*this,m_es) ) ;
		m_resolver->start( m_remote_location ) ;
		emit( "resolving" ) ;
	}
}

void GNet::Client::onResolved( std::string error , Location location )
{
	if( !error.empty() )
		throw DnsError( error ) ;

	G_DEBUG( "GNet::Client::onResolved: " << location.displayString() ) ;
	m_remote_location.update( location.address() , location.name() ) ;
	setState( State::Connecting ) ;
	startConnecting() ;
}

void GNet::Client::startConnecting()
{
	G_DEBUG( "GNet::Client::startConnecting: local: " << m_local_address.displayString() ) ;
	G_DEBUG( "GNet::Client::startConnecting: remote: " << m_remote_location.displayString() ) ;
	if( G::Test::enabled("client-slow-connect") )
		setState( State::Testing ) ;

	// create and open a socket
	//
	m_sp.reset() ;
	m_socket.reset( new StreamSocket(m_remote_location.address().domain()) ) ;
	socket().addWriteHandler( *this , m_es ) ;

	// create a socket protocol object
	//
	m_sp.reset( new SocketProtocol(*this,m_es,*this,*m_socket.get(),m_secure_connection_timeout) ) ;

	// bind a local address to the socket (throws on failure)
	//
	if( m_bind_local_address )
		bindLocalAddress( m_local_address ) ;

	// start connecting
	//
	bool immediate = false ;
	if( !socket().connect( m_remote_location.address() , &immediate ) )
		throw ConnectError( "cannot connect to " + m_remote_location.address().displayString() ) ;

	// deal with immediate connection (typically if connecting locally)
	//
	if( immediate )
	{
		socket().dropWriteHandler() ;
		m_connected_timer.startTimer( 0U ) ; // -> onConnectedTimeout()
	}
	else
	{
		emit( "connecting" ) ;
	}
}

void GNet::Client::finish( bool with_socket_shutdown )
{
	m_finished = true ;
	if( with_socket_shutdown )
	{
		if( m_sp.get() != nullptr )
			m_sp->shutdown() ;
		else if( m_socket.get() != nullptr )
			m_socket->shutdown() ;
	}
}

bool GNet::Client::finished() const
{
	return m_finished ;
}

bool GNet::Client::hasConnected() const
{
	return m_has_connected ;
}

void GNet::Client::doOnDelete( const std::string & reason , bool done )
{
	onDelete( (done||m_finished) ? std::string() : reason ) ;
}

void GNet::Client::emit( const std::string & action )
{
	m_event_signal.emit( action , m_remote_location.displayString() , std::string() ) ;
}

void GNet::Client::onConnectTimeout()
{
	std::ostringstream ss ;
	ss << "cannot connect to " << m_remote_location << ": timed out out after " << m_connection_timeout << "s" ;
	G_DEBUG( "GNet::Client::onConnectTimeout: " << ss.str() ) ;
	throw ConnectError( ss.str() ) ;
}

void GNet::Client::onResponseTimeout()
{
	std::ostringstream ss ;
	ss << "no response after " << m_response_timeout << " while connected to " << m_remote_location ;
	G_DEBUG( "GNet::Client::onResponseTimeout: response timeout: " << ss.str() ) ;
	throw ResponseTimeout( ss.str() ) ;
}

void GNet::Client::onIdleTimeout()
{
	std::ostringstream ss ;
	ss << "no activity after " << m_idle_timeout << "s while connected to " << m_remote_location ;
	throw IdleTimeout( ss.str() ) ;
}

void GNet::Client::onConnectedTimeout()
{
	G_DEBUG( "GNet::Client::onConnectedTimeout: immediate connection" ) ;
	onWriteable() ;
}

void GNet::Client::writeEvent()
{
	G_DEBUG( "GNet::Client::writeEvent" ) ;
	onWriteable() ;
}

void GNet::Client::onWriteable()
{
	if( m_state == State::Connected )
	{
		if( m_sp->writeEvent() )
			onSendComplete() ;
	}
	else if( m_state == State::Testing )
	{
		socket().dropWriteHandler() ;
		setState( State::Connecting ) ;
		m_connected_timer.startTimer( 2U , 100000U ) ; // -> onConnectedTimeout()
	}
	else if( m_state == State::Connecting && socket().hasPeer() && m_remote_location.socks() )
	{
		setState( State::Socksing ) ;
		m_socks.reset( new Socks(m_remote_location) ) ;
		if( m_socks->send( socket() ) )
		{
			socket().addOtherHandler( *this , m_es ) ;
			socket().dropWriteHandler() ;
			socket().addReadHandler( *this , m_es ) ; // wait for the socks response
		}
		else
		{
			socket().addOtherHandler( *this , m_es ) ;
			socket().addWriteHandler( *this , m_es ) ;
			socket().dropReadHandler() ;
		}
	}
	else if( m_state == State::Connecting && socket().hasPeer() )
	{
		socket().dropWriteHandler() ;
		socket().addReadHandler( *this , m_es ) ;
		socket().addOtherHandler( *this , m_es ) ;

		setState( State::Connected ) ;
		doOnConnect() ;
	}
	else if( m_state == State::Connecting )
	{
		socket().dropWriteHandler() ;
		throw ConnectError( "cannot connect to " + m_remote_location.address().displayString() ) ;
	}
	else if( m_state == State::Socksing )
	{
		G_ASSERT( m_socks.get() != nullptr ) ;
		if( m_socks->send( socket() ) )
		{
			socket().dropWriteHandler() ;
			socket().addReadHandler( *this , m_es ) ;

			setState( State::Connected ) ;
			doOnConnect() ;
		}
	}
	else if( m_state == State::Disconnected )
	{
		// never gets here
	}
}

void GNet::Client::doOnConnect()
{
	G::CallFrame this_( m_call_stack ) ;
	onConnect() ;
	if( this_.deleted() ) return ;
	emit( "connected" ) ;
}

void GNet::Client::otherEvent( EventHandler::Reason reason )
{
	if( m_state == State::Socksing || m_sp.get() == nullptr )
		EventHandler::otherEvent( reason ) ; // default implementation
	else
		m_sp->otherEvent( reason ) ;
}

void GNet::Client::readEvent()
{
	G_ASSERT( m_sp.get() != nullptr ) ;
	if( m_state == State::Socksing )
	{
		G_ASSERT( m_socks.get() != nullptr ) ;
		bool complete = m_socks->read( socket() ) ;
		if( complete )
		{
			setState( State::Connected ) ;
			doOnConnect() ;
		}
	}
	else
	{
		if( m_sp.get() != nullptr )
			m_sp->readEvent() ;
	}
}

void GNet::Client::onData( const char * data , size_t size )
{
	if( m_response_timeout && m_line_buffer.transparent() ) // anything will do
		m_response_timer.cancelTimer() ;

	if( m_idle_timeout )
		m_idle_timer.startTimer( m_idle_timeout ) ;

	bool fragments = m_line_buffer.transparent() ;
	m_line_buffer.apply( this , &Client::onDataImp , data , size , fragments ) ;
}

bool GNet::Client::onDataImp( const char * data , size_t size , size_t eolsize , size_t linesize , char c0 )
{
	if( m_response_timeout && eolsize ) // end of a complete line
		m_response_timer.cancelTimer() ;

	return onReceive( data , size , eolsize , linesize , c0 ) ;
}

bool GNet::Client::connected() const
{
	return m_state == State::Connected ;
}

void GNet::Client::bindLocalAddress( const Address & local_address )
{
	{
		G::Root claim_root ;
		socket().bind( local_address ) ;
	}

	if( local_address.isLoopback() && !m_remote_location.address().isLoopback() )
		G_WARNING_ONCE( "GNet::Client::bindLocalAddress: binding the loopback address for "
			"outgoing connections may result in connection failures" ) ;
}

void GNet::Client::setState( State new_state )
{
	if( new_state != State::Connecting && new_state != State::Resolving )
		m_connect_timer.cancelTimer() ;

	if( new_state == State::Connected )
		m_has_connected = true ;

	if( new_state == State::Connected && m_idle_timeout )
		m_idle_timer.startTimer( m_idle_timeout ) ;

	m_state = new_state ;
}

std::pair<bool,GNet::Address> GNet::Client::localAddress() const
{
	return
		m_socket.get() != nullptr ?
			socket().getLocalAddress() :
			std::make_pair(false,GNet::Address::defaultAddress()) ;
}

std::pair<bool,GNet::Address> GNet::Client::peerAddress() const
{
	return
		m_socket.get() != nullptr ?
			socket().getPeerAddress() :
			std::make_pair(false,GNet::Address::defaultAddress()) ;
}

std::string GNet::Client::connectionState() const
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

std::string GNet::Client::peerCertificate() const
{
	return m_sp->peerCertificate() ;
}

void GNet::Client::secureConnect()
{
	if( m_sp.get() == nullptr )
		throw NotConnected( "for secure-connect" ) ;
	m_sp->secureConnect() ;
}

bool GNet::Client::send( const std::string & data , size_t offset )
{
	if( m_response_timeout && data.size() > offset )
		m_response_timer.startTimer( m_response_timeout ) ;
	return m_sp->send( data , offset ) ;
}

GNet::LineBufferState GNet::Client::lineBuffer() const
{
	return m_line_buffer.state() ;
}

// ==

namespace
{
	bool sync_default()
	{
		if( G::Test::enabled("client-dns-asynchronous") ) return false ;
		if( G::Test::enabled("client-dns-synchronous") ) return true ;
		return false ;
	}
}

GNet::Client::Config::Config() :
	sync_dns(sync_default()) ,
	auto_start(true) ,
	bind_local_address(false) ,
	local_address(Address::defaultAddress()) ,
	connection_timeout(0U) ,
	secure_connection_timeout(0U) ,
	response_timeout(0U) ,
	idle_timeout(0U) ,
	line_buffer_config(LineBufferConfig::transparent())
{
}

GNet::Client::Config::Config( LineBufferConfig lbc ) :
	sync_dns(sync_default()) ,
	auto_start(true) ,
	bind_local_address(false) ,
	local_address(Address::defaultAddress()) ,
	connection_timeout(0U) ,
	secure_connection_timeout(0U) ,
	response_timeout(0U) ,
	idle_timeout(0U) ,
	line_buffer_config(lbc)
{
}

GNet::Client::Config::Config( LineBufferConfig lbc , unsigned int all_timeouts ) :
	sync_dns(sync_default()) ,
	auto_start(true) ,
	bind_local_address(false) ,
	local_address(Address::defaultAddress()) ,
	connection_timeout(all_timeouts) ,
	secure_connection_timeout(all_timeouts) ,
	response_timeout(all_timeouts) ,
	idle_timeout(all_timeouts*2U) ,
	line_buffer_config(lbc)
{
}

GNet::Client::Config::Config( LineBufferConfig lbc , unsigned int connection_timeout_in ,
	unsigned int secure_connection_timeout_in , unsigned int response_timeout_in ,
	unsigned int idle_timeout_in ) :
		sync_dns(sync_default()) ,
		auto_start(true) ,
		bind_local_address(false) ,
		local_address(Address::defaultAddress()) ,
		connection_timeout(connection_timeout_in) ,
		secure_connection_timeout(secure_connection_timeout_in) ,
		response_timeout(response_timeout_in) ,
		idle_timeout(idle_timeout_in) ,
		line_buffer_config(lbc)
{
}

GNet::Client::Config & GNet::Client::Config::setTimeouts( unsigned int all_timeouts )
{
	connection_timeout = all_timeouts ;
	secure_connection_timeout = all_timeouts ;
	response_timeout = all_timeouts ;
	idle_timeout = all_timeouts * 2U ;
	return *this ;
}

GNet::Client::Config & GNet::Client::Config::setAutoStart( bool auto_start_in )
{
	auto_start = auto_start_in ;
	return *this ;
}

/// \file gclient.cpp
