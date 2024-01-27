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
/// \file gclient.cpp
///

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
#include <numeric>
#include <sstream>
#include <cstdlib>

GNet::Client::Client( ExceptionSink es , const Location & remote , const Config & config ) :
	m_es(es) ,
	m_config(config) ,
	m_line_buffer(config.line_buffer_config) ,
	m_remote_location(remote) ,
	m_start_timer(*this,&GNet::Client::onStartTimeout,es) ,
	m_connect_timer(*this,&GNet::Client::onConnectTimeout,es) ,
	m_connected_timer(*this,&GNet::Client::onConnectedTimeout,es) ,
	m_response_timer(*this,&GNet::Client::onResponseTimeout,es) ,
	m_idle_timer(*this,&GNet::Client::onIdleTimeout,es)
{
	G_DEBUG( "Client::ctor" ) ;
	if( m_config.auto_start )
		m_start_timer.startTimer( 0U ) ;
	Monitor::addClient( *this ) ;
}

GNet::Client::~Client()
{
	Monitor::removeClient( *this ) ;
}

#ifndef G_LIB_SMALL
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
#endif

G::Slot::Signal<const std::string&,const std::string&,const std::string&> & GNet::Client::eventSignal() noexcept
{
	return m_event_signal ;
}

GNet::Location GNet::Client::remoteLocation() const
{
	return m_remote_location ;
}

GNet::StreamSocket & GNet::Client::socket()
{
	if( m_socket == nullptr )
		throw NotConnected() ;
	return *m_socket ;
}

