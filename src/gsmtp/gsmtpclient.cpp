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
#include "gfilterfactory.h"
#include "gresolver.h"
#include "gassert.h"
#include "glog.h"

GSmtp::Client::Client( GNet::ExceptionSink es , MessageStore & store ,
	FilterFactory & ff , const GNet::Location & remote ,
	const GAuth::SaslClientSecrets & secrets ,
	const Config & config ) :
		GNet::Client(es,remote,netConfig(config)) ,
		m_store(&store) ,
		m_filter(ff.newFilter(es,false,config.filter_address,config.filter_timeout)) ,
		m_protocol(es,*this,secrets,config.sasl_client_config,config.client_protocol_config,config.secure_tunnel) ,
		m_secure_tunnel(config.secure_tunnel) ,
		m_message_count(0U)
{
	m_protocol.doneSignal().connect( G::Slot::slot(*this,&Client::protocolDone) ) ;
	m_protocol.filterSignal().connect( G::Slot::slot(*this,&Client::filterStart) ) ;
	m_filter->doneSignal().connect( G::Slot::slot(*this,&Client::filterDone) ) ;
}

GSmtp::Client::Client( GNet::ExceptionSink es ,
	FilterFactory & ff , const GNet::Location & remote ,
	const GAuth::SaslClientSecrets & secrets ,
	const Config & config ) :
		GNet::Client(es,remote,netConfig(config)) ,
		m_store(nullptr) ,
		m_filter(ff.newFilter(es,false,config.filter_address,config.filter_timeout)) ,
		m_protocol(es,*this,secrets,config.sasl_client_config,config.client_protocol_config,config.secure_tunnel) ,
		m_secure_tunnel(config.secure_tunnel) ,
		m_message_count(0U)
{
	m_protocol.doneSignal().connect( G::Slot::slot(*this,&Client::protocolDone) ) ;
	m_protocol.filterSignal().connect( G::Slot::slot(*this,&Client::filterStart) ) ;
	m_filter->doneSignal().connect( G::Slot::slot(*this,&Client::filterDone) ) ;
}

GSmtp::Client::~Client()
{
	m_protocol.doneSignal().disconnect() ;
	m_protocol.filterSignal().disconnect() ;
	m_filter->doneSignal().disconnect() ;
}

GNet::Client::Config GSmtp::Client::netConfig( const Config & smtp_config )
{
	GNet::Client::Config net_config ;
	net_config.set_line_buffer_config( GNet::LineBufferConfig::smtp() ) ;
	net_config.bind_local_address = smtp_config.bind_local_address ;
	net_config.local_address = smtp_config.local_address ;
	net_config.connection_timeout = smtp_config.connection_timeout ;
	net_config.socket_protocol_config.secure_connection_timeout = smtp_config.secure_connection_timeout ;
	//net_config.response_timeout = 0U ; // the protocol class does this
	//net_config.idle_timeout = 0U ; // not needed
	return net_config ;
}

G::Slot::Signal<const std::string&> & GSmtp::Client::messageDoneSignal()
{
	return m_message_done_signal ;
}

void GSmtp::Client::sendMessage( std::unique_ptr<StoredMessage> message )
{
	G_ASSERT_OR_DO( message && message->toCount() , return ) ;
	m_message.reset( message.release() ) ;
	if( connected() )
		start() ;
}

void GSmtp::Client::onConnect()
{
	if( m_secure_tunnel )
		secureConnect() ; // GNet::SocketProtocol
	else
		startSending() ;
}

void GSmtp::Client::onSecure( const std::string & , const std::string & , const std::string & )
{
	if( m_secure_tunnel )
		startSending() ;
	else
		m_protocol.secure() ; // tell the protocol that STARTTLS is done
}

