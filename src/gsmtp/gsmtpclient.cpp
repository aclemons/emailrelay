//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
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
#include "gassert.h"
#include "glog.h"

//static
std::string GSmtp::Client::crlf()
{
	return std::string("\015\012") ;
}

GSmtp::Client::Client( const GNet::ResolverInfo & remote , MessageStore & store , const Secrets & secrets , 
	Config config ) :
		GNet::Client(remote,config.connection_timeout,0U,crlf(),config.local_address) ,
		m_store(&store) ,
		m_storedfile_preprocessor(config.storedfile_preprocessor) ,
		m_protocol(*this,secrets,config.client_protocol_config) ,
		m_force_message_fail(false)
{
	G_ASSERT( !store.empty() ) ; // (new)

	m_protocol.doneSignal().connect( G::slot(*this,&Client::protocolDone) ) ;
	m_protocol.preprocessorSignal().connect( G::slot(*this,&Client::preprocessorStart) ) ;
	m_storedfile_preprocessor.doneSignal().connect( G::slot(*this,&Client::preprocessorDone) ) ;
}

GSmtp::Client::Client( const GNet::ResolverInfo & remote , std::auto_ptr<StoredMessage> message , 
	const Secrets & secrets , Config config ) :
		GNet::Client(remote,config.connection_timeout,0U,crlf(),config.local_address) ,
		m_store(NULL) ,
		m_storedfile_preprocessor(config.storedfile_preprocessor) ,
		m_message(message) ,
		m_protocol(*this,secrets,config.client_protocol_config) ,
		m_force_message_fail(false)
{
	// The m_force_message_fail member could be set true here to accommodate
	// clients which ignore return codes. If set true the message is
	// failed rather than deleted if the connection is lost during
	// submission.

	m_protocol.doneSignal().connect( G::slot(*this,&Client::protocolDone) ) ;
	m_protocol.preprocessorSignal().connect( G::slot(*this,&Client::preprocessorStart) ) ;
	m_storedfile_preprocessor.doneSignal().connect( G::slot(*this,&Client::preprocessorDone) ) ;
}

GSmtp::Client::~Client()
{
	m_protocol.doneSignal().disconnect() ;
	m_protocol.preprocessorSignal().disconnect() ;
	m_storedfile_preprocessor.doneSignal().disconnect() ;
}

bool GSmtp::Client::protocolSend( const std::string & line , size_t offset )
{
	return send( line , offset ) ; // BufferedClient::send()
}

void GSmtp::Client::preprocessorStart()
{
	G_ASSERT( m_message.get() != NULL ) ;
	if( m_message.get() )
		m_storedfile_preprocessor.start( m_message->location() ) ;
}

void GSmtp::Client::preprocessorDone( bool ok )
{
	G_ASSERT( m_message.get() != NULL ) ;
	if( ok && m_message.get() != NULL )
	{
		m_message->sync() ;
	}
	std::string reason = m_storedfile_preprocessor.text("preprocessing error") ;
	m_protocol.preprocessorDone( ok ? std::string() : reason ) ;
}

void GSmtp::Client::onConnect()
{
	if( m_store != NULL )
	{
		m_iter = m_store->iterator(true) ;
		if( !sendNext() )
			doDelete( std::string() ) ;
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

	// discard the previous message's "." response
	clearInput() ;

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

void GSmtp::Client::protocolDone( bool ok , bool abort , std::string reason )
{
	G_DEBUG( "GSmtp::Client::protocolDone: " << ok << ": \"" << reason << "\"" ) ;

	std::string error_message ;
	if( ok )
	{
		messageDestroy() ;
	}
	else
	{
		m_storedfile_preprocessor.abort() ;
		error_message = std::string("smtp client protocol failure: ") + reason ;
		messageFail( error_message ) ;
	}

	if( m_store == NULL || abort || !sendNext() )
	{
		doDelete( error_message ) ;
	}
}

void GSmtp::Client::messageDestroy()
{
	if( m_message.get() != NULL )
	{
		StoredMessage * message = m_message.release() ;
		message->destroy() ;
	}
}

void GSmtp::Client::messageFail( const std::string & reason )
{
	if( m_message.get() != NULL )
	{
		StoredMessage * message = m_message.release() ;
		message->fail( reason ) ;
	}
}

bool GSmtp::Client::onReceive( const std::string & line )
{
	bool done = m_protocol.apply( line ) ;
	return !done ; // if the protocol is done don't apply() any more
}

void GSmtp::Client::onDelete( const std::string & error , bool )
{
	if( ! error.empty() )
	{
		G_LOG( "GSmtp::Client: smtp client error: \"" << error << "\"" ) ; // was warning
		if( m_force_message_fail )
			messageFail( error ) ;
	}
}

void GSmtp::Client::onSendComplete()
{
	m_protocol.sendDone() ;
}

// ==

GSmtp::Client::Config::Config( G::Executable exe , GNet::Address address , ClientProtocol::Config protocol_config ,
	unsigned int connection_timeout_ ) :
		storedfile_preprocessor(exe) ,
		local_address(address) ,
		client_protocol_config(protocol_config) ,
		connection_timeout(connection_timeout_)
{
}

/// \file gsmtpclient.cpp
