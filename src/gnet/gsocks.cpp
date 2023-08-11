//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gsocks.cpp
///

#include "gdef.h"
#include "gsocks.h"
#include "gassert.h"
#include <array>
#include <algorithm>

GNet::Socks::Socks( const Location & location ) :
	m_request_offset(0U)
{
	if( location.socks() )
	{
		unsigned int far_port = location.socksFarPort() ;
		if( !Address::validPort(far_port) || far_port > 0xffffU )
			throw SocksError( "invalid port" ) ;

		m_request = buildPdu( location.socksFarHost() , far_port ) ;
		m_request_offset = 0U ;
	}
}

std::string GNet::Socks::buildPdu( const std::string & far_host , unsigned int far_port )
{
	g_port_t far_port_n = htons( static_cast<g_port_t>(far_port) ) ;
	g_port_t far_port_lo = far_port_n & 0xffU ;
	g_port_t far_port_hi = (far_port_n>>8U) & g_port_t(0xffU) ;

	std::string userid ; // TODO - socks userid
	std::string data ;
	data.reserve( far_host.size() + 10U ) ;
	data.append( 1U , 4 ) ; // version 4
	data.append( 1U , 1 ) ; // connect request
	data.append( 1U , static_cast<char>(far_port_lo) ) ;
	data.append( 1U , static_cast<char>(far_port_hi) ) ;
	data.append( 1U , 0 ) ; // invalid ipv4 (signals 4A protocol extension)
	data.append( 1U , 0 ) ; // invalid ipv4
	data.append( 1U , 0 ) ; // invalid ipv4
	data.append( 1U , 1 ) ; // invalid ipv4
	data.append( userid ) ;
	data.append( 1U , 0 ) ; // NUL terminator
	data.append( far_host ) ; // 4A protocol extension: get the socks server to do dns
	data.append( 1U , 0 ) ; // NUL terminator

	return data ;
}

bool GNet::Socks::send( G::ReadWrite & io )
{
	if( m_request_offset >= m_request.size() )
		return true ;

	const char * p = m_request.data() + m_request_offset ;
	std::size_t n = m_request.size() - m_request_offset ;

	ssize_t rc = io.write( p , n ) ;
	if( rc < 0 && !io.eWouldBlock() )
	{
		throw SocksError( "socket write error" ) ;
	}
	else if( rc < 0 || static_cast<std::size_t>(rc) < n )
	{
		std::size_t nsent = rc < 0 ? std::size_t(0U) : static_cast<std::size_t>(rc) ;
		m_request_offset += nsent ;
		return false ;
	}
	else
	{
		m_request_offset = m_request.size() ;
		return true ;
	}
}

bool GNet::Socks::read( G::ReadWrite & io )
{
	std::array<char,8U> buffer {} ;
	ssize_t rc = io.read( &buffer[0] , buffer.size() ) ;
	if( rc == 0 )
	{
		throw SocksError( "disconnected" ) ;
	}
	else if( rc == -1 && !io.eWouldBlock() )
	{
		throw SocksError( "socket read error" ) ;
	}
	else if( rc < 0 )
	{
		return false ; // go again
	}
	else
	{
		G_ASSERT( rc >= 1 && rc <= 8 ) ;
		std::size_t n = std::min( buffer.size() , static_cast<std::size_t>(rc) ) ;
		m_response.append( &buffer[0] , n ) ;
	}

	if( m_response.size() >= 8U )
	{
		G_ASSERT( m_response.size() == 8U ) ;
		if( m_response[0] != 0 )
		{
			throw SocksError( "invalid response" ) ;
		}
		else if( m_response[1] != 'Z' )
		{
			throw SocksError( "request rejected" ) ;
		}
		return true ;
	}
	else
	{
		return false ;
	}
}

