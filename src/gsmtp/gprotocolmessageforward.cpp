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
// gprotocolmessageforward.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gprotocolmessageforward.h"
#include "gprotocolmessagestore.h"
#include "gnullprocessor.h"
#include "gexecutableprocessor.h"
#include "gmessagestore.h"
#include "gmemory.h"
#include "gstr.h"
#include "gassert.h"
#include "glog.h"

GSmtp::ProtocolMessageForward::ProtocolMessageForward( MessageStore & store , 
	std::auto_ptr<ProtocolMessage> pm ,
	const GSmtp::Client::Config & client_config ,
	const GAuth::Secrets & client_secrets , const std::string & server ,
	unsigned int connection_timeout ) :
		m_store(store) ,
		m_client_resolver_info(server) ,
		m_client_config(client_config) ,
		m_client_secrets(client_secrets) ,
		m_pm(pm) ,
		m_client(NULL,true) ,
		m_id(0) ,
		m_connection_timeout(connection_timeout) ,
		m_done_signal(true) // one-shot, but reset()able
{
	// signal plumbing to receive 'done' events
	m_pm->doneSignal().connect( G::slot(*this,&ProtocolMessageForward::processDone) ) ;
	m_client.doneSignal().connect( G::slot(*this,&ProtocolMessageForward::clientDone) ) ;
}

GSmtp::ProtocolMessageForward::~ProtocolMessageForward()
{
	m_pm->doneSignal().disconnect() ;
	m_client.doneSignal().disconnect() ;
	if( m_client.get() != NULL )
		m_client->messageDoneSignal().disconnect() ;
}

G::Signal3<bool,unsigned long,std::string> & GSmtp::ProtocolMessageForward::storageDoneSignal()
{
	return m_pm->doneSignal() ;
}

G::Signal3<bool,unsigned long,std::string> & GSmtp::ProtocolMessageForward::doneSignal()
{
	return m_done_signal ;
}

void GSmtp::ProtocolMessageForward::reset()
{
	m_pm->reset() ;
	m_client.reset() ;
}

void GSmtp::ProtocolMessageForward::clear()
{
	m_pm->clear() ;
}

bool GSmtp::ProtocolMessageForward::setFrom( const std::string & from )
{
	return m_pm->setFrom( from ) ;
}

bool GSmtp::ProtocolMessageForward::addTo( const std::string & to , VerifierStatus to_status )
{
	return m_pm->addTo( to , to_status ) ;
}

void GSmtp::ProtocolMessageForward::addReceived( const std::string & line )
{
	m_pm->addReceived( line ) ;
}

bool GSmtp::ProtocolMessageForward::addText( const std::string & line )
{
	return m_pm->addText( line ) ;
}

std::string GSmtp::ProtocolMessageForward::from() const
{
	return m_pm->from() ;
}

void GSmtp::ProtocolMessageForward::process( const std::string & auth_id , const std::string & peer_socket_address ,
	const std::string & peer_socket_name , const std::string & peer_certificate )
{
	m_done_signal.reset() ; // one-shot reset
	m_pm->process( auth_id , peer_socket_address , peer_socket_name , peer_certificate ) ;
}

void GSmtp::ProtocolMessageForward::processDone( bool success , unsigned long id , std::string reason )
{
	G_DEBUG( "ProtocolMessageForward::processDone: " << (success?1:0) << ", " << id << ", \"" << reason << "\"" ) ;
	if( success && id != 0UL )
	{
		m_id = id ;

		// the message is now stored -- start the forwarding using the Client object
		bool nothing_to_do = false ;
		success = forward( id , nothing_to_do , &reason ) ;
		if( !success || nothing_to_do )
		{
			// immediate failure or no recipients
			m_done_signal.emit( success , id , reason ) ;
		}
	}
	else
	{
		// message storage failed or cancelled
		m_done_signal.emit( success , id , reason ) ;
	}
}

bool GSmtp::ProtocolMessageForward::forward( unsigned long id , bool & nothing_to_do , std::string * reason_p )
{
	try
	{
		nothing_to_do = false ;
		*reason_p = std::string() ;
		G_DEBUG( "GSmtp::ProtocolMessageForward::forward: forwarding message " << id ) ;

		std::auto_ptr<StoredMessage> message = m_store.get( id ) ;

		if( message->remoteRecipientCount() == 0U )
		{
			// use our local delivery mechanism, not the downstream server's
			nothing_to_do = true ;
			message->destroy() ; // (already copied to "*.local")
		}
		else
		{
			if( m_client.get() == NULL )
			{
				m_client.reset( new Client(m_client_resolver_info,m_client_secrets,m_client_config) ) ;
				m_client->messageDoneSignal().connect( G::slot(*this,&GSmtp::ProtocolMessageForward::messageDone) ) ;
			}
			m_client->sendMessage( message ) ;
		}
		return true ;
	}
	catch( std::exception & e ) // send forwarding errors back to the remote client via the server protocol
	{
		if( reason_p != NULL )
		{
			G_DEBUG( "GSmtp::ProtocolMessageForward::forward: exception" ) ;
			*reason_p = e.what() ;
		}
		return false ;
	}
}

void GSmtp::ProtocolMessageForward::messageDone( std::string reason )
{
	G_DEBUG( "GSmtp::ProtocolMessageForward::clientDone: \"" << reason << "\"" ) ;
	const bool ok = reason.empty() ;
	m_done_signal.emit( ok , m_id , reason ) ; // one-shot
}

void GSmtp::ProtocolMessageForward::clientDone( std::string reason , bool )
{
	G_DEBUG( "GSmtp::ProtocolMessageForward::clientDone: \"" << reason << "\"" ) ;
	const bool ok = reason.empty() ;
	m_done_signal.emit( ok , m_id , reason ) ; // one-shot
}

/// \file gprotocolmessageforward.cpp
