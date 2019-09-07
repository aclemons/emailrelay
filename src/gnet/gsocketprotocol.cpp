//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gcall.h"
#include "gtest.h"
#include "gtimer.h"
#include "gssl.h"
#include "gsocketprotocol.h"
#include "gstr.h"
#include "gtest.h"
#include "gassert.h"
#include "glog.h"

/// \class GNet::SocketProtocolImp
/// A pimple-pattern implementation class used by GNet::SocketProtocol.
///
class GNet::SocketProtocolImp
{
public:
	typedef GSsl::Protocol::Result Result ;
	typedef std::pair<const char *,size_t> Segment ;
	typedef std::vector<Segment> Segments ;
	struct Position /// A pointer into the scatter/gather payload of GNet::SocketProtocolImp::send().
	{
		size_t segment ;
		size_t offset ;
		Position( size_t segment_ , size_t offset_ ) : segment(segment_) , offset(offset_) {}
		Position() : segment(0U) , offset(0U) {}
	} ;

	SocketProtocolImp( EventHandler & , ExceptionSink , SocketProtocol::Sink & ,
		StreamSocket & , unsigned int secure_connection_timeout ) ;
	~SocketProtocolImp() ;
	void readEvent() ;
	bool writeEvent() ;
	void otherEvent( EventHandler::Reason ) ;
	bool send( const std::string & data , size_t offset ) ;
	bool send( const Segments & ) ;
	void shutdown() ;
	void secureConnect() ;
	void secureAccept() ;
	bool secure() const ;
	std::string peerCertificate() const ;
	static void setReadBufferSize( size_t ) ;

private:
	g__enum(State) { raw , connecting , accepting , writing , idle , shuttingdown } ; g__enum_end(State)
	SocketProtocolImp( const SocketProtocolImp & ) g__eq_delete ;
	void operator=( const SocketProtocolImp & ) g__eq_delete ;
	static unique_ptr<GSsl::Protocol> newProtocol( const std::string & ) ;
	static void log( int level , const std::string & line ) ;
	bool failed() const ;
	void rawReadEvent() ;
	bool rawWriteEvent() ;
	void rawOtherEvent() ;
	bool rawSend( const Segments & , Position , bool = false ) ;
	bool rawSendImp( const Segments & , Position , Position & ) ;
	void sslReadImp() ;
	bool sslSend( const Segments & segments , Position pos ) ;
	bool sslSendImp() ;
	bool sslSendImp( const Segments & segments , Position pos , Position & ) ;
	void secureConnectImp() ;
	void secureAcceptImp() ;
	void shutdownImp() ;
	void logSecure( const std::string & ) const ;
	void onSecureConnectionTimeout() ;
	static size_t size( const Segments & ) ;
	static bool finished( const Segments & , Position ) ;
	static Position newPosition( const Segments & , Position , size_t ) ;
	static const char * chunk_p( const Segments & , Position ) ;
	static size_t chunk_n( const Segments & , Position ) ;
	#ifdef GCONFIG_HAVE_CXX_ENUM_CLASS
	friend std::ostream & operator<<( std::ostream & , State ) ;
	#endif

private:
	EventHandler & m_handler ;
	ExceptionSink m_es ;
	SocketProtocol::Sink & m_sink ;
	StreamSocket & m_socket ;
	G::CallStack m_stack ;
	unsigned int m_secure_connection_timeout ;
	Segments m_one_segment ;
	Segments m_segments ;
	Position m_position ;
	std::string m_data_copy ;
	bool m_failed ;
	unique_ptr<GSsl::Protocol> m_ssl ;
	State m_state ;
	std::vector<char> m_read_buffer ;
	ssize_t m_read_buffer_n ;
	Timer<SocketProtocolImp> m_secure_connection_timer ;
	std::string m_peer_certificate ;
	static size_t m_read_buffer_size ;
} ;

