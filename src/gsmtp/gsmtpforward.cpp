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
/// \file gsmtpforward.cpp
///

#include "gdef.h"
#include "gsmtpforward.h"
#include "geventloggingcontext.h"
#include "gcall.h"
#include "glog.h"
#include <algorithm>
#include <sstream>

GSmtp::Forward::Forward( GNet::EventState es , GStore::MessageStore & store ,
	FilterFactoryBase & ff , const GNet::Location & forward_to_default ,
	const GAuth::SaslClientSecrets & secrets , const Config & config ) :
		Forward( es , ff , forward_to_default , secrets , config )
{
	m_store = &store ; // NOLINT
	m_iter = m_store->iterator( /*lock=*/true ) ;
	m_continue_timer.startTimer( 0U ) ;
}

GSmtp::Forward::Forward( GNet::EventState es ,
	FilterFactoryBase & ff , const GNet::Location & forward_to_default ,
	const GAuth::SaslClientSecrets & secrets , const Config & config ) :
		m_es(es) ,
		m_store(nullptr) ,
		m_ff(ff) ,
		m_forward_to_default(forward_to_default) ,
		m_forward_to_location(forward_to_default) ,
		m_secrets(secrets) ,
		m_config(config) ,
		m_error_timer(*this,&Forward::onErrorTimeout,m_es) ,
		m_continue_timer(*this,&Forward::onContinueTimeout,m_es) ,
		m_message_count(0U) ,
		m_has_connected(false) ,
		m_finished(false)
{
	m_client_ptr.eventSignal().connect( G::Slot::slot(*this,&Forward::onEventSignal) ) ;
	m_client_ptr.deleteSignal().connect( G::Slot::slot(*this,&Forward::onDeleteSignal) ) ;
	m_client_ptr.deletedSignal().connect( G::Slot::slot(*this,&Forward::onDeletedSignal) ) ;
}

GSmtp::Forward::~Forward()
{
	if( m_client_ptr.get() )
		m_client_ptr->messageDoneSignal().disconnect() ;
	if( m_routing_filter )
		m_routing_filter->doneSignal().disconnect() ;
	m_client_ptr.deletedSignal().disconnect() ;
	m_client_ptr.deleteSignal().disconnect() ;
	m_client_ptr.eventSignal().disconnect() ;
}

void GSmtp::Forward::onContinueTimeout()
{
	G_ASSERT( m_store != nullptr ) ;
	if( !sendNext() )
	{
		quitAndFinish() ;
		throw GNet::Done() ; // terminates us
	}
}

bool GSmtp::Forward::sendNext()
{
	// start() the next message from the store, or return false if none
	for(;;)
	{
		std::unique_ptr<GStore::StoredMessage> message( ++m_iter ) ;
		if( message == nullptr )
			break ;

		// change the logging context asap to reflect the new message being forwarded
		GNet::EventLoggingContext inner( m_es , Client::eventLoggingString(message.get(),m_config) ) ;

		if( message->toCount() == 0U && m_config.fail_if_no_remote_recipients )
		{
			G_WARNING( "GSmtp::Forward::sendNext: forwarding [" << message->id().str() << "]: failing message with no remote recipients" ) ;
			message->fail( "no remote recipients" , 0 ) ;
		}
		else if( message->toCount() == 0U )
		{
			G_DEBUG( "GSmtp::Forward::sendNext: forwarding [" << message->id().str() << "]: skipping message with no remote recipients" ) ;
		}
		else
		{
			G_LOG( "GSmtp::Forward::sendNext: forwarding [" << message->id().str() << "]" << messageInfo(*message) ) ;
			start( std::move(message) ) ;
			return true ;
		}
	}
	if( m_message_count != 0U )
		G_LOG( "GSmtp::Forward: forwarding: no more messages to send" ) ;
	return false ;
}

void GSmtp::Forward::sendMessage( std::unique_ptr<GStore::StoredMessage> message )
{
	G_LOG( "GSmtp::Forward::sendMessage: forwarding [" << message->id().str() << "]" << messageInfo(*message) ) ;
	start( std::move(message) ) ;
}

void GSmtp::Forward::start( std::unique_ptr<GStore::StoredMessage> message )
{
	m_message_count++ ;
	if( !message->forwardTo().empty() )
	{
		message->close() ;
		m_message = std::move( message ) ;

		m_routing_filter = m_ff.newFilter( m_es , Filter::Type::routing , m_config.filter_config , m_config.filter_spec ) ;
		G_LOG_MORE( "GSmtp::Forward::start: routing-filter [" << m_routing_filter->id() << "]: [" << m_message->id().str() << "]" ) ;
		m_routing_filter->doneSignal().connect( G::Slot::slot(*this,&Forward::routingFilterDone) ) ;
		m_routing_filter->start( m_message->id() ) ;
	}
	else if( updateClient( *message ) )
	{
		m_client_ptr->sendMessage( std::move(message) ) ;
	}
	else
	{
		m_continue_timer.startTimer( 0U ) ;
	}
}

