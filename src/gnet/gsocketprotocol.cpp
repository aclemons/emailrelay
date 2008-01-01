//
// Copyright (C) 2001-2008 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gsocketprotocol.cpp
//

#include "gdef.h"
#include "glimits.h"
#include "gnet.h"
#include "gssl.h"
#include "gsocketprotocol.h"
#include "gtest.h"
#include "gassert.h"
#include "glog.h"

namespace
{
	const size_t c_buffer_size = G::limits::net_buffer ;
	class LocalSocket : public GNet::StreamSocket // TODO remove access hack
	{
		public: int fd() const { return m_socket ; }
	} ;
}

class GNet::SocketProtocolImp 
{
private:
	enum State { State_raw , State_connecting , State_accepting , State_writing , State_idle } ;

	EventHandler & m_handler ;
	SocketProtocol::Sink & m_sink ;
	StreamSocket & m_socket ;
	std::string m_raw_residue ;
	std::string m_ssl_send_data ;
	bool m_failed ;
	unsigned long m_n ;
	GSsl::Protocol * m_ssl ;
	State m_state ;

public:
	SocketProtocolImp( EventHandler & handler , SocketProtocol::Sink & sink , StreamSocket & socket ) ;
	~SocketProtocolImp() ;
	void readEvent() ;
	bool writeEvent() ;
	bool send( const std::string & data , std::string::size_type offset ) ;
	void sslConnect() ;
	void sslAccept() ;
	bool sslEnabled() const ;

private:
	SocketProtocolImp( const SocketProtocolImp & ) ;
	void operator=( const SocketProtocolImp & ) ;
	static GSsl::Protocol * newProtocol() ;
	static void log( int level , const std::string & line ) ;
	StreamSocket & socket() ;
	bool failed() const ;
	bool rawSendImp( const std::string & , std::string::size_type , std::string & ) ;
	void rawReadEvent() ;
	bool rawWriteEvent() ;
	bool rawSend( const std::string & data , std::string::size_type offset ) ;
	void sslReadImp() ;
	bool sslSendImp() ;
	void sslConnectImp() ;
	void sslAcceptImp() ;
	void logFlowControlReleased() ;
	void logFlowControlAsserted() ;
	void logFlowControlReasserted() ;
} ;

GNet::SocketProtocolImp::SocketProtocolImp( EventHandler & handler , 
	SocketProtocol::Sink & sink , StreamSocket & socket ) :
		m_handler(handler) ,
		m_sink(sink) ,
		m_socket(socket) ,
		m_failed(false) ,
		m_n(0UL) ,
		m_ssl(NULL) ,
		m_state(State_raw)
{
}

GNet::SocketProtocolImp::~SocketProtocolImp()
{
	delete m_ssl ;
}

GNet::StreamSocket & GNet::SocketProtocolImp::socket()
{
	return m_socket ;
}

void GNet::SocketProtocolImp::readEvent()
{
	G_DEBUG( "SocketProtocolImp::readEvent: state=" << m_state ) ;
	if( m_state == State_raw )
		rawReadEvent() ;
	else if( m_state == State_connecting )
		sslConnectImp() ;
	else if( m_state == State_accepting )
		sslAcceptImp() ;
	else if( m_state == State_writing )
		sslSendImp() ;
	else // State_idle
		sslReadImp() ;
}

bool GNet::SocketProtocolImp::writeEvent()
{
	G_DEBUG( "GNet::SocketProtocolImp::writeEvent: state=" << m_state ) ;
	bool rc = true ;
	if( m_state == State_raw )
		rc = rawWriteEvent() ;
	else if( m_state == State_connecting )
		sslConnectImp() ;
	else if( m_state == State_accepting )
		sslAcceptImp() ;
	else
		sslSendImp() ;
	return rc ;
}

