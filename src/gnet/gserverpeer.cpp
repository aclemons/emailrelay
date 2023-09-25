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
/// \file gserverpeer.cpp
///

#include "gdef.h"
#include "gserver.h"
#include "gmonitor.h"
#include "gtest.h"
#include "glog.h"
#include "gassert.h"
#include <sstream>
#include <utility>

GNet::ServerPeer::ServerPeer( ExceptionSink es , ServerPeerInfo && peer_info , const LineBuffer::Config & line_buffer_config ) :
	m_es(es) ,
	m_address(peer_info.m_address) ,
	m_socket(std::move(peer_info.m_socket)) ,
	m_sp(*this,es,*this,*m_socket,peer_info.m_server_peer_config.socket_protocol_config) ,
	m_line_buffer(line_buffer_config) ,
	m_config(peer_info.m_server_peer_config) ,
	m_idle_timer(*this,&ServerPeer::onIdleTimeout,es)
{
	G_ASSERT( peer_info.m_server != nullptr ) ;
	G_ASSERT( m_socket.get() ) ;
	//G_ASSERT( es.esrc() != nullptr ) ; // moot
	G_DEBUG( "GNet::ServerPeer::ctor: [" << this << "]: port " << m_address.port() ) ;

	if( m_config.idle_timeout )
		m_idle_timer.startTimer( m_config.idle_timeout ) ;

	m_socket->addReadHandler( *this , es ) ;
	m_socket->addOtherHandler( *this , es ) ;
	Monitor::addServerPeer( *this ) ;
}

GNet::ServerPeer::~ServerPeer()
{
	G_DEBUG( "GNet::ServerPeer::dtor: [" << this << "]: port " << m_address.port() ) ;
	Monitor::removeServerPeer( *this ) ;
}

void GNet::ServerPeer::secureAccept()
{
	m_sp.secureAccept() ;
}

bool GNet::ServerPeer::secureAcceptCapable() const
{
	return m_sp.secureAcceptCapable() ;
}

#ifndef G_LIB_SMALL
void GNet::ServerPeer::expect( std::size_t n )
{
	m_line_buffer.expect( n ) ;
}
#endif

GNet::StreamSocket & GNet::ServerPeer::socket()
{
	G_ASSERT( m_socket != nullptr ) ;
	return *m_socket ;
}

void GNet::ServerPeer::dropReadHandler()
{
	socket().dropReadHandler() ;
}

void GNet::ServerPeer::addReadHandler()
{
	socket().addReadHandler( *this , m_es ) ;
}

void GNet::ServerPeer::otherEvent( EventHandler::Reason reason )
{
	m_sp.otherEvent( reason , m_config.no_throw_on_peer_disconnect ) ;
}

void GNet::ServerPeer::readEvent()
{
	if( m_sp.readEvent( m_config.no_throw_on_peer_disconnect ) )
		onSendComplete() ;
}

GNet::Address GNet::ServerPeer::localAddress() const
{
	G_ASSERT( m_socket != nullptr ) ;
	return m_socket->getLocalAddress() ;
}

GNet::Address GNet::ServerPeer::peerAddress() const
{
	return m_address ;
}

std::string GNet::ServerPeer::connectionState() const
{
	return m_address.displayString() ;
}

std::string GNet::ServerPeer::peerCertificate() const
{
	return m_sp.peerCertificate() ;
}

bool GNet::ServerPeer::send( const std::string & data )
{
	if( m_config.kick_idle_timer_on_send && m_config.idle_timeout )
		m_idle_timer.startTimer( m_config.idle_timeout ) ;
	return m_sp.send( data , 0U ) ;
}

bool GNet::ServerPeer::send( G::string_view data )
{
	if( m_config.kick_idle_timer_on_send && m_config.idle_timeout )
		m_idle_timer.startTimer( m_config.idle_timeout ) ;
	return m_sp.send( data ) ;
}

#ifndef G_LIB_SMALL
bool GNet::ServerPeer::send( const std::vector<G::string_view> & segments , std::size_t offset )
{
	if( m_config.kick_idle_timer_on_send && m_config.idle_timeout )
		m_idle_timer.startTimer( m_config.idle_timeout ) ;
	return m_sp.send( segments , offset ) ;
}
#endif

void GNet::ServerPeer::writeEvent()
{
	if( m_sp.writeEvent() )
		onSendComplete() ;
}

void GNet::ServerPeer::doOnDelete( const std::string & reason , bool done )
{
	G_DEBUG( "GNet::ServerPeer::doOnDelete: reason=[" << reason << "]" ) ;
	onDelete( done ? std::string() : reason ) ;
}

void GNet::ServerPeer::onIdleTimeout()
{
	std::ostringstream ss ;
	ss << "no activity after " << m_config.idle_timeout << "s" ;
	throw IdleTimeout( ss.str() ) ;
}

void GNet::ServerPeer::onData( const char * data , std::size_t size )
{
	if( m_config.idle_timeout )
		m_idle_timer.startTimer( m_config.idle_timeout ) ;

	bool fragments = m_line_buffer.transparent() ;
	m_line_buffer.apply( this , &ServerPeer::onDataImp , data , size , fragments ) ;
}

bool GNet::ServerPeer::onDataImp( const char * data , std::size_t size , std::size_t eolsize ,
	std::size_t linesize , char c0 )
{
	return onReceive( data , size , eolsize , linesize , c0 ) ;
}

GNet::LineBufferState GNet::ServerPeer::lineBuffer() const
{
	return LineBufferState( m_line_buffer ) ;
}

std::string GNet::ServerPeer::exceptionSourceId() const
{
	if( m_exception_source_id.empty() )
	{
		m_exception_source_id = peerAddress().hostPartString() ; // GNet::Connection
		if( G::Test::enabled("log-full-address") )
			m_exception_source_id = peerAddress().displayString() ;
	}
	return m_exception_source_id ;
}

void GNet::ServerPeer::setIdleTimeout( unsigned int s )
{
	m_config.idle_timeout = s ;
	m_idle_timer.cancelTimer() ;
	if( m_config.idle_timeout )
		m_idle_timer.startTimer( m_config.idle_timeout ) ;
}

#ifndef G_LIB_SMALL
void GNet::ServerPeer::finish()
{
	m_sp.shutdown() ;
}
#endif

void GNet::ServerPeer::onPeerDisconnect()
{
}

