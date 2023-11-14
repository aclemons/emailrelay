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
/// \file gsmtpclientprotocol.cpp
///

#include "gdef.h"
#include "gsmtpclientprotocol.h"
#include "gsaslclient.h"
#include "gbase64.h"
#include "gtest.h"
#include "gstr.h"
#include "gstringfield.h"
#include "gstringtoken.h"
#include "gxtext.h"
#include "glog.h"
#include "gassert.h"
#include <algorithm>
#include <numeric>
#include <cstring> // std::memcpy

namespace GSmtp
{
	namespace ClientProtocolImp
	{
		class EhloReply ;
		struct AuthError ;
	}
}

struct GSmtp::ClientProtocolImp::AuthError : public ClientProtocol::SmtpError /// An exception class.
{
	AuthError( const GAuth::SaslClient & , const ClientReply & ) ;
	std::string str() const ;
} ;

class GSmtp::ClientProtocolImp::EhloReply /// Holds the parameters of an EHLO reply.
{
public:
	explicit EhloReply( const ClientReply & ) ;
	bool has( const std::string & option ) const ;
	G::StringArray values( const std::string & option ) const ;

private:
	ClientReply m_reply ;
} ;

// ==

GSmtp::ClientProtocol::ClientProtocol( GNet::ExceptionSink es , Sender & sender ,
	const GAuth::SaslClientSecrets & secrets , const std::string & sasl_client_config ,
	const Config & config , bool in_secure_tunnel ) :
		GNet::TimerBase(es) ,
		m_sender(sender) ,
		m_sasl(std::make_unique<GAuth::SaslClient>(secrets,sasl_client_config)) ,
		m_config(config) ,
		m_in_secure_tunnel(in_secure_tunnel) ,
		m_eightbit_warned(false) ,
		m_binarymime_warned(false) ,
		m_utf8_warned(false) ,
		m_done_signal(true)
{
	m_config.bdat_chunk_size = std::max( std::size_t(64U) , m_config.bdat_chunk_size ) ;
	m_config.reply_size_limit = std::max( std::size_t(100U) , m_config.reply_size_limit ) ;
	m_message_line.reserve( 200U ) ;
}

void GSmtp::ClientProtocol::start( std::weak_ptr<GStore::StoredMessage> message_in )
{
	G_DEBUG( "GSmtp::ClientProtocol::start" ) ;

	// reinitialise for the new message
	m_message_state = MessageState() ;
	m_message_state.ptr = message_in ;
	m_message_p = message_in.lock().get() ;
	m_message_state.selector = m_message_p->clientAccountSelector() ;
	m_message_state.id = m_message_p->id().str() ;

	// (re)start the protocol
	m_done_signal.reset() ;
	applyEvent( ClientReply::start() ) ;
}

void GSmtp::ClientProtocol::finish()
{
	G_DEBUG( "GSmtp::ClientProtocol::finish" ) ;
	m_protocol.state = State::Quitting ;
	send( "QUIT\r\n"_sv ) ;
}

void GSmtp::ClientProtocol::secure()
{
	applyEvent( ClientReply::secure() ) ;
}

void GSmtp::ClientProtocol::sendComplete()
{
	if( m_protocol.state == State::Data )
	{
		std::size_t n = sendContentLines() ;
		n++ ; // since the socket protocol has now sent the line that was blocked

		G_LOG( "GSmtp::ClientProtocol: tx>>: [" << n << " line(s) of content]" ) ;
		if( endOfContent() )
		{
			m_protocol.state = State::SentDot ;
			sendEot() ;
		}
	}
}

G::Slot::Signal<const GSmtp::ClientProtocol::DoneInfo &> & GSmtp::ClientProtocol::doneSignal() noexcept
{
	return m_done_signal ;
}

G::Slot::Signal<> & GSmtp::ClientProtocol::filterSignal() noexcept
{
	return m_filter_signal ;
}

bool GSmtp::ClientProtocol::apply( const std::string & rx )
{
	G_LOG( "GSmtp::ClientProtocol: rx<<: \"" << G::Str::printable(rx) << "\"" ) ;

	m_protocol.reply_lines.push_back( rx ) ;

	G::StringArray lines ;
	std::swap( lines , m_protocol.reply_lines ) ; // clear

	if( !ClientReply::valid(lines) )
	{
		throw SmtpError( "invalid response" ) ;
	}
	else if( ClientReply::complete(lines) )
	{
		bool done = applyEvent( ClientReply(lines) ) ;
		return done ;
	}
	else if( m_protocol.replySize() > m_config.reply_size_limit )
	{
		throw SmtpError( "overflow on input" ) ;
	}
	else
	{
		std::swap( lines , m_protocol.reply_lines ) ; // restore
		return false ;
	}
}