bool GNet::SocketProtocolImp::send( const std::string & data , std::string::size_type offset )
{
	if( data.empty() || offset >= data.length() )
		return true ;

	bool rc = true ;
	if( m_state == State_raw )
	{
		rc = rawSend( data , offset ) ;
	}
	else if( m_state == State_connecting || m_state == State_accepting )
	{
		throw SocketProtocol::SendError( "still busy negotiating" ) ;
	}
	else if( m_state == State_writing )
	{
		// throw here rather than add to the pending buffer because openssl 
		// requires that the parameters stay the same -- we could use double
		// buffering, with a buffer switch and a call to sslSendImp() 
		// rather than returning to the idle state, but in practice
		// we rely on the client code taking account of the return value 
		// from send() and waiting for onSendComplete() when required
		//
		throw SocketProtocol::SendError( "still busy sending the last packet" ) ;
	}
	else
	{
		m_state = State_writing ;
		m_ssl_send_data.append( data.substr(offset) ) ;
		rc = sslSendImp() ;
	}
	return rc ;
}

void GNet::SocketProtocolImp::log( int level , const std::string & line )
{
	if( level == 2 )
		G_WARNING( "GNet::SocketProtocolImp::log: " << line ) ;
	else
		G_DEBUG( "ssl: " << line ) ;
}

GSsl::Protocol * GNet::SocketProtocolImp::newProtocol()
{
	GSsl::Library * library = GSsl::Library::instance() ;
	if( library == NULL )
		throw G::Exception( "SocketProtocolImp::newProtocol: internal error: no library instance" ) ;

	return new GSsl::Protocol( *library , log ) ;
}

void GNet::SocketProtocolImp::sslConnect()
{
	G_DEBUG( "SocketProtocolImp::sslConnect" ) ;
	G_ASSERT( m_ssl == NULL ) ;

	m_ssl = newProtocol() ;
	m_state = State_connecting ;
	sslConnectImp() ;
}

void GNet::SocketProtocolImp::sslConnectImp()
{
	G_DEBUG( "SocketProtocolImp::sslConnectImp" ) ;
	G_ASSERT( m_ssl != NULL ) ;
	G_ASSERT( m_state == State_connecting ) ;
	LocalSocket & p = reinterpret_cast<LocalSocket&>(socket()) ;
	GSsl::Protocol::Result rc = m_ssl->connect( p.fd() ) ;
	G_DEBUG( "SocketProtocolImp::sslConnectImp: result=" << GSsl::Protocol::str(rc) ) ;
	if( rc == GSsl::Protocol::Result_error )
	{
		socket().dropWriteHandler() ;
		m_state = State_raw ;
		throw SocketProtocol::ReadError( "ssl connect" ) ;
	}
	else if( rc == GSsl::Protocol::Result_read )
	{
		socket().dropWriteHandler() ;
	}
	else if( rc == GSsl::Protocol::Result_write )
	{
		socket().addWriteHandler( m_handler ) ;
	}
	else
	{
		socket().dropWriteHandler() ;
		m_state = State_idle ;
		G_DEBUG( "SocketProtocolImp::sslConnectImp: calling onSecure" ) ;
		m_sink.onSecure() ;
	}
}

void GNet::SocketProtocolImp::sslAccept()
{
	G_DEBUG( "SocketProtocolImp::sslAccept" ) ;
	G_ASSERT( m_ssl == NULL ) ;
	m_ssl = newProtocol() ;
	m_state = State_accepting ;
	sslAcceptImp() ;
}

void GNet::SocketProtocolImp::sslAcceptImp()
{
	G_DEBUG( "SocketProtocolImp::sslAcceptImp" ) ;
	G_ASSERT( m_ssl != NULL ) ;
	G_ASSERT( m_state == State_accepting ) ;
	LocalSocket & p = reinterpret_cast<LocalSocket&>(socket()) ;
	GSsl::Protocol::Result rc = m_ssl->accept( p.fd() ) ;
	G_DEBUG( "SocketProtocolImp::sslAcceptImp: result=" << GSsl::Protocol::str(rc) ) ;
	if( rc == GSsl::Protocol::Result_error )
	{
		socket().dropWriteHandler() ;
		m_state = State_raw ;
		throw SocketProtocol::ReadError( "ssl accept" ) ;
	}
	else if( rc == GSsl::Protocol::Result_read )
	{
		socket().dropWriteHandler() ;
	}
	else if( rc == GSsl::Protocol::Result_write )
	{
		socket().addWriteHandler( m_handler ) ;
	}
	else
	{
		socket().dropWriteHandler() ;
		m_state = State_idle ;
		G_DEBUG( "SocketProtocolImp::sslAcceptImp: calling onSecure" ) ;
		m_sink.onSecure() ;
	}
}

