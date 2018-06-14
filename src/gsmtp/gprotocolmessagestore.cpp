//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gstr.h"
#include "gassert.h"
#include "glog.h"

GSmtp::ProtocolMessageStore::ProtocolMessageStore( MessageStore & store , unique_ptr<Filter> filter ) :
	m_store(store) ,
	m_filter(filter.release())
{
	m_filter->doneSignal().connect( G::Slot::slot(*this,&ProtocolMessageStore::filterDone) ) ;
}

GSmtp::ProtocolMessageStore::~ProtocolMessageStore()
{
	m_filter->doneSignal().disconnect() ;
}

void GSmtp::ProtocolMessageStore::reset()
{
	G_DEBUG( "GSmtp::ProtocolMessageStore::reset" ) ;
	clear() ;
}

void GSmtp::ProtocolMessageStore::clear()
{
	G_DEBUG( "GSmtp::ProtocolMessageStore::clear" ) ;
	m_new_msg.reset() ;
	m_from.erase() ;
	m_filter->cancel() ;
}

bool GSmtp::ProtocolMessageStore::setFrom( const std::string & from , const std::string & from_auth )
{
	G_DEBUG( "GSmtp::ProtocolMessageStore::setFrom: " << from ) ;

	if( from.length() == 0U ) // => probably a failure notification message
		G_WARNING( "GSmtp::ProtocolMessageStore: empty MAIL-FROM return path" ) ;

	G_ASSERT( m_new_msg.get() == nullptr ) ;
	clear() ; // just in case

	m_new_msg.reset( m_store.newMessage(from,from_auth,"").release() ) ;

	m_from = from ;
	return true ; // accept any name
}

bool GSmtp::ProtocolMessageStore::addTo( const std::string & to , VerifierStatus to_status )
{
	G_DEBUG( "GSmtp::ProtocolMessageStore::addTo: " << to ) ;

	G_ASSERT( m_new_msg.get() != nullptr ) ;
	if( to.length() > 0U && m_new_msg.get() != nullptr )
	{
		if( !to_status.is_valid )
		{
			G_WARNING( "GSmtp::ProtocolMessage: rejecting recipient \"" << to << "\": "
				<< to_status.response << (to_status.reason.empty()?"":": ") << to_status.reason ) ;
			return false ;
		}
		else
		{
			m_new_msg->addTo( to_status.address , to_status.is_local ) ;
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
	if( m_new_msg.get() != nullptr )
		m_new_msg->addTextLine( received_line ) ;
}

bool GSmtp::ProtocolMessageStore::addText( const char * line_data , size_t line_size )
{
	G_ASSERT( m_new_msg.get() != nullptr ) ;
	if( m_new_msg.get() == nullptr )
		return true ;
	return m_new_msg->addText( line_data , line_size ) ;
}

std::string GSmtp::ProtocolMessageStore::from() const
{
	return m_new_msg.get() ? m_from : std::string() ;
}

void GSmtp::ProtocolMessageStore::process( const std::string & session_auth_id , const std::string & peer_socket_address ,
	const std::string & peer_certificate )
{
	try
	{
		G_DEBUG( "GSmtp::ProtocolMessageStore::process: \"" << session_auth_id << "\", \"" << peer_socket_address << "\"" ) ;
		G_ASSERT( m_new_msg.get() != nullptr ) ;
		if( m_new_msg.get() == nullptr )
			throw G::Exception( "internal error" ) ; // never gets here

		// write ".new" envelope
		std::string message_location = m_new_msg->prepare( session_auth_id , peer_socket_address , peer_certificate ) ;

		// start the filter
		if( !m_filter->simple() )
			G_LOG( "GSmtp::ProtocolMessageStore::process: filter start: [" << m_filter->id() << "] [" << message_location << "]" ) ;
		m_filter->start( message_location ) ;
	}
	catch( std::exception & e ) // catch filtering errors
	{
		G_WARNING( "GSmtp::ProtocolMessageStore::process: message processing exception: " << e.what() ) ;
		clear() ;
		m_done_signal.emit( false , 0UL , "failed" , e.what() ) ;
	}
}

void GSmtp::ProtocolMessageStore::filterDone( int filter_result )
{
	try
	{
		G_DEBUG( "GSmtp::ProtocolMessageStore::filterDone: " << filter_result ) ;
		G_ASSERT( m_new_msg.get() != nullptr ) ;
		if( m_new_msg.get() == nullptr )
			throw G::Exception( "internal error" ) ; // never gets here

		const bool ok = filter_result == 0 ;
		const bool abandon = filter_result == 1 ;
		const bool rescan = m_filter->special() ;
		G_ASSERT( m_filter->reason().empty() == (ok || abandon) ) ; // filter post-condition

		if( !m_filter->simple() )
			G_LOG( "GSmtp::ProtocolMessageStore::filterDone: filter done: " << m_filter->str(true) ) ;

		unsigned long id = 0UL ;
		if( ok )
		{
			// commit the message to the store
			m_new_msg->commit( true ) ;
			id = m_new_msg->id() ;
		}
		else if( abandon )
		{
			// make a token effort to commit the message
			// but ignore errors and dont set the id
			m_new_msg->commit( false ) ;
		}
		else
		{
			G_LOG_S( "GSmtp::ProtocolMessageStore::filterDone: rejected by filter: [" << m_filter->reason() << "]" ) ;
		}

		if( rescan )
		{
			// pick up any new messages create by the filter
			m_store.rescan() ;
		}

		std::string filter_response = m_filter->response() ;
		std::string filter_reason = m_filter->reason() ;
		clear() ;
		G_DEBUG( "GSmtp::ProtocolMessageStore::filterDone: emiting done signal" ) ;
		m_done_signal.emit( ok || abandon , id , filter_response , filter_reason ) ;
	}
	catch( std::exception & e ) // catch filtering errors
	{
		G_WARNING( "GSmtp::ProtocolMessageStore::filterDone: filter exception: " << e.what() ) ;
		clear() ;
		m_done_signal.emit( false , 0UL , "rejected" , e.what() ) ;
	}
}

G::Slot::Signal4<bool,unsigned long,std::string,std::string> & GSmtp::ProtocolMessageStore::doneSignal()
{
	return m_done_signal ;
}

/// \file gprotocolmessagestore.cpp