const GNet::StreamSocket & GNet::Client::socket() const
{
	if( m_socket == nullptr )
		throw NotConnected() ;
	return *m_socket ;
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
	G_DEBUG( "GNet::Client::connect: [" << m_remote_location.displayString() << "] "
		<< "(" << static_cast<int>(m_state) << ")" ) ;
	if( m_state != State::Idle )
		throw ConnectError( "wrong state" ) ;

	// (one timer covers dns resolution and socket connection)
	if( m_config.connection_timeout )
		m_connect_timer.startTimer( m_config.connection_timeout ) ;

	m_remote_location.resolveTrivially() ; // if host:service is already address:port
	if( m_remote_location.resolved() )
	{
		setState( State::Connecting ) ;
		startConnecting() ;
	}
	else if( m_config.sync_dns || !Resolver::async() )
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
		if( m_resolver == nullptr )
		{
			Resolver::Callback & resolver_callback = *this ;
			m_resolver = std::make_unique<Resolver>( resolver_callback , m_es ) ;
		}
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
	G_DEBUG( "GNet::Client::startConnecting: local: " << m_config.local_address.displayString() ) ;
	G_DEBUG( "GNet::Client::startConnecting: remote: " << m_remote_location.displayString() ) ;
	if( G::Test::enabled("client-slow-connect") )
		setState( State::Testing ) ;

	// create and open a socket
	//
	m_sp.reset() ;
	m_socket = std::make_unique<StreamSocket>( m_remote_location.address().family() , m_config.stream_socket_config ) ;
	socket().addWriteHandler( *this , m_es ) ;
	socket().addOtherHandler( *this , m_es ) ;

	// create a socket protocol object
	//
	EventHandler & eh = *this ;
	SocketProtocolSink & sp_sink = *this ;
	m_sp = std::make_unique<SocketProtocol>( eh , m_es , sp_sink , *m_socket , m_config.socket_protocol_config ) ;

	// bind a local address to the socket (throws on failure)
	//
	if( m_config.bind_local_address )
		bindLocalAddress( m_config.local_address ) ;

	// start connecting
	//
	bool immediate = false ;
	if( !socket().connect( m_remote_location.address() , &immediate ) )
		throw ConnectError( "cannot connect to " + m_remote_location.address().displayString() + ": " + socket().reason() ) ;

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

void GNet::Client::finish()
{
	m_finished = true ;
	if( m_sp != nullptr )
	{
		m_sp->shutdown() ;
	}
	else if( m_socket != nullptr )
	{
		m_socket->dropWriteHandler() ;
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
	m_event_signal.emit( std::string(action) , m_remote_location.displayString() , std::string() ) ;
}

void GNet::Client::onConnectTimeout()
{
	std::ostringstream ss ;
	ss << "cannot connect to " << m_remote_location << ": timed out out after " << m_config.connection_timeout << "s" ;
	G_DEBUG( "GNet::Client::onConnectTimeout: " << ss.str() ) ;
	throw ConnectError( ss.str() ) ;
}

void GNet::Client::onResponseTimeout()
{
	std::ostringstream ss ;
	ss << "no response after " << m_config.response_timeout << "s while connected to " << m_remote_location ;
	G_DEBUG( "GNet::Client::onResponseTimeout: response timeout: " << ss.str() ) ;
	throw ResponseTimeout( ss.str() ) ;
}

void GNet::Client::onIdleTimeout()
{
	std::ostringstream ss ;
	ss << "no activity after " << m_config.idle_timeout << "s while connected to " << m_remote_location ;
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
	bool has_peer = m_state == State::Connecting && socket().getPeerAddress().first ;
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
	else if( m_state == State::Connecting && has_peer && m_remote_location.socks() )
	{
		setState( State::Socksing ) ;
		m_socks = std::make_unique<Socks>( m_remote_location ) ;
		if( m_socks->send( socket() ) )
		{
			socket().dropWriteHandler() ;
			socket().addReadHandler( *this , m_es ) ; // wait for the socks response
		}
		else
		{
			socket().addWriteHandler( *this , m_es ) ;
			socket().dropReadHandler() ;
		}
	}
	else if( m_state == State::Connecting && has_peer )
	{
		socket().dropWriteHandler() ;
		socket().addReadHandler( *this , m_es ) ;

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
		G_ASSERT( m_socks != nullptr ) ;
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
	if( m_state == State::Socksing || m_sp == nullptr )
		EventHandler::otherEvent( reason ) ; // default implementation throws
	else
		m_sp->otherEvent( reason , m_config.no_throw_on_peer_disconnect ) ;
}

void GNet::Client::readEvent()
{
	if( m_state == State::Socksing )
	{
		G_ASSERT( m_socks != nullptr ) ;
		bool complete = m_socks->read( socket() ) ;
		if( complete )
		{
			setState( State::Connected ) ;
			doOnConnect() ;
		}
	}
	else
	{
		G_ASSERT( m_sp != nullptr ) ;
		if( m_sp->readEvent( m_config.no_throw_on_peer_disconnect ) )
			onSendComplete() ;
	}
}

void GNet::Client::onData( const char * data , std::size_t size )
{
	if( m_config.response_timeout && m_line_buffer.transparent() ) // anything will do if transparent
		m_response_timer.cancelTimer() ;

	if( m_config.idle_timeout )
		m_idle_timer.startTimer( m_config.idle_timeout ) ;

	bool fragments = m_line_buffer.transparent() ;
	m_line_buffer.apply( this , &Client::onDataImp , data , size , fragments ) ;
}

bool GNet::Client::onDataImp( const char * data , std::size_t size , std::size_t eolsize ,
	std::size_t linesize , char c0 )
{
	if( m_config.response_timeout && eolsize ) // end of a complete line
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

	if( new_state == State::Connected && m_config.idle_timeout )
		m_idle_timer.startTimer( m_config.idle_timeout ) ;

	m_state = new_state ;
}

GNet::Address GNet::Client::localAddress() const
{
	return socket().getLocalAddress() ;
}

GNet::Address GNet::Client::peerAddress() const
{
	if( m_state != State::Connected )
		throw NotConnected() ;

	auto pair = socket().getPeerAddress() ;
	if( !pair.first )
		throw NotConnected() ;

	return pair.second ;
}

std::string GNet::Client::peerAddressString( bool with_port ) const
{
	if( m_state == State::Connected )
	{
		auto pair = socket().getPeerAddress() ;
		if( pair.first && with_port )
			return pair.second.displayString() ;
		else if( pair.first )
			return pair.second.hostPartString() ;
	}
	return {} ;
}

std::string GNet::Client::connectionState() const
{
	if( m_state == State::Connected )
		return socket().getPeerAddress().second.displayString() ;
	else
		return "("+m_remote_location.displayString()+")" ;
}

std::string GNet::Client::peerCertificate() const
{
	return m_sp->peerCertificate() ;
}

#ifndef G_LIB_SMALL
bool GNet::Client::secureConnectCapable() const
{
	return m_sp && m_sp->secureConnectCapable() ;
}
#endif

void GNet::Client::secureConnect()
{
	if( m_sp == nullptr )
		throw NotConnected( "for secure-connect" ) ;
	m_sp->secureConnect() ;
}

bool GNet::Client::send( const std::string & data )
{
	if( m_config.response_timeout )
		m_response_timer.startTimer( m_config.response_timeout ) ;
	return m_sp->send( data , 0U ) ;
}

bool GNet::Client::send( G::string_view data )
{
	if( m_config.response_timeout )
		m_response_timer.startTimer( m_config.response_timeout ) ;
	return m_sp->send( data ) ;
}

#ifndef G_LIB_SMALL
bool GNet::Client::send( const std::vector<G::string_view> & data , std::size_t offset )
{
    std::size_t total_size = std::accumulate( data.begin() , data.end() , std::size_t(0) ,
        [](std::size_t n,G::string_view s){return n+s.size();} ) ;
	if( m_config.response_timeout && offset < total_size )
		m_response_timer.startTimer( m_config.response_timeout ) ;
	return m_sp->send( data , offset ) ;
}
#endif

#ifndef G_LIB_SMALL
GNet::LineBufferState GNet::Client::lineBuffer() const
{
	return m_line_buffer.state() ;
}
#endif

void GNet::Client::onPeerDisconnect()
{
}

// ==

#ifndef G_LIB_SMALL
GNet::Client::Config & GNet::Client::Config::set_all_timeouts( unsigned int all_timeouts ) noexcept
{
	socket_protocol_config.secure_connection_timeout = all_timeouts ;
	connection_timeout = all_timeouts ;
	response_timeout = all_timeouts ;
	idle_timeout = all_timeouts * 2U ;
	return *this ;
}
#endif

