//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gclientprotocol.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gsmtp.h"
#include "glocal.h"
#include "gfile.h"
#include "gsasl.h"
#include "gbase64.h"
#include "gstr.h"
#include "gmemory.h"
#include "gxtext.h"
#include "gclientprotocol.h"
#include "gsocketprotocol.h"
#include "gresolver.h"
#include "glog.h"
#include "gassert.h"

GSmtp::ClientProtocol::ClientProtocol( Sender & sender , const Secrets & secrets , Config config ) :
	m_sender(sender) ,
	m_secrets(secrets) ,
	m_thishost(config.thishost_name) ,
	m_state(sInit) ,
	m_to_accepted(0U) ,
	m_server_has_auth(false) ,
	m_server_has_8bitmime(false) ,
	m_server_has_tls(false) ,
	m_message_is_8bit(false) ,
	m_authenticated_with_server(false) ,
	m_must_authenticate(config.must_authenticate) ,
	m_strict(config.eight_bit_strict) ,
	m_warned(false) ,
	m_response_timeout(config.response_timeout) ,
	m_ready_timeout(config.ready_timeout) ,
	m_preprocessor_timeout(config.preprocessor_timeout) ,
	m_done_signal(true)
{
}

void GSmtp::ClientProtocol::start( const std::string & from , const G::Strings & to , bool eight_bit ,
	std::string authentication , std::string server_name , std::auto_ptr<std::istream> content )
{
	G_DEBUG( "GSmtp::ClientProtocol::start" ) ;

	// reinitialise for the new message & server
	m_to = to ;
	m_to_accepted = 0U ;
	m_from = from ;
	m_content = content ;
	m_message_is_8bit = eight_bit ;
	m_message_authentication = authentication ;
	m_reply = Reply() ;
	m_sasl <<= new SaslClient( m_secrets , server_name ) ;
	m_done_signal.reset() ;

	// (re)start the protocol
	applyEvent( Reply() , true ) ;
}

void GSmtp::ClientProtocol::preprocessorDone( const std::string & reason )
{
	// convert the preprocessor response into a pretend smtp Reply
	applyEvent( reason.empty() ? 
		Reply::ok(Reply::Internal_2xx) : Reply::error(std::string("preprocessing: ")+reason) ) ;
}

void GSmtp::ClientProtocol::secure()
{
	// convert the event into a pretend smtp Reply
	applyEvent( Reply::ok(Reply::Internal_2yy) ) ;
}

void GSmtp::ClientProtocol::sendDone()
{
	if( m_state == sData )
	{
		size_t n = sendLines() ;

		G_LOG( "GSmtp::ClientProtocol: tx>>: [" << n << " line(s) of content]" ) ;
		if( endOfContent() )
		{
			m_state = sSentDot ;
			send(".",true) ;
		}
	}
}

bool GSmtp::ClientProtocol::parseReply( Reply & stored_reply , const std::string & rx , std::string & reason )
{
	Reply this_reply = Reply( rx ) ;
	if( ! this_reply.validFormat() )
	{
		stored_reply = Reply() ;
		reason = "invalid reply format" ;
		return false ;
	}
	else if( stored_reply.validFormat() && stored_reply.incomplete() )
	{
		if( ! stored_reply.add(this_reply) )
		{
			stored_reply = Reply() ;
			reason = "invalid continuation line" ;
			return false ;
		}
	}
	else
	{
		stored_reply = this_reply ;
	}
	return ! stored_reply.incomplete() ;
}

bool GSmtp::ClientProtocol::apply( const std::string & rx )
{
	G_LOG( "GSmtp::ClientProtocol: rx<<: \"" << G::Str::printable(rx) << "\"" ) ;

	std::string reason ;
	bool protocol_done = false ;
	bool complete_reply = parseReply( m_reply , rx , reason ) ;
	if( complete_reply )
	{
		protocol_done = applyEvent( m_reply ) ;
	}
	else 
	{
		if( reason.length() != 0U )
			send( std::string("550 syntax error: ")+reason ) ;
	}
	return protocol_done ;
}

