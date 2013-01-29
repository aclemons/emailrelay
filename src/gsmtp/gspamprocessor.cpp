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
// gspamprocessor.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gspamprocessor.h"
#include "glog.h"

GSmtp::SpamProcessor::SpamProcessor( const std::string & server , 
	unsigned int connection_timeout , unsigned int response_timeout ) :
		m_resolver_info(server) ,
		m_connection_timeout(connection_timeout) ,
		m_response_timeout(response_timeout)
{
	m_client.eventSignal().connect( G::slot(*this,&GSmtp::SpamProcessor::clientEvent) ) ;
}

GSmtp::SpamProcessor::~SpamProcessor()
{
	m_client.eventSignal().disconnect() ;
}

void GSmtp::SpamProcessor::start( const std::string & path )
{
	m_client.reset( new SpamClient(m_resolver_info,m_connection_timeout,m_response_timeout));

	m_text.erase() ;
	m_client->request( path ) ; // (no need to wait for connection)
}

void GSmtp::SpamProcessor::clientEvent( std::string s1 , std::string s2 )
{
	G_DEBUG( "GSmtp::SpamProcessor::clientEvent: [" << s1 << "] [" << s2 << "]" ) ;
	if( s1 == "spam" )
	{
		m_text = s2 ;
		m_done_signal.emit( s2.empty() ) ;
	}
}

bool GSmtp::SpamProcessor::cancelled() const
{
	return false ;
}

bool GSmtp::SpamProcessor::repoll() const
{
	return false ;
}

std::string GSmtp::SpamProcessor::text() const
{
	return m_text ;
}

G::Signal1<bool> & GSmtp::SpamProcessor::doneSignal()
{
	return m_done_signal ;
}

void GSmtp::SpamProcessor::abort()
{
	m_text.erase() ;
	if( m_client.get() != NULL && m_client->busy() )
		m_client.reset() ;
}

/// \file gspamprocessor.cpp