namespace GNet
{
	#ifdef GCONFIG_HAVE_CXX_ENUM_CLASS
	std::ostream & operator<<( std::ostream & stream , SocketProtocolImp::State state )
	{
		return stream << static_cast<int>(state) ;
	}
	#endif
	std::ostream & operator<<( std::ostream & stream , const SocketProtocolImp::Position & pos )
	{
		return stream << "(" << pos.segment << "," << pos.offset << ")" ;
	}
	std::ostream & operator<<( std::ostream & stream , const SocketProtocolImp::Segment & segment )
	{
		return stream << "(" << (const void*)(segment.first) << ":" << segment.second << ")" ;
	}
	std::ostream & operator<<( std::ostream & stream , const SocketProtocolImp::Segments & segments )
	{
		stream << "[" ;
		const char * sep = "" ;
		for( size_t i = 0U ; i < segments.size() ; i++ , sep = "," )
			stream << sep << segments.at(i) ;
		stream << "]" ;
		return stream ;
	}
}

size_t GNet::SocketProtocolImp::m_read_buffer_size = G::limits::net_buffer ;

GNet::SocketProtocolImp::SocketProtocolImp( EventHandler & handler , ExceptionSink es ,
	SocketProtocol::Sink & sink , StreamSocket & socket , unsigned int secure_connection_timeout ) :
		m_handler(handler) ,
		m_es(es) ,
		m_sink(sink) ,
		m_socket(socket) ,
		m_secure_connection_timeout(secure_connection_timeout) ,
		m_one_segment(1U) ,
		m_failed(false) ,
		m_state(State::raw) ,
		m_read_buffer(m_read_buffer_size) ,
		m_read_buffer_n(0) ,
		m_secure_connection_timer(*this,&SocketProtocolImp::onSecureConnectionTimeout,es)
{
	G_ASSERT( m_read_buffer.size() == m_read_buffer_size ) ;
}

GNet::SocketProtocolImp::~SocketProtocolImp()
{
}

void GNet::SocketProtocolImp::setReadBufferSize( size_t n )
{
	m_read_buffer_size = n ;
	if( m_read_buffer_size == 0U )
		m_read_buffer_size = 1U ;
}

void GNet::SocketProtocolImp::onSecureConnectionTimeout()
{
	G_DEBUG( "GNet::SocketProtocolImp::onSecureConnectionTimeout: timed out" ) ;
	throw SocketProtocol::SecureConnectionTimeout() ;
}

void GNet::SocketProtocolImp::readEvent()
{
	G_DEBUG( "SocketProtocolImp::readEvent: read event: " << m_socket.asString() << ": state=" << m_state ) ;
	if( m_state == State::raw )
		rawReadEvent() ;
	else if( m_state == State::connecting )
		secureConnectImp() ;
	else if( m_state == State::accepting )
		secureAcceptImp() ;
	else if( m_state == State::writing )
		sslSendImp() ;
	else if( m_state == State::shuttingdown )
		shutdownImp() ;
	else // State::idle
		sslReadImp() ;
}

bool GNet::SocketProtocolImp::writeEvent()
{
	G_DEBUG( "GNet::SocketProtocolImp::writeEvent: write event: " << m_socket.asString() << ": state=" << m_state ) ;
	bool rc = false ; // was true
	if( m_state == State::raw )
		rc = rawWriteEvent() ;
	else if( m_state == State::connecting )
		secureConnectImp() ;
	else if( m_state == State::accepting )
		secureAcceptImp() ;
	else if( m_state == State::writing )
		rc = sslSendImp() ;
	else if( m_state == State::shuttingdown )
		shutdownImp() ;
	else // State::idle
		sslReadImp() ;
	return rc ;
}

void GNet::SocketProtocolImp::otherEvent( EventHandler::Reason reason )
{
	if( m_state == State::raw && reason == EventHandler::Reason::closed && !G::Test::enabled("socket-protocol-noflush") )
		rawOtherEvent() ;
	else
		throw G::Exception( "socket disconnect event" , EventHandler::str(reason) ) ;
}

size_t GNet::SocketProtocolImp::size( const Segments & segments )
{
	size_t n = 0U ;
	for( Segments::const_iterator p = segments.begin() ; p != segments.end() ; ++p )
	{
		G_ASSERT( (*p).first != nullptr ) ;
		G_ASSERT( (*p).second != 0U ) ; // for chunk_p()
		n += (*p).second ;
	}
	return n ;
}

