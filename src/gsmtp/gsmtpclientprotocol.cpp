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
// gsmtpclientprotocol.cpp
//

#include "gdef.h"
#include "glocal.h"
#include "gfile.h"
#include "gsaslclient.h"
#include "gbase64.h"
#include "gstr.h"
#include "gssl.h"
#include "gxtext.h"
#include "gsmtpclientprotocol.h"
#include "gsocketprotocol.h"
#include "gresolver.h"
#include "glog.h"
#include "gassert.h"

GSmtp::ClientProtocol::ClientProtocol( GNet::ExceptionSink es , Sender & sender ,
	const GAuth::Secrets & secrets , const std::string & sasl_client_config ,
	const Config & config , bool in_secure_tunnel ) :
		GNet::TimerBase(es) ,
		m_sender(sender) ,
		m_secrets(secrets) ,
		m_sasl(new GAuth::SaslClient(m_secrets,sasl_client_config)) ,
		m_config(config) ,
		m_state(State::sInit) ,
		m_to_index(0U) ,
		m_to_accepted(0U) ,
		m_server_has_starttls(false) ,
		m_server_has_auth(false) ,
		m_server_secure(false) ,
		m_server_has_8bitmime(false) ,
		m_authenticated_with_server(false) ,
		m_in_secure_tunnel(in_secure_tunnel) ,
		m_warned(false) ,
		m_done_signal(true)
{
}

void GSmtp::ClientProtocol::start( weak_ptr<StoredMessage> message )
{
	G_DEBUG( "GSmtp::ClientProtocol::start" ) ;

	// reinitialise for the new message
	m_message = message ;
	m_to_index = 0U ;
	m_to_accepted = 0U ;
	m_reply = Reply() ;

	// (re)start the protocol
	m_done_signal.reset() ;
	applyEvent( Reply() , true ) ;
}

shared_ptr<GSmtp::StoredMessage> GSmtp::ClientProtocol::message()
{
	G_ASSERT( !m_message.expired() ) ;
	if( m_message.expired() )
		return shared_ptr<StoredMessage>( new StoredMessageStub ) ;

	return m_message.lock() ;
}

void GSmtp::ClientProtocol::finish()
{
	G_DEBUG( "GSmtp::ClientProtocol::finish" ) ;
	m_config.response_timeout = 1U ;
	m_state = State::sQuitting ;
	send( "QUIT" ) ;
}

void GSmtp::ClientProtocol::secure()
{
	// convert the event into a pretend smtp Reply
	applyEvent( Reply::ok(Reply::Value::Internal_secure) ) ;
}

