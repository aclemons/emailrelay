//
// Copyright (C) 2001-2004 Graeme Walker <graeme_walker@users.sourceforge.net>
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

namespace
{
	const bool must_authenticate = true ;
}

//static
std::string GSmtp::Client::crlf()
{
	return std::string("\015\012") ;
}

GSmtp::Client::Client( MessageStore & store , const Secrets & secrets , const GNet::Address & local_address ,
	bool quit_on_disconnect , unsigned int response_timeout ) :
		GNet::Client(local_address,false,quit_on_disconnect) ,
		m_store(&store) ,
		m_buffer(crlf()) ,
		m_protocol(*this,secrets,GNet::Local::fqdn(),response_timeout,must_authenticate) ,
		m_socket(NULL) ,
		m_connect_timer(*this) ,
		m_preprocess_timer(*this) ,
		m_message_index(0U) ,
		m_busy(true) ,
		m_force_message_fail(false)
{
	m_protocol.doneSignal().connect( G::slot(*this,&Client::protocolDone) ) ;
}

GSmtp::Client::Client( std::auto_ptr<StoredMessage> message , const Secrets & secrets ,
	const GNet::Address & local_address , unsigned int response_timeout ) :
		GNet::Client(local_address,false,false) ,
		m_store(NULL) ,
		m_message(message) ,
		m_buffer(crlf()) ,
		m_protocol(*this,secrets,GNet::Local::fqdn(),response_timeout,must_authenticate) ,
		m_socket(NULL) ,
		m_connect_timer(*this) ,
		m_preprocess_timer(*this) ,
		m_message_index(0U) ,
		m_busy(true) ,
		m_force_message_fail(false)
{
	// The m_force_message_fail member could be set true here to accommodate
	// clients which ignore return codes. If set true the message is
	// failed rather than deleted if the connection is lost during
	// submission.

	m_protocol.doneSignal().connect( G::slot(*this,&Client::protocolDone) ) ;
}

GSmtp::Client::~Client()
{
	m_protocol.doneSignal().disconnect() ;
}

std::string GSmtp::Client::startSending( const std::string & s , unsigned int connection_timeout )
{
	size_t pos = s.find(':') ;
	if( pos == std::string::npos )
		return "invalid address string: no colon (<host/ip>:<service/port>)" ;

	return init( s.substr(0U,pos) , s.substr(pos+1U) , connection_timeout ) ;
}

std::string GSmtp::Client::init( const std::string & host , const std::string & service , 
	unsigned int connection_timeout )
{
	m_message_index = 0U ;
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
	m_message_index++ ;
	raiseEventSignal( "sending" , message.name() ) ;

	std::string server_name = peerName() ; // (from GNet::Client)
	if( server_name.empty() )
		server_name = m_host ;

	// synchronous, client-side preprocessing -- treat errors
	// asynchronously with a zero-length timer
	//
	if( ! message.preprocess() )
	{
		G_DEBUG( "GSmtp::Client::start: client-side pre-processing failed" ) ;
		m_preprocess_timer.startTimer( 0U ) ;
		return ;
	}

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
	if( &timer == &m_connect_timer )
	{
		G_DEBUG( "GSmtp::Client::onTimeout: connection timeout" ) ;
		std::string reason = "connection timeout" ;
		if( m_force_message_fail )
			messageFail( reason ) ;
		finish( reason ) ;
	}
	else
	{
		G_DEBUG( "GSmtp::Client::onTimeout: preprocessing failure timeout" ) ;
		G_ASSERT( &timer == &m_preprocess_timer ) ;
		messageFail( "pre-processing failed" ) ; // could do better
		if( m_store == NULL || !sendNext() )
			finish() ;
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
		m_protocol.apply( m_buffer.current() ) ;
		if( m_protocol.done() )
		{
			finish() ;
			return ;
		}
	}
}

void GSmtp::Client::onError( const std::string & error )
{
	G_LOG( "GSmtp::Client: smtp client error: \"" << error << "\"" ) ; // was warning

	std::string reason = "error on connection to server: " ;
	reason += error ;
	if( m_force_message_fail )
		messageFail( "connection failure" ) ;

	finish( reason , false ) ;
}

void GSmtp::Client::finish( const std::string & reason , bool do_disconnect )
{
	raiseDoneSignal( reason ) ;
	if( do_disconnect )
		disconnect() ; // GNet::Client::disconnect()
	m_socket = NULL ;
}

void GSmtp::Client::raiseDoneSignal( const std::string & reason )
{
	if( m_busy )
	{
		m_event_signal.emit( "done" , reason ) ;
		m_done_signal.emit( reason ) ;
		m_busy = false ;
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

