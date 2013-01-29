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
// gnetworkprocessor.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gnetworkprocessor.h"
#include "glog.h"

GSmtp::NetworkProcessor::NetworkProcessor( const std::string & server , 
	unsigned int connection_timeout , unsigned int response_timeout ) :
		m_resolver_info(server) ,
		m_connection_timeout(connection_timeout) ,
		m_response_timeout(response_timeout) ,
		m_lazy(true)
{
	m_client.eventSignal().connect( G::slot(*this,&GSmtp::NetworkProcessor::clientEvent) ) ;
}

GSmtp::NetworkProcessor::~NetworkProcessor()
{
	m_client.eventSignal().disconnect() ;
}

void GSmtp::NetworkProcessor::start( const std::string & path )
{
	if( !m_lazy || m_client.get() == NULL )
	{
		m_client.reset(new RequestClient("scanner","ok","\n",m_resolver_info,m_connection_timeout,m_response_timeout));
	}
	m_text.erase() ;
	m_client->request( path ) ; // (no need to wait for connection)
}

void GSmtp::NetworkProcessor::clientEvent( std::string s1 , std::string s2 )
{
	G_DEBUG( "GSmtp::NetworkProcessor::clientEvent: [" << s1 << "] [" << s2 << "]" ) ;
	if( s1 == "scanner" )
	{
		m_text = s2 ;
		m_done_signal.emit( s2.empty() ) ;
	}
}

bool GSmtp::NetworkProcessor::cancelled() const
{
	return false ;
}

bool GSmtp::NetworkProcessor::repoll() const
{
	return false ;
}

std::string GSmtp::NetworkProcessor::text() const
{
	return m_text ;
}

G::Signal1<bool> & GSmtp::NetworkProcessor::doneSignal()
{
	return m_done_signal ;
}

void GSmtp::NetworkProcessor::abort()
{
	m_text.erase() ;
	if( m_client.get() != NULL && m_client->busy() )
		m_client.reset() ;
}

/// \file gnetworkprocessor.cpp
