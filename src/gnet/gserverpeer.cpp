//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "glog.h"
#include "gassert.h"
#include <sstream>

GNet::ServerPeer::ServerPeer( ExceptionSink es , ServerPeerInfo && peer_info , LineBufferConfig line_buffer_config ) :
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

	if( m_config.socket_linger_onoff >= 0 )
		m_socket->setOptionLinger( m_config.socket_linger_onoff , m_config.socket_linger_time ) ;

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

void GNet::ServerPeer::expect( std::size_t n )
{
	m_line_buffer.expect( n ) ;
}

GNet::StreamSocket & GNet::ServerPeer::socket()
{
	G_ASSERT( m_socket != nullptr ) ;
	return *m_socket ;
}

void GNet::ServerPeer::otherEvent( Descriptor , EventHandler::Reason reason )
{
	m_sp.otherEvent( reason ) ;
}

void GNet::ServerPeer::readEvent( Descriptor )
{
	m_sp.readEvent() ;
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
	return m_sp.send( data , 0U ) ;
}

bool GNet::ServerPeer::send( G::string_view data )
{
	return m_sp.send( data ) ;
}

bool GNet::ServerPeer::send( const std::vector<G::string_view> & segments , std::size_t offset )
{
	return m_sp.send( segments , offset ) ;
}

void GNet::ServerPeer::writeEvent( Descriptor )
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

// ==

GNet::ServerPeer::Config & GNet::ServerPeer::Config::set_socket_linger( std::pair<int,int> pair )
{
	socket_linger_onoff = pair.first ;
	socket_linger_time = pair.second ;
	return *this ;
}

