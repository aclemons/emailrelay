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
// gspamfilter.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gspamfilter.h"
#include "glog.h"

GSmtp::SpamFilter::SpamFilter( GNet::ExceptionHandler & exception_handler ,
	bool server_side , const std::string & server ,
	unsigned int connection_timeout , unsigned int response_timeout ) :
		m_exception_handler(exception_handler) ,
		m_server_side(server_side) ,
		m_location(server) ,
		m_connection_timeout(connection_timeout) ,
		m_response_timeout(response_timeout)
{
	m_client.eventSignal().connect( G::Slot::slot(*this,&GSmtp::SpamFilter::clientEvent) ) ;
}

GSmtp::SpamFilter::~SpamFilter()
{
	m_client.eventSignal().disconnect() ;
}

std::string GSmtp::SpamFilter::id() const
{
	return m_location.displayString() ;
}

bool GSmtp::SpamFilter::simple() const
{
	return false ;
}

void GSmtp::SpamFilter::start( const std::string & path )
{
	m_client.reset( new SpamClient(m_location,m_connection_timeout,m_response_timeout) ) ;

	m_text.erase() ;
	m_client->request( path ) ; // (no need to wait for connection)
}

void GSmtp::SpamFilter::clientEvent( std::string s1 , std::string s2 )
{
	G_DEBUG( "GSmtp::SpamFilter::clientEvent: [" << s1 << "] [" << s2 << "]" ) ;
	if( s1 == "spam" )
	{
		m_text = s2.empty() ? std::string() : ("spam: "+s2) ;
		emit( s2.empty() ) ;
	}
	else if( s1 == "failed" )
	{
		m_text = s2 ;
		emit( s2.empty() ) ;
	}
}

void GSmtp::SpamFilter::emit( bool b )
{
	try
	{
		m_done_signal.emit( b ) ;
		m_client.reset() ;
	}
	catch( std::exception & e )
	{
		m_client.reset() ;
		m_exception_handler.onException( e ) ;
	}
}

bool GSmtp::SpamFilter::specialCancelled() const
{
	return false ;
}

bool GSmtp::SpamFilter::specialOther() const
{
	return false ;
}

std::string GSmtp::SpamFilter::text() const
{
	return m_text ;
}

G::Slot::Signal1<bool> & GSmtp::SpamFilter::doneSignal()
{
	return m_done_signal ;
}

void GSmtp::SpamFilter::cancel()
{
	m_text.erase() ;
	if( m_client.get() != nullptr && m_client->busy() )
		m_client.reset() ;
}

/// \file gspamfilter.cpp
