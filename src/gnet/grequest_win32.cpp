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
// grequest_win32.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "grequest.h"
#include "gwinhid.h"
#include "gdebug.h"
#include "gassert.h"
#include "glog.h"

GNet::Request::Request( bool host ) :
	m_magic(magic) ,
	m_error(0) ,
	m_handle(0) ,
	m_host(host) ,
	m_done(false) ,
	m_numeric(false) ,
	m_address(Address::invalidAddress())
{
}

const char * GNet::Request::reason( bool host_error , int error )
{
	switch( error )
	{
		case WSAHOST_NOT_FOUND: return "host not found" ;
		case WSAENOBUFS: return "buffer overflow" ;
		case WSATRY_AGAIN: return "resource error" ;
		case WSANO_RECOVERY: return "general failure" ;
		case WSANO_DATA: return host_error ? "no such host" : "no such service" ;
		case WSANOTINITIALISED: return "not initialised" ;
		case WSAEWOULDBLOCK: return "would block" ;
		case WSAENETDOWN: return "network down" ;
		case WSAEINPROGRESS: return "blocking operation in progress" ;
		case WSAEINTR: return "interrupted" ;
	}
	return "undefined error" ;
}

bool GNet::Request::valid() const
{
	return m_numeric || m_handle != 0 ;
}

std::string GNet::Request::reason() const
{
	G_ASSERT( m_handle == 0 ) ;
	G_DEBUG( "GNet::Request::reason: \"" << reason(m_host,m_error) << "\"" ) ;
	return reason(m_host,m_error) ;
}

GNet::Request::~Request()
{
	if( m_handle )
		::WSACancelAsyncRequest( m_handle ) ;

	m_magic = 0 ;
}

bool GNet::Request::onMessage( WPARAM wparam , LPARAM lparam )
{
	if( m_numeric )
	{
		G_ASSERT( wparam == 0 ) ;
		G_ASSERT( lparam == 0 ) ;
		G_ASSERT( m_handle == 0 ) ;
		G_ASSERT( !m_done ) ;
		G_ASSERT( m_error == 0 ) ;
	}
	else
	{
		WORD error = WSAGETASYNCERROR( lparam ) ;
		HANDLE handle = (HANDLE)wparam ;

		G_DEBUG( "GNet::Request::onMessage: handle = " << handle << " , error = " << error ) ;
		G_DEBUG( "GNet::Request::onMessage: m_handle = " << m_handle ) ;

		G_ASSERT( m_magic == magic ) ;
		G_ASSERT( handle == m_handle ) ;
		G_ASSERT( m_handle != 0 ) ;
		G_ASSERT( !m_done ) ;

		m_error = (int)error ;
	}
	
	m_done = true ;
	m_handle = 0 ;

	return m_error == 0 ;
}

// HOST...

GNet::HostRequest::HostRequest( std::string host_name , HWND hwnd , unsigned msg ) :
	Request(true)
{
	if( numeric(host_name,m_address) )
	{
		m_numeric = true ;
		::PostMessage( hwnd , msg , 0 , 0 ) ;
		return ;
	}

	m_handle = ::WSAAsyncGetHostByName( hwnd , msg ,
		host_name.c_str() , m_buffer , sizeof(m_buffer) ) ;

	if( m_handle == 0 )
		m_error = ::WSAGetLastError() ;

	G_DEBUG( "GNet::HostRequest::ctor: host \"" << host_name << "\", "
		<< "handle " << m_handle ) ;
}

bool GNet::HostRequest::numeric( std::string string , Address & address )
{
	string.append( ":0" ) ;
	bool rc = Address::validString(string) ;
	if( rc )
	{
		G_DEBUG( "GNet::HostRequest::numeric: host part of \"" << string << "\" is already numeric" ) ;
		m_address = Address(string) ;
	}

	return rc ;
}

GNet::Address GNet::HostRequest::result() const
{
	G_ASSERT( m_done && m_handle == 0 ) ;
	if( m_numeric )
	{
		return m_address ;
	}
	else
	{
		const hostent *h = reinterpret_cast<const hostent*>(m_buffer) ;
		G_ASSERT( h != NULL ) ;
		return Address( *h , 0U ) ;
	}
}

std::string GNet::HostRequest::fqdn() const
{
	G_ASSERT( m_done && m_handle == 0 ) ;
	if( m_numeric )
	{
		return std::string() ;
	}
	else
	{
		const hostent *h = reinterpret_cast<const hostent*>(m_buffer) ;
		G_ASSERT( h != NULL && h->h_name != NULL ) ;
		return std::string( h->h_name ) ;
	}
}

// SERVICE...

GNet::ServiceRequest::ServiceRequest( std::string service_name , bool udp , HWND hwnd , unsigned msg ) :
	Request(false)
{
	if( numeric(service_name,m_address) )
	{
		m_numeric = true ;
		::PostMessage( hwnd , msg , 0 , 0 ) ;
		return ;
	}

	m_handle = ::WSAAsyncGetServByName( hwnd , msg ,
		service_name.c_str() , protocol(udp) ,
		m_buffer , sizeof(m_buffer) ) ;

	if( m_handle == 0 )
		m_error = ::WSAGetLastError() ;

	G_DEBUG( "GNet::ServiceRequest::ctor: service \"" 
		<< service_name << "\", handle " << m_handle ) ;
}

bool GNet::ServiceRequest::numeric( std::string string , Address & address )
{
	string = std::string("0.0.0.0:") + string ;
	bool rc = Address::validString(string) ;
	if( rc )
	{
		G_DEBUG( "GNet::ServiceRequest::numeric: service part of \"" << string << "\" is already numeric" ) ;
		m_address = Address(string) ;
	}

	return rc ;
}

const char * GNet::ServiceRequest::protocol( bool udp )
{
	return udp ? "udp" : "tcp" ;
}

GNet::Address GNet::ServiceRequest::result() const
{
	G_ASSERT( m_done && m_handle == 0 ) ;
	if( m_numeric )
	{
		return m_address ;
	}
	else
	{
		const servent *s = reinterpret_cast<const servent*>(m_buffer) ;
		G_ASSERT( s != NULL ) ;
		return Address( *s ) ;
	}
}

/// \file grequest_win32.cpp
