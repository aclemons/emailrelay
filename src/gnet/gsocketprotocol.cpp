//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gsocketprotocol.cpp
///

#include "gdef.h"
#include "glimits.h"
#include "gstringview.h"
#include "gcall.h"
#include "gtest.h"
#include "gtimer.h"
#include "gssl.h"
#include "gsocketprotocol.h"
#include "gstr.h"
#include "gtest.h"
#include "gassert.h"
#include "glog.h"
#include <memory>
#include <numeric>

//| \class GNet::SocketProtocolImp
/// A pimple-pattern implementation class used by GNet::SocketProtocol.
///
class GNet::SocketProtocolImp
{
public:
	using Result = GSsl::Protocol::Result ;
	using Segment = G::string_view ;
	using Segments = std::vector<Segment> ;
	struct Position /// A pointer into the scatter/gather payload of GNet::SocketProtocolImp::send().
	{
		Position() = default ;
		Position( std::size_t s , std::size_t o ) : segment(s) , offset(o) {}
		std::size_t segment{0U} ;
		std::size_t offset{0U} ;
	} ;

public:
	SocketProtocolImp( EventHandler & , ExceptionSink , SocketProtocol::Sink & ,
		StreamSocket & , const SocketProtocol::Config & ) ;
	~SocketProtocolImp() ;
	bool readEvent( bool ) ;
	bool writeEvent() ;
	void otherEvent( EventHandler::Reason , bool ) ;
	bool send( G::string_view data , std::size_t offset ) ;
	bool send( const Segments & , std::size_t ) ;
	void shutdown() ;
	void secureConnect() ;
	bool secureConnectCapable() const ;
	void secureAccept() ;
	bool secureAcceptCapable() const ;
	bool secure() const ;
	bool raw() const ;
	std::string peerCertificate() const ;

public:
	SocketProtocolImp( const SocketProtocolImp & ) = delete ;
	SocketProtocolImp( SocketProtocolImp && ) = delete ;
	SocketProtocolImp & operator=( const SocketProtocolImp & ) = delete ;
	SocketProtocolImp & operator=( SocketProtocolImp && ) = delete ;

private:
	enum class State { raw , connecting , accepting , writing , idle , shuttingdown } ;
	static std::unique_ptr<GSsl::Protocol> newProtocol( const std::string & ) ;
	static void log( int level , const std::string & line ) ;
	bool failed() const ;
	bool rawReadEvent( bool ) ;
	bool rawWriteEvent() ;
	bool rawOtherEvent( EventHandler::Reason ) ;
	bool rawSend( const Segments & , Position , bool = false ) ;
	bool rawSendImp( const Segments & , Position , Position & ) ;
	void rawReset() ;
	void sslReadImp() ;
	bool sslSend( const Segments & segments , Position pos ) ;
	bool sslSendImp() ;
	bool sslSendImp( const Segments & segments , Position pos , Position & ) ;
	void secureConnectImp() ;
	void secureAcceptImp() ;
	void shutdownImp() ;
	void logSecure( const std::string & , const std::string & ) const ;
	void onSecureConnectionTimeout() ;
	static std::size_t size( const Segments & ) ;
	static bool finished( const Segments & , Position ) ;
	static Position firstPosition( const Segments & , std::size_t ) ;
	static Position newPosition( const Segments & , Position , std::size_t ) ;
	static G::string_view chunk( const Segments & , Position ) ;
	friend std::ostream & operator<<( std::ostream & , State ) ;

private:
	EventHandler & m_handler ;
	ExceptionSink m_es ;
	SocketProtocol::Sink & m_sink ;
	StreamSocket & m_socket ;
	G::CallStack m_stack ;
	SocketProtocol::Config m_config ;
	Segments m_one_segment ;
	Segments m_segments ;
	Position m_position ;
	std::string m_data_copy ;
	bool m_failed ;
	std::unique_ptr<GSsl::Protocol> m_ssl ;
	State m_state ;
	std::vector<char> m_read_buffer ;
	ssize_t m_read_buffer_n ;
	Timer<SocketProtocolImp> m_secure_connection_timer ;
	std::string m_peer_certificate ;
} ;