GNet::SocketProtocolImp::Position GNet::SocketProtocolImp::newPosition( const Segments & s , Position pos , size_t offset )
{
	G_ASSERT( pos.segment < s.size() ) ;
	G_ASSERT( (pos.offset+offset) <= s.at(pos.segment).second ) ; // because chunk_p()
	pos.offset += offset ;
	if( pos.offset >= s.at(pos.segment).second ) // in practice if==
	{
		pos.segment++ ;
		pos.offset = 0U ;
	}
	return pos ;
}

const char * GNet::SocketProtocolImp::chunk_p( const Segments & s , Position pos )
{
	G_ASSERT( pos.segment < s.size() ) ;
	G_ASSERT( pos.offset < s[pos.segment].second ) ;

	const Segment & segment = s.at( pos.segment ) ;
	return segment.first + pos.offset ;
}

size_t GNet::SocketProtocolImp::chunk_n( const Segments & s , Position pos )
{
	G_ASSERT( pos.segment < s.size() ) ;
	G_ASSERT( pos.offset < s[pos.segment].second ) ;

	const Segment & segment = s.at( pos.segment ) ;
	return segment.second - pos.offset ;
}

bool GNet::SocketProtocolImp::send( const std::string & data , size_t offset )
{
	if( data.empty() || offset >= data.length() )
		return true ;

	bool rc = true ;
	if( m_state == State::raw )
	{
		G_ASSERT( m_one_segment.size() == 1U ) ;
		m_one_segment[0].first = data.data() ;
		m_one_segment[0].second = data.size() ;
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
		// make a copy here because we have to do retries with the
		// same pointer -- prefer the Segments overload
		m_data_copy = data.substr( offset ) ;

		G_ASSERT( m_one_segment.size() == 1U ) ;
		m_one_segment[0].first = m_data_copy.data() ;
		m_one_segment[0].second = m_data_copy.size() ;
		rc = sslSend( m_one_segment , Position() ) ;
	}
	return rc ;
}

bool GNet::SocketProtocolImp::send( const Segments & segments )
{
	if( segments.empty() || size(segments) == 0U )
		return true ;

	bool rc = true ;
	if( m_state == State::raw )
	{
		rc = rawSend( segments , Position() ) ;
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
		rc = sslSend( segments , Position() ) ;
	}
	return rc ;
}

void GNet::SocketProtocolImp::shutdown()
{
	if( m_state == State::idle )
	{
		m_state = State::shuttingdown ;
		shutdownImp() ;
	}
}

void GNet::SocketProtocolImp::shutdownImp()
{
	G_ASSERT( m_ssl.get() != nullptr ) ;
	G_ASSERT( m_state == State::shuttingdown ) ;
	Result rc = m_ssl->shutdown() ;
	if( rc == Result::ok )
	{
		m_socket.dropWriteHandler() ;
		m_socket.shutdown() ;
	}
	else if( rc == Result::error )
	{
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
	return m_state == State::writing || m_state == State::idle ;
}

void GNet::SocketProtocolImp::secureConnect()
{
	G_DEBUG( "SocketProtocolImp::secureConnect" ) ;
	G_ASSERT( m_ssl.get() == nullptr ) ;

	m_ssl.reset( newProtocol("client").release() ) ;
	m_state = State::connecting ;
	if( m_secure_connection_timeout != 0U )
		m_secure_connection_timer.startTimer( m_secure_connection_timeout ) ;
	secureConnectImp() ;
}

void GNet::SocketProtocolImp::secureConnectImp()
{
	G_DEBUG( "SocketProtocolImp::secureConnectImp" ) ;
	G_ASSERT( m_ssl.get() != nullptr ) ;
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
		if( m_secure_connection_timeout != 0U )
			m_secure_connection_timer.cancelTimer() ;
		m_peer_certificate = m_ssl->peerCertificate() ;
		logSecure( m_ssl->cipher() ) ;
		m_sink.onSecure( m_peer_certificate , m_ssl->cipher() ) ;
	}
}

