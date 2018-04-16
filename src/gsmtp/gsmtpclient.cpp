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
		config.secure_connection_timeout,std::string("\r\n"),config.bind_local_address,config.local_address) ,
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
	G_ASSERT( !store.empty() ) ;
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
		if( m_filter->id() != "null" )
		{
			G_LOG( "GSmtp::Client::filterStart: client filter: [" << m_filter->id() << "]" ) ;
		}
		m_filter->start( m_message->location() ) ;
	}
}

void GSmtp::Client::filterDone( bool ok )
{
	G_ASSERT( m_message.get() != nullptr ) ;

	// interpret the filter result -- note different semantics wrt. server-side
	bool ignore_this = !ok && m_filter->specialCancelled() ;
	bool break_after = m_filter->specialOther() ;

	if( ok )
	{
		// re-read the envelope after the filtering
		m_message->sync( !m_filter->simple() ) ;
	}

	if( break_after )
	{
		G_DEBUG( "GSmtp::Client::filterDone: making this the last message" ) ;
		m_iter.last() ; // so next next() returns nothing
	}

	if( m_filter->id() != "null" )
	{
		G_LOG( "GSmtp::Client::filterDone: client filter: ok=" << ok << " "
			<< "ignore=" << ignore_this << " break=" << break_after << " text=[" << G::Str::printable(m_filter->text()) << "]" ) ;
	}

	// pass the event on to the protocol
	if( ok )
	{
		m_protocol.filterDone( true , std::string() ) ;
	}
	else if( ignore_this )
	{
		m_protocol.filterDone( false , std::string() ) ; // protocolDone(-1)
	}
	else
	{
		m_protocol.filterDone( false , m_filter->text() ) ; // protocolDone(-2)
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
	if( m_store != nullptr )
	{
		// initialise the message iterator
		m_iter = m_store->iterator( true ) ;

		// start sending the first message
		bool started = sendNext() ;
		if( !started )
		{
			G_DEBUG( "GSmtp::Client::onConnect: deleting" ) ;
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
				G_LOG_S( "GSmtp::Client: no more messages to send" ) ;
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

void GSmtp::Client::protocolDone( std::string reason , int reason_code )
{
	G_DEBUG( "GSmtp::Client::protocolDone: \"" << reason << "\"" ) ;
	if( ! reason.empty() )
		reason = "smtp client failure: " + reason ;

	if( reason_code == -1 )
	{
		// we called protocol.filterDone(false,""), so ignore
		// this message if eg. already deleted
	}
	else if( reason_code == -2 )
	{
		// we called protocol.filterDone(false,"..."), so fail this
		// message (550 => "action not taken, eg. for policy reasons")
		messageFail( reason , 550 ) ;
	}
	else if( reason.empty() )
	{
		// forwarded ok, so delete our copy
		messageDestroy() ;
	}
	else
	{
		// eg. rejected by the server, so fail the message
		m_filter->cancel() ;
		messageFail( reason , reason_code ) ;
	}

	if( m_store != nullptr )
	{
		if( !sendNext() )
		{
			G_DEBUG( "GSmtp::Client::protocolDone: deleting" ) ;
			doDelete( std::string() ) ;
		}
	}
	else
	{
		messageDoneSignal().emit( reason ) ;
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

void GSmtp::Client::messageFail( const std::string & reason , int reason_code )
{
	if( m_message.get() != nullptr )
	{
		m_message->fail( reason , reason_code ) ;
		m_message.reset() ;
	}
}

bool GSmtp::Client::onReceive( const std::string & line )
{
	G_DEBUG( "GSmtp::Client::onReceive: [" << G::Str::printable(line) << "]" ) ;
	bool done = m_protocol.apply( line ) ;
	return !done ; // if the protocol is done don't apply() any more
}

void GSmtp::Client::onDelete( const std::string & error )
{
	G_DEBUG( "GSmtp::Client::onDelete: error [" << error << "]" ) ;
	if( ! error.empty() )
	{
		G_LOG( "GSmtp::Client: smtp client error: \"" << error << "\"" ) ; // was warning
		messageFail( error , 0 ) ; // if not already failed or destroyed
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
