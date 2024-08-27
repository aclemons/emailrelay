//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
///
/// \file grequestclient.cpp
///

#include "gdef.h"
#include "gstr.h"
#include "grequestclient.h"
#include "glog.h"

GSmtp::RequestClient::RequestClient( GNet::EventState es , const std::string & key , const std::string & ok ,
	const GNet::Location & location , unsigned int connection_timeout , unsigned int response_timeout ,
	unsigned int idle_timeout ) :
		GNet::Client(es,location,
			GNet::Client::Config()
				.set_line_buffer_config(GNet::LineBuffer::Config::newline())
				.set_connection_timeout(connection_timeout)
				.set_response_timeout(response_timeout)
				.set_idle_timeout(idle_timeout)) ,
		m_eol(1U,'\n') ,
		m_key(key) ,
		m_ok(ok) ,
		m_timer(*this,&RequestClient::onTimeout,es)
{
	G_DEBUG( "GSmtp::RequestClient::ctor: " << location.displayString() << ": "
		<< connection_timeout << " " << response_timeout ) ;
}

void GSmtp::RequestClient::onConnect()
{
	G_DEBUG( "GSmtp::RequestClient::onConnect" ) ;
	if( busy() )
		send( requestLine(m_request) ) ; // GNet::Client::send()
}

void GSmtp::RequestClient::request( const std::string & request_payload )
{
	G_DEBUG( "GSmtp::RequestClient::request: \"" << request_payload << "\"" ) ;
	if( busy() )
		throw ProtocolError() ;

	m_request = request_payload ;
	m_timer.startTimer( 0U ) ;

	// clear the base-class line buffer of any incomplete line
	// data from a previous request -- this is racey for servers
	// that incorrectly reply with more than one line
	clearInput() ;
}

void GSmtp::RequestClient::onTimeout()
{
	if( connected() )
		send( requestLine(m_request) ) ; // GNet::Client::send()
}

bool GSmtp::RequestClient::busy() const
{
	return !m_request.empty() ;
}

void GSmtp::RequestClient::onDelete( const std::string & reason )
{
	if( !reason.empty() )
		G_WARNING( "GSmtp::RequestClient::onDelete: error: " << reason ) ;
}

void GSmtp::RequestClient::onSecure( const std::string & , const std::string & , const std::string & )
{
}

bool GSmtp::RequestClient::onReceive( const char * line_data , std::size_t line_size , std::size_t ,
	std::size_t , char )
{
	std::string line( line_data , line_size ) ;
	G_DEBUG( "GSmtp::RequestClient::onReceive: [" << G::Str::printable(line) << "]" ) ;
	if( busy() )
	{
		m_request.erase() ;
		eventSignal().emit( std::string(m_key) , result(line) , std::string() ) ; // empty string if matching m_ok
		return false ;
	}
	else
	{
		return true ;
	}
}

void GSmtp::RequestClient::onSendComplete()
{
}

std::string GSmtp::RequestClient::requestLine( const std::string & request_payload ) const
{
	return request_payload + m_eol ;
}

std::string GSmtp::RequestClient::result( std::string line ) const
{
	G::Str::trimRight( line , "\r" ) ;
	return !m_ok.empty() && line.find(m_ok) == 0U ? std::string() : line ;
}

