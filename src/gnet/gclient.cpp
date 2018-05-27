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
// gclient.cpp
//

#include "gdef.h"
#include "gclient.h"
#include "gstr.h"

GNet::Client::Client( const Location & remote_info , unsigned int connection_timeout ,
	unsigned int response_timeout , unsigned int secure_connection_timeout ,
	LineBufferConfig eol , bool bind_local_address , const Address & local_address ,
	bool sync_dns ) :
		HeapClient(remote_info,bind_local_address,local_address,sync_dns,secure_connection_timeout) ,
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
	m_event_signal.emit( "connecting" , remoteLocation().displayString() ) ;
}

void GNet::Client::onConnectImp()
{
	if( m_connection_timeout != 0U )
		m_connection_timer.cancelTimer() ;

	m_connected_signal.emit() ;
	m_event_signal.emit( "connected" , remoteLocation().address().displayString() + " " + remoteLocation().name() ) ;
}

void GNet::Client::onDeleteImp( const std::string & reason )
{
	m_connection_timer.cancelTimer() ;
	m_response_timer.cancelTimer() ;
	m_event_signal.emit( reason.empty() ? "done" : "failed" , reason ) ;
	m_done_signal.emit( reason ) ;
}

void GNet::Client::onSendImp()
{
	if( m_response_timeout != 0U )
		m_response_timer.startTimer( m_response_timeout ) ;
}

void GNet::Client::onConnectionTimeout()
{
	doDelete( "connection timeout after " + G::Str::fromUInt(m_connection_timeout) + "s connecting to " + remoteLocation().displayString() ) ;
}

void GNet::Client::onResponseTimeout()
{
	doDelete( "response timeout after " + G::Str::fromUInt(m_response_timeout) + "s talking to " + remoteLocation().displayString() ) ;
}

G::Slot::Signal1<std::string> & GNet::Client::doneSignal()
{
	return m_done_signal ;
}

G::Slot::Signal2<std::string,std::string> & GNet::Client::eventSignal()
{
	return m_event_signal ;
}

G::Slot::Signal0 & GNet::Client::connectedSignal()
{
	return m_connected_signal ;
}

void GNet::Client::onData( const char * p , SimpleClient::size_type n )
{
	m_line_buffer.add( p , n ) ;

	LineBufferIterator iter( m_line_buffer ) ;
	for( bool first = true ; iter.more() ; first = false )
	{
		if( first && m_response_timeout != 0U )
			m_response_timer.cancelTimer() ;

		bool ok = onReceive( iter.lineData() , iter.lineSize() , iter.eolSize() ) ;
		if( !ok )
			break ;
	}
}

std::string GNet::Client::lineBufferEndOfLine() const
{
	return m_line_buffer.eol() ;
}

void GNet::Client::clearInput()
{
	LineBufferIterator iter( m_line_buffer ) ;
	while( iter.more() )
		; // no-op to discard each line
}

/// \file gclient.cpp
