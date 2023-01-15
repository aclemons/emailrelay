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
/// \file gsmtpclient.cpp
///

#include "gdef.h"
#include "glocal.h"
#include "gfile.h"
#include "gstr.h"
#include "gtimer.h"
#include "gnetdone.h"
#include "gsmtpclient.h"
#include "gresolver.h"
#include "gfilterfactorybase.h"
#include "gresolver.h"
#include "gassert.h"
#include "glog.h"
#include <utility>

GSmtp::Client::Client( GNet::ExceptionSink es , GStore::MessageStore & store ,
	FilterFactoryBase & ff , const GNet::Location & remote ,
	const GAuth::SaslClientSecrets & secrets ,
	const Config & config ) :
		GNet::Client(es,remote,netConfig(config)) ,
		m_es(es) ,
		m_ff(ff) ,
		m_config(config) ,
		m_secrets(secrets) ,
		m_store(&store) ,
		m_filter(ff.newFilter(es,Filter::Type::client,config.filter_config,config.filter_spec,{})) ,
		m_routing_filter(ff.newFilter(es,Filter::Type::routing,config.filter_config,config.filter_spec,"routing-filter")) ,
		m_protocol(es,*this,secrets,config.sasl_client_config,config.client_protocol_config,config.secure_tunnel) ,
		m_message_count(0U)
{
	m_iter = m_store->iterator( true ) ;
	m_protocol.doneSignal().connect( G::Slot::slot(*this,&Client::protocolDone) ) ;
	m_protocol.filterSignal().connect( G::Slot::slot(*this,&Client::filterStart) ) ;
	m_filter->doneSignal().connect( G::Slot::slot(*this,&Client::filterDone) ) ;
	m_routing_filter->doneSignal().connect( G::Slot::slot(*this,&Client::routingFilterDone) ) ;
}

GSmtp::Client::Client( GNet::ExceptionSink es ,
	FilterFactoryBase & ff , const GNet::Location & remote ,
	const GAuth::SaslClientSecrets & secrets ,
	const Config & config ) :
		GNet::Client(es,remote,netConfig(config)) ,
		m_es(es) ,
		m_ff(ff) ,
		m_config(config) ,
		m_secrets(secrets) ,
		m_store(nullptr) ,
		m_filter(ff.newFilter(es,Filter::Type::client,config.filter_config,config.filter_spec,{})) ,
		m_routing_filter(ff.newFilter(es,Filter::Type::routing,config.filter_config,config.filter_spec,"routing-filter")) ,
		m_protocol(es,*this,secrets,config.sasl_client_config,config.client_protocol_config,config.secure_tunnel) ,
		m_message_count(0U)
{
	m_protocol.doneSignal().connect( G::Slot::slot(*this,&Client::protocolDone) ) ;
	m_protocol.filterSignal().connect( G::Slot::slot(*this,&Client::filterStart) ) ;
	m_filter->doneSignal().connect( G::Slot::slot(*this,&Client::filterDone) ) ;
	m_routing_filter->doneSignal().connect( G::Slot::slot(*this,&Client::routingFilterDone) ) ;
}

GSmtp::Client::~Client()
{
	m_protocol.doneSignal().disconnect() ;
	m_protocol.filterSignal().disconnect() ;
	m_filter->doneSignal().disconnect() ;
	m_routing_filter->doneSignal().disconnect() ;
}

GNet::Client::Config GSmtp::Client::netConfig( const Config & smtp_config )
{
	return
		GNet::Client::Config()
			.set_stream_socket_config( smtp_config.stream_socket_config )
			.set_line_buffer_config( GNet::LineBufferConfig::smtp() )
			.set_bind_local_address( smtp_config.bind_local_address )
			.set_local_address( smtp_config.local_address )
			.set_connection_timeout( smtp_config.connection_timeout )
			.set_socket_protocol_config(
				GNet::SocketProtocol::Config()
					.set_client_tls_profile( smtp_config.client_tls_profile )
					.set_secure_connection_timeout( smtp_config.secure_connection_timeout ) ) ;
			//.set_response_timeout = 0U ; // the protocol class does this
			//.set_idle_timeout = 0U ; // not needed
}

G::Slot::Signal<const std::string&> & GSmtp::Client::messageDoneSignal()
{
	return m_message_done_signal ;
}

void GSmtp::Client::sendMessage( std::unique_ptr<GStore::StoredMessage> message )
{
	G_ASSERT( message && message->toCount() ) ;
	m_message = std::move( message ) ;
	if( connected() )
		start() ;
}

void GSmtp::Client::sendMessage( std::shared_ptr<GStore::StoredMessage> message )
{
	G_ASSERT( message && message->toCount() ) ;
	m_message = std::move( message ) ;
	if( connected() )
		start() ;
}

void GSmtp::Client::onConnect()
{
	if( m_config.secure_tunnel )
		secureConnect() ; // GNet::SocketProtocol
	else
		startSending() ;
}

void GSmtp::Client::onSecure( const std::string & , const std::string & , const std::string & )
{
	if( m_config.secure_tunnel )
		startSending() ;
	else
		m_protocol.secure() ; // tell the protocol that STARTTLS is done
}

