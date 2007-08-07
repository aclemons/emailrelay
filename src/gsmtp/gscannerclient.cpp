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
// gscannerclient.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gsmtp.h"
#include "gstr.h"
#include "gscannerclient.h"
#include "gassert.h"

GSmtp::ScannerClient::ScannerClient( const GNet::ResolverInfo & resolver_info ,
	unsigned int connect_timeout , unsigned int response_timeout ) :
		GNet::Client(resolver_info,connect_timeout,response_timeout,eol()) ,
		m_timer(*this,&ScannerClient::onTimeout,*this)
{
	G_DEBUG( "GSmtp::ScannerClient::ctor: " << resolver_info.displayString() << ": " << response_timeout ) ;
}

GSmtp::ScannerClient::~ScannerClient()
{
}

void GSmtp::ScannerClient::onConnect()
{
	G_DEBUG( "GSmtp::ScannerClient::onConnect" ) ;
	if( busy() )
		send( request(m_path) ) ;
}

void GSmtp::ScannerClient::startScanning( const G::Path & path )
{
	G_DEBUG( "GSmtp::ScannerClient::startScanning: \"" << path << "\"" ) ;
	if( busy() ) throw ProtocolError() ;
	m_path = path ;
	m_timer.startTimer( 0U ) ;
	clearInput() ; // ... but race condition possible for servers which reply with more that one line
}

void GSmtp::ScannerClient::onTimeout()
{
	if( connected() )
		send( request(m_path) ) ;
}

bool GSmtp::ScannerClient::busy() const
{
	return m_path != G::Path() ;
}

void GSmtp::ScannerClient::onDelete( const std::string & , bool )
{
}

void GSmtp::ScannerClient::onDeleteImp( const std::string & reason , bool b )
{
	// we have to override onDeleteImp() rather than onDelete() so that we 
	// can get in early enough to guarantee that every scanner 
	// request gets a response

	if( !reason.empty() )
		G_WARNING( "GSmtp::ScannerClient::onDeleteImp: scanner error: " << reason ) ;

	if( busy() )
	{
		m_path = G::Path() ;
		eventSignal().emit( "scanner" , reason.empty() ? std::string("error") : reason ) ;
	}
	GNet::Client::onDeleteImp( reason , b ) ;
}

bool GSmtp::ScannerClient::onReceive( const std::string & line )
{
	G_DEBUG( "GSmtp::ScannerClient::onReceive: [" << G::Str::printable(line) << "]" ) ;
	if( busy() )
	{
		m_path = G::Path() ;
		std::string scan_result = result(line) ; // empty string if scanned okay
		scan_result = G::Str::printable( scan_result ) ; // paranoia
		eventSignal().emit( "scanner" , scan_result ) ;
	}
	return true ;
}

void GSmtp::ScannerClient::onSendComplete()
{
}

// scanner customisation...

std::string GSmtp::ScannerClient::eol()
{
	return "\n" ;
}

std::string GSmtp::ScannerClient::request( const G::Path & path ) const
{
	return path.str() + "\n" ;
}

std::string GSmtp::ScannerClient::result( std::string line ) const
{
	G::Str::trim( line , "\r" ) ;
	return line.find("ok") == 0U ? std::string() : line ;
}

/// \file gscannerclient.cpp