void GSmtp::Client::startSending()
{
	G_LOG_S( "GSmtp::Client::startSending: smtp connection to " << peerAddress().displayString() ) ;
	if( m_store != nullptr )
	{
		// initialise the message iterator
		m_iter = m_store->iterator( true ) ;

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
	m_message.reset() ;

	// fetch the next message from the store, or return false if none
	{
		std::unique_ptr<StoredMessage> message( ++m_iter ) ;
		if( message == nullptr )
		{
			if( m_message_count != 0U )
				G_LOG( "GSmtp::Client: no more messages to send" ) ;
			m_message_count = 0U ;
			return false ;
		}
		m_message.reset( message.release() ) ;
	}

	start() ;
	return true ;
}

void GSmtp::Client::start()
{
	G_ASSERT( message()->toCount() != 0U ) ;
	m_message_count++ ;

	G::CallFrame this_( m_stack ) ;
	eventSignal().emit( "sending" , message()->location() , std::string() ) ;
	if( this_.deleted() ) return ;

	m_protocol.start( std::weak_ptr<StoredMessage>(message()) ) ;
}

std::shared_ptr<GSmtp::StoredMessage> GSmtp::Client::message()
{
	G_ASSERT_OR_DO( m_message != nullptr , return (m_message=std::make_shared<StoredMessageStub>()) ) ;
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
	if( !m_filter->simple() )
	{
		G_LOG( "GSmtp::Client::filterStart: client filter: [" << m_filter->id() << "]" ) ;
		message()->close() ; // allow external editing
	}
	m_filter->start( message()->id() ) ;
}

void GSmtp::Client::filterDone( int filter_result )
{
	const bool ok = filter_result == 0 ;
	const bool abandon = filter_result == 1 ;
	const bool stop_scanning = m_filter->special() ;
	G_ASSERT( m_filter->reason().empty() == (ok || abandon) ) ;

	if( stop_scanning )
	{
		G_DEBUG( "GSmtp::Client::filterDone: making this the last message" ) ;
		m_iter.reset() ; // so next iterNext() returns nothing
	}

	std::string reopen_error ;
	if( !m_filter->simple() )
	{
		G_LOG( "GSmtp::Client::filterDone: client filter done: " << m_filter->str(false) ) ;
		if( ok && !abandon )
			reopen_error = message()->reopen() ;
	}

	// pass the event on to the client protocol
	if( ok && reopen_error.empty() )
	{
		m_protocol.filterDone( true , std::string() , std::string() ) ;
	}
	else if( abandon )
	{
		m_protocol.filterDone( false , std::string() , std::string() ) ; // protocolDone(-1)
	}
	else if( !reopen_error.empty() )
	{
		m_protocol.filterDone( false , "failed" , reopen_error ) ; // protocolDone(-2)
	}
	else
	{
		m_protocol.filterDone( false , m_filter->response() , m_filter->reason() ) ; // protocolDone(-2)
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
		message()->edit( rejectees ) ;
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
		messageDoneSignal().emit( response ) ;
	}
}

void GSmtp::Client::quitAndFinish()
{
	m_protocol.finish() ; // send QUIT
	finish( true ) ; // GNet::Client::finish() -- expect a disconnect
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

// ==

GSmtp::Client::Config::Config() :
	local_address(GNet::Address::defaultAddress())
{
}

GSmtp::Client::Config::Config( const std::string & filter_address_ , unsigned int filter_timeout_ ,
	bool bind_local_address_ , const GNet::Address & local_address_ ,
	const ClientProtocol::Config & protocol_config_ , unsigned int connection_timeout_ ,
	unsigned int secure_connection_timeout_ , bool secure_tunnel_ ,
	const std::string & sasl_client_config_ ) :
		filter_address(filter_address_) ,
		filter_timeout(filter_timeout_) ,
		bind_local_address(bind_local_address_) ,
		local_address(local_address_) ,
		client_protocol_config(protocol_config_) ,
		connection_timeout(connection_timeout_) ,
		secure_connection_timeout(secure_connection_timeout_) ,
		secure_tunnel(secure_tunnel_) ,
		sasl_client_config(sasl_client_config_)
{
}

