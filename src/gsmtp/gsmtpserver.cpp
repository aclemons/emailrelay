//
// Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gsmtpserver.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gsmtpserver.h"
#include "gprotocolmessagestore.h"
#include "gprotocolmessageforward.h"
#include "gprotocolmessagescanner.h"
#include "gmemory.h"
#include "glocal.h"
#include "glog.h"
#include "gdebug.h"
#include "gassert.h"
#include <string>

GSmtp::ServerPeer::ServerPeer( GNet::Server::PeerInfo peer_info ,
	Server & server , std::auto_ptr<ProtocolMessage> pmessage , const std::string & ident ,
	const Secrets & server_secrets , const Verifier & verifier ) :
		GNet::ServerPeer( peer_info ) ,
		m_server( server ) ,
		m_buffer( crlf() ) ,
		m_verifier( verifier ) ,
		m_pmessage( pmessage ) ,
		m_protocol( *this , m_verifier , *m_pmessage.get() , server_secrets ,
			thishost(), peer_info.m_address )
{
	G_LOG_S( "GSmtp::ServerPeer: smtp connection from " << peer_info.m_address.displayString() ) ;
	m_protocol.init( ident ) ;
}

std::string GSmtp::ServerPeer::thishost() const
{
	return GNet::Local::fqdn() ;
}

std::string GSmtp::ServerPeer::crlf()
{
	return std::string("\015\012") ;
}

void GSmtp::ServerPeer::onDelete()
{
	G_LOG_S( "GSmtp::ServerPeer: smtp connection closed: " << peerAddress().second.displayString() ) ;
}

void GSmtp::ServerPeer::onData( const char * p , size_t n )
{
	std::string s( p , n ) ;
	m_buffer.add( s ) ;
	while( m_buffer.more() )
	{
		bool this_deleted = processLine( m_buffer.line() ) ;
		if( this_deleted )
			break ;
	}
}

bool GSmtp::ServerPeer::processLine( const std::string & line )
{
	return m_protocol.apply( line ) ;
}

void GSmtp::ServerPeer::protocolSend( const std::string & line , bool allow_delete_this )
{
	if( line.length() == 0U )
		return ;

	ssize_t rc = socket().write( line.data() , line.length() ) ;
	if( rc < 0 && ! socket().eWouldBlock() )
	{
		if( allow_delete_this )
			doDelete() ; // onDelete() and "delete this"
	}
	else if( rc < 0 || static_cast<size_t>(rc) < line.length() )
	{
		G_ERROR( "GSmtp::ServerPeer::protocolSend: " <<
			"flow-control asserted: connection blocked" ) ;

		// an SMTP server only sends short status messages
		// back to the client so it is pretty wierd if the 
		// client/network cannot cope -- so just drop the 
		// connection
		//
		doDelete() ;
	}
}

void GSmtp::ServerPeer::protocolDone()
{
	G_DEBUG( "GSmtp::ServerPeer: disconnecting from " << peerAddress().second.displayString() ) ;
	doDelete() ; // onDelete() and "delete this"
}

// ===

GSmtp::Server::Server( MessageStore & store , 
	const Secrets & server_secrets , const Verifier & verifier ,
	const std::string & ident , bool allow_remote , 
	unsigned int port , const AddressList & interfaces ,
	const std::string & smtp_server , 
	unsigned int smtp_response_timeout , unsigned int smtp_connection_timeout ,
	const Secrets & client_secrets ,
	const std::string & scanner_server ,
	unsigned int scanner_response_timeout , unsigned int scanner_connection_timeout ) :
		m_store(store) ,
		m_ident(ident) ,
		m_allow_remote( allow_remote ) ,
		m_server_secrets(server_secrets) ,
		m_verifier(verifier) ,
		m_smtp_server(smtp_server) ,
		m_smtp_response_timeout(smtp_response_timeout) ,
		m_smtp_connection_timeout(smtp_connection_timeout) ,
		m_scanner_server(scanner_server) ,
		m_scanner_response_timeout(scanner_response_timeout) ,
		m_scanner_connection_timeout(scanner_connection_timeout) ,
		m_client_secrets(client_secrets) ,
		m_gnet_server_1( *this ) ,
		m_gnet_server_2( *this ) ,
		m_gnet_server_3( *this )
{
	if( interfaces.size() == 0U )
	{
		bind( m_gnet_server_1 , GNet::Address(port) , port ) ;
	}
	else
	{
		size_t i = 0U ;
		for( AddressList::const_iterator p = interfaces.begin() ; p != interfaces.end() ; ++p , ++i )
		{
			bind( imp(i) , *p , port ) ;
		}
	}
}

