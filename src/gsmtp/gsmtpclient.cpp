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
/// \file gsmtpclient.cpp
///

#include "gdef.h"
#include "glocal.h"
#include "gfile.h"
#include "gstr.h"
#include "gtimer.h"
#include "gnetdone.h"
#include "gsmtpclient.h"
#include "gfilterfactorybase.h"
#include "gresolver.h"
#include "gassert.h"
#include "glog.h"
#include <utility>

GSmtp::Client::Client( GNet::EventState es , FilterFactoryBase & ff , const GNet::Location & remote ,
	const GAuth::SaslClientSecrets & secrets , const Config & config ) :
		GNet::Client(es.logging(this),remote,normalise(config.net_client_config)) ,
		GNet::EventLogging(es.logging()) ,
		m_es(es.logging(this)) ,
		m_config(config) ,
		m_nofilter_timer(*this,&Client::onNoFilterTimeout,m_es) ,
		m_filter(ff.newFilter(m_es,Filter::Type::client,config.filter_config,config.filter_spec)) ,
		m_protocol(m_es,*this,secrets,config.sasl_client_config,config.client_protocol_config,config.secure_tunnel)
{
	G_ASSERT( m_filter.get() != nullptr ) ;
	m_protocol.doneSignal().connect( G::Slot::slot(*this,&Client::protocolDone) ) ;
	m_protocol.filterSignal().connect( G::Slot::slot(*this,&Client::filterStart) ) ;
	m_filter->doneSignal().connect( G::Slot::slot(*this,&Client::filterDone) ) ;
}

GSmtp::Client::~Client()
{
	m_filter->doneSignal().disconnect() ;
	m_protocol.filterSignal().disconnect() ;
	m_protocol.doneSignal().disconnect() ;
}

GNet::Client::Config GSmtp::Client::normalise( GNet::Client::Config net_client_config )
{
	return net_client_config.set_line_buffer_config( GNet::LineBuffer::Config::smtp() ) ;
}

G::Slot::Signal<const GSmtp::Client::MessageDoneInfo&> & GSmtp::Client::messageDoneSignal() noexcept
{
	return m_message_done_signal ;
}

void GSmtp::Client::sendMessage( std::unique_ptr<GStore::StoredMessage> message )
{
	G_ASSERT( message.get() != nullptr ) ;
	m_message = std::move( message ) ;
	m_event_logging_string = eventLoggingString( m_message.get() , m_config ) ;
	if( ready() )
		start() ;
}

bool GSmtp::Client::ready() const
{
	return m_config.secure_tunnel ? ( connected() && m_secure ) : connected() ;
}

void GSmtp::Client::onConnect()
{
	if( m_config.client_protocol_config.ehlo.find('.') == std::string::npos )
		m_protocol.reconfigure( localAddress().hostPartString() ) ; // RFC-2821 3.6

	if( m_config.secure_tunnel )
		secureConnect() ; // GNet::SocketProtocol
	else
		start() ;
}

void GSmtp::Client::onSecure( const std::string & , const std::string & , const std::string & )
{
	m_secure = true ;
	if( m_config.secure_tunnel )
		start() ;
	else
		m_protocol.secure() ; // tell the protocol that STARTTLS is done
}

void GSmtp::Client::start()
{
	G_LOG_S( "GSmtp::Client::start: smtp connection to " << peerAddress().displayString() ) ;

	G::CallFrame this_( m_stack ) ;
	eventSignal().emit( "sending" , std::string(message()->id().str()) , std::string() ) ;
	if( this_.deleted() ) return ;

	m_protocol.start( std::weak_ptr<GStore::StoredMessage>(message()) ) ;
}

std::shared_ptr<GStore::StoredMessage> GSmtp::Client::message()
{
	G_ASSERT( m_message != nullptr ) ;
	return m_message ;
}

