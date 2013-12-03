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
// grequestclient.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gsmtp.h"
#include "gstr.h"
#include "grequestclient.h"
#include "gassert.h"

GSmtp::RequestClient::RequestClient( const std::string & key , const std::string & ok , const std::string & eol ,
	const GNet::ResolverInfo & resolver_info ,
	unsigned int connect_timeout , unsigned int response_timeout ) :
		GNet::Client(resolver_info,connect_timeout,response_timeout,0U,eol) ,
		m_key(key) ,
		m_ok(ok) ,
		m_eol(eol) ,
		m_timer(*this,&RequestClient::onTimeout,*this)
{
	G_DEBUG( "GSmtp::RequestClient::ctor: " << resolver_info.displayString() << ": " 
		<< connect_timeout << " " << response_timeout ) ;
}

GSmtp::RequestClient::~RequestClient()
{
}

void GSmtp::RequestClient::onConnect()
{
	G_DEBUG( "GSmtp::RequestClient::onConnect" ) ;
	if( busy() )
		send( requestLine(m_request) ) ;
}

void GSmtp::RequestClient::request( const std::string & payload )
{
	G_DEBUG( "GSmtp::RequestClient::request: \"" << payload << "\"" ) ;
	if( busy() ) throw ProtocolError() ;
	m_request = payload ;
	m_timer.startTimer( 0U ) ;
	clearInput() ; // ... but race condition possible for servers which reply with more that one line
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

void GSmtp::RequestClient::onDelete( const std::string & , bool )
{
}

void GSmtp::RequestClient::onDeleteImp( const std::string & reason , bool b )
{
	// we have to override onDeleteImp() rather than onDelete() so that we 
	// can get in early enough to guarantee that every request gets a response

	if( !reason.empty() )
		G_WARNING( "GSmtp::RequestClient::onDeleteImp: error: " << reason ) ;

	if( busy() )
	{
		m_request.erase() ;
		eventSignal().emit( m_key , reason.empty() ? std::string("error") : reason ) ;
	}
	Base::onDeleteImp( reason , b ) ; // use typedef because of ms compiler bug
}

void GSmtp::RequestClient::onSecure( const std::string & )
{
}

bool GSmtp::RequestClient::onReceive( const std::string & line )
{
	G_DEBUG( "GSmtp::RequestClient::onReceive: [" << G::Str::printable(line) << "]" ) ;
	if( busy() )
	{
		m_request.erase() ;
		std::string scan_result = result(line) ; // empty string if scanned okay
		scan_result = G::Str::printable( scan_result ) ; // paranoia
		eventSignal().emit( m_key , scan_result ) ;
	}
	return true ;
}

void GSmtp::RequestClient::onSendComplete()
{
}

// customisation...

std::string GSmtp::RequestClient::requestLine( const std::string & payload ) const
{
	return payload + m_eol ;
}

std::string GSmtp::RequestClient::result( std::string line ) const
{
	G::Str::trim( line , "\r" ) ;
	return !m_ok.empty() && line.find(m_ok) == 0U ? std::string() : line ;
}

/// \file grequestclient.cpp