void GNet::SocketProtocolImp::secureAccept()
{
	G_DEBUG( "SocketProtocolImp::secureAccept" ) ;
	G_ASSERT( m_ssl.get() == nullptr ) ;

	m_ssl.reset( newProtocol("server").release() ) ;
	m_state = State::accepting ;
	secureAcceptImp() ;
}

void GNet::SocketProtocolImp::secureAcceptImp()
{
	G_DEBUG( "SocketProtocolImp::secureAcceptImp" ) ;
	G_ASSERT( m_ssl.get() != nullptr ) ;
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
		logSecure( m_ssl->cipher() ) ;
		m_sink.onSecure( m_peer_certificate , m_ssl->cipher() ) ;
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
		const char * chunk_data = chunk_p( segments , pos ) ;
		size_t chunk_size = chunk_n( segments , pos ) ;

		ssize_t nsent = 0 ;
		GSsl::Protocol::Result result = m_ssl->write( chunk_data , chunk_size , nsent ) ;
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
			pos_out = pos = newPosition( segments , pos , nsent >= 0 ? static_cast<size_t>(nsent) : size_t(0U) ) ;
		}
	}
	m_state = State::idle ;
	return true ; // all sent
}

void GNet::SocketProtocolImp::sslReadImp()
{
	G_DEBUG( "SocketProtocolImp::sslReadImp" ) ;
	G_ASSERT( m_state == State::idle ) ;
	G_ASSERT( m_ssl.get() != nullptr ) ;

	Result rc = Result::more ;
	for( int sanity = 0 ; rc == Result::more && sanity < 1000 ; sanity++ )
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
			size_t n = static_cast<size_t>(m_read_buffer_n) ;
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

void GNet::SocketProtocolImp::rawOtherEvent()
{
	// got a clean socket shutdown indication on windows --  no read events will
	// follow but there might be data to read -- so try reading in a loop
	G_DEBUG( "GNet::SocketProtocolImp::rawOtherEvent: clearing receive queue" ) ;
	for(;;)
	{
		const ssize_t rc = m_socket.read( &m_read_buffer[0] , m_read_buffer.size() ) ;
		G_DEBUG( "GNet::SocketProtocolImp::rawOtherEvent: read " << m_socket.asString() << ": " << rc ) ;
		if( rc <= 0 )
		{
			m_socket.shutdown() ;
			throw SocketProtocol::ReadError( m_socket.reason() ) ;
		}
		G_ASSERT( static_cast<size_t>(rc) <= m_read_buffer.size() ) ;
		G::CallFrame this_( m_stack ) ;
		m_sink.onData( &m_read_buffer[0] , static_cast<size_t>(rc) ) ;
		if( this_.deleted() ) break ;
	}
}

void GNet::SocketProtocolImp::rawReadEvent()
{
	const ssize_t rc = m_socket.read( &m_read_buffer[0] , m_read_buffer.size() ) ;
	if( rc == 0 || ( rc == -1 && !m_socket.eWouldBlock() ) )
	{
		throw SocketProtocol::ReadError( m_socket.reason() ) ;
	}
	else if( rc != -1 )
	{
		G_ASSERT( static_cast<size_t>(rc) <= m_read_buffer.size() ) ;
		m_sink.onData( &m_read_buffer[0] , static_cast<size_t>(rc) ) ;
	}
	else
	{
		G_DEBUG( "GNet::SocketProtocolImp::rawReadEvent: read event read nothing" ) ;
		; // -1 && eWouldBlock() -- no-op (esp. for windows)
	}
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
		throw SocketProtocol::SendError() ;
	}
	if( all_sent )
	{
		m_segments.clear() ;
		m_position = Position() ;
		m_data_copy.clear() ;
	}
	else if( do_copy )
	{
		// keep the residue in m_segments/m_position/m_data_copy
		G_ASSERT( segments.size() == 1U ) ; // precondition
		G_ASSERT( pos_out.offset < segments[0].second ) ; // since not all sent
		m_segments = segments ;
		m_data_copy.assign( segments[0].first+pos_out.offset , segments[0].second-pos_out.offset ) ;
		m_segments[0].first = m_data_copy.data() ;
		m_segments[0].second = m_data_copy.size() ;
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
		const char * chunk_data = chunk_p( segments , pos ) ;
		size_t chunk_size = chunk_n( segments , pos ) ;

		ssize_t rc = m_socket.write( chunk_data , chunk_size ) ;
		if( rc < 0 && ! m_socket.eWouldBlock() )
		{
			// fatal error, eg. disconnection
			pos_out = Position() ;
			m_failed = true ;
			return false ; // failed()
		}
		else if( rc < 0 || static_cast<size_t>(rc) < chunk_size )
		{
			// flow control asserted -- return the position where we stopped
			size_t nsent = rc > 0 ? static_cast<size_t>(rc) : 0U ;
			pos_out = newPosition( segments , pos , nsent ) ;
			G_ASSERT( !finished(segments,pos_out) ) ;
			return false ; // not all sent
		}
		else
		{
			pos = newPosition( segments , pos , static_cast<size_t>(rc) ) ;
		}
	}
	return true ; // all sent
}

