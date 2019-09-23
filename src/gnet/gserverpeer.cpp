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
// gserverpeer.cpp
//

#include "gdef.h"
#include "gserver.h"
#include "gmonitor.h"
#include "glog.h"
#include "gassert.h"
#include <sstream>

GNet::ServerPeer::ServerPeer( ExceptionSink es , ServerPeerInfo peer_info , LineBufferConfig line_buffer_config ) :
	m_address(peer_info.m_address) ,
	m_socket(peer_info.m_socket) ,
	m_sp(*this,es,*this,*m_socket.get(),0U) ,
	m_line_buffer(line_buffer_config) ,
	m_config(peer_info.m_config) ,
	m_idle_timer(*this,&ServerPeer::onIdleTimeout,es)
{
	G_ASSERT( peer_info.m_server != nullptr ) ;
	G_ASSERT( m_socket.get() != nullptr ) ;
	G_ASSERT( es.esrc() != nullptr ) ; // moot
	G_DEBUG( "GNet::ServerPeer::ctor: [" << this << "]: port " << m_address.port() ) ;

	if( m_config.idle_timeout )
		m_idle_timer.startTimer( m_config.idle_timeout ) ;

	m_socket->addReadHandler( *this , es ) ;
	m_socket->addOtherHandler( *this , es ) ;
	Monitor::addServerPeer(*this) ;
}

GNet::ServerPeer::~ServerPeer()
{
	G_DEBUG( "GNet::ServerPeer::dtor: [" << this << "]: port " << m_address.port() ) ;
	Monitor::removeServerPeer(*this) ;
	m_socket->dropReadHandler() ;
	m_socket->dropOtherHandler() ;
}

void GNet::ServerPeer::secureAccept()
{
	m_sp.secureAccept() ;
}

GNet::StreamSocket & GNet::ServerPeer::socket()
{
	G_ASSERT( m_socket.get() != nullptr ) ;
	return *m_socket.get() ;
}

void GNet::ServerPeer::otherEvent( EventHandler::Reason reason )
{
	m_sp.otherEvent( reason ) ;
}

void GNet::ServerPeer::readEvent()
{
	m_sp.readEvent() ;
}

std::pair<bool,GNet::Address> GNet::ServerPeer::localAddress() const
{
	G_ASSERT( m_socket.get() != nullptr ) ;
	return m_socket->getLocalAddress() ;
}

std::pair<bool,GNet::Address> GNet::ServerPeer::peerAddress() const
{
	return std::pair<bool,Address>( true , m_address ) ;
}

std::string GNet::ServerPeer::connectionState() const
{
	return m_address.displayString() ;
}

std::string GNet::ServerPeer::peerCertificate() const
{
	return m_sp.peerCertificate() ;
}

bool GNet::ServerPeer::send( const std::string & data , std::string::size_type offset )
{
	return m_sp.send( data , offset ) ;
}

bool GNet::ServerPeer::send( const std::vector<std::pair<const char*,size_t> > & segments )
{
	return m_sp.send( segments ) ;
}

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

void GNet::ServerPeer::onData( const char * data , size_t size )
{
	if( m_config.idle_timeout )
		m_idle_timer.startTimer( m_config.idle_timeout ) ;

	m_line_buffer.apply( this , &ServerPeer::onDataImp , data , size , /*fragments=*/m_line_buffer.transparent() ) ;
}

bool GNet::ServerPeer::onDataImp( const char * data , size_t size , size_t eolsize , size_t linesize , char c0 )
{
	return onReceive( data , size , eolsize , linesize , c0 ) ;
}

GNet::LineBufferState GNet::ServerPeer::lineBuffer() const
{
	return LineBufferState( m_line_buffer ) ;
}

// ==

GNet::ServerPeerConfig::ServerPeerConfig( unsigned int idle_timeout_in ) :
	idle_timeout(idle_timeout_in)
{
}

/// \file gserverpeer.cpp