void GSmtp::Server::bind( GSmtp::ServerImp & gnet_server , GNet::Address address , unsigned int port )
{
	address.setPort( port ) ;
	gnet_server.init( address ) ;
}

GSmtp::ServerImp & GSmtp::Server::imp( size_t i )
{
	G_ASSERT( i < 3U ) ;
	if( i >= 3U ) throw Overflow() ;
	ServerImp * p[] = { &m_gnet_server_1 , &m_gnet_server_2 , &m_gnet_server_3 } ;
	return *(p[i]) ;
}

void GSmtp::Server::report() const
{
	for( size_t i = 0U ; i < 3U ; i++ )
	{
		Server * This = const_cast<Server*>(this) ;
		if( This->imp(i).address().first )
			G_LOG_S( "GSmtp::Server: listening on " << This->imp(i).address().second.displayString() ) ;
	}
}

GNet::ServerPeer * GSmtp::Server::newPeer( GNet::Server::PeerInfo peer_info )
{
	if( ! m_allow_remote && 
		!peer_info.m_address.sameHost(GNet::Local::canonicalAddress()) &&
		!peer_info.m_address.sameHost(GNet::Local::localhostAddress()) )
	{
		G_WARNING( "GSmtp::Server: configured to reject non-local connection: " 
			<< peer_info.m_address.displayString(false) << " is not one of " 
			<< GNet::Local::canonicalAddress().displayString(false) << ","
			<< GNet::Local::localhostAddress().displayString(false) ) ;
		return NULL ;
	}

	std::auto_ptr<ProtocolMessage> pmessage( newProtocolMessage() ) ;
	return new ServerPeer( peer_info , *this , pmessage , m_ident , m_server_secrets , m_verifier ) ;
}

GSmtp::ProtocolMessage * GSmtp::Server::newProtocolMessage()
{
	const bool immediate = ! m_smtp_server.empty() ;
	const bool scan = ! m_scanner_server.empty() ;
	if( immediate && scan )
	{
		G_DEBUG( "GSmtp::Server::newProtocolMessage: new ProtocolMessageScanner" ) ;
		return new ProtocolMessageScanner(m_store,m_client_secrets,
			m_smtp_server,m_smtp_response_timeout,m_smtp_connection_timeout,
			m_scanner_server,m_scanner_response_timeout,m_scanner_connection_timeout) ;
	}
	else if( immediate )
	{
		G_DEBUG( "GSmtp::Server::newProtocolMessage: new ProtocolMessageForward" ) ;
		return new ProtocolMessageForward(m_store,m_client_secrets,
			m_smtp_server,m_smtp_response_timeout,m_smtp_connection_timeout) ;
	}
	else
	{
		G_DEBUG( "GSmtp::Server::newProtocolMessage: new ProtocolMessageStore" ) ;
		return new ProtocolMessageStore(m_store) ;
	}
}

// ===

GSmtp::ServerImp::ServerImp( GSmtp::Server & server ) :
	m_server(server)
{
}

GNet::ServerPeer * GSmtp::ServerImp::newPeer( GNet::Server::PeerInfo peer_info )
{
	return m_server.newPeer( peer_info ) ;
}

