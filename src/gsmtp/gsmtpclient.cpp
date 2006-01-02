//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gresolve.h"
#include "gassert.h"
#include "glog.h"

//static
std::string GSmtp::Client::crlf()
{
	return std::string("\015\012") ;
}

GSmtp::Client::Client( MessageStore & store , const Secrets & secrets , Config config , bool quit_on_disconnect ) :
	GNet::Client(config.local_address,false,quit_on_disconnect) ,
	m_store(&store) ,
	m_storedfile_preprocessor(config.storedfile_preprocessor) ,
	m_buffer(crlf()) ,
	m_protocol(*this,secrets,config.client_protocol_config) ,
	m_socket(NULL) ,
	m_connect_timer(*this) ,
	m_busy(true) ,
	m_force_message_fail(false)
{
	m_protocol.doneSignal().connect( G::slot(*this,&Client::protocolDone) ) ;
	m_protocol.preprocessorSignal().connect( G::slot(*this,&Client::preprocessorStart) ) ;
	m_storedfile_preprocessor.doneSignal().connect( G::slot(*this,&Client::preprocessorDone) ) ;
}

GSmtp::Client::Client( std::auto_ptr<StoredMessage> message , const Secrets & secrets , Config config ) :
		GNet::Client(config.local_address,false,false) ,
		m_store(NULL) ,
		m_storedfile_preprocessor(config.storedfile_preprocessor) ,
		m_message(message) ,
		m_buffer(crlf()) ,
		m_protocol(*this,secrets,config.client_protocol_config) ,
		m_socket(NULL) ,
		m_connect_timer(*this) ,
		m_busy(true) ,
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

void GSmtp::Client::reset()
{
	// (not used, not tested...)
	m_protocol.doneSignal().disconnect() ;
	m_protocol.preprocessorSignal().disconnect() ;
	m_storedfile_preprocessor.doneSignal().disconnect() ;
	m_connect_timer.cancelTimer() ;
	if( m_socket != NULL )
		disconnect() ;
}

std::string GSmtp::Client::startSending( const std::string & s , unsigned int connection_timeout )
{
	size_t pos = s.rfind(':') ;
	if( pos == std::string::npos )
		return "invalid address string: no colon (<host/ip>:<service/port>)" ;

	return init( s.substr(0U,pos) , s.substr(pos+1U) , connection_timeout ) ;
}

std::string GSmtp::Client::init( const std::string & host , const std::string & service , 
	unsigned int connection_timeout )
{
	m_host = host ;

	std::string result ;
	bool empty = m_store != NULL && m_store->empty() ;
	if( empty )
	{
		result = none() ;
	}
	else
	{
		raiseEventSignal( "connecting" , host ) ;

		if( connection_timeout != 0U )
			m_connect_timer.startTimer( connection_timeout ) ;

		bool ok = connect( host , service , &result ) ;
		if( !ok )
		{
			result = result.empty() ? std::string("error") : result ; // just in case
			raiseEventSignal( "failed" , result ) ;
		}
	}
	if( !result.empty() )
	{
		m_busy = false ;
	}
	return result ;
}

//static
std::string GSmtp::Client::none()
{
	return "no messages to send" ;
}

//static
bool GSmtp::Client::nothingToSend( const std::string & reason )
{
	return reason == none() ;
}

bool GSmtp::Client::busy() const
{
	return m_busy ; // (was GNet::Client::connected())
}

