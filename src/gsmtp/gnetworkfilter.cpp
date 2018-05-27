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
// gnetworkfilter.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gnetworkfilter.h"
#include "gstr.h"
#include "glog.h"

GSmtp::NetworkFilter::NetworkFilter( GNet::ExceptionHandler & exception_handler ,
	bool server_side , const std::string & server ,
	unsigned int connection_timeout , unsigned int response_timeout ) :
		m_exception_handler(exception_handler) ,
		m_server_side(server_side) ,
		m_location(server) ,
		m_connection_timeout(connection_timeout) ,
		m_response_timeout(response_timeout) ,
		m_lazy(true)
{
	m_client.eventSignal().connect( G::Slot::slot(*this,&GSmtp::NetworkFilter::clientEvent) ) ;
}

GSmtp::NetworkFilter::~NetworkFilter()
{
	m_client.eventSignal().disconnect() ;
}

std::string GSmtp::NetworkFilter::id() const
{
	return m_location.displayString() ;
}

bool GSmtp::NetworkFilter::simple() const
{
	return false ;
}

void GSmtp::NetworkFilter::start( const std::string & path )
{
	if( !m_lazy || m_client.get() == nullptr )
	{
		m_client.reset( new RequestClient("scanner","ok",m_location,m_connection_timeout,m_response_timeout) );
	}
	m_text.erase() ;
	m_client->request( path ) ; // (no need to wait for connection)
}

void GSmtp::NetworkFilter::clientEvent( std::string s1 , std::string s2 )
{
	G_DEBUG( "GSmtp::NetworkFilter::clientEvent: [" << s1 << "] [" << s2 << "]" ) ;
	if( s1 == "scanner" )
	{
		m_text = s2 ;
		try
		{
			m_done_signal.emit( m_text.empty() ? 0 : 2 ) ;
		}
		catch( std::exception & e )
		{
			m_exception_handler.onException( e ) ;
		}
	}
}

bool GSmtp::NetworkFilter::special() const
{
	return false ;
}

std::string GSmtp::NetworkFilter::response() const
{
	// allow "<response><tab><reason>"
	return G::Str::printable( G::Str::head( m_text , "\t" , false ) ) ;
}

std::string GSmtp::NetworkFilter::reason() const
{
	return G::Str::printable( G::Str::tail( m_text , "\t" , false ) ) ;
}

G::Slot::Signal1<int> & GSmtp::NetworkFilter::doneSignal()
{
	return m_done_signal ;
}

void GSmtp::NetworkFilter::cancel()
{
	m_client.reset() ;
	m_text.erase() ;
}

bool GSmtp::NetworkFilter::abandoned() const
{
	return false ;
}
/// \file gnetworkfilter.cpp
