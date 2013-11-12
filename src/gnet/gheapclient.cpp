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
// gheapclient.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gheapclient.h"
#include "gdebug.h"

GNet::HeapClient::HeapClient( const ResolverInfo & remote_info ,
	const Address & local_interface , bool privileged , bool sync_dns ,
	unsigned int secure_connection_timeout ) :
		SimpleClient(remote_info,local_interface,privileged,sync_dns,secure_connection_timeout) ,
		m_connect_timer(*this,&HeapClient::onConnectionTimeout,*this) ,
		m_delete_timer(*this,&HeapClient::onDeletionTimeout,*this)
{
	m_connect_timer.startTimer( 0U ) ;
}

GNet::HeapClient::~HeapClient()
{
}

void GNet::HeapClient::onConnectionTimeout()
{
	onConnecting() ;
	connect() ;
}

void GNet::HeapClient::onDeletionTimeout()
{
	try
	{
		doDeleteThis() ;
	}
	catch( std::exception & e ) // never gets here
	{
		G_ERROR( "HeapClientTimer::onTimeout: exception: " << e.what() ) ;
	}
}

void GNet::HeapClient::doDeleteForExit()
{
	delete this ;
}

void GNet::HeapClient::doDelete( const std::string & reason )
{
	m_connect_timer.cancelTimer() ;
	m_delete_timer.startTimer( 0U ) ; // before the callbacks, in case they throw
	onDeleteImp( reason , canRetry(reason) ) ; // first -- for 'internal' library classes
	onDelete( reason , canRetry(reason) ) ; // second -- for 'external' client classes
}

void GNet::HeapClient::doDeleteThis()
{
	delete this ;
}

void GNet::HeapClient::onException( std::exception & e )
{
	G_WARNING( "GNet::HeapClient::onException: exception: " << e.what() ) ;
	doDelete( e.what() ) ;
}

void GNet::HeapClient::onDeleteImp( const std::string & , bool )
{
}

void GNet::HeapClient::onConnecting()
{
}

/// \file gheapclient.cpp