bool GSmtp::Client::protocolSend( const std::string & line , size_t offset )
{
	G_ASSERT( line.length() > offset ) ;
	size_t n = line.length() - offset ;
	ssize_t rc = socket().write( line.data() + offset , n ) ;
	if( rc < 0 )
	{
		m_pending = line.substr(offset) ;
		if( socket().eWouldBlock() )
			blocked() ;
		return false ;
	}
	else if( static_cast<size_t>(rc) < n )
	{
		size_t urc = static_cast<size_t>(rc) ;
		m_pending = line.substr(urc+offset) ;
		blocked() ; // GNet::Client::blocked() => addWriteHandler()
		return false ;
	}
	else
	{
		return true ;
	}
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

void GSmtp::Client::onConnect( GNet::Socket & socket )
{
	m_connect_timer.cancelTimer() ;

	raiseEventSignal( "connected" , 
		socket.getPeerAddress().second.displayString() ) ;

	m_socket = &socket ;
	if( m_store != NULL )
	{
		m_iter = m_store->iterator(true) ;
		if( !sendNext() )
			finish() ;
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
	while( m_buffer.more() ) 
		m_buffer.discard() ;

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
	raiseEventSignal( "sending" , message.name() ) ;

	std::string server_name = peerName() ; // (from GNet::Client)
	if( server_name.empty() )
		server_name = m_host ;

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
		finish( error_message ) ;
	}
}

void GSmtp::Client::messageDestroy()
{
	if( m_message.get() != NULL )
		m_message.get()->destroy() ;
	m_message <<= 0 ;
}

void GSmtp::Client::messageFail( const std::string & reason )
{
	if( m_message.get() != NULL )
		m_message.get()->fail( reason ) ;
	m_message <<= 0 ;
}

void GSmtp::Client::onDisconnect()
{
	std::string reason = "connection to server lost" ;
	if( m_force_message_fail )
		messageFail( reason ) ;

	finish( reason , false ) ;
}

void GSmtp::Client::onTimeout( GNet::Timer & timer )
{
	G_ASSERT( &timer == &m_connect_timer ) ;
	if( &timer == &m_connect_timer )
	{
		G_DEBUG( "GSmtp::Client::onTimeout: connection timeout" ) ;
		std::string reason = "connection timeout" ;
		if( m_force_message_fail )
			messageFail( reason ) ;
		finish( reason ) ;
	}
}

GNet::Socket & GSmtp::Client::socket()
{
	if( m_socket == NULL )
		throw NotConnected() ;
	return * m_socket ;
}

void GSmtp::Client::onData( const char * data , size_t size )
{
	for( m_buffer.add(data,size) ; m_buffer.more() ; m_buffer.discard() )
	{
		bool done = m_protocol.apply( m_buffer.current() ) ;
		if( done )
			break ; // if the protocol is done don't apply() any more
	}
}

void GSmtp::Client::onError( const std::string & error )
{
	G_LOG( "GSmtp::Client: smtp client error: \"" << error << "\"" ) ; // was warning

	std::string reason = "error connecting to server: " ;
	reason += error ;
	if( m_force_message_fail )
		messageFail( "connection failure" ) ;

	finish( reason , false ) ;
}

void GSmtp::Client::finish( const std::string & reason , bool do_disconnect )
{
	if( do_disconnect )
		disconnect() ; // GNet::Client::disconnect()
	m_socket = NULL ;

	raiseDoneSignal( reason ) ;
}

void GSmtp::Client::raiseDoneSignal( const std::string & reason )
{
	if( m_busy )
	{
		m_event_signal.emit( "done" , reason ) ;
		m_busy = false ;
		m_done_signal.emit( reason ) ;
	}
}

void GSmtp::Client::raiseEventSignal( const std::string & s1 , const std::string & s2 )
{
	if( m_busy )
		m_event_signal.emit( s1 , s2 ) ;
}

void GSmtp::Client::onWriteable()
{
	G_DEBUG( "GSmtp::Client::onWriteable" ) ;
	if( protocolSend(m_pending,0U) )
	{
		m_protocol.sendDone() ;
	}
}

G::Signal1<std::string> & GSmtp::Client::doneSignal()
{
	return m_done_signal ;
}

G::Signal2<std::string,std::string> & GSmtp::Client::eventSignal()
{
	return m_event_signal ;
}

// ==

GSmtp::Client::Config::Config( G::Executable exe , GNet::Address address , ClientProtocol::Config protocol_config ) :
	storedfile_preprocessor(exe) ,
	local_address(address) ,
	client_protocol_config(protocol_config)
{
}