void GSmtp::Forward::routingFilterDone( int filter_result )
{
	G_ASSERT( m_routing_filter.get() != nullptr ) ;
	G_ASSERT( m_message.get() != nullptr ) ;
	G_ASSERT( static_cast<int>(m_routing_filter->result()) == filter_result ) ;
	G_DEBUG( "GSmtp::Forward::routingFilterDone: result=" << filter_result ) ;

	G_LOG_IF( !m_routing_filter->quiet() , "GSmtp::Forward::routingFilterDone: routing-filter "
		"[" << m_routing_filter->id() << "]: [" << m_message->id().str() << "]: "
		<< m_routing_filter->str(Filter::Type::client) ) ;

	const bool ok = filter_result == 0 && m_message ;
	const bool abandon = filter_result == 1 ;
	//const bool fail = filter_result == 2 ;

	std::string reopen_error = ok ? m_message->reopen() : std::string() ;
	bool continue_ = true ;
	if( ok && reopen_error.empty() )
	{
		if( updateClient( *m_message ) )
		{
			continue_ = false ;
			m_client_ptr->sendMessage( std::move(m_message) ) ;
		}
	}
	else if( abandon )
	{
		// no-op
	}
	else
	{
		m_message->fail( "routing filter failed" , 0 ) ;
		m_message.reset() ;
	}

	if( continue_ )
	{
		if( m_store == nullptr )
			m_message_done_signal.emit( { 0 , abandon?"":"routing failed" , false } ) ;
		else
			m_continue_timer.startTimer( 0U ) ;
	}
}

bool GSmtp::Forward::updateClient( const GStore::StoredMessage & message )
{
	bool new_address = m_forward_to_address != message.forwardToAddress() ;
	bool new_selector = m_selector != message.clientAccountSelector() ;
	if( unconnectable( message.forwardToAddress() ) )
	{
		G_LOG( "GSmtp::Forward::updateClient: forwarding [" << message.id().str() << "]: "
			"skipping message with unconnectable address [" << message.forwardToAddress() << "]" ) ;
		return false ;
	}
	else if( m_client_ptr.get() == nullptr )
	{
		G_DEBUG( "GSmtpForward::updateClient: new client [" << message.forwardToAddress() << "][" << message.clientAccountSelector() << "]" ) ;
		newClient( message ) ;
	}
	else if( m_forward_to_address != message.forwardToAddress() ||
		m_selector != message.clientAccountSelector() )
	{
		m_client_ptr->quitAndFinish() ;

		std::ostringstream ss ;
		if( new_address ) ss << "[" << message.forwardToAddress() << "]" ;
		if( new_address && new_selector ) ss << " and " ;
		if( new_selector ) ss << "account selector [" << message.clientAccountSelector() << "]" ;
		G_LOG( "GSmtpForward::updateClient: forwarding [" << message.id().str() << "]: new connection for " << ss.str() ) ;

		newClient( message ) ;
	}
	G_ASSERT( m_client_ptr.get() ) ;
	return true ;
}

void GSmtp::Forward::newClient( const GStore::StoredMessage & message )
{
	m_has_connected = false ;
	m_forward_to_address = message.forwardToAddress() ;
	m_selector = message.clientAccountSelector() ;

	if( m_client_ptr.get() )
		m_client_ptr->messageDoneSignal().disconnect() ;

	if( m_forward_to_address.empty() )
		m_forward_to_location = m_forward_to_default ;
	else
		m_forward_to_location = GNet::Location( m_forward_to_address ) ;

	m_client_ptr.reset( std::make_unique<GSmtp::Client>( m_es.eh(m_client_ptr) ,
		m_ff , m_forward_to_location , m_secrets , m_config ) ) ;

	m_client_ptr->messageDoneSignal().connect( G::Slot::slot(*this,&Forward::onMessageDoneSignal) ) ;
}

void GSmtp::Forward::quitAndFinish()
{
	m_finished = true ;
	if( m_client_ptr.get() )
		m_client_ptr->quitAndFinish() ;
}

void GSmtp::Forward::onEventSignal( const std::string & p1 , const std::string & p2 , const std::string & p3 )
{
	m_event_signal.emit( std::string(p1) , std::string(p2) , std::string(p3) ) ;
}

