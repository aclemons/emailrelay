//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gbufferedserverpeer.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gbufferedserverpeer.h"
#include "gstr.h"
#include "gtest.h"
#include "gassert.h"
#include "glog.h"

GNet::BufferedServerPeer::BufferedServerPeer( Server::PeerInfo peer_info , const std::string & eol , bool throw_ ) :
	ServerPeer(peer_info) ,
	m_sender(*this) ,
	m_line_buffer(eol) ,
	m_throw(throw_)
{
}

GNet::BufferedServerPeer::~BufferedServerPeer()
{
}

bool GNet::BufferedServerPeer::send( const std::string & data , std::string::size_type offset )
{
	bool all_sent = m_sender.send( socket() , data , offset ) ;
	if( !all_sent && ( m_sender.failed() || m_throw ) )
		throw SendError() ;
	if( !all_sent )
		logFlowControlAsserted() ;
	return all_sent ;
}

void GNet::BufferedServerPeer::writeEvent()
{
	try
	{
		logFlowControlReleased() ;
		if( m_sender.resumeSending( socket() ) )
			onSendComplete() ;
		else if( m_sender.failed() )
			throw SendError() ;
		else 
			logFlowControlReasserted() ;
	}
	catch( std::exception & e ) // strategy
	{
		G_WARNING( "GNet::BufferedServerPeer::writeEvent: exception: " << e.what() ) ;
		doDelete() ;
	}
}

void GNet::BufferedServerPeer::onData( const char * p , ServerPeer::size_type n )
{
	for( m_line_buffer.add(p,n) ; m_line_buffer.more() ; m_line_buffer.discard() )
	{
		bool ok = onReceive( m_line_buffer.current() ) ;
		if( !ok )
			break ;
	}
}

void GNet::BufferedServerPeer::logFlowControlAsserted() const
{
	const bool log = G::Test::enabled("log-flow-control") ;
	if( log )
		G_LOG( "GNet::BufferedServerPeer::send: server " << logId() << ": flow control asserted" ) ;
}

void GNet::BufferedServerPeer::logFlowControlReleased() const
{
	const bool log = G::Test::enabled("log-flow-control") ;
	if( log )
		G_LOG( "GNet::BufferedServerPeer::writeEvent: server " << logId() << ": flow control released" ) ;
}

void GNet::BufferedServerPeer::logFlowControlReasserted() const
{
	const bool log = G::Test::enabled("log-flow-control") ;
	if( log )
		G_LOG( "GNet::BufferedServerPeer::writeEvent: server " << logId() << ": flow control reasserted" ) ;
}

/// \file gbufferedserverpeer.cpp