bool GSmtp::Client::protocolSend( std::string_view line , std::size_t offset , bool go_secure )
{
	offset = std::min( offset , line.size() ) ;
	std::string_view data( line.data()+offset , line.size()-offset ) ;
	bool rc = data.empty() ? true : send( data ) ;
	if( go_secure )
		secureConnect() ; // GNet::Client -> GNet::SocketProtocol
	return rc ;
}

void GSmtp::Client::filterStart()
{
	if( !message()->forwardTo().empty() )
	{
		// no client filter if "ForwardTo" is populated -- see GSmtp::Forward
		m_nofilter_timer.startTimer( 0U ) ;
	}
	else
	{
		G_LOG_MORE( "GSmtp::Client::filterStart: client-filter [" << m_filter->id() << "]: [" << message()->id().str() << "]" ) ;
		message()->close() ; // allow external editing
		m_filter_special = false ;
		m_filter->start( message()->id() ) ;
	}
}

void GSmtp::Client::onNoFilterTimeout()
{
	m_protocol.filterDone( Filter::Result::ok , {} , {} ) ;
}

void GSmtp::Client::filterDone( int filter_result )
{
	G_ASSERT( static_cast<int>(m_filter->result()) == filter_result ) ;

	const bool ok = filter_result == 0 ;
	const bool abandon = filter_result == 1 ;
	m_filter_special = m_filter->special() ;

	G_LOG_IF( !m_filter->quiet() , "GSmtp::Client::filterDone: client-filter "
		"[" << m_filter->id() << "]: [" << message()->id().str() << "]: "
		<< m_filter->str(Filter::Type::client) ) ;

	std::string reopen_error ;
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

void GSmtp::Client::protocolDone( const ClientProtocol::DoneInfo & info )
{
	G_ASSERT( info.response_code >= -2 ) ;
	G_DEBUG( "GSmtp::Client::protocolDone: \"" << info.response << "\"" ) ;

	std::string reason = info.reason.empty() ? info.response : info.reason ;
	std::string short_reason = ( info.response.empty() || info.reason.empty() ) ? info.response : info.reason ;
	std::string message_id = message()->id().str() ;

	if( info.response_code == -1 ) // filter abandon
	{
		// abandon this message if eg. already deleted
		short_reason = "abandoned" ;
	}
	else if( info.response_code == -2 ) // filter error
	{
		messageFail( 550 , reason ) ;
		short_reason = "rejected" ;
	}
	else if( info.response.empty() )
	{
		// forwarded ok to all, so delete our copy
		messageDestroy() ;
	}
	else if( info.rejects.empty() )
	{
		// eg. rejected by the server, so fail the message
		G_ASSERT( !reason.empty() ) ;
		m_filter->cancel() ;
		messageFail( info.response_code , reason ) ;
	}
	else
	{
		// some recipients rejected by the server, so update the to-list and fail the message
		m_filter->cancel() ;
		message()->editRecipients( info.rejects ) ;
		messageFail( info.response_code , reason ) ;
	}

	m_event_logging_string.clear() ;

	G::CallFrame this_( m_stack ) ;
	eventSignal().emit( "sent" , message_id , short_reason ) ;
	if( this_.deleted() ) return ; // just in case

	m_message.reset() ;
	messageDoneSignal().emit( { std::max(0,info.response_code) , info.response , m_filter_special } ) ;
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
	if( !error.empty() )
	{
		if( m_message && hasConnected() )
			messageFail( 0 , error ) ; // if not already failed or destroyed
	}
	m_message.reset() ;
}

void GSmtp::Client::onSendComplete()
{
	m_protocol.sendComplete() ;
}

std::string GSmtp::Client::eventLoggingString( const GStore::StoredMessage * msg , const Config & config )
{
	if( !msg || !config.log_msgid ) return {} ;
	return std::string(1U,'(').append(G::Str::tail(msg->id().str(),"."_sv,false)).append(") ",2U) ;
}

std::string_view GSmtp::Client::eventLoggingString() const noexcept
{
	if( m_event_logging_string.empty() ) return {} ;
	return m_event_logging_string ;
}

