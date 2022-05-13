//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gnullfilter.h"
#include "gexecutablefilter.h"
#include "gmessagestore.h"
#include "gstr.h"
#include "glog.h"

GSmtp::ProtocolMessageForward::ProtocolMessageForward( GNet::ExceptionSink es ,
	MessageStore & store , FilterFactory & ff , std::unique_ptr<ProtocolMessage> pm ,
	const GSmtp::Client::Config & client_config ,
	const GAuth::SaslClientSecrets & client_secrets , const std::string & server ) :
		m_es(es) ,
		m_store(store) ,
		m_ff(ff) ,
		m_client_location(server) ,
		m_client_config(client_config) ,
		m_client_secrets(client_secrets) ,
		m_pm(pm.release()) ,
		m_id(MessageId::none()) ,
		m_done_signal(true) // one-shot, but reset()able
{
	// signal plumbing to receive 'done' events
	m_pm->doneSignal().connect( G::Slot::slot(*this,&ProtocolMessageForward::processDone) ) ;
	m_client_ptr.deleteSignal().connect( G::Slot::slot(*this,&ProtocolMessageForward::clientDone) ) ;
}

GSmtp::ProtocolMessageForward::~ProtocolMessageForward()
{
	m_pm->doneSignal().disconnect() ;
	m_client_ptr.deleteSignal().disconnect() ;
	if( m_client_ptr.get() != nullptr )
		m_client_ptr->messageDoneSignal().disconnect() ;
}

GSmtp::ProtocolMessage::DoneSignal & GSmtp::ProtocolMessageForward::storageDoneSignal()
{
	return m_pm->doneSignal() ;
}

GSmtp::ProtocolMessage::DoneSignal & GSmtp::ProtocolMessageForward::doneSignal()
{
	return m_done_signal ;
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

GSmtp::MessageId GSmtp::ProtocolMessageForward::setFrom( const std::string & from , const FromInfo & from_info )
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

bool GSmtp::ProtocolMessageForward::addTo( VerifierStatus to_status )
{
	return m_pm->addTo( to_status ) ;
}

void GSmtp::ProtocolMessageForward::addReceived( const std::string & line )
{
	m_pm->addReceived( line ) ;
}

GSmtp::NewMessage::Status GSmtp::ProtocolMessageForward::addContent( const char * line_data , std::size_t line_size )
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
	m_done_signal.reset() ; // one-shot reset
	m_pm->process( auth_id , peer_socket_address , peer_certificate ) ;
}

void GSmtp::ProtocolMessageForward::processDone( bool success , const MessageId & id , const std::string & response ,
	const std::string & reason )
{
	G_DEBUG( "ProtocolMessageForward::processDone: " << (success?1:0) << " "
		<< id.str() << " [" << response << "] [" << reason << "]" ) ;
	G::CallFrame this_( m_call_stack ) ;
	if( success && id.valid() )
	{
		m_id = id ;

		// the message is now stored -- start the forwarding using the Client object
		bool nothing_to_do = false ;
		std::string error = forward( id , nothing_to_do ) ;
		if( this_.deleted() ) return ; // just in case
		if( !error.empty() || nothing_to_do )
		{
			// immediate failure or no recipients
			m_done_signal.emit( success , id , "forwarding failed" , error ) ;
		}
	}
	else
	{
		// filter fail-or-abandon, or message storage failed
		m_done_signal.emit( success , id , std::string(response) , std::string(reason) ) ;
	}
}

std::string GSmtp::ProtocolMessageForward::forward( const MessageId & id , bool & nothing_to_do )
{
	try
	{
		nothing_to_do = false ;
		G_DEBUG( "GSmtp::ProtocolMessageForward::forward: forwarding message " << id.str() ) ;

		std::unique_ptr<StoredMessage> message = m_store.get( id ) ;
		G_LOG( "GSmtp::ProtocolMessageForward::forward: processing message \"" << message->location() << "\"" ) ;

		if( message->toCount() == 0U )
		{
			// use our local delivery mechanism, not the downstream server's
			nothing_to_do = true ;
			message->destroy() ; // (already copied to "*.local")
		}
		else
		{
			if( m_client_ptr.get() == nullptr )
			{
				m_client_ptr.reset( std::make_unique<Client>( GNet::ExceptionSink(m_client_ptr,m_es.esrc()),
					m_ff , m_client_location , m_client_secrets , m_client_config ) ) ;
				m_client_ptr->messageDoneSignal().connect( G::Slot::slot( *this ,
					&GSmtp::ProtocolMessageForward::messageDone ) ) ;
			}
			m_client_ptr->sendMessage( std::unique_ptr<StoredMessage>(message.release()) ) ;
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

void GSmtp::ProtocolMessageForward::messageDone( const std::string & reason )
{
	G_DEBUG( "GSmtp::ProtocolMessageForward::messageDone: \"" << reason << "\"" ) ;
	const bool ok = reason.empty() ;
	m_done_signal.emit( ok , m_id , ok?"":"forwarding failed" , std::string(reason) ) ; // one-shot
}

void GSmtp::ProtocolMessageForward::clientDone( const std::string & reason )
{
	G_DEBUG( "GSmtp::ProtocolMessageForward::clientDone: \"" << reason << "\"" ) ;
	const bool ok = reason.empty() ;
	m_done_signal.emit( ok , m_id , ok?"":"forwarding failed" , std::string(reason) ) ; // one-shot
}