bool GSmtp::ClientProtocol::applyEvent( const ClientReply & reply )
{
	using AuthError = ClientProtocolImp::AuthError ;
	G_DEBUG( "GSmtp::ClientProtocol::applyEvent: " << reply.value() << ": " << G::Str::printable(reply.text()) ) ;

	cancelTimer() ;

	bool protocol_done = false ;
	bool is_start_event = reply.is( ClientReply::Value::Internal_start ) ;
	if( m_protocol.state == State::Init && is_start_event )
	{
		// got start-event -- wait for 220 greeting
		m_protocol.state = State::Started ;
		if( m_config.ready_timeout != 0U )
			startTimer( m_config.ready_timeout ) ;
	}
	else if( m_protocol.state == State::Init && reply.is(ClientReply::Value::ServiceReady_220) )
	{
		// got greeting before start-event
		G_DEBUG( "GSmtp::ClientProtocol::applyEvent: init -> ready" ) ;
		m_protocol.state = State::ServiceReady ;
	}
	else if( m_protocol.state == State::ServiceReady && is_start_event )
	{
		// got start-event after greeting
		G_DEBUG( "GSmtp::ClientProtocol::applyEvent: ready -> sent-ehlo" ) ;
		m_protocol.state = State::SentEhlo ;
		sendEhlo() ;
	}
	else if( m_protocol.state == State::Started && reply.is(ClientReply::Value::ServiceReady_220) )
	{
		// got greeting after start-event
		G_DEBUG( "GSmtp::ClientProtocol::applyEvent: start -> sent-ehlo" ) ;
		m_protocol.state = State::SentEhlo ;
		sendEhlo() ;
	}
	else if( m_protocol.state == State::MessageDone && is_start_event && m_session.ok(m_message_state.selector) )
	{
		// new message within the current session, start the client filter
		m_protocol.state = State::Filtering ;
		startFiltering() ;
	}
	else if( m_protocol.state == State::MessageDone && is_start_event )
	{
		// new message with changed client account selector -- start a new session
		G_DEBUG( "GSmtp::ClientProtocol::applyEvent: new account selector [" << m_message_state.selector << "]" ) ;
		if( !m_config.try_reauthentication )
			throw SmtpError( "cannot switch client account" ) ;
		m_protocol.state = m_session.secure ? State::SentTlsEhlo : State::SentEhlo ;
		sendEhlo() ;
	}
	else if( m_protocol.state == State::SentEhlo && (
		reply.is(ClientReply::Value::SyntaxError_500) ||
		reply.is(ClientReply::Value::SyntaxError_501) ||
		reply.is(ClientReply::Value::NotImplemented_502) ) )
	{
		// server didn't like EHLO so fall back to HELO
		if( m_config.must_use_tls && !m_in_secure_tunnel )
			throw SmtpError( "tls is mandated but the server cannot do esmtp" ) ;
		m_protocol.state = State::SentHelo ;
		sendHelo() ;
	}
	else if( ( m_protocol.state == State::SentEhlo ||
			m_protocol.state == State::SentHelo ||
			m_protocol.state == State::SentTlsEhlo ) &&
		reply.is(ClientReply::Value::Ok_250) )
	{
		// hello accepted, start a new session
		G_DEBUG( "GSmtp::ClientProtocol::applyEvent: ehlo reply \"" << G::Str::printable(reply.text()) << "\"" ) ;
		m_session = SessionState() ;
		if( m_protocol.state != State::SentHelo ) // esmtp -- parse server's extensions
		{
			ClientProtocolImp::EhloReply ehlo_reply( reply ) ;
			m_session.server.has_starttls = m_protocol.state == State::SentEhlo && ehlo_reply.has( "STARTTLS" ) ;
			m_session.server.has_8bitmime = ehlo_reply.has( "8BITMIME" ) ;
			m_session.server.has_binarymime = ehlo_reply.has( "BINARYMIME" ) ;
			m_session.server.has_chunking = ehlo_reply.has( "CHUNKING" ) ;
			m_session.server.auth_mechanisms = ehlo_reply.values( "AUTH" ) ;
			m_session.server.has_auth = !m_session.server.auth_mechanisms.empty() ;
			m_session.server.has_pipelining = ehlo_reply.has( "PIPELINING" ) ;
			m_session.server.has_smtputf8 = ehlo_reply.has( "SMTPUTF8" ) ;
			m_session.secure = m_protocol.state == State::SentTlsEhlo || m_in_secure_tunnel ;
		}

		// choose the authentication mechanism
		m_session.auth_mechanism = m_sasl->mechanism( m_session.server.auth_mechanisms , m_message_state.selector ) ;

		// start encryption, authentication or client-filtering
		if( !m_sasl->validSelector( m_message_state.selector ) )
		{
			throw BadSelector( std::string("selector [").append(m_message_state.selector).append(1U,']') ) ;
		}
		else if( !m_session.secure && m_config.must_use_tls )
		{
			if( !m_session.server.has_starttls )
				throw SmtpError( "tls is mandated but the server cannot do starttls" ) ;
			m_protocol.state = State::StartTls ;
			send( "STARTTLS\r\n"_sv ) ;
		}
		else if( !m_session.secure && m_config.use_starttls_if_possible && m_session.server.has_starttls )
		{
			m_protocol.state = State::StartTls ;
			send( "STARTTLS\r\n"_sv ) ;
		}
		else if( m_sasl->mustAuthenticate(m_message_state.selector) && m_session.server.has_auth && m_session.auth_mechanism.empty() )
		{
			std::string e = "cannot do authentication: check for a compatible client secret" ;
			if( !m_message_state.selector.empty() )
				e.append(" with selector [").append(G::Str::printable(m_message_state.selector)).append(1U,']') ;
			throw SmtpError( e ) ;
		}
		else if( m_sasl->mustAuthenticate(m_message_state.selector) && !m_session.server.has_auth )
		{
			throw SmtpError( "authentication is not supported by the remote smtp server" ) ;
		}
		else if( m_sasl->mustAuthenticate(m_message_state.selector) )
		{
			m_protocol.state = State::Auth ;
			GAuth::SaslClient::Response rsp = initialResponse( *m_sasl , m_message_state.selector ) ;
			std::string rsp_data = rsp.data.empty() ? std::string() : std::string(1U,' ').append(G::Base64::encode(rsp.data)) ;
			send( "AUTH "_sv , m_session.auth_mechanism , rsp_data , "\r\n"_sv , rsp.sensitive ) ;
		}
		else
		{
			m_protocol.state = State::Filtering ;
			startFiltering() ;
		}
	}
	else if( m_protocol.state == State::StartTls && reply.is(ClientReply::Value::ServiceReady_220) )
	{
		// greeting for new secure session -- start tls handshake
		m_sender.protocolSend( {} , 0U , true ) ;
	}
	else if( m_protocol.state == State::StartTls && reply.is(ClientReply::Value::NotAvailable_454) )
	{
		// starttls rejected
		throw TlsError( reply.errorText() ) ;
	}
	else if( m_protocol.state == State::StartTls && reply.is(ClientReply::Value::Internal_secure) )
	{
		// tls session established -- send hello again
		m_protocol.state = State::SentTlsEhlo ;
		sendEhlo() ;
	}
	else if( m_protocol.state == State::Auth && reply.is(ClientReply::Value::Challenge_334) &&
		( reply.text() == "=" || G::Base64::valid(reply.text()) || m_session.auth_mechanism == "PLAIN" ) )
	{
		// authentication challenge -- send the response
		std::string challenge = G::Base64::valid(reply.text()) ? G::Base64::decode(reply.text()) : std::string() ;
		GAuth::SaslClient::Response rsp = m_sasl->response( m_session.auth_mechanism , challenge , m_message_state.selector ) ;
		if( rsp.error )
			send( "*\r\n"_sv ) ; // expect 501
		else
			sendRsp( rsp ) ;
	}
	else if( m_protocol.state == State::Auth && reply.is(ClientReply::Value::Challenge_334) )
	{
		// invalid authentication challenge -- send cancel (RFC-4954 p5)
		send( "*\r\n"_sv ) ; // expect 501
	}
	else if( m_protocol.state == State::Auth && reply.positive()/*235*/ )
	{
		// authenticated -- proceed to first message
		m_session.authenticated = true ;
		m_session.auth_selector = m_message_state.selector ;
		G_LOG( "GSmtp::ClientProtocol::applyEvent: successful authentication with remote server "
			<< (m_session.secure?"over tls ":"") << m_sasl->info() ) ;
		m_protocol.state = State::Filtering ;
		startFiltering() ;
	}
	else if( m_protocol.state == State::Auth && !reply.positive() && m_sasl->next() )
	{
		// authentication failed -- try the next mechanism
		G_LOG( "GSmtp::ClientProtocol::applyEvent: " << AuthError(*m_sasl,reply).str()
			<< ": trying [" << G::Str::lower(m_sasl->mechanism()) << "]" ) ;
		m_session.auth_mechanism = m_sasl->mechanism() ;
		GAuth::SaslClient::Response rsp = initialResponse( *m_sasl , m_message_state.selector ) ;
		std::string rsp_data = rsp.data.empty() ? std::string() : std::string(1U,' ').append(G::Base64::encode(rsp.data)) ;
		send( "AUTH "_sv , m_session.auth_mechanism , rsp_data , "\r\n"_sv , rsp.sensitive ) ;
	}
	else if( m_protocol.state == State::Auth && !reply.positive() && !m_config.authentication_fallthrough )
	{
		// authentication failed and no more mechanisms and no fallthrough -- abort
		throw AuthError( *m_sasl , reply ) ;
	}
	else if( m_protocol.state == State::Auth && !reply.positive() )
	{
		// authentication failed, but fallthrough enabled -- continue and expect submission errors
		G_ASSERT( !m_session.authenticated ) ;
		G_WARNING( "GSmtp::ClientProtocol::applyEvent: " << AuthError(*m_sasl,reply).str() << ": continuing" ) ;
		m_protocol.state = State::Filtering ;
		startFiltering() ;
	}
	else if( m_protocol.state == State::Filtering && reply.is(ClientReply::Value::Internal_filter_abandon) )
	{
		// filter failed with 'abandon' -- finish
		m_protocol.state = State::MessageDone ;
		raiseDoneSignal( reply.doneCode() , std::string() ) ;
	}
	else if( m_protocol.state == State::Filtering && reply.is(ClientReply::Value::Internal_filter_error) )
	{
		// filter failed with 'error' -- finish
		m_protocol.state = State::MessageDone ;
		raiseDoneSignal( reply.doneCode() , reply.errorText() , reply.reason() ) ;
	}
	else if( m_protocol.state == State::Filtering && reply.is(ClientReply::Value::Internal_filter_ok) )
	{
		// filter finished with 'ok' -- send MAIL-FROM if ok
		std::string reason = checkSendable() ; // eg. eight-bit message to seven-bit server
		if( !reason.empty() )
		{
			m_protocol.state = State::MessageDone ;
			raiseDoneSignal( 0 , "failed" , reason ) ;
		}
		else
		{
			m_protocol.state = State::SentMail ;
			sendMailFrom() ;
		}
	}
	else if( m_protocol.state == State::SentMail && reply.is(ClientReply::Value::Ok_250) )
	{
		// got ok response to MAIL-FROM -- send first RCPT-TO
		m_protocol.state = State::SentRcpt ;
		sendRcptTo() ;
	}
	else if( m_protocol.state == State::SentMail && !reply.positive() )
	{
		// got error response to MAIL-FROM (new)
		m_protocol.state = State::MessageDone ;
		raiseDoneSignal( reply.doneCode() , reply.errorText() ) ;
	}
	else if( m_protocol.state == State::SentRcpt && m_message_state.to_index < message().toCount() )
	{
		// got response to RCTP-TO and more recipients to go -- send next RCPT-TO
		bool accepted = reply.positive() ;
		if( accepted )
			m_message_state.to_accepted++ ;
		else
			m_message_state.to_rejected.push_back( message().to(m_message_state.to_index-1U) ) ;
		sendRcptTo() ;
	}
	else if( m_protocol.state == State::SentRcpt )
	{
		// got response to the last RCTP-TO -- send DATA or BDAT command

		bool accepted = reply.positive() ;
		if( accepted )
			m_message_state.to_accepted++ ;
		else
			m_message_state.to_rejected.push_back( message().to(m_message_state.to_index-1U) ) ;

		if( ( m_config.must_accept_all_recipients && m_message_state.to_accepted < message().toCount() ) || m_message_state.to_accepted == 0U )
		{
			m_protocol.state = State::SentDataStub ;
			send( "RSET\r\n"_sv ) ;
		}
		else if( ( message().bodyType() == BodyType::BinaryMime || G::Test::enabled("smtp-client-prefer-bdat") ) &&
			m_session.server.has_binarymime && m_session.server.has_chunking )
		{
			// RFC-3030
			m_message_state.content_size = message().contentSize() ;
			std::string content_size_str = std::to_string( m_message_state.content_size ) ;

			bool one_chunk = (m_message_state.content_size+5U) <= m_config.bdat_chunk_size ; // 5 for " LAST"
			if( one_chunk )
			{
				m_protocol.state = State::SentBdatLast ;
				sendBdatAndChunk( m_message_state.content_size , content_size_str , true ) ;
			}
			else
			{
				m_protocol.state = State::SentBdatMore ;

				m_message_state.chunk_data_size = m_config.bdat_chunk_size ;
				m_message_state.chunk_data_size_str = std::to_string(m_message_state.chunk_data_size) ;

				bool last = sendBdatAndChunk( m_message_state.chunk_data_size , m_message_state.chunk_data_size_str , false ) ;
				if( last )
					m_protocol.state = State::SentBdatLast ;
			}
		}
		else
		{
			m_protocol.state = State::SentData ;
			send( "DATA\r\n"_sv ) ;
		}
	}
	else if( m_protocol.state == State::SentData && reply.is(ClientReply::Value::OkForData_354) )
	{
		// DATA command accepted -- send content until flow-control asserted or all sent
		m_protocol.state = State::Data ;
		std::size_t n = sendContentLines() ;
		G_LOG( "GSmtp::ClientProtocol: tx>>: [" << n << " line(s) of content]" ) ;
		if( endOfContent() )
		{
			m_protocol.state = State::SentDot ;
			sendEot() ;
		}
	}
	else if( m_protocol.state == State::SentDataStub )
	{
		// got response to RSET following rejection of recipients
		m_protocol.state = State::MessageDone ;
		std::string how_many = m_config.must_accept_all_recipients ? std::string("one or more") : std::string("all") ;
		raiseDoneSignal( reply.doneCode() , how_many + " recipients rejected" ) ;
	}
	else if( m_protocol.state == State::SentBdatMore )
	{
		// got response to BDAT chunk -- send the next chunk
		if( reply.positive() )
		{
			bool last = sendBdatAndChunk( m_message_state.chunk_data_size , m_message_state.chunk_data_size_str , false ) ;
			if( last )
				m_protocol.state = State::SentBdatLast ;
		}
		else
		{
			raiseDoneSignal( reply.doneCode() , reply.errorText() ) ;
		}
	}
	else if( m_protocol.state == State::SentDot || m_protocol.state == State::SentBdatLast )
	{
		// got response to DATA EOT or BDAT LAST -- finish
		m_protocol.state = State::MessageDone ;
		m_message_line.clear() ;
		m_message_buffer.clear() ;
		if( reply.positive() && m_message_state.to_accepted < message().toCount() )
			raiseDoneSignal( 0 , "one or more recipients rejected" ) ;
		else
			raiseDoneSignal( reply.doneCode() , reply.errorText() ) ;
	}
	else if( m_protocol.state == State::Quitting && reply.value() == 221 )
	{
		// got QUIT response
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

GStore::StoredMessage & GSmtp::ClientProtocol::message()
{
	// the state machine ensures that message() is not used while in the
	// MessageDone/Init/ServiceReady states, so we can assert that the
	// current message is valid
	G_ASSERT( !m_message_state.ptr.expired() ) ;
	G_ASSERT( m_message_p != nullptr ) ;
	if( m_message_state.ptr.expired() || m_message_p == nullptr )
		throw SmtpError( "invalid internal state" ) ;

	return *m_message_p ;
}

GAuth::SaslClient::Response GSmtp::ClientProtocol::initialResponse( const GAuth::SaslClient & sasl , G::string_view selector )
{
	return sasl.initialResponse( selector , 450U ) ; // RFC-2821 total command line length of 512
}

void GSmtp::ClientProtocol::onTimeout()
{
	if( m_protocol.state == State::Started )
	{
		// no 220 greeting seen -- go on regardless
		G_WARNING( "GSmtp::ClientProtocol: timeout: no greeting from remote server after "
			<< m_config.ready_timeout << "s: continuing" ) ;
		m_protocol.state = State::SentEhlo ;
		sendEhlo() ;
	}
	else if( m_protocol.state == State::Filtering )
	{
		throw SmtpError( "filtering timeout" ) ; // never gets here
	}
	else if( m_protocol.state == State::Data )
	{
		throw SmtpError( "flow-control timeout after " + G::Str::fromUInt(m_config.response_timeout) + "s" ) ;
	}
	else
	{
		throw SmtpError( "response timeout after " + G::Str::fromUInt(m_config.response_timeout) + "s" ) ;
	}
}

void GSmtp::ClientProtocol::startFiltering()
{
	G_ASSERT( m_protocol.state == State::Filtering ) ;
	m_filter_signal.emit() ;
}

void GSmtp::ClientProtocol::filterDone( Filter::Result result , const std::string & response , const std::string & reason )
{
	if( result == Filter::Result::ok )
	{
		// apply filter response event to continue with this message
		applyEvent( ClientReply::filterOk() ) ;
	}
	else if( result == Filter::Result::abandon )
	{
		// apply filter response event to abandon this message (done-code -1)
		applyEvent( ClientReply::filterAbandon() ) ;
	}
	else
	{
		// apply filter response event to fail this message (done-code -2)
		applyEvent( ClientReply::filterError(response,reason) ) ;
	}
}

void GSmtp::ClientProtocol::raiseDoneSignal( int response_code , const std::string & response ,
	const std::string & reason )
{
	if( !response.empty() && response_code == 0 )
		G_WARNING( "GSmtp::ClientProtocol: smtp client protocol: " << response ) ;

	m_message_p = nullptr ;
	cancelTimer() ;

	m_done_signal.emit( { response_code , response , reason , G::StringArray(m_message_state.to_rejected) } ) ;
}

bool GSmtp::ClientProtocol::endOfContent()
{
	return !message().contentStream().good() ;
}

std::string GSmtp::ClientProtocol::checkSendable()
{
	const bool eightbitmime_mismatch =
		message().bodyType() == BodyType::EightBitMime &&
		!m_session.server.has_8bitmime ;

	const bool utf8_mismatch =
		message().utf8Mailboxes() &&
		!m_session.server.has_smtputf8 ;

	const bool binarymime_mismatch =
		message().bodyType() == BodyType::BinaryMime &&
		!( m_session.server.has_binarymime && m_session.server.has_chunking ) ;

	if( eightbitmime_mismatch && m_config.eightbit_strict )
	{
		// message failure as per RFC-6152
		return "cannot send 8-bit message to 7-bit server" ;
	}
	else if( binarymime_mismatch && m_config.binarymime_strict )
	{
		// RFC-3030 p7 "third, it may treat this as a permanent error"
		return "cannot send binarymime message to a non-chunking server" ;
	}
	else if( utf8_mismatch && m_config.smtputf8_strict )
	{
		// message failure as per RFC-6531
		return "cannot send utf8 message to non-smtputf8 server" ;
	}
	else
	{
		if( eightbitmime_mismatch && !m_eightbit_warned )
		{
			m_eightbit_warned = true ;
			G_WARNING( "GSmtp::ClientProtocol::checkSendable: sending an eight-bit message "
				"to a server that has not advertised the 8BITMIME extension" ) ;
		}
		if( binarymime_mismatch && !m_binarymime_warned )
		{
			m_binarymime_warned = true ;
			G_WARNING( "GSmtp::ClientProtocol::checkSendable: sending a binarymime message "
				"to a server that has not advertised the BINARYMIME/CHUNKING extension" ) ;
		}
		if( utf8_mismatch && !m_utf8_warned )
		{
			m_utf8_warned = true ;
			G_WARNING( "GSmtp::ClientProtocol::checkSendable: sending a message with utf8 mailbox names"
				" to a server that has not advertised the SMTPUTF8 extension" ) ;
		}
		return std::string() ;
	}
}

bool GSmtp::ClientProtocol::sendMailFrom()
{
	bool use_bdat = false ;
	std::string mail_from_tail = message().from() ;
	mail_from_tail.append( 1U , '>' ) ;

	if( message().bodyType() == BodyType::SevenBit )
	{
		if( m_session.server.has_8bitmime )
			mail_from_tail.append( " BODY=7BIT" ) ; // RFC-6152
	}
	else if( message().bodyType() == BodyType::EightBitMime )
	{
		if( m_session.server.has_8bitmime )
			mail_from_tail.append( " BODY=8BITMIME" ) ; // RFC-6152
	}
	else if( message().bodyType() == BodyType::BinaryMime )
	{
		if( m_session.server.has_binarymime && m_session.server.has_chunking )
		{
			mail_from_tail.append( " BODY=BINARYMIME" ) ; // RFC-3030
			use_bdat = true ;
		}
	}

	if( m_session.server.has_smtputf8 && message().utf8Mailboxes() )
	{
		mail_from_tail.append( " SMTPUTF8" ) ; // RFC-6531 3.4
	}

	if( m_session.authenticated )
	{
		if( m_config.anonymous )
		{
			mail_from_tail.append( " AUTH=<>" ) ;
		}
		else if( message().fromAuthOut().empty() && !m_sasl->id().empty() )
		{
			// default policy is to use the session authentication id, although
			// this is not strictly conforming with RFC-2554/RFC-4954
			mail_from_tail.append( " AUTH=" ) ;
			mail_from_tail.append( G::Xtext::encode(m_sasl->id()) ) ;
		}
		else if( m_session.authenticated && G::Xtext::valid(message().fromAuthOut()) )
		{
			mail_from_tail.append( " AUTH=" ) ;
			mail_from_tail.append( message().fromAuthOut() ) ;
		}
		else
		{
			mail_from_tail.append( " AUTH=<>" ) ;
		}
	}

	if( m_config.pipelining && m_session.server.has_pipelining )
	{
		// pipeline the MAIL-FROM with RCTP-TO commands
		//
		// don't pipeline the DATA command here, even though it's allowed,
		// so that we don't have to mess about if all recipients are
		// rejected but the server still accepts the pipelined DATA
		// command (see RFC-2920)
		//
		std::string commands ;
		commands.reserve( 2000U ) ;
		commands.append("MAIL FROM:<").append(mail_from_tail).append("\r\n",2U) ;
		const std::size_t n = message().toCount() ;
		for( std::size_t i = 0U ; i < n ; i++ )
			commands.append("RCPT TO:<").append(message().to(i)).append(">\r\n",3U) ;
		m_message_state.to_index = 0 ;
		sendCommandLines( commands ) ;
	}
	else
	{
		send( "MAIL FROM:<"_sv , mail_from_tail , "\r\n"_sv ) ;
	}
	return use_bdat ;
}

void GSmtp::ClientProtocol::sendRcptTo()
{
	if( m_config.pipelining && m_session.server.has_pipelining )
	{
		m_message_state.to_index++ ;
	}
	else
	{
		G_ASSERT( m_message_state.to_index < message().toCount() ) ;
		std::string to = message().to( m_message_state.to_index++ ) ;
		send( "RCPT TO:<"_sv , to , ">\r\n"_sv ) ;
	}
}

std::size_t GSmtp::ClientProtocol::sendContentLines()
{
	cancelTimer() ; // response timer only when blocked

	m_message_line.resize( 1U ) ;
	m_message_line.at(0) = '.' ;

	std::size_t line_count = 0U ;
	while( sendNextContentLine(m_message_line) )
		line_count++ ;

	return line_count ;
}

bool GSmtp::ClientProtocol::sendNextContentLine( std::string & line )
{
	// read one line of content including any unterminated last line -- all
	// content should be in reasonably-sized lines with CR-LF endings, even
	// if BINARYMIME (see RFC-3030 p7 "In particular...") -- content is
	// allowed to have 'bare' CR and LF characters (RFC-2821 4.1.1.4) but
	// we should pass them on as CR-LF (RFC-2821 2.3.7), although this is
	// made configurable here -- bad content filters might also result in
	// bare LF line endings -- to avoid data shuffling the dot-escaping is
	// done by keeping a leading dot in the string buffer
	G_ASSERT( !line.empty() && line.at(0) == '.' ) ;
	bool ok = false ;
	line.erase( 1U ) ; // leave "."
	if( G::Str::readLine( message().contentStream() , line ,
		m_config.crlf_only ? G::Str::Eol::CrLf : G::Str::Eol::Cr_Lf_CrLf ,
		/*pre_erase_result=*/false ) )
	{
		line.append( "\r\n" , 2U ) ;
		ok = sendContentLineImp( line , line.at(1U) == '.' ? 0U : 1U ) ;
	}
	return ok ;
}

void GSmtp::ClientProtocol::sendEhlo()
{
	send( "EHLO "_sv , m_config.thishost_name , "\r\n"_sv ) ;
}

void GSmtp::ClientProtocol::sendHelo()
{
	send( "HELO "_sv , m_config.thishost_name , "\r\n"_sv ) ;
}

void GSmtp::ClientProtocol::sendEot()
{
	sendImp( ".\r\n"_sv ) ;
}

void GSmtp::ClientProtocol::sendRsp( const GAuth::SaslClient::Response & rsp )
{
	std::string s = G::Base64::encode(rsp.data).append("\r\n",2U) ;
	sendImp( s , rsp.sensitive ? 0U : std::string::npos ) ;
}

void GSmtp::ClientProtocol::sendCommandLines( const std::string & lines )
{
	sendImp( lines.data() ) ;
}

void GSmtp::ClientProtocol::send( G::string_view s )
{
	sendImp( s ) ;
}

void GSmtp::ClientProtocol::send( G::string_view s0 , G::string_view s1 , G::string_view s2 , G::string_view s3 , bool s2_sensitive )
{
	std::string line = std::string(s0.data(),s0.size()).append(s1.data(),s1.size()).append(s2.data(),s2.size()).append(s3.data(),s3.size()) ;
	sendImp( line , ( s2_sensitive && !s2.empty() ) ? (s0.size()+s1.size()) : std::string::npos ) ;
}

bool GSmtp::ClientProtocol::sendBdatAndChunk( std::size_t size , const std::string & size_str , bool last )
{
	// the configured bdat chunk size is the maximum size of the payload within
	// the TPDU -- to target a particular TPDU size (N) the configured value (n)
	// should be 12 less than a 5-digit TPDU size, 13 less than a 6-digit TPDU
	// size etc. -- the TPDU buffer is notionally allocated as the chunk size
	// plus 7 plus the number of chunk size digits, N=n+7+(int(log10(n))+1), but
	// to allow for "LAST" at EOF the actual allocation includes a small leading
	// margin

	std::size_t buffer_size = size + (last?12U:7U) + size_str.size() ;
	std::size_t eolpos = (last?10U:5U) + size_str.size() ;
	std::size_t datapos = eolpos + 2U ;
	std::size_t margin = last ? 0U : 10U ;

	m_message_buffer.resize( buffer_size + margin ) ;
	char * out = &m_message_buffer[0] + margin ;

	std::memcpy( out , "BDAT " , 5U ) ; // NOLINT bugprone-not-null-terminated-result
	std::memcpy( out+5U , size_str.data() , size_str.size() ) ; // NOLINT
	if( last )
		std::memcpy( out+5U+size_str.size() , " LAST" , 5U ) ; // NOLINT
	std::memcpy( out+eolpos , "\r\n" , 2U ) ; // NOLINT

	G_ASSERT( buffer_size > datapos ) ;
	G_ASSERT( (out+datapos) < (&m_message_buffer[0]+m_message_buffer.size()) ) ;
	message().contentStream().read( out+datapos , buffer_size-datapos ) ;
	std::streamsize gcount = message().contentStream().gcount() ;

	G_ASSERT( gcount >= 0 ) ;
	//static_assert( sizeof(std::streamsize) == sizeof(std::size_t) , "" ) ; // not msvc
	std::size_t nread = static_cast<std::size_t>( gcount ) ;

	bool eof = (datapos+nread) < buffer_size ;
	if( eof && !last )
	{
		// if EOF then redo the BDAT command with "LAST", making
		// use of the the buffer margin
		last = true ;
		std::string n = std::to_string( nread ) ;
		std::size_t cmdsize = 12U + n.size() ;
		out = out + datapos - cmdsize ;
		datapos = cmdsize ;
		G_ASSERT( n.size() <= size_str.size() ) ;
		G_ASSERT( out >= &m_message_buffer[0] ) ;
		std::memcpy( out , "BDAT " , 5U ) ; // NOLINT
		std::memcpy( out+5U , n.data() , n.size() ) ; // NOLINT
		std::memcpy( out+5U+n.size(), " LAST\r\n" , 7U ) ; // NOLINT
	}

	sendChunkImp( out , datapos+nread ) ;
	return last ;
}

// --

void GSmtp::ClientProtocol::sendChunkImp( const char * p , std::size_t n )
{
	G::string_view sv( p , n ) ;

	if( m_config.response_timeout != 0U )
		startTimer( m_config.response_timeout ) ; // response timer on every bdat block

	if( G::Log::atVerbose() )
	{
		std::size_t pos = sv.find( "\r\n"_sv ) ;
		G::string_view cmd = G::Str::headView( sv , pos , {p,std::size_t(0U)} ) ;
		G::StringTokenView t( cmd , " "_sv ) ;
		G::string_view count = t.next()() ;
		G::string_view end = count.size() == 1U && count[0] == '1' ? "]"_sv : "s]"_sv ;
		G_LOG( "GSmtp::ClientProtocol: tx>>: \"" << cmd << "\" [" << count << " byte" << end ) ;
	}

	m_sender.protocolSend( sv , 0U , false ) ;
}

bool GSmtp::ClientProtocol::sendContentLineImp( const std::string & line , std::size_t offset )
{
	bool all_sent = m_sender.protocolSend( line , offset , false ) ;
	if( !all_sent && m_config.response_timeout != 0U )
		startTimer( m_config.response_timeout ) ; // response timer while blocked by flow-control
	return all_sent ;
}

bool GSmtp::ClientProtocol::sendImp( G::string_view line , std::size_t sensitive_from )
{
	G_ASSERT( line.size() > 2U && line.rfind('\n') == (line.size()-1U) ) ;

	if( m_protocol.state == State::Quitting )
		startTimer( 1U ) ;
	else if( m_config.response_timeout != 0U )
		startTimer( m_config.response_timeout ) ; // response timer on every smtp command

	std::size_t pos = 0U ;
	for( G::StringFieldT<G::string_view> f(line,"\r\n",2U) ; f && !f.last() ; pos += (f.size()+2U) , ++f )
	{
		if( sensitive_from == std::string::npos || (pos+f.size()) < sensitive_from )
			G_LOG( "GSmtp::ClientProtocol: tx>>: "
				"\"" << G::Str::printable(f()) << "\"" ) ;
		else if( pos >= sensitive_from )
			G_LOG( "GSmtp::ClientProtocol: tx>>: [response not logged]" ) ;
		else
			G_LOG( "GSmtp::ClientProtocol: tx>>: "
				"\"" << G::Str::printable(f().substr(0U,sensitive_from-pos)) << " [not logged]\"" ) ;
	}

	return m_sender.protocolSend( line , 0U , false ) ;
}

// ==

GSmtp::ClientProtocolImp::EhloReply::EhloReply( const ClientReply & reply ) :
	m_reply(reply)
{
	G_ASSERT( reply.is(ClientReply::Value::Ok_250) ) ;
}

bool GSmtp::ClientProtocolImp::EhloReply::has( const std::string & option ) const
{
	return m_reply.text().find(std::string(1U,'\n').append(option)) != std::string::npos ; // (eg. "hello\nPIPELINE\n")
}

G::StringArray GSmtp::ClientProtocolImp::EhloReply::values( const std::string & option ) const
{
	G::StringArray result ;
	std::string text = m_reply.text() ; // (eg. "hello\nAUTH FOO\n")
	std::size_t start_pos = text.find( std::string(1U,'\n').append(option).append(1U,' ') ) ;
	if( start_pos != std::string::npos )
	{
		std::size_t end_pos = text.find( '\n' , start_pos+1U ) ;
		std::size_t size = end_pos == std::string::npos ? end_pos : ( end_pos - start_pos ) ;
		result = G::Str::splitIntoTokens( text.substr(start_pos,size) , G::Str::ws() ) ;
		G_ASSERT( result.at(0U) == option ) ;
		if( !result.empty() ) result.erase( result.begin() ) ;
	}
	return result ;
}

// ==

std::size_t GSmtp::ClientProtocol::Protocol::replySize() const
{
	return std::accumulate( reply_lines.begin() , reply_lines.end() , std::size_t(0U) ,
		[](std::size_t n,const std::string& s){return n+s.size();} ) ;
}

// ==

GSmtp::ClientProtocol::Config::Config()
= default;

// ==

GSmtp::ClientProtocolImp::AuthError::AuthError( const GAuth::SaslClient & sasl ,
	const ClientReply & reply ) :
		SmtpError( "authentication failed " + sasl.info() + ": [" + G::Str::printable(reply.text()) + "]" )
{
}

std::string GSmtp::ClientProtocolImp::AuthError::str() const
{
	return std::string( what() ) ;
}