unique_ptr<GSsl::Protocol> GNet::SocketProtocolImp::newProtocol( const std::string & profile_name )
{
	GSsl::Library * library = GSsl::Library::instance() ;
	if( library == nullptr )
		throw G::Exception( "SocketProtocolImp::newProtocol: no tls library available" ) ;

	return unique_ptr<GSsl::Protocol>( new GSsl::Protocol( library->profile(profile_name) ) ) ;
}

bool GNet::SocketProtocolImp::finished( const Segments & segments , Position pos )
{
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

void GNet::SocketProtocolImp::log( int level , const std::string & log_line )
{
	if( level == 1 )
		G_DEBUG( "GNet::SocketProtocolImp::log: tls: " << log_line ) ;
	else if( level == 2 )
		G_LOG( "GNet::SocketProtocolImp::log: tls: " << log_line ) ;
	else
		G_WARNING( "GNet::SocketProtocolImp::log: tls: " << log_line ) ;
}

void GNet::SocketProtocolImp::logSecure( const std::string & cipher ) const
{
	G_LOG( "GNet::SocketProtocolImp: tls protocol established with "
		<< m_socket.getPeerAddress().second.displayString()
		<< (cipher.empty()?"":" cipher ") << G::Str::printable(cipher) ) ;
}

//

GNet::SocketProtocol::SocketProtocol( EventHandler & handler , ExceptionSink es ,
	Sink & sink , StreamSocket & socket , unsigned int secure_connection_timeout ) :
		m_imp( new SocketProtocolImp(handler,es,sink,socket,secure_connection_timeout) )
{
}

GNet::SocketProtocol::~SocketProtocol()
{
}

void GNet::SocketProtocol::readEvent()
{
	m_imp->readEvent() ;
}

bool GNet::SocketProtocol::writeEvent()
{
	return m_imp->writeEvent() ;
}

void GNet::SocketProtocol::otherEvent( EventHandler::Reason reason )
{
	m_imp->otherEvent( reason ) ;
}

bool GNet::SocketProtocol::send( const std::string & data , size_t offset )
{
	return m_imp->send( data , offset ) ;
}

bool GNet::SocketProtocol::send( const std::vector<std::pair<const char *,size_t> > & data )
{
	return m_imp->send( data ) ;
}

void GNet::SocketProtocol::shutdown()
{
	m_imp->shutdown() ;
}

bool GNet::SocketProtocol::secureConnectCapable()
{
	return GSsl::Library::enabledAs( "client" ) ;
}

void GNet::SocketProtocol::secureConnect()
{
	m_imp->secureConnect() ;
}

bool GNet::SocketProtocol::secureAcceptCapable()
{
	return GSsl::Library::enabledAs( "server" ) ;
}

void GNet::SocketProtocol::secureAccept()
{
	m_imp->secureAccept() ;
}

std::string GNet::SocketProtocol::peerCertificate() const
{
	return m_imp->peerCertificate() ;
}

void GNet::SocketProtocol::setReadBufferSize( size_t n )
{
	SocketProtocolImp::setReadBufferSize( n ) ;
}

//

GNet::SocketProtocolSink::~SocketProtocolSink()
{
}