void GSmtp::Forward::onDeleteSignal( const std::string & )
{
	// save the state of the Client before it goes away
	G_ASSERT( m_client_ptr.get() ) ;
	m_has_connected = m_client_ptr->hasConnected() ;
}

void GSmtp::Forward::onDeletedSignal( const std::string & reason )
{
	G_DEBUG( "GSmtp::Forward::onDeletedSignal: [" << reason << "]" ) ;
	if( m_store && !m_has_connected && !m_forward_to_address.empty() )
	{
		// ignore connection failures to routed addresses -- just go on to the next message
		G_ASSERT( !reason.empty() ) ; // GNet::Done only after connected
		G_WARNING( "GSmtp::Forward::onDeletedSignal: smtp connection failed: " << reason ) ;
		insert( m_unconnectable , m_forward_to_address ) ;
		m_client_ptr.reset() ;
		m_continue_timer.startTimer( 0U ) ;
	}
	else
	{
		// async throw
		m_error = reason ;
		m_error_timer.startTimer( 0U ) ;
	}
}

void GSmtp::Forward::onErrorTimeout()
{
	throw G::Exception( m_error ) ; // terminates us
}

void GSmtp::Forward::onMessageDoneSignal( const Client::MessageDoneInfo & info )
{
	// optimise away repeated DNS queries on the default forward-to address
	G_ASSERT( m_client_ptr.get() ) ;
	if( m_client_ptr.get() && m_client_ptr->hasConnected() && m_forward_to_address.empty() &&
		!m_forward_to_default.resolved() && m_client_ptr->remoteLocation().resolved() )
	{
		m_forward_to_default = m_client_ptr->remoteLocation() ;
	}

	if( m_store )
	{
		if( !sendNext() )
		{
			quitAndFinish() ;
			throw GNet::Done() ; // terminates the client -- m_client_ptr calls onDeletedSignal()
		}
	}
	else
	{
		m_message_done_signal.emit( info ) ;
	}
}

void GSmtp::Forward::doOnDelete( const std::string & reason , bool /*done*/ )
{
	// (our owning ClientPtr is handling an exception by deleting us)
	onDelete( reason ) ;
}

void GSmtp::Forward::onDelete( const std::string & reason )
{
	G_WARNING_IF( !reason.empty() , "GSmtp::Forward::onDelete: smtp client error: " << reason ) ;
	if( m_message ) // if we own the message ie. while filtering
	{
		// fail the message, otherwise the dtor will just unlock it
		G_ASSERT( !reason.empty() ) ; // filters dont throw GNet::Done
		m_message->fail( reason , 0 ) ;
	}
}

std::string GSmtp::Forward::peerAddressString() const
{
	// (used for logging)
	return m_client_ptr.get() ? m_client_ptr->peerAddressString() : std::string() ;
}

bool GSmtp::Forward::finished() const
{
	// (our owning ClientPtr treats exceptions as non-errors after quitAndFinish())
	return m_finished ;
}

bool GSmtp::Forward::unconnectable( const std::string & forward_to ) const
{
	return !forward_to.empty() && contains( m_unconnectable , forward_to ) ;
}

void GSmtp::Forward::insert( G::StringArray & array , const std::string & value )
{
	G_ASSERT( !value.empty() ) ;
	if( !contains( array , value ) )
		array.insert( std::lower_bound(array.begin(),array.end(),value) , value ) ;
}

bool GSmtp::Forward::contains( const G::StringArray & array , const std::string & value )
{
	return !value.empty() && std::binary_search( array.begin() , array.end() , value ) ;
}

std::string GSmtp::Forward::messageInfo( const GStore::StoredMessage & message )
{
	std::ostringstream ss ;
	if( !message.clientAccountSelector().empty() )
		ss << " selector=[" << G::Str::printable(message.clientAccountSelector()) << "]" ;
	if( !message.forwardTo().empty() )
		ss << " forward-to=[" << G::Str::printable(message.forwardTo()) << "]" ;
	if( !message.forwardToAddress().empty() )
		ss << " forward-to-address=[" << G::Str::printable(message.forwardToAddress()) << "]" ;
	return ss.str() ;
}

G::Slot::Signal<const GSmtp::Client::MessageDoneInfo&> & GSmtp::Forward::messageDoneSignal() noexcept
{
	return m_message_done_signal ;
}

G::Slot::Signal<const std::string&,const std::string&,const std::string&> & GSmtp::Forward::eventSignal() noexcept
{
	return m_event_signal ;
}