void GSmtp::Client::startSending()
{
	G_LOG_S( "GSmtp::Client::startSending: smtp connection to " << peerAddress().displayString() ) ;
	if( m_store != nullptr && m_message.get() == nullptr )
	{
		// start sending the first message
		bool started = sendNext() ;
		if( !started )
		{
			G_DEBUG( "GSmtp::Client::startSending: nothing to send" ) ;
			quitAndFinish() ;
			throw GNet::Done() ;
		}
	}
	else
	{
		start() ;
	}
}

bool GSmtp::Client::sendNext()
{
	// fetch the next message from the store, or return false if none
	m_message.reset() ;
	for(;;)
	{
		std::unique_ptr<GStore::StoredMessage> message( ++m_iter ) ;
		if( message == nullptr )
		{
			if( m_message_count != 0U )
				G_LOG( "GSmtp::Client: no more messages to send" ) ;
			m_message_count = 0U ;
			return false ;
		}
		else if( message->toCount() == 0U )
		{
			message->fail( "no recipients" , 501 ) ;
		}
		else
		{
			m_message = std::move( message ) ;
			break ;
		}
	}
	start() ;
	return true ;
}

void GSmtp::Client::start()
{
	G_ASSERT( message()->toCount() != 0U ) ;
	m_message_count++ ;

	// basic routing if forward-to is defined in the envelope
	if( m_config.with_routing && !message()->forwardTo().empty() )
	{
		message()->close() ;
		m_routing_filter->start( message()->id() ) ;
		return ;
	}

	G::CallFrame this_( m_stack ) ;
	eventSignal().emit( "sending" , message()->location() , std::string() ) ;
	if( this_.deleted() ) return ;

	m_protocol.start( std::weak_ptr<GStore::StoredMessage>(message()) ) ;
}

std::shared_ptr<GStore::StoredMessage> GSmtp::Client::message()
{
	G_ASSERT( m_message != nullptr ) ;
	return m_message ;
}

bool GSmtp::Client::protocolSend( G::string_view line , std::size_t offset , bool go_secure )
{
	offset = std::min( offset , line.size() ) ;
	G::string_view data( line.data()+offset , line.size()-offset ) ;
	bool rc = data.empty() ? true : send( data ) ;
	if( go_secure )
		secureConnect() ; // GNet::Client -> GNet::SocketProtocol
	return rc ;
}

void GSmtp::Client::filterStart()
{
	if( !m_filter->quiet() )
		G_LOG( "GSmtp::Client::filterStart: client-filter: start [" << m_filter->id() << "]" ) ;
	message()->close() ; // allow external editing
	m_filter->start( message()->id() ) ;
}

void GSmtp::Client::filterDone( int filter_result )
{
	G_ASSERT( static_cast<int>(m_filter->result()) == filter_result ) ;

	const bool ok = filter_result == 0 ;
	const bool abandon = filter_result == 1 ;
	const bool stop_scanning = m_filter->special() ;

	if( stop_scanning )
	{
		G_DEBUG( "GSmtp::Client::filterDone: making this the last message" ) ;
		m_iter.reset() ; // so next iterNext() returns nothing
	}

	std::string reopen_error ;
	if( !m_filter->quiet() )
		G_LOG( "GSmtp::Client::filterDone: client-filter: done: " << m_filter->str(Filter::Type::client) ) ;
	if( ok && !abandon )
		reopen_error = message()->reopen() ;

	// pass the event on to the client protocol
	if( ok && reopen_error.empty() )
	{
		m_protocol.filterDone( Filter::Result::ok , {} , {} ) ;
	}
	else if( abandon )
	{
		m_protocol.filterDone( Filter::Result::abandon , {} , {} ) ; // protocolDone(-1)
	}
	else if( !reopen_error.empty() )
	{
		m_protocol.filterDone( Filter::Result::fail , "failed" , reopen_error ) ; // protocolDone(-2)
	}
	else
	{
		m_protocol.filterDone( Filter::Result::fail , m_filter->response() , m_filter->reason() ) ; // protocolDone(-2)
	}
}

