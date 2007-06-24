//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
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
		GNet::Client(resolver_info,connect_timeout,response_timeout)
{
	G_DEBUG( "GSmtp::ScannerClient::ctor: " << resolver_info.displayString() ) ;
}

void GSmtp::ScannerClient::onConnect()
{
	G_DEBUG( "GSmtp::ScannerClient::onConnect" ) ;
	if( m_path == G::Path() )
		send( request(m_path) ) ;
}

void GSmtp::ScannerClient::startScanning( const G::Path & path )
{
	G_DEBUG( "GSmtp::ScannerClient::startScanning: \"" << path << "\"" ) ;
	m_path = connected() ? G::Path() : path ;
	if( connected() )
		send( request(path) ) ;
}

void GSmtp::ScannerClient::onDelete( const std::string & reason , bool )
{
	G_WARNING( "GSmtp::ScannerClient::onDelete: scanner error: " << reason ) ;
}

bool GSmtp::ScannerClient::onReceive( const std::string & line )
{
	G_DEBUG( "GSmtp::ScannerClient::onReceive: " << G::Str::toPrintableAscii(line) ) ;
	std::string scan_result = result(line) ; // empty if scanned okay
	eventSignal().emit( "scanner" , scan_result ) ;
	return true ;
}

void GSmtp::ScannerClient::onSendComplete()
{
	// not interested
}

// scanner customisation...

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