bool GNet::SocketProtocolImp::sslEnabled() const
{
	return m_state == State_writing || m_state == State_idle ;
}

bool GNet::SocketProtocolImp::sslSendImp()
{
	G_ASSERT( m_state == State_writing ) ;
	bool rc = false ;
	GSsl::Protocol::ssize_type n = 0 ;
	GSsl::Protocol::Result result = m_ssl->write( m_ssl_send_data.data() , m_ssl_send_data.size() , n ) ;
	if( result == GSsl::Protocol::Result_error )
	{
		socket().dropWriteHandler() ;
		m_state = State_idle ;
		throw SocketProtocol::SendError( "ssl write" ) ;
	}
	else if( result == GSsl::Protocol::Result_read )
	{
		socket().dropWriteHandler() ;
	}
	else if( result == GSsl::Protocol::Result_write )
	{
		socket().addWriteHandler( m_handler ) ;
	}
	else
	{
		socket().dropWriteHandler() ;
		rc = n == static_cast<GSsl::Protocol::ssize_type>(m_ssl_send_data.size()) ;
		m_ssl_send_data.erase( 0U , n ) ;
		m_state = State_idle ;
	}
	return rc ;
}

void GNet::SocketProtocolImp::sslReadImp()
{
	G_DEBUG( "SocketProtocolImp::sslReadImp" ) ;
	G_ASSERT( m_state == State_idle ) ;
	G_ASSERT( m_ssl != NULL ) ;
	static char buffer[c_buffer_size] ;
	GSsl::Protocol::ssize_type n = 0 ;
	GSsl::Protocol::Result rc = m_ssl->read( buffer , sizeof(buffer) , n ) ;
	G_DEBUG( "SocketProtocolImp::sslReadImp: result=" << GSsl::Protocol::str(rc) ) ;
	if( rc == GSsl::Protocol::Result_error )
	{
		socket().dropWriteHandler() ;
		m_state = State_idle ;
		throw SocketProtocol::ReadError( "ssl read" ) ;
	}
	else if( rc == GSsl::Protocol::Result_read )
	{
		socket().dropWriteHandler() ;
	}
	else if( rc == GSsl::Protocol::Result_write )
	{
		socket().addWriteHandler( m_handler ) ;
	}
	else
	{
		socket().dropWriteHandler() ;
		m_state = State_idle ;
		G_DEBUG( "SocketProtocolImp::sslReadImp: calling onData(): " << n ) ;
		m_sink.onData( buffer , static_cast<std::string::size_type>(n) ) ;
	}
}

void GNet::SocketProtocolImp::rawReadEvent()
{
	char buffer[c_buffer_size] ;
	buffer[0] = '\0' ;
	const size_t buffer_size = G::Test::enabled("small-client-input-buffer") ? 3 : sizeof(buffer) ;
	ssize_t rc = socket().read( buffer , buffer_size ) ;

	if( rc == 0 || ( rc == -1 && !socket().eWouldBlock() ) )
	{
		throw SocketProtocol::ReadError() ;
	}
	else if( rc != -1 )
	{
		G_ASSERT( static_cast<size_t>(rc) <= buffer_size ) ;
		m_sink.onData( buffer , static_cast<std::string::size_type>(rc) ) ;
	}
	else
	{
		; // -1 && eWouldBlock() -- no-op (esp. for windows)
	}
}