void GSmtp::ClientProtocol::sendEhlo()
{
	send( std::string("EHLO ") + m_thishost ) ;
}

void GSmtp::ClientProtocol::sendHelo()
{
	send( std::string("HELO ") + m_thishost ) ;
}

void GSmtp::ClientProtocol::sendMail()
{
	const bool dodgy = m_message_is_8bit && !m_server_has_8bitmime ;
	if( dodgy && m_strict )
	{
		m_state = sDone ;
		raiseDoneSignal( "cannot send 8-bit message to 7-bit server" , true ) ;
	}
	else
	{
		if( dodgy && !m_warned )
		{
			m_warned = true ;
			G_WARNING( "GSmtp::ClientProtocol::sendMail: sending an eight-bit message "
				"to a server which has not advertised the 8BITMIME extension" ) ;
		}
		sendMailCore() ;
	}
}

void GSmtp::ClientProtocol::sendMailCore()
{
	std::string mail_from = std::string("MAIL FROM:<") + m_from + ">" ;
	if( m_server_has_8bitmime )
	{
		mail_from.append( " BODY=8BITMIME" ) ;
	}
	if( m_authenticated_with_server && !m_message_authentication.empty() )
	{
		mail_from.append( std::string(" AUTH=") + G::Xtext::encode(m_message_authentication) ) ;
	}
	else if( m_authenticated_with_server )
	{
		mail_from.append( " AUTH=<>" ) ;
	}
	send( mail_from ) ;
}

void GSmtp::ClientProtocol::startPreprocessing()
{
	G_ASSERT( m_state == sPreprocessing ) ;
	if( m_preprocessor_timeout != 0U )
		startTimer( m_preprocessor_timeout ) ;
	m_preprocessor_signal.emit() ;
}

