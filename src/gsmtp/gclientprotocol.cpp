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
// gclientprotocol.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "glocal.h"
#include "gfile.h"
#include "gsaslclient.h"
#include "gbase64.h"
#include "gstr.h"
#include "gssl.h"
#include "gxtext.h"
#include "gclientprotocol.h"
#include "gsocketprotocol.h"
#include "gresolver.h"
#include "glog.h"
#include "gassert.h"

GSmtp::ClientProtocol::ClientProtocol( GNet::ExceptionHandler & eh , Sender & sender ,
	const GAuth::Secrets & secrets , Config config , bool in_secure_tunnel ) :
		GNet::TimerBase(eh) ,
		m_sender(sender) ,
		m_secrets(secrets) ,
		m_thishost(config.thishost_name) ,
		m_state(sInit) ,
		m_to_index(0U) ,
		m_to_accepted(0U) ,
		m_server_has_starttls(false) ,
		m_server_has_auth(false) ,
		m_server_secure(false) ,
		m_server_has_8bitmime(false) ,
		m_message_is_8bit(false) ,
		m_authenticated_with_server(false) ,
		m_must_authenticate(config.must_authenticate) ,
		m_anonymous(config.anonymous) ,
		m_must_accept_all_recipients(config.must_accept_all_recipients) ,
		m_use_starttls_if_possible(config.use_starttls_if_possible) ,
		m_must_use_tls(config.must_use_tls) ,
		m_in_secure_tunnel(in_secure_tunnel) ,
		m_strict(config.eight_bit_strict) ,
		m_warned(false) ,
		m_response_timeout(config.response_timeout) ,
		m_ready_timeout(config.ready_timeout) ,
		m_filter_timeout(config.filter_timeout) ,
		m_done_signal(true)
{
	m_sasl.reset( new GAuth::SaslClient(m_secrets) ) ;
}

void GSmtp::ClientProtocol::start( const std::string & from , const G::StringArray & to ,
	bool eight_bit , std::string mail_from_auth , unique_ptr<std::istream> content )
{
	G_DEBUG( "GSmtp::ClientProtocol::start" ) ;

	// reinitialise for the new message
	m_to = to ;
	m_to_index = 0U ;
	m_to_accepted = 0U ;
	m_from = from ;
	m_content.reset( content.release() ) ;
	m_message_is_8bit = eight_bit ;
	m_message_mail_from_auth = mail_from_auth ; // MAIL..AUTH=
	m_reply = Reply() ;


	// (re)start the protocol
	m_done_signal.reset() ;
	applyEvent( Reply() , true ) ;
}

void GSmtp::ClientProtocol::filterDone( bool ok , const std::string & reason )
{
	if( ok )
	{
		// filter response event to continue with this message
		applyEvent( Reply::ok(Reply::Internal_filter_ok) ) ;
	}
	else if( reason.empty() )
	{
		// filter response event to abandon this message (done-code -1)
		applyEvent( Reply::ok(Reply::Internal_filter_abandon) ) ;
	}
	else
	{
		// filter response event to fail this message (done-code -2)
		applyEvent( Reply::error(Reply::Internal_filter_error,reason) ) ;
	}
}

void GSmtp::ClientProtocol::secure()
{
	// convert the event into a pretend smtp Reply
	applyEvent( Reply::ok(Reply::Internal_secure) ) ;
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
			send( "." , true ) ;
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
			send( "550 syntax error: " , reason ) ;
	}
	return protocol_done ;
}

void GSmtp::ClientProtocol::sendEhlo()
{
	send( "EHLO " , m_thishost ) ;
}

void GSmtp::ClientProtocol::sendHelo()
{
	send( "HELO " , m_thishost ) ;
}