void GSmtp::Client::protocolDone( int response_code , const std::string & response_in ,
	const std::string & reason_in , const G::StringArray & rejectees )
{
	G_DEBUG( "GSmtp::Client::protocolDone: \"" << response_in << "\"" ) ;

	std::string response = response_in.empty() ? std::string() : ( "smtp client failure: " + response_in ) ;
	std::string reason = reason_in.empty() ? response : reason_in ;
	std::string short_reason = ( response_in.empty() || reason_in.empty() ) ? response_in : reason_in ;
	std::string message_location = message()->location() ;

	if( response_code == -1 ) // filter abandon
	{
		// abandon this message if eg. already deleted
		short_reason = "abandoned" ;
	}
	else if( response_code == -2 ) // filter error
	{
		messageFail( 550 , reason ) ;
		short_reason = "rejected" ;
	}
	else if( response.empty() )
	{
		// forwarded ok to all, so delete our copy
		messageDestroy() ;
	}
	else if( rejectees.empty() )
	{
		// eg. rejected by the server, so fail the message
		G_ASSERT( !reason.empty() ) ;
		m_filter->cancel() ;
		messageFail( response_code , reason ) ;
	}
	else
	{
		// some recipients rejected by the server, so update the to-list and fail the message
		m_filter->cancel() ;
		message()->editRecipients( rejectees ) ;
		messageFail( response_code , reason ) ;
	}

	G::CallFrame this_( m_stack ) ;
	eventSignal().emit( "sent" , message_location , short_reason ) ;
	if( this_.deleted() ) return ; // just in case

	if( m_store != nullptr )
	{
		if( !sendNext() )
		{
			G_DEBUG( "GSmtp::Client::protocolDone: all sent" ) ;
			quitAndFinish() ;
			throw GNet::Done() ;
		}
	}
	else
	{
		m_message.reset() ;
		messageDoneSignal().emit( response ) ;
	}
}

void GSmtp::Client::quitAndFinish()
{
	m_protocol.finish() ; // send QUIT
	finish() ; // GNet::Client::finish() -- expect a disconnect
}

void GSmtp::Client::messageDestroy()
{
	message()->destroy() ;
	m_message.reset() ;
}

void GSmtp::Client::messageFail( int response_code , const std::string & reason )
{
	message()->fail( reason , response_code ) ;
	m_message.reset() ;
}

bool GSmtp::Client::onReceive( const char * line_data , std::size_t line_size , std::size_t , std::size_t , char )
{
	std::string line( line_data , line_size ) ;
	G_DEBUG( "GSmtp::Client::onReceive: [" << G::Str::printable(line) << "]" ) ;

	G::CallFrame this_( m_stack ) ;
	bool done = m_protocol.apply( line ) ;
	if( this_.deleted() ) return false ;

	if( done )
	{
		m_message.reset() ;
		quitAndFinish() ;
		throw GNet::Done() ;
	}
	return !done ; // discard line-buffer input if done
}

void GSmtp::Client::onDelete( const std::string & error )
{
	G_DEBUG( "GSmtp::Client::onDelete: error [" << error << "]" ) ;
	if( ! error.empty() )
	{
		G_LOG( "GSmtp::Client: smtp client error: " << error ) ;
		if( m_message )
			messageFail( 0 , error ) ; // if not already failed or destroyed
	}
	m_message.reset() ;
}

void GSmtp::Client::onSendComplete()
{
	m_protocol.sendComplete() ;
}

void GSmtp::Client::routingFilterDone( int filter_result )
{
	G_ASSERT( static_cast<int>(m_routing_filter->result()) == filter_result ) ;
	G_ASSERT( m_config.with_routing ) ;

	const bool ok = filter_result == 0 ;
	const bool abandon = filter_result == 1 ;
	const bool fail = filter_result == 2 ;
	std::string reopen_error = ok ? message()->reopen() : std::string() ;

	bool move_on = false ;
	if( abandon )
	{
		move_on  = true ;
	}
	else if( fail )
	{
		messageFail( 550 , "routing filter failed" ) ;
		move_on = true ;
	}
	else if( !reopen_error.empty() ) // ok but cannot open
	{
		messageFail( 550 , "routing filter error" ) ;
		move_on = true ;
	}

	G_LOG( "GSmtp::Client::routingFilterDone: routing: routing-filter: done: "
		<< m_routing_filter->str(Filter::Type::client)
		<< (reopen_error.empty()?std::string():(": "+reopen_error))
		<< (move_on?": moving on":"") ) ;

	if( move_on )
	{
		G_ASSERT( m_store != nullptr ) ;
		if( !sendNext() )
		{
			quitAndFinish() ;
			throw GNet::Done() ;
		}
	}
	else // ok and reopened
	{
		std::string forward_to_address = message()->forwardToAddress() ;
		G_LOG( "GSmtp::Client::routingFilterDone: routing: forward-to-address from envelope: [" << forward_to_address << "]" ) ;

		G::CallFrame this_( m_stack ) ;
		eventSignal().emit( "sending" , message()->location() , std::string() ) ;
		if( this_.deleted() ) return ;

		if( forward_to_address.empty() )
		{
			// no special routing -- send to the default smarthost
			m_protocol.start( std::weak_ptr<GStore::StoredMessage>(message()) ) ;
		}
		else
		{
			m_routing_client.reset( new Client( m_es , m_ff , GNet::Location(forward_to_address) ,
				m_secrets , Config(m_config).set_with_routing(false) ) ) ;
			m_routing_client->messageDoneSignal().connect( G::Slot::slot( *this , &GSmtp::Client::routedMessageDone ) ) ;
			m_routing_client->sendMessage( message() ) ;
		}
	}
}

void GSmtp::Client::routedMessageDone( const std::string & )
{
	if( !sendNext() )
	{
		quitAndFinish() ;
		throw GNet::Done() ;
	}
}

// ==

GSmtp::Client::Config::Config() :
	local_address(GNet::Address::defaultAddress())
{
}