namespace GNet
{
	std::ostream & operator<<( std::ostream & stream , SocketProtocolImp::State state )
	{
		return stream << static_cast<int>(state) ;
	}
	std::ostream & operator<<( std::ostream & stream , const SocketProtocolImp::Position & pos )
	{
		return stream << "(" << pos.segment << "," << pos.offset << ")" ;
	}
	std::ostream & operator<<( std::ostream & stream , const SocketProtocolImp::Segments & segments )
	{
		stream << "[" ;
		const char * sep = "" ;
		for( std::size_t i = 0U ; i < segments.size() ; i++ , sep = "," )
			stream << sep << "(" << static_cast<const void*>(segments.at(i).data())
				<< ":" << segments.at(i).size() << ")" ;
		stream << "]" ;
		return stream ;
	}
}

GNet::SocketProtocolImp::SocketProtocolImp( EventHandler & handler , ExceptionSink es ,
	SocketProtocol::Sink & sink , StreamSocket & socket , const SocketProtocol::Config & config ) :
		m_handler(handler) ,
		m_es(es) ,
		m_sink(sink) ,
		m_socket(socket) ,
		m_config(config) ,
		m_one_segment(1U) ,
		m_failed(false) ,
		m_state(State::raw) ,
		m_read_buffer(std::max(std::size_t(1U),config.read_buffer_size)) ,
		m_read_buffer_n(0) ,
		m_secure_connection_timer(*this,&SocketProtocolImp::onSecureConnectionTimeout,es)
{
	if( m_config.server_tls_profile.empty() ) m_config.server_tls_profile = "server" ;
	if( m_config.client_tls_profile.empty() ) m_config.client_tls_profile = "client" ;
}

GNet::SocketProtocolImp::~SocketProtocolImp()
= default;

void GNet::SocketProtocolImp::onSecureConnectionTimeout()
{
	G_DEBUG( "GNet::SocketProtocolImp::onSecureConnectionTimeout: timed out" ) ;
	throw SocketProtocol::SecureConnectionTimeout() ;
}

bool GNet::SocketProtocolImp::readEvent( bool no_throw_on_peer_disconnect )
{
	G_DEBUG( "SocketProtocolImp::readEvent: read event: " << m_socket.asString() << ": "
		<< "state=" << static_cast<int>(m_state) ) ;
	bool all_sent = false ;
	if( m_state == State::raw )
		rawReadEvent( no_throw_on_peer_disconnect ) ;
	else if( m_state == State::connecting )
		secureConnectImp() ;
	else if( m_state == State::accepting )
		secureAcceptImp() ;
	else if( m_state == State::writing )
		all_sent = sslSendImp() ;
	else if( m_state == State::shuttingdown )
		shutdownImp() ;
	else // State::idle
		sslReadImp() ;
	return all_sent ;
}

bool GNet::SocketProtocolImp::writeEvent()
{
	G_DEBUG( "GNet::SocketProtocolImp::writeEvent: write event: " << m_socket.asString() << ": "
		<< "state=" << static_cast<int>(m_state) ) ;
	bool all_sent = false ;
	if( m_state == State::raw )
		all_sent = rawWriteEvent() ;
	else if( m_state == State::connecting )
		secureConnectImp() ;
	else if( m_state == State::accepting )
		secureAcceptImp() ;
	else if( m_state == State::writing )
		all_sent = sslSendImp() ;
	else if( m_state == State::shuttingdown )
		shutdownImp() ;
	else // State::idle
		sslReadImp() ;
	return all_sent ;
}

void GNet::SocketProtocolImp::otherEvent( EventHandler::Reason reason , bool no_throw_on_peer_disconnect )
{
	m_socket.dropReadHandler() ;
	m_socket.dropOtherHandler() ; // since event cannot be cleared

	if( m_state == State::raw )
	{
		bool peer_disconnect = rawOtherEvent( reason ) ;
		if( peer_disconnect && no_throw_on_peer_disconnect )
		{
			m_sink.onPeerDisconnect() ;
			return ;
		}
	}

	if( reason == EventHandler::Reason::closed )
		throw SocketProtocol::Shutdown() ;
	else
		throw SocketProtocol::OtherEventError( EventHandler::str(reason) ) ;
}

std::size_t GNet::SocketProtocolImp::size( const Segments & segments )
{
	return std::accumulate( segments.begin() , segments.end() , std::size_t(0) ,
		[](std::size_t n,G::string_view s){return n+s.size();} ) ;
}

GNet::SocketProtocolImp::Position GNet::SocketProtocolImp::firstPosition( const Segments & s , std::size_t offset )
{
	return newPosition( s , Position() , offset ) ;
}