void GSmtp::ClientProtocol::sendComplete()
{
	if( m_state == State::sData )
	{
		size_t n = sendLines() ;

		G_LOG( "GSmtp::ClientProtocol: tx>>: [" << n << " line(s) of content]" ) ;
		if( endOfContent() )
		{
			m_state = State::sSentDot ;
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
	send( "EHLO " , m_config.thishost_name ) ;
}

void GSmtp::ClientProtocol::sendHelo()
{
	send( "HELO " , m_config.thishost_name ) ;
}

void GSmtp::ClientProtocol::sendMail()
{
	const bool mismatch = message()->eightBit() == 1 && !m_server_has_8bitmime ;
	if( mismatch && m_config.eight_bit_strict )
	{
		// message failure as per RFC-6152
		m_state = State::sMessageDone ;
		raiseDoneSignal( 0 , "failed" , "cannot send 8-bit message to 7-bit server" ) ;
	}
	else
	{
		if( mismatch && !m_warned )
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
	std::string mail_from_tail = message()->from() ;
	mail_from_tail.append( 1U , '>' ) ;
	if( m_server_has_8bitmime && message()->eightBit() != -1 )
	{
		mail_from_tail.append( message()->eightBit() ? " BODY=8BITMIME" : " BODY=7BIT" ) ; // RFC-6152
	}
	if( m_authenticated_with_server && message()->fromAuthOut().empty() && !m_sasl->id().empty() )
	{
		// default policy is to use the session authentication id, although
		// this is not strictly conforming with RFC-2554
		mail_from_tail.append( " AUTH=" ) ;
		mail_from_tail.append( G::Xtext::encode(m_sasl->id()) ) ;
	}
	else if( m_authenticated_with_server && G::Xtext::valid(message()->fromAuthOut()) )
	{
		mail_from_tail.append( " AUTH=" ) ;
		mail_from_tail.append( message()->fromAuthOut() ) ;
	}
	else if( m_authenticated_with_server )
	{
		mail_from_tail.append( " AUTH=<>" ) ;
	}
	send( "MAIL FROM:<" , mail_from_tail ) ;
}

bool GSmtp::ClientProtocol::applyEvent( const Reply & reply , bool is_start_event )
{
	G_DEBUG( "GSmtp::ClientProtocol::applyEvent: " << reply.value() << ": " << G::Str::printable(reply.text()) ) ;

	cancelTimer() ;

	bool protocol_done = false ;
	if( m_state == State::sInit && is_start_event )
	{
		// got start-event -- wait for 220 greeting
		m_state = State::sStarted ;
		if( m_config.ready_timeout != 0U )
			startTimer( m_config.ready_timeout ) ;
	}
	else if( m_state == State::sInit && reply.is(Reply::Value::ServiceReady_220) )
	{
		// got greeting before start-event
		G_DEBUG( "GSmtp::ClientProtocol::applyEvent: init -> ready" ) ;
		m_state = State::sServiceReady ;
	}
	else if( m_state == State::sServiceReady && is_start_event )
	{
		// got start-event after greeting
		G_DEBUG( "GSmtp::ClientProtocol::applyEvent: ready -> sent-ehlo" ) ;
		m_state = State::sSentEhlo ;
		sendEhlo() ;
	}
	else if( m_state == State::sStarted && reply.is(Reply::Value::ServiceReady_220) )
	{
		// got greeting after start-event
		G_DEBUG( "GSmtp::ClientProtocol::applyEvent: start -> sent-ehlo" ) ;
		m_state = State::sSentEhlo ;
		sendEhlo() ;
	}
	else if( m_state == State::sMessageDone && is_start_event )
	{
		// new message within the current session, start the client filter
		m_state = State::sFiltering ;
		startFiltering() ;
	}
	else if( m_state == State::sSentEhlo && (
		reply.is(Reply::Value::SyntaxError_500) ||
		reply.is(Reply::Value::SyntaxError_501) ||
		reply.is(Reply::Value::NotImplemented_502) ) )
	{
		// server didn't like EHLO so fall back to HELO
		if( m_config.must_use_tls && !m_in_secure_tunnel ) throw SmtpError( "tls is mandated but the server cannot do esmtp" ) ;
		m_state = State::sSentHelo ;
		sendHelo() ;
	}
	else if( ( m_state == State::sSentEhlo || m_state == State::sSentHelo || m_state == State::sSentTlsEhlo ) && reply.is(Reply::Value::Ok_250) )
	{
		// hello accepted, start a new session
		G_DEBUG( "GSmtp::ClientProtocol::applyEvent: ehlo/rset reply \"" << G::Str::printable(reply.text()) << "\"" ) ;
		if( m_state == State::sSentEhlo || m_state == State::sSentTlsEhlo ) // esmtp
		{
			m_server_has_starttls = m_state == State::sSentEhlo && reply.textContains("\nSTARTTLS") ;
			m_server_has_8bitmime = reply.textContains("\n8BITMIME");
			m_server_has_auth = serverAuth( reply ) ;
			m_server_auth_mechanisms = serverAuthMechanisms( reply ) ;
			m_server_secure = m_state == State::sSentTlsEhlo || m_in_secure_tunnel ;
		}
		m_auth_mechanism = m_sasl->mechanism( m_server_auth_mechanisms ) ;

		if( !m_server_secure && m_config.must_use_tls )
		{
			if( !m_server_has_starttls )
				throw SmtpError( "tls is mandated but the server cannot do starttls" ) ;
			m_state = State::sStartTls ;
			send( "STARTTLS" ) ;
		}
		else if( !m_server_secure && m_config.use_starttls_if_possible && m_server_has_starttls )
		{
			m_state = State::sStartTls ;
			send( "STARTTLS" ) ;
		}
		else if( m_server_has_auth && !m_sasl->active() )
		{
			// continue -- the server will complain later if it considers authentication is mandatory
			G_LOG( "GSmtp::ClientProtocol: not authenticating with the remote server since no "
				"client authentication secret has been configured" ) ;
			m_state = State::sFiltering ;
			startFiltering() ;
		}
		else if( m_server_has_auth && m_sasl->active() && m_auth_mechanism.empty() )
		{
			throw SmtpError( "cannot do authentication required by remote server "
				"(" + G::Str::printable(G::Str::join(",",m_server_auth_mechanisms)) + "): "
				"check for a compatible client secret" ) ;
		}
		else if( m_server_has_auth && m_sasl->active() )
		{
			m_state = State::sAuth ;
			send( "AUTH " , m_auth_mechanism , initialResponse(*m_sasl) ) ;
		}
		else if( !m_server_has_auth && m_sasl->active() && m_config.must_authenticate )
		{
			throw SmtpError( "authentication is not supported by the remote smtp server" ) ;
		}
		else
		{
			m_state = State::sFiltering ;
			startFiltering() ;
		}
	}
	else if( m_state == State::sStartTls && reply.is(Reply::Value::ServiceReady_220) )
	{
		// greeting for new secure session -- start tls handshake
		m_sender.protocolSend( std::string() , 0U , true ) ;
	}
	else if( m_state == State::sStartTls && reply.is(Reply::Value::NotAvailable_454) )
	{
		// starttls rejected
		throw TlsError( reply.errorText() ) ;
	}
	else if( m_state == State::sStartTls && reply.is(Reply::Value::Internal_secure) )
	{
		// tls session established -- send ehlo
		m_state = State::sSentTlsEhlo ;
		sendEhlo() ;
	}
	else if( m_state == State::sAuth && reply.is(Reply::Value::Challenge_334) &&
		( reply.text() == "=" || G::Base64::valid(reply.text()) || m_auth_mechanism == "PLAIN" ) )
	{
		// authentication challenge -- send the response
		std::string challenge = G::Base64::valid(reply.text()) ? G::Base64::decode(reply.text()) : std::string() ;
		GAuth::SaslClient::Response rsp = m_sasl->response( m_auth_mechanism , challenge ) ;
		if( rsp.error )
			send( "*" ) ; // expect 501
		else
			send( G::Base64::encode(rsp.data) , false , rsp.sensitive ) ;
	}
	else if( m_state == State::sAuth && reply.is(Reply::Value::Challenge_334) )
	{
		// invalid authentication challenge -- send cancel (RFC-4954 p5)
		send( "*" ) ; // expect 501
	}
	else if( m_state == State::sAuth && reply.positive()/*235*/ )
	{
		// authenticated -- proceed to first message
		m_authenticated_with_server = true ;
		G_LOG( "GSmtp::ClientProtocol::applyEvent: successful authentication with remote server " << (m_server_secure?"over tls ":"") << m_sasl->info() ) ;
		m_state = State::sFiltering ;
		startFiltering() ;
	}
	else if( m_state == State::sAuth && !reply.positive() && m_sasl->next() )
	{
		// authentication failed -- try the next mechanism
		G_LOG( "GSmtp::ClientProtocol::applyEvent: " << AuthError(*m_sasl.get(),reply).str() << ": trying [" << G::Str::lower(m_sasl->mechanism()) << "]" ) ;
		m_auth_mechanism = m_sasl->mechanism() ;
		send( "AUTH " , m_auth_mechanism , initialResponse(*m_sasl) ) ;
	}
	else if( m_state == State::sAuth && !reply.positive() && m_config.must_authenticate )
	{
		// authentication failed and mandatory and no more mechanisms
		throw AuthError( *m_sasl.get() , reply ) ;
	}
	else if( m_state == State::sAuth && !reply.positive() )
	{
		// authentication failed, but optional -- continue and expect submission errors
		G_ASSERT( !m_authenticated_with_server ) ;
		G_WARNING( "GSmtp::ClientProtocol::applyEvent: " << AuthError(*m_sasl.get(),reply).str() << ": continuing" ) ;
		m_state = State::sFiltering ;
		startFiltering() ;
	}
	else if( m_state == State::sFiltering && reply.is(Reply::Value::Internal_filter_ok) )
	{
		// filter finished with 'ok'
		m_state = State::sSentMail ;
		sendMail() ;
	}
	else if( m_state == State::sFiltering && reply.is(Reply::Value::Internal_filter_abandon) )
	{
		// filter failed with 'abandon'
		m_state = State::sMessageDone ;
		raiseDoneSignal( -1 , std::string() ) ;
	}
	else if( m_state == State::sFiltering && reply.is(Reply::Value::Internal_filter_error) )
	{
		// filter failed with 'error'
		m_state = State::sMessageDone ;
		raiseDoneSignal( -2 , reply.errorText() , reply.errorReason() ) ;
	}
	else if( m_state == State::sSentMail && reply.is(Reply::Value::Ok_250) )
	{
		// got reponse to mail-from -- send rcpt-to
		std::string to = message()->to( m_to_index++ ) ;
		m_state = State::sSentRcpt ;
		send( "RCPT TO:<" , to , ">" ) ;
	}
	else if( m_state == State::sSentRcpt && m_to_index < message()->toCount() )
	{
		// got reponse to rctp-to and more recipients to go
		if( reply.positive() )
			m_to_accepted++ ;
		else
			G_WARNING( "GSmtp::ClientProtocol: recipient rejected" ) ;

		std::string to = message()->to( m_to_index++ ) ;
		send( "RCPT TO:<" , to , ">" ) ;
	}
	else if( m_state == State::sSentRcpt )
	{
		// got reponse to rctp-to and all recipients requested
		if( reply.positive() )
			m_to_accepted++ ;
		else
			G_WARNING( "GSmtp::ClientProtocol: recipient rejected" ) ;

		if( ( m_config.must_accept_all_recipients && m_to_accepted < message()->toCount() ) || m_to_accepted == 0U )
		{
			m_state = State::sSentDataStub ;
			send( "RSET" ) ;
		}
		else
		{
			m_state = State::sSentData ;
			send( "DATA" ) ;
		}
	}
	else if( m_state == State::sSentData && reply.is(Reply::Value::OkForData_354) )
	{
		// data command accepted -- send content until flow-control asserted or all sent
		m_state = State::sData ;
		size_t n = sendLines() ;
		G_LOG( "GSmtp::ClientProtocol: tx>>: [" << n << " line(s) of content]" ) ;
		if( endOfContent() )
		{
			m_state = State::sSentDot ;
			send( "." , true ) ;
		}
	}
	else if( m_state == State::sSentDataStub )
	{
		// got response to rst following rejection of recipients
		m_state = State::sMessageDone ;
		std::string how_many = m_config.must_accept_all_recipients ? std::string("one or more") : std::string("all") ;
		raiseDoneSignal( reply.value() , how_many + " recipients rejected" ) ;
	}
	else if( m_state == State::sSentDot )
	{
		// got response to data eot
		m_state = State::sMessageDone ;
		raiseDoneSignal( reply.value() , reply.errorText() ) ;
	}
	else if( m_state == State::sQuitting && reply.value() == 221 )
	{
		// got quit response
		protocol_done = true ;
	}
	else if( is_start_event )
	{
		// got a start-event for new message, but not in a valid state
		throw NotReady() ;
	}
	else
	{
		G_WARNING( "GSmtp::ClientProtocol: client protocol: "
			<< "unexpected response [" << G::Str::printable(reply.text()) << "]" ) ;
		throw SmtpError( "unexpected response" , reply.errorText() ) ;
	}
	return protocol_done ;
}

std::string GSmtp::ClientProtocol::initialResponse( const GAuth::SaslClient & sasl )
{
	std::string rsp = sasl.initialResponse( 450U ) ; // RFC-2821 total command line length of 512
	return rsp.empty() ? rsp : ( " " + G::Base64::encode(rsp) ) ;
}

void GSmtp::ClientProtocol::onTimeout()
{
	if( m_state == State::sStarted )
	{
		// no 220 greeting seen -- go on regardless
		G_WARNING( "GSmtp::ClientProtocol: timeout: no greeting from remote server after " << m_config.ready_timeout << "s: continuing" ) ;
		m_state = State::sSentEhlo ;
		sendEhlo() ;
	}
	else if( m_state == State::sFiltering )
	{
		throw SmtpError( "filtering timeout after " + G::Str::fromUInt(m_config.filter_timeout) + "s" ) ;
	}
	else if( m_state == State::sData )
	{
		throw SmtpError( "flow-control timeout after " + G::Str::fromUInt(m_config.response_timeout) + "s" ) ;
	}
	else
	{
		throw SmtpError( "response timeout after " + G::Str::fromUInt(m_config.response_timeout) + "s" ) ;
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

void GSmtp::ClientProtocol::startFiltering()
{
	G_ASSERT( m_state == State::sFiltering ) ;
	if( m_config.filter_timeout != 0U )
		startTimer( m_config.filter_timeout ) ; // cancelled in applyEvent()
	m_filter_signal.emit() ;
}

void GSmtp::ClientProtocol::filterDone( bool ok , const std::string & response , const std::string & reason )
{
	if( ok )
	{
		// apply filter response event to continue with this message
		applyEvent( Reply::ok(Reply::Value::Internal_filter_ok) ) ;
	}
	else if( response.empty() )
	{
		// apply filter response event to abandon this message (done-code -1)
		applyEvent( Reply::ok(Reply::Value::Internal_filter_abandon) ) ;
	}
	else
	{
		// apply filter response event to fail this message (done-code -2)
		applyEvent( Reply::error(Reply::Value::Internal_filter_error,response,reason) ) ;
	}
}

void GSmtp::ClientProtocol::raiseDoneSignal( int response_code , const std::string & response , const std::string & reason )
{
	if( ! response.empty() && response_code == 0 )
		G_WARNING( "GSmtp::ClientProtocol: smtp client protocol: " << response ) ;

	cancelTimer() ;
	m_done_signal.emit( response_code , response , reason ) ;
}

bool GSmtp::ClientProtocol::endOfContent()
{
	return !message()->contentStream().good() ;
}

size_t GSmtp::ClientProtocol::sendLines()
{
	cancelTimer() ; // no response expected during data transfer

	// the read buffer -- capacity grows to longest line, but start with something reasonable
	std::string read_buffer( 200U , '.' ) ;

	size_t n = 0U ;
	while( sendLine(read_buffer) )
		n++ ;
	return n ;
}

bool GSmtp::ClientProtocol::sendLine( std::string & line )
{
	line.erase( 1U ) ; // leave "."

	bool ok = false ;
	if( message()->contentStream().good() )
	{
		std::istream & stream = message()->contentStream() ;
		const bool pre_erase = false ;
		G::Str::readLineFrom( stream , std::string(1U,'\n') , line , pre_erase ) ;
		G_ASSERT( line.length() >= 1U && line.at(0U) == '.' ) ;

		if( !stream.fail() )
		{
			// read file wrt. lf -- send with cr-lf
			if( !line.empty() && line.at(line.size()-1U) != '\r' )
				line.append( 1U , '\r' ) ; // moot
			line.append( 1U , '\n' ) ;

			bool all_sent = m_sender.protocolSend( line , line.at(1U) == '.' ? 0U : 1U , false ) ;
			if( !all_sent && m_config.response_timeout != 0U )
				startTimer( m_config.response_timeout ) ; // use response timer for when flow-control asserted
			ok = all_sent ;
		}
	}
	return ok ;
}

void GSmtp::ClientProtocol::send( const char * p )
{
	send( std::string(p) , false , false ) ;
}

void GSmtp::ClientProtocol::send( const char * p , const std::string & s , const std::string & p2 )
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
	if( m_config.response_timeout != 0U )
		startTimer( m_config.response_timeout ) ;

	bool dot_prefix = !eot && line.length() && line.at(0U) == '.' ;
	if( sensitive )
	{
		G_LOG( "GSmtp::ClientProtocol: tx>>: [response not logged]" ) ;
	}
	else
	{
		G_LOG( "GSmtp::ClientProtocol: tx>>: \"" << (dot_prefix?".":"") << G::Str::printable(line) << "\"" ) ;
	}
	return m_sender.protocolSend( (dot_prefix?".":"") + line + "\r\n" , 0U , false ) ;
}

G::Slot::Signal3<int,std::string,std::string> & GSmtp::ClientProtocol::doneSignal()
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
	reply.m_value = static_cast<int>(v) ;
	if( !text.empty() )
		reply.m_text = "OK\n" + text ;
	G_ASSERT( reply.positive() ) ; if( !reply.positive() ) reply.m_value = 250 ;
	return reply ;
}

GSmtp::ClientProtocolReply GSmtp::ClientProtocolReply::error( Value v , const std::string & response , const std::string & reason )
{
	ClientProtocolReply reply( std::string("500 ")+G::Str::printable(response) ) ;
	int vv = static_cast<int>(v) ;
	reply.m_value = ( vv >= 500 && vv < 600 ) ? vv : 500 ;
	reply.m_reason = reason ;
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
	return m_valid && m_value < 400 ;
}

int GSmtp::ClientProtocolReply::value() const
{
	return m_valid ? m_value : 0 ;
}

bool GSmtp::ClientProtocolReply::is( Value v ) const
{
	return value() == static_cast<int>(v) ;
}

std::string GSmtp::ClientProtocolReply::errorText() const
{
	const bool positive_completion = type() == Type::PositiveCompletion ;
	return positive_completion ? std::string() : ( m_text.empty() ? std::string("error") : m_text ) ;
}

std::string GSmtp::ClientProtocolReply::errorReason() const
{
	return m_reason ;
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
		return SubType::Invalid_SubType ;
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

// ==

GSmtp::ClientProtocol::AuthError::AuthError( const GAuth::SaslClient & sasl , const GSmtp::ClientProtocolReply & reply ) :
	SmtpError( "authentication failed " + sasl.info() + ": [" + G::Str::printable(reply.text()) + "]" )
{
}

std::string GSmtp::ClientProtocol::AuthError::str() const
{
	return std::string( what() ) ;
}

/// \file gsmtpclientprotocol.cpp
