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
				<< to_status.reason << (to_status.help.empty()?"":": ") << to_status.help ) ;
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
	addText( received_line ) ;
}

bool GSmtp::ProtocolMessageStore::addText( const std::string & line )
{
	G_ASSERT( m_new_msg.get() != nullptr ) ;
	if( m_new_msg.get() == nullptr )
		return true ;
	return m_new_msg->addText( line ) ;
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
		G_LOG( "GSmtp::ProtocolMessageStore::process: filter: [" << m_filter->id() << "] [" << message_location << "]" ) ;
		m_filter->start( message_location ) ;
	}
	catch( std::exception & e ) // catch filtering errors
	{
		G_DEBUG( "GSmtp::ProtocolMessageStore::process: exception: " << e.what() ) ;
		clear() ;
		m_done_signal.emit( false , 0UL , e.what() ) ;
	}
}

void GSmtp::ProtocolMessageStore::filterDone( bool ok )
{
	try
	{
		G_DEBUG( "GSmtp::ProtocolMessageStore::filterDone: " << (ok?1:0) ) ;
		G_ASSERT( m_new_msg.get() != nullptr ) ;
		if( m_new_msg.get() == nullptr )
			throw G::Exception( "internal error" ) ; // never gets here

		// interpret the filter result -- note different semantics wrt. client-side
		bool ignore_this = !ok && m_filter->specialCancelled() ;
		bool rescan = m_filter->specialOther() ;

		unsigned long id = 0UL ;
		std::string reason ;
		if( ok )
		{
			// commit the message to the store
			m_new_msg->commit() ;
			id = m_new_msg->id() ;
		}
		else if( ignore_this )
		{
			; // no-op
		}
		else
		{
			reason = m_filter->text() ;
			G_LOG_S( "GSmtp::ProtocolMessageStore::filterDone: error storing message: [" << reason << "]" ) ;
			reason = "rejected" ; // don't tell the client too much
		}

		G_LOG( "GSmtp::ProtocolMessageStore::process: filter: ok=" << ok << " "
			<< "ignore=" << ignore_this << " rescan=" << rescan << " text=[" << G::Str::printable(reason) << "]" ) ;

		if( rescan )
		{
			// pick up any new messages create by the filter
			m_store.rescan() ;
		}

		clear() ;
		G_DEBUG( "GSmtp::ProtocolMessageStore::filterDone: emiting done signal" ) ;
		m_done_signal.emit( reason.empty() , id , reason ) ;
	}
	catch( std::exception & e ) // catch filtering errors
	{
		G_DEBUG( "GSmtp::ProtocolMessageStore::filterDone: exception: " << e.what() ) ;
		clear() ;
		m_done_signal.emit( false , 0UL , e.what() ) ;
	}
}

G::Slot::Signal3<bool,unsigned long,std::string> & GSmtp::ProtocolMessageStore::doneSignal()
{
	return m_done_signal ;
}

/// \file gprotocolmessagestore.cpp