GNet::SocketProtocolImp::Position GNet::SocketProtocolImp::newPosition( const Segments & s , Position pos ,
	std::size_t offset )
{
	pos.offset += offset ;
	for( ; pos.segment < s.size() && pos.offset >= s[pos.segment].size() ; pos.segment++ )
	{
		pos.offset -= s[pos.segment].size() ;
	}
	return pos ;
}

G::string_view GNet::SocketProtocolImp::chunk( const Segments & s , Position pos )
{
	G_ASSERT( pos.segment < s.size() ) ;
	G_ASSERT( pos.offset < s[pos.segment].size() ) ;
	return s.at(pos.segment).substr( pos.offset ) ;
}

bool GNet::SocketProtocolImp::send( G::string_view data , std::size_t offset )
{
	if( data.empty() || offset >= data.size() )
		return true ;

	bool rc = true ;
	if( m_state == State::raw )
	{
		G_ASSERT( m_one_segment.size() == 1U ) ;
		m_one_segment[0] = data ;
		rc = rawSend( m_one_segment , Position(0U,offset) , true/*copy*/ ) ;
	}
	else if( m_state == State::connecting || m_state == State::accepting )
	{
		throw SocketProtocol::SendError( "still busy negotiating" ) ;
	}
	else if( m_state == State::writing )
	{
		throw SocketProtocol::SendError( "still busy sending the last packet" ) ;
	}
	else if( m_state == State::shuttingdown )
	{
		throw SocketProtocol::SendError( "shuting down" ) ;
	}
	else
	{
		// make a copy here because we have to do retries with the same pointer
		m_data_copy.assign( data.data()+offset , data.size()-offset ) ;
		G_ASSERT( m_one_segment.size() == 1U ) ;
		m_one_segment[0] = G::string_view( m_data_copy.data() , m_data_copy.size() ) ;
		rc = sslSend( m_one_segment , Position() ) ;
	}
	return rc ;
}

bool GNet::SocketProtocolImp::send( const Segments & segments , std::size_t offset )
{
	if( segments.empty() || offset >= size(segments) )
		return true ;

	bool rc = true ;
	if( m_state == State::raw )
	{
		rc = rawSend( segments , firstPosition(segments,offset) ) ;
	}
	else if( m_state == State::connecting || m_state == State::accepting )
	{
		throw SocketProtocol::SendError( "still busy negotiating" ) ;
	}
	else if( m_state == State::writing )
	{
		throw SocketProtocol::SendError( "still busy sending the last packet" ) ;
	}
	else if( m_state == State::shuttingdown )
	{
		throw SocketProtocol::SendError( "shutting down" ) ;
	}
	else
	{
		rc = sslSend( segments , firstPosition(segments,offset) ) ;
	}
	return rc ;
}

void GNet::SocketProtocolImp::shutdown()
{
	if( m_state == State::raw )
	{
		m_socket.dropWriteHandler() ;
		m_socket.shutdown() ;
	}
	else if( m_state == State::idle )
	{
		m_state = State::shuttingdown ;
		shutdownImp() ;
	}
}

void GNet::SocketProtocolImp::shutdownImp()
{
	G_ASSERT( m_ssl != nullptr ) ;
	G_ASSERT( m_state == State::shuttingdown ) ;
	Result rc = m_ssl->shutdown() ;
	if( rc == Result::ok )
	{
		m_socket.dropWriteHandler() ;
		m_socket.shutdown() ;
		m_state = State::idle ; // but possibly only half-open
	}
	else if( rc == Result::error )
	{
		m_socket.dropReadHandler() ;
		m_socket.dropWriteHandler() ;
		throw SocketProtocol::ShutdownError() ;
	}
	else if( rc == Result::read )
	{
		m_socket.dropWriteHandler() ;
	}
	else if( rc == Result::write )
	{
		m_socket.addWriteHandler( m_handler , m_es ) ;
	}
}

bool GNet::SocketProtocolImp::secure() const
{
	return
		m_state == State::writing ||
		m_state == State::idle ||
		m_state == State::shuttingdown ;
}

bool GNet::SocketProtocolImp::raw() const
{
	return m_state == State::raw ;
}

bool GNet::SocketProtocolImp::secureConnectCapable() const
{
	return GSsl::Library::enabledAs( m_config.client_tls_profile ) ;
}