bool GNet::SocketProtocolImp::rawSend( const std::string & data , std::string::size_type offset )
{
	bool all_sent = rawSendImp( data , offset , m_raw_residue ) ;
	if( !all_sent && failed() )
		throw SocketProtocol::SendError() ;
	if( !all_sent )
	{
		socket().addWriteHandler( m_handler ) ;
		logFlowControlAsserted() ;
	}
	return all_sent ;
}

bool GNet::SocketProtocolImp::rawWriteEvent()
{
	socket().dropWriteHandler() ;
	logFlowControlReleased() ;
	bool all_sent = rawSendImp( m_raw_residue , 0 , m_raw_residue ) ;
	if( !all_sent && failed() )
		throw SocketProtocol::SendError() ;
	if( !all_sent )
	{
		socket().addWriteHandler( m_handler ) ;
		logFlowControlReasserted() ;
	}
	return all_sent ;
}

bool GNet::SocketProtocolImp::rawSendImp( const std::string & data , std::string::size_type offset , 
	std::string & residue )
{
	if( data.length() <= offset )
		return true ; // nothing to do

	ssize_t rc = socket().write( data.data()+offset , data.length()-offset ) ;
	if( rc < 0 && ! socket().eWouldBlock() )
	{
		// fatal error, eg. disconnection
		m_failed = true ;
		residue.erase() ;
		return false ; 
	}
	else if( rc < 0 || static_cast<std::string::size_type>(rc) < (data.length()-offset) )
	{
		// flow control asserted
		std::string::size_type sent = rc > 0 ? static_cast<size_t>(rc) : 0U ;
		m_n += sent ;

		residue = data ;
		if( (sent+offset) != 0U )
			residue.erase( 0U , sent+offset ) ;

		return false ;
	}
	else
	{
		// all sent
		m_n += data.length() ;
		residue.erase() ;
		return true ;
	}
}

bool GNet::SocketProtocolImp::failed() const
{
	return m_failed ;
}

void GNet::SocketProtocolImp::logFlowControlAsserted()
{
	const bool log = G::Test::enabled("log-flow-control") ;
	if( log )
		G_LOG( "GNet::SocketProtocolImp::send: @" << socket().asString() << ": flow control asserted" ) ;
}

void GNet::SocketProtocolImp::logFlowControlReleased()
{
	const bool log = G::Test::enabled("log-flow-control") ;
	if( log )
		G_LOG( "GNet::SocketProtocolImp::send: @" << socket().asString() << ": flow control released" ) ;
}

void GNet::SocketProtocolImp::logFlowControlReasserted()
{
	const bool log = G::Test::enabled("log-flow-control") ;
	if( log )
		G_LOG( "GNet::SocketProtocolImp::send: @" << socket().asString() << ": flow control reasserted" ) ;
}

// 

GNet::SocketProtocol::SocketProtocol( EventHandler & handler , Sink & sink , StreamSocket & socket ) :
	m_imp( new SocketProtocolImp(handler,sink,socket) )
{
}

GNet::SocketProtocol::~SocketProtocol()
{
	delete m_imp ;
}

void GNet::SocketProtocol::readEvent()
{
	m_imp->readEvent() ;
}

bool GNet::SocketProtocol::writeEvent()
{
	return m_imp->writeEvent() ;
}

bool GNet::SocketProtocol::send( const std::string & data , std::string::size_type offset )
{
	return m_imp->send( data , offset ) ;
}

bool GNet::SocketProtocol::sslCapable()
{
	return GSsl::Library::instance() != NULL && GSsl::Library::instance()->enabled() ;
}

void GNet::SocketProtocol::sslConnect()
{
	m_imp->sslConnect() ;
}

void GNet::SocketProtocol::sslAccept()
{
	m_imp->sslAccept() ;
}

bool GNet::SocketProtocol::sslEnabled() const
{
	return m_imp->sslEnabled() ;
}

//

GNet::SocketProtocolSink::~SocketProtocolSink()
{
}

/// \file gsocketprotocol.cpp