bool GSmtp::ClientProtocol::applyEvent( const Reply & reply , bool is_start_event )
{
	G_DEBUG( "GSmtp::ClientProtocol::applyEvent: " << reply.value() << ": " << reply.text() ) ;

	cancelTimer() ;

	bool protocol_done = false ;
	if( m_state == sInit && is_start_event )
	{
		// wait for 220 greeting
		m_state = sStarted ;
		if( m_ready_timeout != 0U )
			startTimer( m_ready_timeout ) ;
	}
	else if( m_state == sServiceReady && is_start_event )
	{
		// already got greeting
		m_state = sSentEhlo ;
		sendEhlo() ;
	}
	else if( m_state == sDone && is_start_event )
	{
		m_state = sPreprocessing ;
		startPreprocessing() ;
	}
	else if( m_state == sInit && reply.is(Reply::ServiceReady_220) )
	{
		G_DEBUG( "GSmtp::ClientProtocol::applyEvent: init -> ready" ) ;
		m_state = sServiceReady ;
	}
	else if( m_state == sStarted && reply.is(Reply::ServiceReady_220) )
	{
		G_DEBUG( "GSmtp::ClientProtocol::applyEvent: start -> sent-ehlo" ) ;
		m_state = sSentEhlo ;
		sendEhlo() ;
	}
	else if( m_state == sSentEhlo && (
		reply.is(Reply::SyntaxError_500) ||
		reply.is(Reply::SyntaxError_501) ||
		reply.is(Reply::NotImplemented_502) ) )
	{
		// it didn't like EHLO so fall back to HELO
		m_state = sSentHelo ;
		sendHelo() ;
	}
	else if( ( m_state == sSentEhlo || m_state == sSentHelo || m_state == sSentTlsEhlo ) && reply.is(Reply::Ok_250) )
	{
		G_ASSERT( m_sasl.get() != NULL ) ;
		G_DEBUG( "GSmtp::ClientProtocol::applyEvent: ehlo reply \""
			<< G::Str::printable(reply.text()) << "\"" ) ;

		m_server_has_auth = serverAuth( reply ) ;
		m_server_has_8bitmime = ( m_state == sSentEhlo || m_state == sSentTlsEhlo ) && reply.textContains("\n8BITMIME");
		m_server_has_tls = m_state == sSentTlsEhlo || ( m_state == sSentEhlo && reply.textContains("\nSTARTTLS") ) ;
		m_auth_mechanism = m_sasl->preferred( serverAuthMechanisms(reply) ) ;

		if( m_server_has_tls && !GNet::SocketProtocol::sslCapable() )
		{
			std::string msg( "GSmtp::ClientProtocol::applyEvent: cannot do tls/ssl required by remote smtp server" );
			if( !m_auth_mechanism.empty() ) msg.append( ": authentication will probably fail" ) ;
			G_WARNING( msg ) ;
		}

		if( m_state == sSentEhlo && m_server_has_tls && GNet::SocketProtocol::sslCapable() )
		{
			m_state = sStartTls ;
			send( "STARTTLS" ) ;
		}
		else if( m_server_has_auth && !m_sasl->active() )
		{
			throw AuthenticationRequired() ;
		}
		else if( m_server_has_auth && m_sasl->active() && m_auth_mechanism.empty() )
		{
			throw NoMechanism() ;
		}
		else if( m_server_has_auth && m_sasl->active() )
		{
			m_state = sAuth1 ;
			send( std::string("AUTH ") + m_auth_mechanism ) ;
		}
		else if( !m_server_has_auth && m_sasl->active() && m_must_authenticate )
		{
			// (this makes sense if we need to propagate messages' authentication credentials)
			throw AuthenticationNotSupported() ;
		}
		else
		{
			m_state = sPreprocessing ;
			startPreprocessing() ;
		}
	}
	else if( m_state == sStartTls && reply.is(Reply::ServiceReady_220) )
	{
		m_sender.protocolSend( std::string() , 0U , true ) ; // go secure
	}
	else if( m_state == sStartTls && reply.is(Reply::NotAvailable_454) )
	{
		throw TlsError( reply.errorText() ) ;
	}
	else if( m_state == sStartTls && reply.is(Reply::Internal_2yy) )
	{
		m_state = sSentTlsEhlo ;
		sendEhlo() ;
	}
	else if( m_state == sAuth1 && reply.is(Reply::Challenge_334) && Base64::valid(reply.text()) )
	{
		bool done = true ;
		bool error = false ;
		std::string rsp = m_sasl->response( m_auth_mechanism , Base64::decode(reply.text()) , done , error ) ;
		if( error )
		{
			m_state = sAuth2 ;
			send( "*" ) ; // ie. cancel authentication
		}
		else
		{
			m_state = done ? sAuth2 : m_state ;
			send( Base64::encode(rsp,std::string()) ) ;
		}
	}
	else if( m_state == sAuth1 && reply.is(Reply::NotAuthenticated_535) )
	{
		if( m_must_authenticate )
			throw AuthenticationError() ;

		m_state = sPreprocessing ;
		startPreprocessing() ; // (continue without sucessful authentication)
	}
	else if( m_state == sAuth2 )
	{
		m_authenticated_with_server = reply.is(Reply::Authenticated_235) ;
		if( !m_authenticated_with_server && m_must_authenticate )
		{
			throw AuthenticationError() ;
		}
		else
		{
			m_state = sPreprocessing ;
			startPreprocessing() ; // (continue with or without sucessful authentication)
		}
	}
	else if( m_state == sPreprocessing && reply.is(Reply::Internal_2xx) )
	{
		m_state = sSentMail ;
		sendMail() ;
	}
	else if( m_state == sPreprocessing )
	{
		m_state = sDone ;
		protocol_done = true ;
		raiseDoneSignal( reply.errorText() ) ;
	}
	else if( m_state == sSentMail && reply.is(Reply::Ok_250) )
	{
		std::string to ;
		if( m_to.size() != 0U ) // should always be non-zero due to message store guarantees
			to = m_to.front() ;
		m_to.pop_front() ;

		m_state = sSentRcpt ;
		send( std::string("RCPT TO:<") + to + std::string(">") ) ;
	}
	else if( m_state == sSentRcpt && m_to.size() != 0U && reply.positive() )
	{
		if( reply.positive() ) m_to_accepted++ ;
		if( !reply.positive() ) G_WARNING( "GSmtp::ClientProtocol: recipient rejected" ) ;

		std::string to = m_to.front() ;
		m_to.pop_front() ;

		send( std::string("RCPT TO:<") + to + std::string(">") ) ;
	}
	else if( m_state == sSentRcpt )
	{
		if( reply.positive() ) m_to_accepted++ ;
		if( !reply.positive() ) G_WARNING( "GSmtp::ClientProtocol: recipient rejected" ) ;

		m_state = m_to_accepted == 0U ? sSentDataStub : sSentData ;
		send( std::string("DATA") ) ;
	}
	else if( m_state == sSentData && reply.is(Reply::OkForData_354) )
	{
		m_state = sData ;

		size_t n = sendLines() ;

		const bool log_content = false ; // set true here and in sendLine() if debugging
		if( !log_content )
			G_LOG( "GSmtp::ClientProtocol: tx>>: [" << n << " line(s) of content]" ) ;

		if( endOfContent() )
		{
			m_state = sSentDot ;
			send( "." , true , log_content ) ;
		}
	}
	else if( m_state == sSentDataStub && !( reply.is(Reply::OkForData_354) || reply.positive() ) )
	{
		// (we expect this error reply since the server rejected all our recipeints)
		m_state = sDone ;
		protocol_done = true ;
		raiseDoneSignal( "all recipients rejected" ) ;
	}
	else if( m_state == sSentDot )
	{
		m_state = sDone ;
		protocol_done = true ;
		raiseDoneSignal( reply.errorText() ) ;
	}
	else if( is_start_event )
	{
		throw NotReady() ;
	}
	else
	{
		G_WARNING( "GSmtp::ClientProtocol: failure in client protocol: state " << static_cast<int>(m_state) 
			<< ": unexpected response [" << G::Str::printable(reply.text()) << "]" ) ;
		throw ResponseError( reply.errorText() ) ;
	}
	return protocol_done ;
}

