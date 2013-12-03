//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gnet.h"
#include "gsmtp.h"
#include "glocal.h"
#include "gfile.h"
#include "gstr.h"
#include "gmemory.h"
#include "gtimer.h"
#include "gsmtpclient.h"
#include "gresolver.h"
#include "gprocessorfactory.h"
#include "gresolver.h"
#include "gassert.h"
#include "glog.h"

const std::string & GSmtp::Client::crlf()
{
	static const std::string s( "\015\012" ) ;
	return s ;
}

GSmtp::Client::Client( const GNet::ResolverInfo & remote , const GAuth::Secrets & secrets , Config config ) :
	GNet::Client(remote,config.connection_timeout,0U, // the protocol does the response timeout-ing
		config.secure_connection_timeout,crlf(),config.local_address,false) ,
	m_store(NULL) ,
	m_processor(ProcessorFactory::newProcessor(config.processor_address,config.processor_timeout)) ,
	m_protocol(*this,secrets,config.client_protocol_config) ,
	m_secure_tunnel(config.secure_tunnel)
{
	m_protocol.doneSignal().connect( G::slot(*this,&Client::protocolDone) ) ;
	m_protocol.preprocessorSignal().connect( G::slot(*this,&Client::preprocessorStart) ) ;
	m_processor->doneSignal().connect( G::slot(*this,&Client::preprocessorDone) ) ;
}

GSmtp::Client::~Client()
{
	m_protocol.doneSignal().disconnect() ;
	m_protocol.preprocessorSignal().disconnect() ;
	m_processor->doneSignal().disconnect() ;
}

G::Signal1<std::string> & GSmtp::Client::messageDoneSignal()
{
	return m_message_done_signal ;
}

void GSmtp::Client::sendMessagesFrom( MessageStore & store )
{
	G_ASSERT( !store.empty() ) ;
	G_ASSERT( !connected() ) ; // ie. immediately after construction
	m_store = &store ;
}

void GSmtp::Client::sendMessage( std::auto_ptr<StoredMessage> message )
{
	G_ASSERT( m_message.get() == NULL ) ;
	m_message = message ;
	if( connected() )
	{
		start( *m_message.get() ) ;
	}
}

bool GSmtp::Client::protocolSend( const std::string & line , size_t offset , bool go_secure )
{
	bool rc = send( line , offset ) ; // BufferedClient::send()
	if( go_secure )
		sslConnect() ;
	return rc ;
}

void GSmtp::Client::preprocessorStart()
{
	G_ASSERT( m_message.get() != NULL ) ;
	if( m_message.get() )
		m_processor->start( m_message->location() ) ;
}

void GSmtp::Client::preprocessorDone( bool ok )
{
	G_ASSERT( m_message.get() != NULL ) ;

	// (different cancelled/repoll semantics on the client-side)
	bool ignore_this = !ok && m_processor->cancelled() && !m_processor->repoll() ;
	bool break_after = !ok && m_processor->cancelled() && m_processor->repoll() ;

	if( ok || break_after )
		m_message->sync() ; // re-read it after the preprocessing

	if( break_after )
	{
		G_DEBUG( "GSmtp::Client::preprocessorDone: making this the last message" ) ;
		m_iter.last() ; // so next next() returns nothing
	}

	// pass the event on to the protocol
	m_protocol.preprocessorDone( ok || break_after ,
		ok || ignore_this || break_after ? std::string() : m_processor->text() ) ;
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
			G::Strings lines ;
			G::Str::splitIntoFields( certificate , lines , "\n" ) ;
			for( G::Strings::iterator p = lines.begin() ; p != lines.end() ; ++p )
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
		sslConnect() ;
	}
	else
	{
		doOnConnect() ;
	}
}

void GSmtp::Client::doOnConnect()
{
	if( m_store != NULL )
	{
		m_iter = m_store->iterator(true) ;
		if( !sendNext() )
		{
			G_DEBUG( "GSmtp::Client::onConnect: deleting" ) ;
			doDelete( std::string() ) ;
		}
	}
	else
	{
		G_ASSERT( m_message.get() != NULL ) ;
		start( *m_message.get() ) ;
	}
}

bool GSmtp::Client::sendNext()
{
	m_message <<= 0 ;

	// fetch the next message from the store, or return false if none
	{
		std::auto_ptr<StoredMessage> message( m_iter.next() ) ;
		if( message.get() == NULL )
		{
			G_LOG_S( "GSmtp::Client: no more messages to send" ) ;
			return false ;
		}
		m_message = message ;
	}

	start( *m_message.get() ) ;
	return true ;
}

void GSmtp::Client::start( StoredMessage & message )
{
	eventSignal().emit( "sending" , message.name() ) ;

	// prepare the remote server name -- use the dns canonical name if available
	std::string server_name = resolverInfo().name() ;
	if( server_name.empty() )
		server_name = resolverInfo().host() ;

	std::auto_ptr<std::istream> content_stream( message.extractContentStream() ) ;
	m_protocol.start( message.from() , message.to() , message.eightBit() ,
		message.authentication() , server_name , content_stream ) ;
}

void GSmtp::Client::protocolDone( std::string reason , int reason_code )
{
	G_DEBUG( "GSmtp::Client::protocolDone: \"" << reason << "\"" ) ;
	if( ! reason.empty() )
		reason = std::string("smtp client failure: ") + reason ;

	if( reason.empty() )
	{
		if( reason_code != 1 ) // TODO magic number
			messageDestroy() ;
	}
	else
	{
		m_processor->abort() ;
		messageFail( reason , reason_code ) ;
	}

	if( m_store != NULL )
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
	if( m_message.get() != NULL )
	{
		m_message->destroy() ;
		m_message <<= 0 ;
	}
}

void GSmtp::Client::messageFail( const std::string & reason , int reason_code )
{
	if( m_message.get() != NULL )
	{
		m_message->fail( reason , reason_code ) ;
		m_message <<= 0 ;
	}
}

bool GSmtp::Client::onReceive( const std::string & line )
{
	G_DEBUG( "GSmtp::Client::onReceive: [" << G::Str::printable(line) << "]" ) ;
	bool done = m_protocol.apply( line ) ;
	return !done ; // if the protocol is done don't apply() any more
}

void GSmtp::Client::onDelete( const std::string & error , bool )
{
	G_DEBUG( "GSmtp::Client::onDelete: error [" << error << "]" ) ;
	if( ! error.empty() )
	{
		G_LOG( "GSmtp::Client: smtp client error: \"" << error << "\"" ) ; // was warning
		messageFail( error , 0 ) ; // if not already failed or destroyed
	}
	m_message <<= 0 ;
}

void GSmtp::Client::onSendComplete()
{
	m_protocol.sendDone() ;
}

// ==

GSmtp::Client::Config::Config( std::string processor_address_ , unsigned int processor_timeout_ , 
	GNet::Address address , ClientProtocol::Config protocol_config , unsigned int connection_timeout_ ,
	unsigned int secure_connection_timeout_ , bool secure_tunnel_ ) :
		processor_address(processor_address_) ,
		processor_timeout(processor_timeout_) ,
		local_address(address) ,
		client_protocol_config(protocol_config) ,
		connection_timeout(connection_timeout_) ,
		secure_connection_timeout(secure_connection_timeout_) ,
		secure_tunnel(secure_tunnel_)
{
}

/// \file gsmtpclient.cpp