void GSmtp::ClientProtocol::sendMail()
{
	const bool dodgy = m_message_is_8bit && !m_server_has_8bitmime ;
	if( dodgy && m_strict )
	{
		m_state = sDone ;
		raiseDoneSignal( "cannot send 8-bit message to 7-bit server" , 0 , true/*warn*/ ) ;
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
	std::string mail_from_tail = m_from ;
	mail_from_tail.append( 1U , '>' ) ;
	if( m_server_has_8bitmime )
	{
		mail_from_tail.append( " BODY=8BITMIME" ) ;
	}
	if( m_authenticated_with_server && m_message_mail_from_auth.empty() && m_sasl.get() && !m_sasl->id().empty() )
	{
		// default policy is to use the session authentication id, although
		// this is not strictly conforming with RFC-2554
		mail_from_tail.append( " AUTH=" ) ;
		mail_from_tail.append( G::Xtext::encode(m_sasl->id()) ) ;
	}
	else if( m_authenticated_with_server && G::Xtext::valid(m_message_mail_from_auth) )
	{
		mail_from_tail.append( " AUTH=" ) ;
		mail_from_tail.append( m_message_mail_from_auth ) ;
	}
	else if( m_authenticated_with_server )
	{
		mail_from_tail.append( " AUTH=<>" ) ;
	}
	send( "MAIL FROM:<" , mail_from_tail ) ;
}

void GSmtp::ClientProtocol::startFiltering()
{
	G_ASSERT( m_state == sFiltering ) ;
	if( m_filter_timeout != 0U )
		startTimer( m_filter_timeout ) ;
	m_filter_signal.emit() ;
}

bool GSmtp::ClientProtocol::applyEvent( const Reply & reply , bool is_start_event )
{
	G_DEBUG( "GSmtp::ClientProtocol::applyEvent: " << reply.value() << ": " << G::Str::printable(reply.text()) ) ;

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
		m_state = sFiltering ;
		startFiltering() ;
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
		if( m_must_use_tls && !m_in_secure_tunnel ) throw SmtpError( "tls is mandated but the server cannot do esmtp" ) ;
		m_state = sSentHelo ;
		sendHelo() ;
	}
	else if( ( m_state == sSentEhlo || m_state == sSentHelo || m_state == sSentTlsEhlo ) && reply.is(Reply::Ok_250) )
	{
		G_ASSERT( m_sasl.get() != nullptr ) ;
		G_DEBUG( "GSmtp::ClientProtocol::applyEvent: ehlo/rset reply \"" << G::Str::printable(reply.text()) << "\"" ) ;

		if( m_state == sSentEhlo || m_state == sSentTlsEhlo )
		{
			m_server_has_starttls = m_state == sSentEhlo && reply.textContains("\nSTARTTLS") ;
			m_server_has_8bitmime = reply.textContains("\n8BITMIME");
			m_server_has_auth = serverAuth( reply ) ;
			m_server_auth_mechanisms = serverAuthMechanisms( reply ) ;
			m_server_secure = m_state == sSentTlsEhlo || m_in_secure_tunnel ;
		}
		m_auth_mechanism = m_sasl->preferred( m_server_auth_mechanisms ) ;

		if( !m_server_secure && m_must_use_tls )
		{
			if( !m_server_has_starttls )
				throw SmtpError( "tls is mandated but the server cannot do starttls" ) ;
			m_state = sStartTls ;
			send( "STARTTLS" ) ;
		}
		else if( !m_server_secure && m_use_starttls_if_possible && m_server_has_starttls )
		{
			m_state = sStartTls ;
			send( "STARTTLS" ) ;
		}
		else if( m_server_has_auth && !m_sasl->active() )
		{
			// continue -- the server will complain later if it considers authentication is mandatory
			G_LOG( "GSmtp::ClientProtocol: not authenticating with the remote server since no "
				"client authentication secret has been configured" ) ;
			m_state = sFiltering ;
			startFiltering() ;
		}
		else if( m_server_has_auth && m_sasl->active() && m_auth_mechanism.empty() )
		{
			std::ostringstream ss ;
			ss << "cannot do authentication mandated by remote server: add a client secret" ;
			SmtpError error( ss.str() ) ;
			G_WARNING( "GSmtp::ClientProtocol::applyEvent: " << error.what() ) ;
			throw error ;
		}
		else if( m_server_has_auth && m_sasl->active() )
		{
			m_state = sAuth1 ;
			send( "AUTH " , m_auth_mechanism ) ;
		}
		else if( !m_server_has_auth && m_sasl->active() && m_must_authenticate )
		{
			// (this makes sense if we need to propagate messages' authentication credentials)
			SmtpError error( "authentication not supported by the remote smtp server" ) ;
			G_WARNING( "GSmtp::ClientProtocol::applyEvent: " << error.what() ) ;
			throw error ;
		}
		else
		{
			m_state = sFiltering ;
			startFiltering() ;
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
	else if( m_state == sStartTls && reply.is(Reply::Internal_secure) )
	{
		m_state = sSentTlsEhlo ;
		sendEhlo() ;
	}
	else if( m_state == sAuth1 && reply.is(Reply::Challenge_334) && G::Base64::valid(reply.text()) )
	{
		bool done = true ;
		bool sensitive = false ;
		std::string challenge = G::Base64::decode( reply.text() ) ;
		std::string rsp = m_sasl->response( m_auth_mechanism , challenge , done , sensitive ) ;
		if( rsp.empty() )
		{
			m_state = sAuth2 ;
			send( "*" ) ; // ie. cancel authentication
		}
		else
		{
			m_state = done ? sAuth2 : m_state ;
			send( G::Base64::encode(rsp,std::string()) , false , sensitive ) ;
		}
	}
	else if( m_state == sAuth1 && !reply.positive() && m_sasl.get() != nullptr && m_sasl->next() )
	{
		SmtpError error( "authentication failed " + m_sasl->info() + ": [" + G::Str::printable(reply.text()) + "]" ) ;
		G_LOG( "GSmtp::ClientProtocol::applyEvent: " << error.what() << ": trying [" << G::Str::lower(m_sasl->preferred()) << "]" ) ;
		m_auth_mechanism = m_sasl->preferred() ;
		m_state = sAuth1 ;
		send( "AUTH " , m_auth_mechanism ) ;
	}
	else if( m_state == sAuth1 && !reply.positive() ) // was is(Reply::NotAuthenticated_535)
	{
		SmtpError error( "authentication failed " + m_sasl->info() + ": [" + G::Str::printable(reply.text()) + "]" ) ;
		G_WARNING( "GSmtp::ClientProtocol::applyEvent: " << error.what() << (m_must_authenticate?"":": continuing") ) ;
		if( m_must_authenticate )
			throw error ;

		// continue even without sucessful authentication
		m_state = sFiltering ;
		startFiltering() ;
	}
	else if( m_state == sAuth2 && !reply.positive() && m_sasl.get() != nullptr && m_sasl->next() )
	{
		SmtpError error( "authentication failed " + m_sasl->info() + ": [" + G::Str::printable(reply.text()) + "]" ) ;
		G_LOG( "GSmtp::ClientProtocol::applyEvent: " << error.what() << ": trying [" << G::Str::lower(m_sasl->preferred()) << "]" ) ;
		m_auth_mechanism = m_sasl->preferred() ;
		m_state = sAuth1 ;
		send( "AUTH " , m_auth_mechanism ) ;
	}
	else if( m_state == sAuth2 )
	{
		m_authenticated_with_server = reply.is(Reply::Authenticated_235) ;
		if( !m_authenticated_with_server && m_must_authenticate )
		{
			SmtpError error( "authentication failed " + m_sasl->info() ) ;
			G_WARNING( "GSmtp::ClientProtocol::applyEvent: " << error.what() ) ;
			throw error ;
		}
		else if( !m_authenticated_with_server )
		{
			// continue even without sucessful authentication
			G_WARNING( "GSmtp::ClientProtocol::applyEvent: authentication failed " << m_sasl->info() << ": continuing" ) ;
			m_state = sFiltering ;
			startFiltering() ;
		}
		else
		{
			G_LOG( "GSmtp::ClientProtocol::applyEvent: successful authentication with remote server " << m_sasl->info() ) ;
			m_state = sFiltering ;
			startFiltering() ;
		}
	}
	else if( m_state == sFiltering && reply.is(Reply::Internal_filter_ok) )
	{
		m_state = sSentMail ;
		sendMail() ;
	}
	else if( m_state == sFiltering && reply.is(Reply::Internal_filter_abandon) )
	{
		m_state = sDone ;
		protocol_done = true ;
		raiseDoneSignal( std::string() , -1 ) ;
	}
	else if( m_state == sFiltering && reply.is(Reply::Internal_filter_error) )
	{
		m_state = sDone ;
		protocol_done = true ;
		raiseDoneSignal( reply.errorText() , -2 ) ;
	}
	else if( m_state == sFiltering )
	{
		m_state = sDone ;
		protocol_done = true ;
		raiseDoneSignal( reply.errorText() , reply.value() ) ;
	}
	else if( m_state == sSentMail && reply.is(Reply::Ok_250) )
	{
		std::string to = m_to.at( m_to_index++ ) ;
		m_state = sSentRcpt ;
		send( "RCPT TO:<" , to , ">" ) ;
	}
	else if( m_state == sSentRcpt && m_to_index < m_to.size() )
	{
		if( reply.positive() )
			m_to_accepted++ ;
		else
			G_WARNING( "GSmtp::ClientProtocol: recipient rejected" ) ;

		std::string to = m_to.at( m_to_index++ ) ;
		send( "RCPT TO:<" , to , ">" ) ;
	}
	else if( m_state == sSentRcpt )
	{
		if( reply.positive() )
			m_to_accepted++ ;
		else
			G_WARNING( "GSmtp::ClientProtocol: recipient rejected" ) ;

		if( ( m_must_accept_all_recipients && m_to_accepted != m_to.size() ) || m_to_accepted == 0U )
		{
			m_state = sSentDataStub ;
			send( "RSET" ) ;
		}
		else
		{
			m_state = sSentData ;
			send( "DATA" ) ;
		}
	}
	else if( m_state == sSentData && reply.is(Reply::OkForData_354) )
	{
		m_state = sData ;

		size_t n = sendLines() ;

		G_LOG( "GSmtp::ClientProtocol: tx>>: [" << n << " line(s) of content]" ) ;
		if( endOfContent() )
		{
			m_state = sSentDot ;
			send( "." , true ) ;
		}
	}
	else if( m_state == sSentDataStub )
	{
		m_state = sDone ;
		protocol_done = true ;
		std::string how_many = m_must_accept_all_recipients ? std::string("one or more") : std::string("all") ;
		raiseDoneSignal( how_many + " recipients rejected" , reply.value() ) ;
	}
	else if( m_state == sSentDot )
	{
		m_state = sDone ;
		protocol_done = true ;
		raiseDoneSignal( reply.errorText() , reply.value() ) ;
	}
	else if( is_start_event )
	{
		throw NotReady() ;
	}
	else
	{
		G_WARNING( "GSmtp::ClientProtocol: failure in client protocol: state " << static_cast<int>(m_state)
			<< ": unexpected response [" << G::Str::printable(reply.text()) << "]" ) ;
		throw SmtpError( "unexpected response" , reply.errorText() ) ;
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
	else if( m_state == sFiltering )
	{
		throw SmtpError( "filtering timeout" ) ;
	}
	else
	{
		throw SmtpError( "response timeout" ) ;
	}
}

bool GSmtp::ClientProtocol::serverAuth( const ClientProtocolReply & reply ) const
{
	return !reply.textLine("AUTH ").empty() ;
}

G::StringArray GSmtp::ClientProtocol::serverAuthMechanisms( const ClientProtocolReply & reply ) const
{
	G::StringArray result ;
	std::string auth_line = reply.textLine("AUTH ") ; // trailing space to avoid "AUTH="
	if( ! auth_line.empty() )
	{
		std::string tail = G::Str::tail( auth_line , auth_line.find(" ") , std::string() ) ; // after "AUTH "
		G::Str::splitIntoTokens( tail , result , G::Str::ws() ) ; // expect space separators, but ignore CR etc
	}
	return result ;
}

void GSmtp::ClientProtocol::raiseDoneSignal( const std::string & reason , int reason_code , bool warn )
{
	if( ! reason.empty() && warn )
		G_WARNING( "GSmtp::ClientProtocol: smtp client protocol: " << reason ) ;
	cancelTimer() ;
	m_content.reset() ;
	m_done_signal.emit( reason , reason_code ) ;
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
		G::Str::readLineFrom( stream , lf() , line , pre_erase ) ;
		G_ASSERT( line.length() >= 1U && line.at(0U) == '.' ) ;

		if( !stream.fail() )
		{
			line.append( !line.empty() && line.at(line.size()-1U) == '\r' ? lf() : crlf() ) ;
			bool all_sent = m_sender.protocolSend( line , line.at(1U) == '.' ? 0U : 1U , false ) ;
			if( !all_sent && m_response_timeout != 0U )
				startTimer( m_response_timeout ) ; // use response timer for when flow-control asserted
			ok = all_sent ;
		}
	}
	return ok ;
}

void GSmtp::ClientProtocol::send( const char * p )
{
	send( std::string(p) , false , false ) ;
}

void GSmtp::ClientProtocol::send( const char * p , const std::string & s , const char * p2 )
{
	std::string line( p ) ;
	line.append( s ) ;
	line.append( p2 ) ;
	send( line , false , false ) ;
}

void GSmtp::ClientProtocol::send( const char * p , const std::string & s )
{
	send( std::string(p) + s , false , false ) ;
}

bool GSmtp::ClientProtocol::send( const std::string & line , bool eot , bool sensitive )
{
	if( m_response_timeout != 0U )
		startTimer( m_response_timeout ) ;

	std::string prefix( !eot && line.length() && line.at(0U) == '.' ? "." : "" ) ;
	if( sensitive )
	{
		G_LOG( "GSmtp::ClientProtocol: tx>>: [response not logged]" ) ;
	}
	else
	{
		G_LOG( "GSmtp::ClientProtocol: tx>>: \"" << prefix << G::Str::printable(line) << "\"" ) ;
	}
	return m_sender.protocolSend( prefix + line + crlf() , 0U , false ) ;
}

const std::string & GSmtp::ClientProtocol::crlf()
{
	static const std::string s( "\r\n" ) ;
	return s ;
}

const std::string & GSmtp::ClientProtocol::lf()
{
	static const std::string s( "\n" ) ;
	return s ;
}

G::Slot::Signal2<std::string,int> & GSmtp::ClientProtocol::doneSignal()
{
	return m_done_signal ;
}

G::Slot::Signal0 & GSmtp::ClientProtocol::filterSignal()
{
	return m_filter_signal ;
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
		m_value = G::Str::toInt( line.substr(0U,3U) ) ;
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
	return ClientProtocolReply( "250 OK" ) ;
}

GSmtp::ClientProtocolReply GSmtp::ClientProtocolReply::ok( Value v , const std::string & text )
{
	ClientProtocolReply reply = ok() ;
	reply.m_value = v ;
	if( !text.empty() )
		reply.m_text = "OK\n" + text ;
	G_ASSERT( reply.positive() ) ; if( !reply.positive() ) reply.m_value = 250 ;
	return reply ;
}

GSmtp::ClientProtocolReply GSmtp::ClientProtocolReply::error( Value v , const std::string & reason )
{
	ClientProtocolReply reply( std::string("500 ")+G::Str::printable(reason) ) ;
	reply.m_value = v ;
	G_ASSERT( !reply.positive() ) ; if( reply.positive() ) reply.m_value = 500 ;
	return reply ;
}

GSmtp::ClientProtocolReply GSmtp::ClientProtocolReply::error( const std::string & reason )
{
	return ClientProtocolReply( std::string("500 ")+G::Str::printable(reason) ) ;
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
	return m_valid && m_value < 400 ;
}

int GSmtp::ClientProtocolReply::value() const
{
	return m_valid ? m_value : 0 ;
}

bool GSmtp::ClientProtocolReply::is( Value v ) const
{
	return value() == v ;
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
	G_ASSERT( m_valid && (m_value/100) >= 1 && (m_value/100) <= 5 ) ;
	return static_cast<Type>( m_value / 100 ) ;
}

GSmtp::ClientProtocolReply::SubType GSmtp::ClientProtocolReply::subType() const
{
	G_ASSERT( m_valid && m_value >= 0 ) ;
	int n = ( m_value / 10 ) % 10 ;
	if( n < 4 )
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

GSmtp::ClientProtocol::Config::Config( const std::string & name_ ,
	unsigned int response_timeout_ ,
	unsigned int ready_timeout_ , unsigned int filter_timeout_ ,
	bool use_starttls_if_possible_ , bool must_use_tls_ ,
	bool must_authenticate_ , bool anonymous_ ,
	bool must_accept_all_recipients_ , bool eight_bit_strict_ ) :
		thishost_name(name_) ,
		response_timeout(response_timeout_) ,
		ready_timeout(ready_timeout_) ,
		filter_timeout(filter_timeout_) ,
		use_starttls_if_possible(use_starttls_if_possible_) ,
		must_use_tls(must_use_tls_) ,
		must_authenticate(must_authenticate_) ,
		anonymous(anonymous_) ,
		must_accept_all_recipients(must_accept_all_recipients_) ,
		eight_bit_strict(eight_bit_strict_)
{
}
/// \file gclientprotocol.cpp
