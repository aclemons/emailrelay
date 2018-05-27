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
// grequestclient.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gstr.h"
#include "grequestclient.h"
#include "gassert.h"

GSmtp::RequestClient::RequestClient( const std::string & key , const std::string & ok ,
	const GNet::Location & location , unsigned int connect_timeout , unsigned int response_timeout ) :
		GNet::Client(location,connect_timeout,response_timeout,0U,config()) ,
		m_eol(1U,'\n') ,
		m_key(key) ,
		m_ok(ok) ,
		m_timer(*this,&RequestClient::onTimeout,*this)
{
	G_DEBUG( "GSmtp::RequestClient::ctor: " << location.displayString() << ": "
		<< connect_timeout << " " << response_timeout ) ;
}

GSmtp::RequestClient::~RequestClient()
{
}

GNet::LineBufferConfig GSmtp::RequestClient::config()
{
	return GNet::LineBufferConfig::newline() ;
}

void GSmtp::RequestClient::onConnect()
{
	G_DEBUG( "GSmtp::RequestClient::onConnect" ) ;
	if( busy() )
		send( requestLine(m_request) ) ;
}

void GSmtp::RequestClient::request( const std::string & request_payload )
{
	G_DEBUG( "GSmtp::RequestClient::request: \"" << request_payload << "\"" ) ;
	if( busy() ) throw ProtocolError() ;
	m_request = request_payload ;
	m_timer.startTimer( 0U ) ;

	// clear the base-class line buffer of any incomplete line
	// data -- but a race condition is possible for servers
	// which reply with more that one line
	clearInput() ;
}

void GSmtp::RequestClient::onTimeout()
{
	if( connected() )
		send( requestLine(m_request) ) ;
}

bool GSmtp::RequestClient::busy() const
{
	return !m_request.empty() ;
}

void GSmtp::RequestClient::onDelete( const std::string & )
{
}

void GSmtp::RequestClient::onDeleteImp( const std::string & reason )
{
	// we override onDeleteImp() rather than onDelete() so that
	// we get to emit our signal before any other signal handler --
	// consider that they might throw an exception and then we don't
	// get called -- so this guarantees every request gets a response

	if( !reason.empty() )
		G_WARNING( "GSmtp::RequestClient::onDeleteImp: error: " << reason ) ;

	if( busy() )
	{
		m_request.erase() ;
		eventSignal().emit( m_key , reason.empty() ? std::string("error") : reason ) ;
	}
	GNet::Client::onDeleteImp( reason ) ; // base class
}

void GSmtp::RequestClient::onSecure( const std::string & )
{
}

bool GSmtp::RequestClient::onReceive( const char * line_data , size_t line_size , size_t )
{
	std::string line( line_data , line_size ) ;
	G_DEBUG( "GSmtp::RequestClient::onReceive: [" << G::Str::printable(line) << "]" ) ;
	if( busy() )
	{
		m_request.erase() ;
		eventSignal().emit( m_key , result(line) ) ; // empty string if matching m_ok
	}
	return true ;
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

/// \file grequestclient.cpp