void GSmtp::ClientProtocol::onTimeout()
{
	if( m_state == sStarted )
	{
		// no 220 greeting seen -- go on regardless
		G_WARNING( "GSmtp::ClientProtocol: timeout: no greeting from remote server: continuing" ) ;
		m_state = sSentEhlo ;
		sendEhlo() ;
	}
	else if( m_state == sPreprocessing )
	{
		m_state = sDone ;
		raiseDoneSignal( "preprocessing timeout" , true ) ;
	}
	else
	{
		m_state = sDone ;
		raiseDoneSignal( "response timeout" , true ) ;
	}
}

void GSmtp::ClientProtocol::onTimeoutException( std::exception & e )
{
	if( m_state != sDone )
	{
		m_state = sDone ;
		raiseDoneSignal( std::string("exception: ") + e.what() ) ;
	}
}
 
bool GSmtp::ClientProtocol::serverAuth( const ClientProtocolReply & reply ) const
{
	return !reply.textLine("AUTH ").empty() ;
}

G::Strings GSmtp::ClientProtocol::serverAuthMechanisms( const ClientProtocolReply & reply ) const
{
	G::Strings result ;
	std::string auth_line = reply.textLine("AUTH ") ; // trailing space to avoid "AUTH="
	if( ! auth_line.empty() )
	{
		G::Str::splitIntoTokens( auth_line , result , " " ) ;
		if( result.size() )
			result.pop_front() ; // remove "AUTH" ;
	}
	return result ;
}