void GNet::SocketProtocolImp::secureConnect()
{
	G_DEBUG( "SocketProtocolImp::secureConnect" ) ;
	G_ASSERT( m_state == State::raw ) ;
	G_ASSERT( m_ssl == nullptr ) ;
	if( m_state != State::raw || m_ssl != nullptr )
		throw SocketProtocol::ProtocolError() ;

	rawReset() ;
	m_ssl = newProtocol( m_config.client_tls_profile ) ;
	m_state = State::connecting ;
	if( m_config.secure_connection_timeout != 0U )
		m_secure_connection_timer.startTimer( m_config.secure_connection_timeout ) ;
	secureConnectImp() ;
}

void GNet::SocketProtocolImp::secureConnectImp()
{
	G_DEBUG( "SocketProtocolImp::secureConnectImp" ) ;
	G_ASSERT( m_ssl != nullptr ) ;
	G_ASSERT( m_state == State::connecting ) ;

	Result rc = m_ssl->connect( m_socket ) ;
	G_DEBUG( "SocketProtocolImp::secureConnectImp: result=" << GSsl::Protocol::str(rc) ) ;
	if( rc == Result::error )
	{
		m_socket.dropWriteHandler() ;
		m_state = State::raw ;
		throw SocketProtocol::ReadError( "ssl connect" ) ;
	}
	else if( rc == Result::read )
	{
		m_socket.dropWriteHandler() ;
	}
	else if( rc == Result::write )
	{
		m_socket.addWriteHandler( m_handler , m_es ) ;
	}
	else
	{
		m_socket.dropWriteHandler() ;
		m_state = State::idle ;
		if( m_config.secure_connection_timeout != 0U )
			m_secure_connection_timer.cancelTimer() ;
		m_peer_certificate = m_ssl->peerCertificate() ;
		std::string protocol = m_ssl->protocol() ;
		std::string cipher = m_ssl->cipher() ;
		logSecure( protocol , cipher ) ;
		m_sink.onSecure( m_peer_certificate , protocol , cipher ) ;
	}
}

bool GNet::SocketProtocolImp::secureAcceptCapable() const
{
	return GSsl::Library::enabledAs( m_config.server_tls_profile ) ;
}

void GNet::SocketProtocolImp::secureAccept()
{
	G_DEBUG( "SocketProtocolImp::secureAccept" ) ;
	G_ASSERT( m_state == State::raw ) ;
	G_ASSERT( m_ssl == nullptr ) ;
	if( m_state != State::raw || m_ssl != nullptr )
		throw SocketProtocol::ProtocolError() ;

	rawReset() ;
	m_ssl = newProtocol( m_config.server_tls_profile ) ;
	m_state = State::accepting ;
	secureAcceptImp() ;
}

void GNet::SocketProtocolImp::secureAcceptImp()
{
	G_DEBUG( "SocketProtocolImp::secureAcceptImp" ) ;
	G_ASSERT( m_ssl != nullptr ) ;
	G_ASSERT( m_state == State::accepting ) ;

	Result rc = m_ssl->accept( m_socket ) ;
	G_DEBUG( "SocketProtocolImp::secureAcceptImp: result=" << GSsl::Protocol::str(rc) ) ;
	if( rc == Result::error )
	{
		m_socket.dropWriteHandler() ;
		m_state = State::raw ;
		throw SocketProtocol::ReadError( "ssl accept" ) ;
	}
	else if( rc == Result::read )
	{
		m_socket.dropWriteHandler() ;
	}
	else if( rc == Result::write )
	{
		m_socket.addWriteHandler( m_handler , m_es ) ;
	}
	else
	{
		m_socket.dropWriteHandler() ;
		m_state = State::idle ;
		m_peer_certificate = m_ssl->peerCertificate() ;
		std::string protocol = m_ssl->protocol() ;
		std::string cipher = m_ssl->cipher() ;
		logSecure( protocol , cipher ) ;
		m_sink.onSecure( m_peer_certificate , protocol , cipher ) ;
	}
}

bool GNet::SocketProtocolImp::sslSend( const Segments & segments , Position pos )
{
	if( !finished(m_segments,m_position) )
		throw SocketProtocol::SendError( "still busy sending the last packet" ) ;

	G_ASSERT( m_state == State::idle ) ;
	m_state = State::writing ;

	Position pos_out ;
	bool all_sent = sslSendImp( segments , pos , pos_out ) ;
	if( !all_sent && failed() )
	{
		m_segments.clear() ;
		m_position = Position() ;
		throw SocketProtocol::SendError() ;
	}
	if( all_sent )
	{
		m_segments.clear() ;
		m_position = Position() ;
	}
	else
	{
		m_segments = segments ;
		m_position = pos_out ;
	}
	return all_sent ;
}

