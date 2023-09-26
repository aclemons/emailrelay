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
/// \file gprotocolmessageforward.cpp
///

#include "gdef.h"
#include "gprotocolmessageforward.h"
#include "gprotocolmessagestore.h"
#include "gmessagestore.h"
#include "gstr.h"
#include "glog.h"

GSmtp::ProtocolMessageForward::ProtocolMessageForward( GNet::ExceptionSink es ,
	GStore::MessageStore & store , FilterFactoryBase & ff , std::unique_ptr<ProtocolMessage> pm ,
	const GSmtp::Client::Config & client_config ,
	const GAuth::SaslClientSecrets & client_secrets ,
	const std::string & forward_to , int forward_to_family ) :
		m_es(es) ,
		m_store(store) ,
		m_ff(ff) ,
		m_client_location(forward_to,forward_to_family) ,
		m_client_config(client_config) ,
		m_client_secrets(client_secrets) ,
		m_pm(pm.release()) ,
		m_id(GStore::MessageId::none()) ,
		m_processed_signal(true)
{
	// signal plumbing to receive 'done' events
	m_pm->processedSignal().connect( G::Slot::slot(*this,&ProtocolMessageForward::protocolMessageProcessed) ) ;
	m_client_ptr.deleteSignal().connect( G::Slot::slot(*this,&ProtocolMessageForward::clientDone) ) ;
}

GSmtp::ProtocolMessageForward::~ProtocolMessageForward()
{
	m_pm->processedSignal().disconnect() ;
	m_client_ptr.deleteSignal().disconnect() ;
	if( m_client_ptr.get() != nullptr )
		m_client_ptr->messageDoneSignal().disconnect() ;
}

GSmtp::ProtocolMessage::ProcessedSignal & GSmtp::ProtocolMessageForward::processedSignal() noexcept
{
	return m_processed_signal ;
}

void GSmtp::ProtocolMessageForward::reset()
{
	m_pm->reset() ;
	m_client_ptr.reset() ;
}

void GSmtp::ProtocolMessageForward::clear()
{
	m_pm->clear() ;
}

GStore::MessageId GSmtp::ProtocolMessageForward::setFrom( const std::string & from , const FromInfo & from_info )
{
	return m_pm->setFrom( from , from_info ) ;
}

GSmtp::ProtocolMessage::FromInfo GSmtp::ProtocolMessageForward::fromInfo() const
{
	return m_pm->fromInfo() ;
}

std::string GSmtp::ProtocolMessageForward::bodyType() const
{
	return m_pm->bodyType() ;
}

bool GSmtp::ProtocolMessageForward::addTo( const ToInfo & to_info )
{
	return m_pm->addTo( to_info ) ;
}

void GSmtp::ProtocolMessageForward::addReceived( const std::string & line )
{
	m_pm->addReceived( line ) ;
}

GStore::NewMessage::Status GSmtp::ProtocolMessageForward::addContent( const char * line_data , std::size_t line_size )
{
	return m_pm->addContent( line_data , line_size ) ;
}

std::size_t GSmtp::ProtocolMessageForward::contentSize() const
{
	return m_pm->contentSize() ;
}

std::string GSmtp::ProtocolMessageForward::from() const
{
	return m_pm->from() ;
}

void GSmtp::ProtocolMessageForward::process( const std::string & auth_id , const std::string & peer_socket_address ,
	const std::string & peer_certificate )
{
	// commit to the store -- forward when the commit is complete
	m_processed_signal.reset() ; // one-shot reset
	m_pm->process( auth_id , peer_socket_address , peer_certificate ) ;
}

void GSmtp::ProtocolMessageForward::protocolMessageProcessed( const ProtocolMessage::ProcessedInfo & info )
{
	G_ASSERT( info.response.find('\t') == std::string::npos ) ;
	G_DEBUG( "ProtocolMessageForward::protocolMessageProcessed: " << (info.success?1:0) << " "
		<< info.id.str() << " [" << info.response << "] [" << info.reason << "]" ) ;

	G::CallFrame this_( m_call_stack ) ;
	if( info.success && info.id.valid() )
	{
		m_id = info.id ;

		// the message is now stored -- start the forwarding using the Client object
		bool nothing_to_do = false ;
		std::string error = forward( info.id , nothing_to_do ) ;

		if( this_.deleted() ) return ; // just in case
		if( nothing_to_do )
		{
			// no remote recipients
			m_processed_signal.emit( { true , info.id , 0 , std::string() , std::string() } ) ;
		}
		else if( !error.empty() )
		{
			// immediate failure or no recipients etc
			m_processed_signal.emit( { false , info.id , 0 , "forwarding failed" , error } ) ;
		}
	}
	else
	{
		// filter fail, or filter abandon, or message storage failed
		m_processed_signal.emit( info ) ;
	}
}

std::string GSmtp::ProtocolMessageForward::forward( const GStore::MessageId & id , bool & nothing_to_do )
{
	try
	{
		nothing_to_do = false ;
		G_DEBUG( "GSmtp::ProtocolMessageForward::forward: forwarding message " << id.str() ) ;

		std::unique_ptr<GStore::StoredMessage> message = m_store.get( id ) ;
		if( message->toCount() == 0U )
		{
			nothing_to_do = true ;
		}
		else
		{
			if( m_client_ptr.get() == nullptr )
			{
				m_client_ptr.reset( std::make_unique<Forward>( GNet::ExceptionSink(m_client_ptr,m_es.esrc()),
					m_ff , m_client_location , m_client_secrets , m_client_config ) ) ;

				m_client_ptr->messageDoneSignal().connect( G::Slot::slot( *this ,
					&GSmtp::ProtocolMessageForward::messageDone ) ) ;
			}
			m_client_ptr->sendMessage( std::unique_ptr<GStore::StoredMessage>(message.release()) ) ;
		}
		return std::string() ;
	}
	catch( std::exception & e ) // send forwarding errors back to the remote client via the server protocol
	{
		G_WARNING( "GSmtp::ProtocolMessageForward::forward: forwarding exception: " << e.what() ) ;
		std::string e_what = e.what() ;
		if( e_what.empty() )
			e_what = "exception" ;
		return e_what ;
	}
}

void GSmtp::ProtocolMessageForward::messageDone( const Client::MessageDoneInfo & info )
{
	G_DEBUG( "GSmtp::ProtocolMessageForward::messageDone: \"" << info.response << "\"" ) ;
	const bool ok = info.response.empty() ;
	m_processed_signal.emit( { ok , m_id , ok?0:info.response_code , info.response , std::string() } ) ;
}

void GSmtp::ProtocolMessageForward::clientDone( const std::string & reason )
{
	G_DEBUG( "GSmtp::ProtocolMessageForward::clientDone: \"" << reason << "\"" ) ;
	const bool ok = reason.empty() ;
	m_processed_signal.emit( { ok , m_id , 0 , ok?"":"forwarding failed" , reason } ) ;
}

