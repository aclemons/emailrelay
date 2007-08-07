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
// gbufferedclient.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gtest.h"
#include "gdebug.h"
#include "gbufferedclient.h"

GNet::BufferedClient::BufferedClient( const ResolverInfo & remote_info ,
	const Address & local_interface , bool privileged , bool sync_dns ) :
		HeapClient(remote_info,local_interface,privileged,sync_dns) ,
		m_sender(*this)
{
}

GNet::BufferedClient::~BufferedClient()
{
}

bool GNet::BufferedClient::send( const std::string & data , std::string::size_type offset )
{
	bool rc = true ;
	if( m_sender.send( socket() , data , offset ) )
	{
		rc = true ;
	}
	else if( m_sender.failed() )
	{
		throw SendError() ;
	}
	else
	{
		logFlowControlAsserted() ;
		rc = false ; // onSendComplete() will be called
	}
	onSendImp() ;
	return rc ;
}

void GNet::BufferedClient::onWriteable()
{
	logFlowControlReleased() ;
	if( m_sender.resumeSending(socket()) )
		onSendComplete() ;
	else if( m_sender.failed() )
		throw SendError() ;
}

void GNet::BufferedClient::onSendImp()
{
}

void GNet::BufferedClient::logFlowControlAsserted() const
{
	const bool log = G::Test::enabled("log-flow-control") ;
	if( log )
		G_LOG( "GNet::BufferedClient::send: " << logId() << ": flow control asserted" ) ;
}

void GNet::BufferedClient::logFlowControlReleased() const
{
	const bool log = G::Test::enabled("log-flow-control") ;
	if( log )
		G_LOG( "GNet::BufferedClient::send: " << logId() << ": flow control released" ) ;
}

/// \file gbufferedclient.cpp