void GSmtp::ClientProtocol::raiseDoneSignal( const std::string & reason , bool warn )
{
	if( ! reason.empty() && warn )
		G_WARNING( "GSmtp::ClientProtocol: " << reason ) ;
	cancelTimer() ;
	m_content <<= 0 ;
	m_done_signal.emit( reason ) ;
}

bool GSmtp::ClientProtocol::endOfContent() const
{
	return !m_content->good() ;
}

size_t GSmtp::ClientProtocol::sendLines()
{
	cancelTimer() ; // no response expected during data transfer

	// the read buffer -- capacity grows to longest line, but start with something reasonable
	std::string line( 200U , '.' ) ; 

	size_t n = 0U ;
	while( sendLine(line) )
		n++ ;
	return n ;
}

bool GSmtp::ClientProtocol::sendLine( std::string & line )
{
	line.erase( 1U ) ; // leave "."

	bool ok = false ;
	std::istream & stream = *(m_content.get()) ;
	if( stream.good() )
	{
		const bool pre_erase = false ;
		G::Str::readLineFrom( stream , crlf() , line , pre_erase ) ;
		G_ASSERT( line.length() >= 1U && line.at(0U) == '.' ) ;

		if( !stream.fail() )
		{
			line.append( crlf() ) ;
			bool all_sent = m_sender.protocolSend( line , line.at(1U) == '.' ? 0U : 1U , false ) ;
			if( !all_sent && m_response_timeout != 0U )
				startTimer( m_response_timeout ) ; // use response timer for when flow-control asserted
			ok = all_sent ;
		}
	}
	return ok ;
}

bool GSmtp::ClientProtocol::send( const std::string & line , bool eot , bool log )
{
	if( m_response_timeout != 0U )
		startTimer( m_response_timeout ) ;

	std::string prefix( !eot && line.length() && line.at(0U) == '.' ? "." : "" ) ;
	if( log )
		G_LOG( "GSmtp::ClientProtocol: tx>>: \"" << prefix << G::Str::printable(line) << "\"" ) ;

	return m_sender.protocolSend( prefix + line + crlf() , 0U , false ) ;
}

const std::string & GSmtp::ClientProtocol::crlf()
{
	static std::string s("\015\012") ;
	return s ;
}

G::Signal1<std::string> & GSmtp::ClientProtocol::doneSignal()
{
	return m_done_signal ;
}

G::Signal0 & GSmtp::ClientProtocol::preprocessorSignal()
{
	return m_preprocessor_signal ;
}

// ===

GSmtp::ClientProtocolReply::ClientProtocolReply( const std::string & line ) :
	m_complete(false) ,
	m_valid(false)
{
	if( line.length() >= 3U && 
		is_digit(line.at(0U)) &&
		line.at(0U) <= '5' &&
		is_digit(line.at(1U)) &&
		is_digit(line.at(2U)) &&
		( line.length() == 3U || line.at(3U) == ' ' || line.at(3U) == '-' ) )
	{
		m_valid = true ;
		m_complete = line.length() == 3U || line.at(3U) == ' ' ;
		m_value = G::Str::toUInt( line.substr(0U,3U) ) ;
		if( line.length() > 4U )
		{
			m_text = line.substr(4U) ;
			G::Str::trimLeft( m_text , " \t" ) ;
			G::Str::replaceAll( m_text , "\t" , " " ) ;
		}
	}
}

GSmtp::ClientProtocolReply GSmtp::ClientProtocolReply::ok()
{
	ClientProtocolReply reply( "250 OK" ) ;
	G_ASSERT( ! reply.incomplete() ) ;
	G_ASSERT( reply.positive() ) ;
	G_ASSERT( reply.errorText().empty() ) ;
	return reply ;
}

