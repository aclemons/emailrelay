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
// gsmtpclient.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "glocal.h"
#include "gfile.h"
#include "gstr.h"
#include "gtimer.h"
#include "gsmtpclient.h"
#include "gresolver.h"
#include "gfilterfactory.h"
#include "gresolver.h"
#include "gassert.h"
#include "glog.h"

GSmtp::Client::Client( const GNet::Location & remote , const GAuth::Secrets & secrets , Config config ) :
	GNet::Client(remote,config.connection_timeout,0U, // the protocol does the response timeout-ing
		config.secure_connection_timeout,GNet::LineBufferConfig::smtp(),config.bind_local_address,
		config.local_address) ,
	m_store(nullptr) ,
	m_filter(FilterFactory::newFilter(*this,false,config.filter_address,config.filter_timeout)) ,
	m_protocol(*this,*this,secrets,config.client_protocol_config,config.secure_tunnel) ,
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

G::Slot::Signal1<std::string> & GSmtp::Client::messageDoneSignal()
{
	return m_message_done_signal ;
}

void GSmtp::Client::sendMessagesFrom( MessageStore & store )
{
	G_ASSERT( !connected() ) ; // ie. immediately after construction
	m_store = &store ;
}

void GSmtp::Client::sendMessage( unique_ptr<StoredMessage> message )
{
	G_ASSERT( m_message.get() == nullptr ) ;
	m_message.reset( message.release() ) ;
	if( connected() )
	{
		start( *m_message.get() ) ;
	}
}

bool GSmtp::Client::protocolSend( const std::string & line , size_t offset , bool go_secure )
{
	bool rc = send( line , offset ) ; // BufferedClient::send()
	if( go_secure )
		secureConnect() ; // GNet::SocketProtocol
	return rc ;
}

void GSmtp::Client::filterStart()
{
	G_ASSERT( m_message.get() != nullptr ) ;
	if( m_message.get() )
	{
		if( m_filter->id() != "none" )
		{
			G_LOG( "GSmtp::Client::filterStart: client filter: [" << m_filter->id() << "]" ) ;
		}
		m_filter->start( m_message->location() ) ;
	}
}

void GSmtp::Client::filterDone( int filter_result )
{
	G_ASSERT( m_message.get() != nullptr ) ;

	const bool ok = filter_result == 0 ;
	const bool abandon = filter_result == 1 ;
	const bool stop_scanning = m_filter->special() ;
	G_ASSERT( m_filter->reason().empty() == (ok || abandon) ) ;

	if( ok )
	{
		// re-read the envelope after the filtering
		m_message->sync( !m_filter->simple() ) ;
	}

	if( stop_scanning )
	{
		G_DEBUG( "GSmtp::Client::filterDone: making this the last message" ) ;
		m_iter.last() ; // so next next() returns nothing
	}

	if( m_filter->id() != "none" )
	{
		G_LOG( "GSmtp::Client::filterDone: client filter done: " << m_filter->str(false) ) ;
	}

	// pass the event on to the client protocol
	if( ok )
	{
		m_protocol.filterDone( true , std::string() , std::string() ) ;
	}
	else if( abandon )
	{
		m_protocol.filterDone( false , std::string() , std::string() ) ; // protocolDone(-1)
	}
	else
	{
		m_protocol.filterDone( false , m_filter->response() , m_filter->reason() ) ; // protocolDone(-2)
	}
}

void GSmtp::Client::onSecure( const std::string & certificate )
{
	if( m_secure_tunnel )
	{
		doOnConnect() ;
	}
	else
	{
		m_protocol.secure() ;
	}
}

void GSmtp::Client::logCertificate( const std::string & certificate )
{
	if( !certificate.empty() )
	{
		static std::string previous ;
		if( certificate != previous )
		{
			previous = certificate ;
			G::StringArray lines ;
			lines.reserve( 30U ) ;
			G::Str::splitIntoFields( certificate , lines , "\n" ) ;
			for( G::StringArray::iterator p = lines.begin() ; p != lines.end() ; ++p )
			{
				if( !(*p).empty() )
					G_LOG( "GSmtp::Client: certificate: " << (*p) ) ;
			}
		}
	}
}

void GSmtp::Client::onConnect()
{
	if( m_secure_tunnel )
	{
		secureConnect() ; // GNet::SocketProtocol
	}
	else
	{
		doOnConnect() ;
	}
}

