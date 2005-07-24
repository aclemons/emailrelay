//
// Copyright (C) 2001-2005 Graeme Walker <graeme_walker@users.sourceforge.net>
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

GSmtp::ScannerClient::ScannerClient( const std::string & host_and_service ,
	unsigned int connect_timeout , unsigned int response_timeout ) :
		m_timer(*this) ,
		m_connect_timeout(connect_timeout) ,
		m_response_timeout(response_timeout) ,
		m_state("idle") ,
		m_socket(NULL) ,
		m_host(hostPart(host_and_service)) ,
		m_service(servicePart(host_and_service))
{
	G_DEBUG( "GSmtp::ScannerClient::ctor: " << host_and_service ) ;
}

GSmtp::ScannerClient::ScannerClient( const std::string & host , const std::string & service ,
	unsigned int connect_timeout , unsigned int response_timeout ) :
		m_timer(*this) ,
		m_connect_timeout(connect_timeout) ,
		m_response_timeout(response_timeout) ,
		m_state("idle") ,
		m_socket(NULL) ,
		m_host(host) ,
		m_service(service)
{
	G_DEBUG( "GSmtp::ScannerClient::ctor: " << host << ":" << service ) ;
}

G::Signal2<std::string,bool> & GSmtp::ScannerClient::connectedSignal()
{
	return m_connected_signal ;
}

G::Signal2<bool,std::string> & GSmtp::ScannerClient::doneSignal()
{
	return m_done_signal ;
}

void GSmtp::ScannerClient::startConnecting()
{
	G_DEBUG( "GSmtp::ScannerClient::startConnecting" ) ;
	G_ASSERT( m_state == "idle" ) ;

	m_timer.startTimer( m_connect_timeout ) ;
	setState( "connecting" ) ;
	if( ! connect( m_host , m_service ) )
	{
		setState( "failing" ) ;
		m_timer.cancelTimer() ;
		m_timer.startTimer(0U) ;
	}
}

void GSmtp::ScannerClient::onConnect( GNet::Socket & socket )
{
	G_DEBUG( "GSmtp::ScannerClient::onConnect" ) ;
	G_ASSERT( m_state == "connecting" ) ;

	m_socket = &socket ;
	setState( "temp" ) ;
	m_timer.cancelTimer() ;
	m_timer.startTimer(0U) ;
}

void GSmtp::ScannerClient::onError( const std::string & error )
{
	G_WARNING( "GSmtp::ScannerClient::onError: connect error: " << error ) ;
	G_ASSERT( m_state == "connecting" ) ;

	m_timer.cancelTimer() ;
	setState( "end" ) ;
	m_connected_signal.emit( error , GNet::Client::canRetry(error) ) ;
}

std::string GSmtp::ScannerClient::startScanning( const G::Path & path )
{
	G_DEBUG( "GSmtp::ScannerClient::startScanning: \"" << path << "\"" ) ;
	G_ASSERT( m_state == "connected" || m_state == "disconnected" ) ;

	if( m_state == "disconnected" )
	{
		setState( "end" ) ;
		return "disconnected" ;
	}
	else
	{
		m_timer.startTimer( m_response_timeout ) ;
		std::string data = request( path ) ;
		ssize_t rc = m_socket->write( data.c_str() , data.length() ) ;

		std::string result =
			rc < static_cast<ssize_t>(data.length()) ?
				( m_socket->eWouldBlock() ?
					std::string("flow control asserted by peer") :
					std::string("connection lost") ) :
			std::string() ;

		bool ok = result.empty() ;
		if( ok )
		{
			setState( "scanning" ) ;
		}
		else
		{
			setState( "end" ) ;
			m_timer.cancelTimer() ;
		}
		return result ;
	}
}

void GSmtp::ScannerClient::onDisconnect()
{
	G_DEBUG( "GSmtp::ScannerClient::onDisconnect" ) ;
	G_ASSERT( m_state == "connected" || m_state == "scanning" ) ;

	if( m_state == "connected" )
	{
		setState( "disconnected" ) ;
	}
	else
	{
		setState( "end" ) ;
		m_done_signal.emit( false , "disconnected" ) ;
	}
}

void GSmtp::ScannerClient::onData( const char * data , size_t size )
{
	std::string s( data , size ) ;
	G_DEBUG( "GSmtp::ScannerClient::onData: " << G::Str::toPrintableAscii(s) ) ;
	G_ASSERT( m_state == "scanning" ) ;

	m_line_buffer.add( s ) ;
	if( isDone() )
	{
		G_DEBUG( "GSmtp::ScannerClient::onData: done" ) ;
		m_timer.cancelTimer() ;
		m_socket->close() ;
		setState( "end" ) ;
		bool from_scanner = true ;
		m_done_signal.emit( from_scanner , result() ) ;
	}
}

void GSmtp::ScannerClient::onWriteable()
{
	// never gets here
	G_DEBUG( "GSmtp::ScannerClient::onWriteable" ) ;
}

void GSmtp::ScannerClient::onTimeout( GNet::Timer & )
{
	if( m_state == "failing" )
	{
		setState( "end" ) ;
		m_connected_signal.emit( "cannot connect" , false ) ;
	}
	else if( m_state == "temp" )
	{
		setState( "connected" ) ;
		m_connected_signal.emit( std::string() , false ) ;
	}
	else if( m_state == "connecting" )
	{
		setState( "end" ) ;
		m_connected_signal.emit( "connect timeout" , true ) ;
	}
	else if( m_state == "scanning" )
	{
		setState( "end" ) ;
		bool from_scanner = false ;
		m_done_signal.emit( from_scanner , "response timeout" ) ;
	}
}

void GSmtp::ScannerClient::setState( const std::string & new_state )
{
	G_DEBUG( "GSmtp::ScannerClient::setState: \"" << m_state << "\" -> \"" << new_state << "\"" ) ;
	m_state = new_state ;
}

//static
std::string GSmtp::ScannerClient::hostPart( const std::string & s )
{
	size_t pos = s.find(":") ;
	if( pos == std::string::npos )
	{
		throw FormatError(s) ;
	}
	return s.substr( 0U , pos ) ;
}

//static
std::string GSmtp::ScannerClient::servicePart( const std::string & s )
{
	size_t pos = s.find(":") ;
	if( pos == std::string::npos || (pos+1U) == s.length() )
	{
		throw FormatError(s) ;
	}
	return s.substr( pos+1U ) ;
}

// scanner customisation...

std::string GSmtp::ScannerClient::request( const G::Path & path ) const
{
	return path.str() + "\n" ;
}

bool GSmtp::ScannerClient::isDone() const
{
	return m_line_buffer.more() ;
}

std::string GSmtp::ScannerClient::result()
{
	std::string s = m_line_buffer.line() ;
	if( s.find("ok") != std::string::npos )
		return std::string() ;
	else
		return s ;
}


