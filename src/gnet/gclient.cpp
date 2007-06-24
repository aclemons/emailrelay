//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gclient.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gclient.h"

GNet::Client::Client( const ResolverInfo & remote_info , unsigned int connection_timeout ,
	unsigned int response_timeout , const std::string & eol , const Address & local_interface , 
	bool privileged , bool sync_dns ) :
		BufferedClient(remote_info,local_interface,privileged,sync_dns) ,
		m_connection_timeout(connection_timeout) ,
		m_response_timeout(response_timeout) ,
		m_connection_timer(*this,*this) ,
		m_response_timer(*this,*this) ,
		m_line_buffer(eol)
{
	if( connection_timeout != 0U )
		m_connection_timer.startTimer( connection_timeout ) ;
}

GNet::Client::~Client()
{
}

void GNet::Client::onConnecting()
{
	m_event_signal.emit( "connecting" , resolverInfo().displayString() ) ;
}

void GNet::Client::onConnectImp()
{
	if( m_connection_timeout != 0U )
		m_connection_timer.cancelTimer() ;

	m_connected_signal.emit() ;

	m_event_signal.emit( "connected" , resolverInfo().address().displayString() + " " + resolverInfo().name() ) ;
}

void GNet::Client::onDeleteImp( const std::string & reason , bool retry )
{
	m_connection_timer.cancelTimer() ;
	m_response_timer.cancelTimer() ;
	m_event_signal.emit( reason.empty() ? "done" : "failed" , reason ) ;
	m_done_signal.emit( reason , retry ) ;
}

void GNet::Client::onSendImp()
{
	if( m_response_timeout != 0U )
		m_response_timer.startTimer( m_response_timeout ) ;
}

void GNet::Client::onTimeout( GNet::AbstractTimer & timer )
{
	std::string reason = &timer == &m_connection_timer ? "connection timeout" : "response timeout" ;
	m_event_signal.emit( "failed" , reason ) ;
	doDelete( reason ) ;
}

G::Signal2<std::string,bool> & GNet::Client::doneSignal()
{
	return m_done_signal ;
}

G::Signal2<std::string,std::string> & GNet::Client::eventSignal()
{
	return m_event_signal ;
}

G::Signal0 & GNet::Client::connectedSignal()
{
	return m_connected_signal ;
}

void GNet::Client::onData( const char * p , SimpleClient::size_type n )
{
	bool first = true ;
	for( m_line_buffer.add(p,n) ; m_line_buffer.more() ; m_line_buffer.discard() )
	{
		if( first && m_response_timeout != 0U )
			m_response_timer.cancelTimer() ;
		first = false ;

		bool ok = onReceive( m_line_buffer.current() ) ;
		if( !ok )
			break ;
	}
}

void GNet::Client::clearInput()
{
	while( m_line_buffer.more() )
		m_line_buffer.discard() ;
}

/// \file gclient.cpp
