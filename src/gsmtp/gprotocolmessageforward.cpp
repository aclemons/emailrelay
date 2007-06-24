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
	const G::Executable & newfile_preprocessor , const GSmtp::Client::Config & client_config ,
	const Secrets & client_secrets , const std::string & server ,
	unsigned int connection_timeout ) :
		m_store(store) ,
		m_client_resolver_info(server) ,
		m_client_config(client_config) ,
		m_newfile_preprocessor(newfile_preprocessor) ,
		m_client_secrets(client_secrets) ,
		m_pms(store,newfile_preprocessor) ,
		m_client(NULL,true) ,
		m_id(0) ,
		m_connection_timeout(connection_timeout)
{
	// signal plumbing to receive 'done' events
	m_pms.doneSignal().connect( G::slot(*this,&ProtocolMessageForward::processDone) ) ;
	m_client.doneSignal().connect( G::slot(*this,&ProtocolMessageForward::clientDone) ) ;
}

G::Signal3<bool,unsigned long,std::string> & GSmtp::ProtocolMessageForward::storageDoneSignal()
{
	return m_pms.doneSignal() ;
}

GSmtp::ProtocolMessageForward::~ProtocolMessageForward()
{
	m_pms.doneSignal().disconnect() ;
	m_client.doneSignal().disconnect() ;
}

G::Signal3<bool,unsigned long,std::string> & GSmtp::ProtocolMessageForward::doneSignal()
{
	return m_done_signal ;
}

G::Signal3<bool,bool,std::string> & GSmtp::ProtocolMessageForward::preparedSignal()
{
	return m_prepared_signal ;
}

void GSmtp::ProtocolMessageForward::clear()
{
	m_pms.clear() ;
	m_client.reset() ;
}

bool GSmtp::ProtocolMessageForward::setFrom( const std::string & from )
{
	return m_pms.setFrom( from ) ;
}

bool GSmtp::ProtocolMessageForward::prepare()
{
	return false ; // no async preparation required
}

bool GSmtp::ProtocolMessageForward::addTo( const std::string & to , Verifier::Status to_status )
{
	return m_pms.addTo( to , to_status ) ;
}

void GSmtp::ProtocolMessageForward::addReceived( const std::string & line )
{
	m_pms.addReceived( line ) ;
}

void GSmtp::ProtocolMessageForward::addText( const std::string & line )
{
	m_pms.addText( line ) ;
}

std::string GSmtp::ProtocolMessageForward::from() const
{
	return m_pms.from() ;
}

void GSmtp::ProtocolMessageForward::process( const std::string & auth_id , const std::string & client_ip )
{
	m_pms.process( auth_id , client_ip ) ;
}

void GSmtp::ProtocolMessageForward::processDone( bool success , unsigned long id , std::string reason )
{
	G_DEBUG( "ProtocolMessageForward::processDone: " << (success?1:0) << ", " << id << ", \"" << reason << "\"" ) ;
	if( success && id != 0UL )
	{
		m_id = id ;

		// do the forwarding using the Client object
		bool nothing_to_do = false ;
		success = forward( id , nothing_to_do , &reason ) ;
		if( !success || nothing_to_do )
		{
			// failed or no recipients
			m_done_signal.emit( success , id , reason ) ;
		}
	}
	else
	{
		// failed or cancelled
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
			m_client.reset( new Client(m_client_resolver_info,message,m_client_secrets,m_client_config) ) ;
		}
		return true ;
	}
	catch( std::exception & e )
	{
		if( reason_p != NULL )
		{
			G_DEBUG( "GSmtp::ProtocolMessageForward::forward: exception" ) ;
			*reason_p = e.what() ;
		}
		return false ;
	}
}

void GSmtp::ProtocolMessageForward::clientDone( std::string reason , bool )
{
	G_DEBUG( "GSmtp::ProtocolMessageForward::clientDone: \"" << reason << "\"" ) ;
	const bool ok = reason.empty() ;
	m_done_signal.emit( ok , m_id , reason ) ;
}

/// \file gprotocolmessageforward.cpp