bool GNet::SocketProtocolImp::sslSendImp()
{
	return sslSendImp( m_segments , m_position , m_position ) ;
}

bool GNet::SocketProtocolImp::sslSendImp( const Segments & segments , Position pos , Position & pos_out )
{
	while( !finished(segments,pos) )
	{
		ssize_t nsent = 0 ;
		G::string_view c = chunk( segments , pos ) ;
		GSsl::Protocol::Result result = m_ssl->write( c.data() , c.size() , nsent ) ;
		if( result == Result::error )
		{
			m_socket.dropWriteHandler() ;
			m_state = State::idle ;
			m_failed = true ;
			return false ; // failed
		}
		else if( result == Result::read )
		{
			m_socket.dropWriteHandler() ;
			return false ; // not all sent - retry ssl write() on read event
		}
		else if( result == Result::write )
		{
			m_socket.addWriteHandler( m_handler , m_es ) ;
			return false ; // not all sent - retry ssl write() on write event
		}
		else // Result::ok
		{
			// continue to next chunk
			G_ASSERT( nsent >= 0 ) ;
			pos_out = pos = newPosition( segments , pos ,
				nsent >= 0 ? static_cast<std::size_t>(nsent) : std::size_t(0U) ) ;
		}
	}
	m_state = State::idle ;
	return true ; // all sent
}

void GNet::SocketProtocolImp::sslReadImp()
{
	G_DEBUG( "SocketProtocolImp::sslReadImp" ) ;
	G_ASSERT( m_state == State::idle ) ;
	G_ASSERT( m_ssl != nullptr ) ;

	Result rc = Result::more ;
	for( int sanity = 0 ; rc == Result::more && sanity < 100000 ; sanity++ )
	{
		rc = m_ssl->read( &m_read_buffer[0] , m_read_buffer.size() , m_read_buffer_n ) ;
		G_DEBUG( "SocketProtocolImp::sslReadImp: result=" << GSsl::Protocol::str(rc) ) ;
		if( rc == Result::error )
		{
			m_socket.dropWriteHandler() ;
			m_state = State::idle ;
			throw SocketProtocol::ReadError( "ssl read" ) ;
		}
		else if( rc == Result::read )
		{
			m_socket.dropWriteHandler() ;
		}
		else if( rc == Result::write )
		{
			m_socket.addWriteHandler( m_handler , m_es ) ;
		}
		else // Result::ok, Result::more
		{
			G_ASSERT( rc == Result::ok || rc == Result::more ) ;
			G_ASSERT( m_read_buffer_n >= 0 ) ;
			m_socket.dropWriteHandler() ;
			m_state = State::idle ;
			std::size_t n = static_cast<std::size_t>(m_read_buffer_n) ;
			m_read_buffer_n = 0 ;
			G_DEBUG( "SocketProtocolImp::sslReadImp: calling onData(): " << n ) ;
			if( n != 0U )
			{
				G::CallFrame this_( m_stack ) ;
				m_sink.onData( &m_read_buffer[0] , n ) ;
				if( this_.deleted() ) break ;
			}
		}
		if( rc == Result::more )
		{
			G_DEBUG( "SocketProtocolImp::sslReadImp: more available to read" ) ;
		}
	}
}

bool GNet::SocketProtocolImp::rawOtherEvent( EventHandler::Reason reason )
{
	// got a windows socket shutdown indication, connection failure, etc.
	if( reason == EventHandler::Reason::closed )
	{
		// no read events will follow but there might be data to read, so try reading in a loop
		G_DEBUG( "GNet::SocketProtocolImp::rawOtherEvent: shutdown: clearing receive queue" ) ;
		for(;;)
		{
			const ssize_t rc = m_socket.read( &m_read_buffer[0] , m_read_buffer.size() ) ;
			G_DEBUG( "GNet::SocketProtocolImp::rawOtherEvent: read " << m_socket.asString() << ": " << rc ) ;
			if( rc == 0 )
			{
				break ;
			}
			else if( rc < 0 )
			{
				throw SocketProtocol::ReadError( m_socket.reason() ) ;
			}
			G_ASSERT( static_cast<std::size_t>(rc) <= m_read_buffer.size() ) ;
			G::CallFrame this_( m_stack ) ;
			m_sink.onData( &m_read_buffer[0] , static_cast<std::size_t>(rc) ) ;
			if( this_.deleted() ) break ;
		}
		return true ;
	}
	else
	{
		return false ;
	}
}

