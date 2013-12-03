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
// gprotocolmessagestore.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gprotocolmessagestore.h"
#include "gmessagestore.h"
#include "gmemory.h"
#include "gstr.h"
#include "gassert.h"
#include "glog.h"

GSmtp::ProtocolMessageStore::ProtocolMessageStore( MessageStore & store , std::auto_ptr<Processor> processor ) :
	m_store(store) ,
	m_processor(processor)
{
	m_processor->doneSignal().connect( G::slot(*this,&ProtocolMessageStore::preprocessorDone) ) ;
}

GSmtp::ProtocolMessageStore::~ProtocolMessageStore()
{
	m_processor->doneSignal().disconnect() ;
}

void GSmtp::ProtocolMessageStore::reset()
{
	G_DEBUG( "GSmtp::ProtocolMessageStore::reset" ) ;
	clear() ;
}

void GSmtp::ProtocolMessageStore::clear()
{
	G_DEBUG( "GSmtp::ProtocolMessageStore::clear" ) ;
	m_msg <<= 0 ;
	m_from.erase() ;
	m_processor->abort() ;
}

bool GSmtp::ProtocolMessageStore::setFrom( const std::string & from )
{
	G_DEBUG( "GSmtp::ProtocolMessageStore::setFrom: " << from ) ;

	if( from.length() == 0U ) // => probably a failure notification message
		G_WARNING( "GSmtp::ProtocolMessageStore: empty MAIL-FROM return path" ) ;

	G_ASSERT( m_msg.get() == NULL ) ;
	clear() ; // just in case

	std::auto_ptr<NewMessage> new_message( m_store.newMessage(from) ) ;
	m_msg <<= new_message.release() ;

	m_from = from ;
	return true ; // accept any name
}

bool GSmtp::ProtocolMessageStore::addTo( const std::string & to , VerifierStatus to_status )
{
	G_DEBUG( "GSmtp::ProtocolMessageStore::addTo: " << to ) ;

	G_ASSERT( m_msg.get() != NULL ) ;
	if( to.length() > 0U && m_msg.get() != NULL )
	{
		if( !to_status.is_valid )
		{
			G_WARNING( "GSmtp::ProtocolMessage: rejecting recipient \"" << to << "\": "
				<< to_status.reason << ": " << to_status.help );
			return false ;
		}
		else
		{
			m_msg->addTo( to_status.address , to_status.is_local ) ;
			return true ;
		}
	}
	else
	{
		return false ;
	}
}

void GSmtp::ProtocolMessageStore::addReceived( const std::string & received_line )
{
	G_DEBUG( "GSmtp::ProtocolMessageStore::addReceived" ) ;
	addText( received_line ) ;
}

bool GSmtp::ProtocolMessageStore::addText( const std::string & line )
{
	G_ASSERT( m_msg.get() != NULL ) ;
	if( m_msg.get() == NULL )
		return true ;
	return m_msg->addText( line ) ;
}

std::string GSmtp::ProtocolMessageStore::from() const
{
	return m_msg.get() ? m_from : std::string() ;
}

void GSmtp::ProtocolMessageStore::process( const std::string & auth_id , const std::string & peer_socket_address ,
	const std::string & peer_socket_name , const std::string & peer_certificate )
{
	try
	{
		G_DEBUG( "ProtocolMessageStore::process: \"" << auth_id << "\", \"" << peer_socket_address << "\"" ) ;
		G_ASSERT( m_msg.get() != NULL ) ;
		if( m_msg.get() == NULL )
			throw G::Exception( "internal error" ) ; // never gets here

		// write ".new" envelope
		std::string message_location = m_msg->prepare( auth_id , peer_socket_address , peer_socket_name , peer_certificate ) ; 

		// start preprocessing
		m_processor->start( message_location ) ;
	}
	catch( std::exception & e ) // catch preprocessing errors
	{
		G_DEBUG( "GSmtp::ProtocolMessage::process: exception: " << e.what() ) ;
		clear() ;
		m_done_signal.emit( false , 0UL , e.what() ) ;
	}
}

void GSmtp::ProtocolMessageStore::preprocessorDone( bool ok )
{
	try
	{
		G_DEBUG( "ProtocolMessageStore::preprocessorDone: " << (ok?1:0) ) ;
		G_ASSERT( m_msg.get() != NULL ) ;
		if( m_msg.get() == NULL )
			throw G::Exception( "internal error" ) ; // never gets here

		unsigned long id = 0UL ;
		std::string reason ;
		if( ok )
		{
			m_msg->commit() ;
			id = m_msg->id() ;
		}
		else if( m_processor->cancelled() )
		{
			try { m_msg->commit() ; } catch( std::exception & ) {}
		}
		else
		{
			reason = m_processor->text() ;
			reason = reason.empty() ? "error" : reason ;
			G_LOG_S( "GSmtp::ProtocolMessageStore::preprocessorDone: error storing message: " << reason ) ;
		}
		if( m_processor->repoll() )
		{
			m_store.repoll() ;
		}
		clear() ;
		G_DEBUG( "ProtocolMessageStore::preprocessorDone: emiting done signal" ) ;
		m_done_signal.emit( reason.empty() , id , reason ) ;
	}
	catch( std::exception & e ) // catch preprocessing errors
	{
		G_DEBUG( "GSmtp::ProtocolMessage::preprocessorDone: exception: " << e.what() ) ;
		clear() ;
		m_done_signal.emit( false , 0UL , e.what() ) ;
	}
}

G::Signal3<bool,unsigned long,std::string> & GSmtp::ProtocolMessageStore::doneSignal()
{
	return m_done_signal ;
}

/// \file gprotocolmessagestore.cpp