void GSmtp::Client::doOnConnect()
{
	G_LOG_S( "GSmtp::Client::doOnConnect: smtp connection to " << peerAddress().second.displayString() ) ;
	if( m_store != nullptr )
	{
		// initialise the message iterator
		m_iter = m_store->iterator( true ) ;

		// start sending the first message
		bool started = sendNext() ;
		if( !started )
		{
			G_DEBUG( "GSmtp::Client::doOnConnect: deleting" ) ;
			doDelete( std::string() ) ;
		}
	}
	else
	{
		G_ASSERT( m_message.get() != nullptr ) ;
		start( *m_message.get() ) ;
	}
}

bool GSmtp::Client::sendNext()
{
	m_message.reset() ;

	// fetch the next message from the store, or return false if none
	{
		unique_ptr<StoredMessage> message( m_iter.next() ) ;
		if( message.get() == nullptr )
		{
			if( m_message_count != 0U )
				G_LOG( "GSmtp::Client: no more messages to send" ) ;
			m_message_count = 0U ;
			return false ;
		}
		m_message.reset( message.release() ) ;
	}

	start( *m_message.get() ) ;
	return true ;
}

void GSmtp::Client::start( StoredMessage & message )
{
	m_message_count++ ;
	eventSignal().emit( "sending" , message.name() ) ;

	m_protocol.start( message.from() , message.to() ,
		message.eightBit() , message.fromAuthOut() ,
		unique_ptr<std::istream>(message.extractContentStream()) ) ;
}

void GSmtp::Client::protocolDone( int response_code , std::string response , std::string reason )
{
	G_DEBUG( "GSmtp::Client::protocolDone: \"" << response << "\"" ) ;
	if( ! response.empty() )
		response = "smtp client failure: " + response ;

	if( response_code == -1 )
	{
		// we called protocol.filterDone(false,""), so abandon
		// this message if eg. already deleted
	}
	else if( response_code == -2 )
	{
		// we called protocol.filterDone(false,"..."), so fail this
		// message (550 => "action not taken, eg. for policy reasons")
		messageFail( 550 , reason.empty() ? response : reason ) ;
	}
	else if( response.empty() )
	{
		// forwarded ok, so delete our copy
		messageDestroy() ;
	}
	else
	{
		// eg. rejected by the server, so fail the message
		m_filter->cancel() ;
		messageFail( response_code , reason.empty() ? response : reason ) ;
	}

	if( m_store != nullptr )
	{
		if( !sendNext() )
		{
			m_protocol.finish() ; // send quit
			socket().shutdown() ;
			finish() ; // GNet::HeapClient
		}
	}
	else
	{
		messageDoneSignal().emit( response ) ;
	}
}

void GSmtp::Client::messageDestroy()
{
	if( m_message.get() != nullptr )
	{
		m_message->destroy() ;
		m_message.reset() ;
	}
}

void GSmtp::Client::messageFail( int response_code , const std::string & reason )
{
	if( m_message.get() != nullptr )
	{
		m_message->fail( reason , response_code ) ;
		m_message.reset() ;
	}
}

bool GSmtp::Client::onReceive( const char * line_data , size_t line_size , size_t )
{
	std::string line( line_data , line_size ) ;
	G_DEBUG( "GSmtp::Client::onReceive: [" << G::Str::printable(line) << "]" ) ;
	bool done = m_protocol.apply( line ) ;
	if( done )
		doDelete( std::string() ) ;
	return !done ; // discard line-buffer input if done
}

void GSmtp::Client::onDelete( const std::string & error )
{
	G_DEBUG( "GSmtp::Client::onDelete: error [" << error << "]" ) ;
	if( ! error.empty() )
	{
		G_LOG( "GSmtp::Client: smtp client error: " << error ) ; // was warning
		messageFail( 0 , error ) ; // if not already failed or destroyed
	}
	m_message.reset() ;
}

void GSmtp::Client::onSendComplete()
{
	m_protocol.sendDone() ;
}

// ==

GSmtp::Client::Config::Config( std::string filter_address_ , unsigned int filter_timeout_ ,
	bool bind_local_address , const GNet::Address & local_address ,
	const ClientProtocol::Config & protocol_config , unsigned int connection_timeout_ ,
	unsigned int secure_connection_timeout_ , bool secure_tunnel_ ) :
		filter_address(filter_address_) ,
		filter_timeout(filter_timeout_) ,
		bind_local_address(bind_local_address) ,
		local_address(local_address) ,
		client_protocol_config(protocol_config) ,
		connection_timeout(connection_timeout_) ,
		secure_connection_timeout(secure_connection_timeout_) ,
		secure_tunnel(secure_tunnel_)
{
}

/// \file gsmtpclient.cpp