bool GNet::SocketProtocolImp::rawReadEvent( bool no_throw_on_peer_disconnect )
{
	const ssize_t rc = m_socket.read( &m_read_buffer[0] , m_read_buffer.size() ) ;
	if( rc == 0 && no_throw_on_peer_disconnect )
	{
		m_socket.dropReadHandler() ;
		m_sink.onPeerDisconnect() ;
		return true ;
	}
	else if( rc == 0 || ( rc == -1 && !m_socket.eWouldBlock() ) )
	{
		throw SocketProtocol::ReadError( rc == 0 ? std::string() : m_socket.reason() ) ;
	}
	else if( rc != -1 )
	{
		G_ASSERT( static_cast<std::size_t>(rc) <= m_read_buffer.size() ) ;
		m_sink.onData( &m_read_buffer[0] , static_cast<std::size_t>(rc) ) ;
	}
	else
	{
		G_DEBUG( "GNet::SocketProtocolImp::rawReadEvent: read event read nothing" ) ;
		; // -1 && eWouldBlock() -- no-op (esp. for windows)
	}
	return false ;
}

bool GNet::SocketProtocolImp::rawSend( const Segments & segments , Position pos , bool do_copy )
{
	G_ASSERT( !do_copy || segments.size() == 1U ) ; // copy => one segment

	if( !finished(m_segments,m_position) )
		throw SocketProtocol::SendError( "still busy sending the last packet" ) ;

	Position pos_out ;
	bool all_sent = rawSendImp( segments , pos , pos_out ) ;
	if( !all_sent && failed() )
	{
		m_segments.clear() ;
		m_position = Position() ;
		m_data_copy.clear() ;
		throw SocketProtocol::SendError( m_socket.reason() ) ;
	}
	else if( all_sent )
	{
		m_segments.clear() ;
		m_position = Position() ;
		m_data_copy.clear() ;
	}
	else if( do_copy )
	{
		// keep the residue in m_segments/m_position/m_data_copy
		G_ASSERT( segments.size() == 1U ) ; // precondition
		G_ASSERT( pos_out.offset < segments[0].size() ) ; // since not all sent
		m_segments = segments ;
		m_data_copy.assign( segments[0].data()+pos_out.offset , segments[0].size()-pos_out.offset ) ;
		m_segments[0] = G::string_view( m_data_copy.data() , m_data_copy.size() ) ;
		m_position = Position() ;

		m_socket.addWriteHandler( m_handler , m_es ) ;
	}
	else
	{
		// record the new write position
		m_segments = segments ;
		m_data_copy.clear() ;
		m_position = pos_out ;

		m_socket.addWriteHandler( m_handler , m_es ) ;
	}
	return all_sent ;
}

bool GNet::SocketProtocolImp::rawWriteEvent()
{
	m_socket.dropWriteHandler() ;
	bool all_sent = rawSendImp( m_segments , m_position , m_position ) ;
	if( !all_sent && failed() )
	{
		m_segments.clear() ;
		m_position = Position() ;
		m_data_copy.clear() ;
		throw SocketProtocol::SendError() ;
	}
	if( all_sent )
	{
		m_segments.clear() ;
		m_position = Position() ;
		m_data_copy.clear() ;
	}
	else
	{
		m_socket.addWriteHandler( m_handler , m_es ) ;
	}
	return all_sent ;
}

bool GNet::SocketProtocolImp::rawSendImp( const Segments & segments , Position pos , Position & pos_out )
{
	while( !finished(segments,pos) )
	{
		G::string_view c = chunk( segments , pos ) ;
		ssize_t rc = m_socket.write( c.data() , c.size() ) ;
		if( rc < 0 && ! m_socket.eWouldBlock() )
		{
			// fatal error, eg. disconnection
			pos_out = Position() ;
			m_failed = true ;
			return false ; // failed()
		}
		else if( rc < 0 || static_cast<std::size_t>(rc) < c.size() )
		{
			// flow control asserted -- return the position where we stopped
			std::size_t nsent = rc > 0 ? static_cast<std::size_t>(rc) : 0U ;
			pos_out = newPosition( segments , pos , nsent ) ;
			G_ASSERT( !finished(segments,pos_out) ) ;
			return false ; // not all sent
		}
		else
		{
			pos = newPosition( segments , pos , static_cast<std::size_t>(rc) ) ;
		}
	}
	return true ; // all sent
}

