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
// gclient.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gclient.h"

GNet::Client::Client( const ResolverInfo & remote_info , unsigned int connection_timeout ,
	unsigned int response_timeout , unsigned int secure_connection_timeout ,
	const std::string & eol , const Address & local_interface , 
	bool privileged , bool sync_dns ) :
		HeapClient(remote_info,local_interface,privileged,sync_dns,secure_connection_timeout) ,
		m_done_signal(true) ,
		m_connected_signal(true) ,
		m_connection_timeout(connection_timeout) ,
		m_response_timeout(response_timeout) ,
		m_connection_timer(*this,&Client::onConnectionTimeout,*this) ,
		m_response_timer(*this,&Client::onResponseTimeout,*this) ,
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

void GNet::Client::onConnectionTimeout()
{
	doDelete( "connection timeout" ) ;
}

void GNet::Client::onResponseTimeout()
{
	doDelete( "response timeout" ) ;
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

G::Signal0 & GNet::Client::secureSignal()
{
	return m_secure_signal ;
}

void GNet::Client::onData( const char * p , SimpleClient::size_type n )
{
	m_line_buffer.add(p,n) ; 

	bool first = true ;
	while( m_line_buffer.more() )
	{
		if( first && m_response_timeout != 0U )
			m_response_timer.cancelTimer() ;
		first = false ;

		bool ok = onReceive( m_line_buffer.line() ) ;
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
