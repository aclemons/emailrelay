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
// gprotocolmessageforward.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gprotocolmessageforward.h"
#include "gprotocolmessagestore.h"
#include "gmessagestore.h"
#include "gmemory.h"
#include "gstr.h"
#include "gassert.h"
#include "glog.h"

GSmtp::ProtocolMessageForward::ProtocolMessageForward( MessageStore & store , 
	const Secrets & client_secrets , const std::string & server ,
	unsigned int response_timeout , unsigned int connection_timeout ) :
		m_store(store) ,
		m_client_secrets(client_secrets) ,
		m_pm(store) ,
		m_server(server) ,
		m_response_timeout(response_timeout) ,
		m_connection_timeout(connection_timeout)
{
	m_pm.doneSignal().connect( G::slot(*this,&ProtocolMessageForward::processDone) ) ;
}

GSmtp::ProtocolMessageForward::~ProtocolMessageForward()
{
}

G::Signal3<bool,unsigned long,std::string> & GSmtp::ProtocolMessageForward::doneSignal()
{
	return m_signal ;
}

void GSmtp::ProtocolMessageForward::clear()
{
	m_pm.clear() ;
	m_client <<= 0 ;
}

bool GSmtp::ProtocolMessageForward::setFrom( const std::string & from )
{
	return m_pm.setFrom( from ) ;
}

bool GSmtp::ProtocolMessageForward::addTo( const std::string & to , Verifier::Status to_status )
{
	return m_pm.addTo( to , to_status ) ;
}

void GSmtp::ProtocolMessageForward::addReceived( const std::string & line )
{
	m_pm.addReceived( line ) ;
}

void GSmtp::ProtocolMessageForward::addText( const std::string & line )
{
	m_pm.addText( line ) ;
}

std::string GSmtp::ProtocolMessageForward::from() const
{
	return m_pm.from() ;
}

void GSmtp::ProtocolMessageForward::process( const std::string & auth_id ,
	const std::string & client_ip )
{
	m_pm.process( auth_id , client_ip ) ;
}

void GSmtp::ProtocolMessageForward::processDone( bool success , unsigned long id , std::string reason_in )
{
	std::string reason( reason_in ) ;
	bool nothing_to_do = success && id == 0UL ;
	if( success && id != 0UL )
	{
		m_id = id ;
		success = forward( id , nothing_to_do , &reason ) ;
	}

	if( nothing_to_do || !success )
	{
		m_signal.emit( success , id , reason ) ;
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

		bool ok = true ;
		if( message->remoteRecipientCount() == 0U )
		{
			// use our local delivery mechanism, not the downstream server's
			nothing_to_do = true ;
			message->destroy() ; // (already copied to "*.local")
		}
		else
		{
			m_client <<= new GSmtp::Client( message , m_client_secrets , m_response_timeout ) ;
			m_client->doneSignal().connect( G::slot(*this,&ProtocolMessageForward::clientDone) ) ;
			std::string reason = m_client->startSending( m_server , m_connection_timeout ) ;

			ok = reason.empty() ;
			if( !ok && reason_p != NULL )
				*reason_p = reason ;
		}
		return ok ;
	}
	catch( std::exception & e )
	{
		if( reason_p != NULL )
			*reason_p = e.what() ;
		return false ;
	}
}

void GSmtp::ProtocolMessageForward::clientDone( std::string reason )
{
	G_DEBUG( "GSmtp::ProtocolMessageForward::clientDone: \"" << reason << "\"" ) ;
	const bool ok = reason.empty() ;
	m_signal.emit( ok , m_id , reason ) ;
}