void GNet::SocketProtocolImp::rawReset()
{
	m_segments.clear() ;
	m_position = Position() ;
	m_data_copy.clear() ;
	m_socket.dropWriteHandler() ;
}

std::unique_ptr<GSsl::Protocol> GNet::SocketProtocolImp::newProtocol( const std::string & profile_name )
{
	GSsl::Library * library = GSsl::Library::instance() ;
	if( library == nullptr )
		throw G::Exception( "SocketProtocolImp::newProtocol: no tls library available" ) ;

	return std::make_unique<GSsl::Protocol>( library->profile(profile_name) ) ;
}

bool GNet::SocketProtocolImp::finished( const Segments & segments , Position pos )
{
	G_ASSERT( pos.segment <= segments.size() ) ;
	return pos.segment == segments.size() ;
}

bool GNet::SocketProtocolImp::failed() const
{
	return m_failed ;
}

std::string GNet::SocketProtocolImp::peerCertificate() const
{
	return m_peer_certificate ;
}

#ifndef G_LIB_SMALL
void GNet::SocketProtocolImp::log( int level , const std::string & log_line )
{
	if( level == 1 )
		G_DEBUG( "GNet::SocketProtocolImp::log: tls: " << log_line ) ;
	else if( level == 2 )
		G_LOG( "GNet::SocketProtocolImp::log: tls: " << log_line ) ;
	else
		G_WARNING( "GNet::SocketProtocolImp::log: tls: " << log_line ) ;
}
#endif

void GNet::SocketProtocolImp::logSecure( const std::string & protocol , const std::string & cipher ) const
{
	G_LOG( "GNet::SocketProtocolImp: tls protocol established with "
		<< m_socket.getPeerAddress().second.displayString()
		<< (protocol.empty()?"":" protocol ") << protocol
		<< (cipher.empty()?"":" cipher ") << G::Str::printable(cipher) ) ;
}

//

GNet::SocketProtocol::SocketProtocol( EventHandler & handler , ExceptionSink es ,
	Sink & sink , StreamSocket & socket , const Config & config ) :
		m_imp(std::make_unique<SocketProtocolImp>(handler,es,sink,socket,config))
{
}

GNet::SocketProtocol::~SocketProtocol()
= default;

bool GNet::SocketProtocol::readEvent( bool no_throw_on_peer_disconnect )
{
	return m_imp->readEvent( no_throw_on_peer_disconnect ) ;
}

bool GNet::SocketProtocol::writeEvent()
{
	return m_imp->writeEvent() ;
}

void GNet::SocketProtocol::otherEvent( EventHandler::Reason reason , bool no_throw_on_peer_disconnect )
{
	m_imp->otherEvent( reason , no_throw_on_peer_disconnect ) ;
}

bool GNet::SocketProtocol::send( const std::string & data , std::size_t offset )
{
	return m_imp->send( G::string_view(data.data(),data.size()) , offset ) ;
}

bool GNet::SocketProtocol::send( G::string_view data )
{
	return m_imp->send( data , 0U ) ;
}

bool GNet::SocketProtocol::send( const std::vector<G::string_view> & data , std::size_t offset )
{
	return m_imp->send( data , offset ) ;
}

void GNet::SocketProtocol::shutdown()
{
	m_imp->shutdown() ;
}

bool GNet::SocketProtocol::secureConnectCapable() const
{
	return m_imp->secureConnectCapable() ;
}

void GNet::SocketProtocol::secureConnect()
{
	m_imp->secureConnect() ;
}

bool GNet::SocketProtocol::secureAcceptCapable() const
{
	return m_imp->secureAcceptCapable() ;
}

void GNet::SocketProtocol::secureAccept()
{
	m_imp->secureAccept() ;
}

#ifndef G_LIB_SMALL
bool GNet::SocketProtocol::secure() const
{
	return m_imp->secure() ;
}
#endif

#ifndef G_LIB_SMALL
bool GNet::SocketProtocol::raw() const
{
	return m_imp->raw() ;
}
#endif

std::string GNet::SocketProtocol::peerCertificate() const
{
	return m_imp->peerCertificate() ;
}