GSmtp::ClientProtocolReply GSmtp::ClientProtocolReply::ok( Value v )
{
	int i = static_cast<int>(v) ;
	G_ASSERT( i >= 200 && i <= 299 ) ;
	std::ostringstream ss ;
	ss << i << " OK" ;
	ClientProtocolReply reply( ss.str() ) ;
	G_ASSERT( reply.positive() ) ;
	G_ASSERT( reply.errorText().empty() ) ;
	G_ASSERT( reply.is(v) ) ;
	return reply ;
}

GSmtp::ClientProtocolReply GSmtp::ClientProtocolReply::error( const std::string & reason )
{
	ClientProtocolReply reply( std::string("500 ")+G::Str::printable(reason) ) ;
	G_ASSERT( ! reply.incomplete() ) ;
	G_ASSERT( ! reply.positive() ) ;
	G_ASSERT( ! reply.errorText().empty() ) ;
	return reply ;
}

bool GSmtp::ClientProtocolReply::validFormat() const
{
	return m_valid ;
}

bool GSmtp::ClientProtocolReply::incomplete() const
{
	return ! m_complete ;
}

bool GSmtp::ClientProtocolReply::positive() const
{
	return m_valid && m_value < 400U ;
}

unsigned int GSmtp::ClientProtocolReply::value() const
{
	return m_valid ? m_value : 0 ;
}

bool GSmtp::ClientProtocolReply::is( Value v ) const
{
	return value() == static_cast<unsigned int>( v ) ;
}

std::string GSmtp::ClientProtocolReply::errorText() const
{
	const bool positive_completion = type() == PositiveCompletion ;
	return positive_completion ? std::string() : ( m_text.empty() ? std::string("error") : m_text ) ;
}

std::string GSmtp::ClientProtocolReply::text() const
{
	return m_text ;
}

std::string GSmtp::ClientProtocolReply::textLine( const std::string & prefix ) const
{
	size_t start_pos = m_text.find( std::string("\n")+prefix ) ;
	if( start_pos == std::string::npos )
	{
		return std::string() ;
	}
	else
	{
		start_pos++ ;
		size_t end_pos = m_text.find( "\n" , start_pos + prefix.length() ) ;
		return m_text.substr( start_pos , end_pos-start_pos ) ;
	}
}

bool GSmtp::ClientProtocolReply::is_digit( char c )
{
	return c >= '0' && c <= '9' ;
}

GSmtp::ClientProtocolReply::Type GSmtp::ClientProtocolReply::type() const
{
	G_ASSERT( m_valid && (m_value/100U) >= 1U && (m_value/100U) <= 5U ) ;
	return static_cast<Type>( m_value / 100U ) ;
}

GSmtp::ClientProtocolReply::SubType GSmtp::ClientProtocolReply::subType() const
{
	unsigned int n = ( m_value / 10U ) % 10U ;
	if( n < 4U )
		return static_cast<SubType>( n ) ;
	else
		return Invalid_SubType ;
}

bool GSmtp::ClientProtocolReply::add( const ClientProtocolReply & other )
{
	G_ASSERT( other.m_valid ) ;
	G_ASSERT( m_valid ) ;
	G_ASSERT( !m_complete ) ;

	m_complete = other.m_complete ;
	m_text.append( std::string("\n") + other.text() ) ;
	return value() == other.value() ;
}

bool GSmtp::ClientProtocolReply::textContains( std::string key ) const
{
	std::string text( m_text ) ;
	G::Str::toUpper( key ) ;
	G::Str::toUpper( text ) ;
	return text.find(key) != std::string::npos ;
}

// ===

GSmtp::ClientProtocol::Sender::~Sender()
{
}

// ===

GSmtp::ClientProtocol::Config::Config( const std::string & name , 
	unsigned int a , unsigned int b , unsigned int c , bool b1 , bool b2 ) :
		thishost_name(name) ,
		response_timeout(a) ,
		ready_timeout(b) ,
		preprocessor_timeout(c) ,
		must_authenticate(b1) ,
		eight_bit_strict(b2)
{
}

/// \file gclientprotocol.cpp
