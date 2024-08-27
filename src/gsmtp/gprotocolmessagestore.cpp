//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gprotocolmessagestore.cpp
///

#include "gdef.h"
#include "gprotocolmessagestore.h"
#include "gmessagestore.h"
#include "gstr.h"
#include "gassert.h"
#include "glog.h"

GSmtp::ProtocolMessageStore::ProtocolMessageStore( GStore::MessageStore & store ,
	std::unique_ptr<Filter> filter ) :
		m_store(store) ,
		m_filter(std::move(filter))
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
	m_from_info = FromInfo() ;
	m_filter->cancel() ;
}

GStore::MessageId GSmtp::ProtocolMessageStore::setFrom( const std::string & from ,
	const FromInfo & from_info )
{
	G_DEBUG( "GSmtp::ProtocolMessageStore::setFrom: " << from ) ;

	if( from.empty() ) // => probably a failure notification message
		G_WARNING( "GSmtp::ProtocolMessageStore: empty MAIL-FROM return path" ) ;

	G_ASSERT( m_new_msg == nullptr ) ;
	clear() ; // just in case

	GStore::MessageStore::SmtpInfo smtp_info ;
	smtp_info.auth = from_info.auth ;
	smtp_info.body = from_info.body ;
	smtp_info.address_style = from_info.address_style ;
	const std::string & from_auth_out = std::string() ;
	m_new_msg = m_store.newMessage( from , smtp_info , from_auth_out ) ;

	m_from = from ;
	m_from_info = from_info ;
	return m_new_msg->id() ;
}

bool GSmtp::ProtocolMessageStore::addTo( const ToInfo & to_info )
{
	G_DEBUG( "GSmtp::ProtocolMessageStore::addTo: " << to_info.status.recipient ) ;
	G_ASSERT( m_new_msg != nullptr ) ;

	if( to_info.status.recipient.empty() )
	{
		return false ;
	}
	else if( !to_info.status.is_valid )
	{
		G_WARNING( "GSmtp::ProtocolMessage: rejecting recipient \"" << to_info.status.recipient << "\": "
			<< to_info.status.response << (to_info.status.reason.empty()?"":": ") << to_info.status.reason ) ;
		return false ;
	}
	else
	{
		m_new_msg->addTo( to_info.status.address , to_info.status.is_local , to_info.address_style ) ;
		return true ;
	}
}

void GSmtp::ProtocolMessageStore::addReceived( const std::string & received_line )
{
	G_DEBUG( "GSmtp::ProtocolMessageStore::addReceived" ) ;
	if( m_new_msg != nullptr )
		m_new_msg->addContentLine( received_line ) ;
}

GStore::NewMessage::Status GSmtp::ProtocolMessageStore::addContent( const char * data , std::size_t data_size )
{
	G_ASSERT( m_new_msg != nullptr ) ;
	return m_new_msg->addContent( data , data_size ) ;
}

std::size_t GSmtp::ProtocolMessageStore::contentSize() const
{
	return m_new_msg ? m_new_msg->contentSize() : 0U ;
}

std::string GSmtp::ProtocolMessageStore::from() const
{
	return m_from ;
}

GSmtp::ProtocolMessage::FromInfo GSmtp::ProtocolMessageStore::fromInfo() const
{
	return m_from_info ;
}

std::string GSmtp::ProtocolMessageStore::bodyType() const
{
	return m_from_info.body ;
}

void GSmtp::ProtocolMessageStore::process( const std::string & session_auth_id ,
	const std::string & peer_socket_address , const std::string & peer_certificate )
{
	try
	{
		G_DEBUG( "GSmtp::ProtocolMessageStore::process: \""
			<< session_auth_id << "\", \"" << peer_socket_address << "\"" ) ;
		G_ASSERT( m_new_msg != nullptr ) ;

		// write ".new" envelope and close the content
		m_new_msg->prepare( session_auth_id , peer_socket_address , peer_certificate ) ;

		// start filtering
		G_LOG_MORE( "GSmtp::ProtocolMessageStore::process: filter [" << m_filter->id() << "]: [" << m_new_msg->id().str() << "]" ) ;
		m_filter->start( m_new_msg->id() ) ;
	}
	catch( std::exception & e ) // catch filtering errors, size-limit errors, and file i/o errors
	{
		G_WARNING( "GSmtp::ProtocolMessageStore::process: message processing exception: " << e.what() ) ;
		clear() ;
		m_processed_signal.emit( { false , GStore::MessageId::none() , 0 , "failed" , e.what() } ) ;
	}
}

void GSmtp::ProtocolMessageStore::filterDone( int filter_result )
{
	try
	{
		G_DEBUG( "GSmtp::ProtocolMessageStore::filterDone: " << filter_result ) ;
		G_ASSERT( static_cast<int>(m_filter->result()) == filter_result ) ;
		G_ASSERT( m_new_msg != nullptr ) ;

		const bool ok = filter_result == 0 ;
		const bool abandon = filter_result == 1 ;
		const bool rescan = m_filter->special() ;

		std::string filter_response = G::Str::replaced( (ok||abandon) ? std::string() : m_filter->response() , '\t' , ' ' ) ;
		int filter_response_code = (ok||abandon) ? 0 : m_filter->responseCode() ;
		std::string filter_reason = (ok||abandon) ? std::string() : m_filter->reason() ;

		G_LOG_IF( !m_filter->quiet() , "GSmtp::ProtocolMessageStore::filterDone: "
			"filter [" << m_filter->id() << "]: [" << m_new_msg->id().str() << "]: "
			<< m_filter->str(Filter::Type::server) ) ;

		GStore::MessageId message_id = GStore::MessageId::none() ;
		if( ok )
		{
			// commit the message to the store
			m_new_msg->commit( true ) ;
			message_id = m_new_msg->id() ;
		}
		else if( abandon )
		{
			// make a token effort to commit the message
			// but ignore errors and dont set the id
			m_new_msg->commit( false ) ;
		}
		else
		{
			G_LOG_S( "GSmtp::ProtocolMessageStore::filterDone: rejected by filter: [" << filter_reason << "]" ) ;
		}

		if( rescan )
		{
			// pick up any new messages create by the filter
			m_store.rescan() ;
		}

		clear() ;
		m_processed_signal.emit( { ok || abandon , message_id , filter_response_code , filter_response , filter_reason } ) ;
	}
	catch( std::exception & e ) // catch filtering errors
	{
		G_WARNING( "GSmtp::ProtocolMessageStore::filterDone: filter exception: " << e.what() ) ;
		clear() ;
		m_processed_signal.emit( { false , GStore::MessageId::none() , 0 , "rejected" , e.what() } ) ;
	}
}

GSmtp::ProtocolMessage::ProcessedSignal & GSmtp::ProtocolMessageStore::processedSignal() noexcept
{
	return m_processed_signal ;
}

