//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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

struct GSmtp::ClientProtocolImp::AuthError : public ClientProtocol::SmtpError
{
	AuthError( const GAuth::SaslClient & , const ClientProtocol::Reply & ) ;
	std::string str() const ;
} ;

class GSmtp::ClientProtocolImp::EhloReply
{
public:
	explicit EhloReply( const ClientProtocol::Reply & ) ;
	bool has( const std::string & option ) const ;
	G::StringArray values( const std::string & option ) const ;

private:
	ClientProtocol::Reply m_reply ;
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
		m_utf8_warned(false) ,
		m_done_signal(true) // one-shot
{
	m_config.bdat_chunk_size = std::max( std::size_t(64U) , m_config.bdat_chunk_size ) ;
	m_config.reply_size_limit = std::max( std::size_t(100U) , m_config.reply_size_limit ) ;
	m_message_line.reserve( 200U ) ;
}

void GSmtp::ClientProtocol::start( std::weak_ptr<StoredMessage> message_in )
{
	G_DEBUG( "GSmtp::ClientProtocol::start" ) ;

	// reinitialise for the new message
	m_message = MessageState() ;
	m_message.ptr = message_in ; // NOLINT performance-unnecessary-value-param

	// (re)start the protocol
	m_done_signal.reset() ;
	applyEvent( Reply::start() ) ;
}

void GSmtp::ClientProtocol::finish()
{
	G_DEBUG( "GSmtp::ClientProtocol::finish" ) ;
	m_protocol.state = State::Quitting ;
	send( "QUIT\r\n"_sv ) ;
}

void GSmtp::ClientProtocol::secure()
{
	applyEvent( Reply::ok(Reply::Value::Internal_secure) ) ;
}

void GSmtp::ClientProtocol::sendComplete()
{
	if( m_protocol.state == State::Data )
	{
		std::size_t n = sendContentLines() ;

		G_LOG( "GSmtp::ClientProtocol: tx>>: [" << n << " line(s) of content]" ) ;
		if( endOfContent() )
		{
			m_protocol.state = State::SentDot ;
			sendEot() ;
		}
	}
}

G::Slot::Signal<int,const std::string&,const std::string&,const G::StringArray&> & GSmtp::ClientProtocol::doneSignal()
{
	return m_done_signal ;
}

G::Slot::Signal<> & GSmtp::ClientProtocol::filterSignal()
{
	return m_filter_signal ;
}

bool GSmtp::ClientProtocol::apply( const std::string & rx )
{
	G_LOG( "GSmtp::ClientProtocol: rx<<: \"" << G::Str::printable(rx) << "\"" ) ;

	m_protocol.reply_lines.push_back( rx ) ;
	Reply reply( m_protocol.reply_lines ) ;

	G::StringArray save ;
	std::swap( save , m_protocol.reply_lines ) ; // clear

	if( !reply.valid() )
	{
		send( "550 syntax error\r\n"_sv ) ;
		return false ;
	}
	else if( reply.complete() )
	{
		bool done = applyEvent( reply ) ;
		return done ;
	}
	else if( m_protocol.replySize() > m_config.reply_size_limit )
	{
		throw SmtpError( "overflow on input" ) ;
	}
	else
	{
		std::swap( save , m_protocol.reply_lines ) ; // restore
		return false ;
	}
}

bool GSmtp::ClientProtocol::applyEvent( const Reply & reply )
{
	using AuthError = ClientProtocolImp::AuthError ;
	G_DEBUG( "GSmtp::ClientProtocol::applyEvent: " << reply.value() << ": " << G::Str::printable(reply.text()) ) ;

	cancelTimer() ;

	bool protocol_done = false ;
	bool is_start_event = reply.is(Reply::Value::Internal_start) ;
	if( m_protocol.state == State::Init && is_start_event )
	{
		// got start-event -- wait for 220 greeting
		m_protocol.state = State::Started ;
		if( m_config.ready_timeout != 0U )
			startTimer( m_config.ready_timeout ) ;
	}
	else if( m_protocol.state == State::Init && reply.is(Reply::Value::ServiceReady_220) )
	{
		// got greeting before start-event
		G_DEBUG( "GSmtp::ClientProtocol::applyEvent: init -> ready" ) ;
		m_protocol.state = State::ServiceReady ;
	}
	else if( m_protocol.state == State::ServiceReady && is_start_event )
	{
		// got start-event after greeting
		G_DEBUG( "GSmtp::ClientProtocol::applyEvent: ready -> sent-ehlo" ) ;
		if( m_config.lmtp )
		{
			m_protocol.state = State::SentLhlo ;
			sendLhlo() ;
		}
		else
		{
			m_protocol.state = State::SentEhlo ;
			sendEhlo() ;
		}
	}
	else if( m_protocol.state == State::Started && reply.is(Reply::Value::ServiceReady_220) )
	{
		// got greeting after start-event
		G_DEBUG( "GSmtp::ClientProtocol::applyEvent: start -> sent-ehlo" ) ;
		m_protocol.state = State::SentEhlo ;
		sendEhlo() ;
	}
	else if( m_protocol.state == State::MessageDone && is_start_event )
	{
		// new message within the current session, start the client filter
		m_protocol.state = State::Filtering ;
		startFiltering() ;
	}
	else if( m_protocol.state == State::SentEhlo && (
		reply.is(Reply::Value::SyntaxError_500) ||
		reply.is(Reply::Value::SyntaxError_501) ||
		reply.is(Reply::Value::NotImplemented_502) ) )
	{
		// server didn't like EHLO so fall back to HELO
		if( m_config.must_use_tls && !m_in_secure_tunnel )
			throw SmtpError( "tls is mandated but the server cannot do esmtp" ) ;
		m_protocol.state = State::SentHelo ;
		sendHelo() ;
	}
	else if( m_protocol.state == State::SentHelo &&
		m_config.allow_lmtp &&
		reply.type() == Reply::Type::PermanentNegative )
	{
		// server didn't like EHLO or HELO so try LHLO for LMTP
		G_ASSERT( !m_config.must_use_tls || m_in_secure_tunnel ) ;
		m_protocol.state = State::SentLhlo ;
		sendLhlo() ;
	}
	else if( ( m_protocol.state == State::SentEhlo ||
			m_protocol.state == State::SentHelo ||
			m_protocol.state == State::SentTlsEhlo ||
			m_protocol.state == State::SentLhlo ||
			m_protocol.state == State::SentTlsLhlo ) &&
		reply.is(Reply::Value::Ok_250) )
	{
		// hello accepted, start a new session
		G_DEBUG( "GSmtp::ClientProtocol::applyEvent: ehlo reply \"" << G::Str::printable(reply.text()) << "\"" ) ;
		m_protocol.is_lmtp = m_protocol.state == State::SentLhlo || m_protocol.state == State::SentTlsLhlo ;
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
			m_session.secure = m_protocol.state == State::SentTlsEhlo || m_protocol.state == State::SentTlsLhlo || m_in_secure_tunnel ;
		}

		// choose the authentication mechanism
		m_session.auth_mechanism = m_sasl->mechanism( m_session.server.auth_mechanisms ) ;

		// start encryption, authentication or client-filtering
		if( !m_session.secure && m_config.must_use_tls )
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
		else if( m_sasl->active() && m_session.server.has_auth && m_session.auth_mechanism.empty() &&
			m_config.must_authenticate/*new*/ )
		{
			throw SmtpError( "cannot do authentication required by remote server "
				"(" + G::Str::printable(G::Str::join(",",m_session.server.auth_mechanisms)) + "): "
				"check for a compatible client secret" ) ;
		}
		else if( m_sasl->active() && m_session.server.has_auth && !m_session.auth_mechanism.empty() )
		{
			m_protocol.state = State::Auth ;
			send( "AUTH "_sv , m_session.auth_mechanism , initialResponse(*m_sasl) , "\r\n"_sv ) ;
		}
		else if( m_sasl->active() && m_config.must_authenticate )
		{
			throw SmtpError( "authentication is not supported by the remote smtp server" ) ;
		}
		else
		{
			G_ASSERT( !m_sasl->active() || !m_config.must_authenticate ) ;
			m_protocol.state = State::Filtering ;
			startFiltering() ;
		}
	}
	else if( m_protocol.state == State::StartTls && reply.is(Reply::Value::ServiceReady_220) )
	{
		// greeting for new secure session -- start tls handshake
		m_sender.protocolSend( {} , 0U , true ) ;
	}
	else if( m_protocol.state == State::StartTls && reply.is(Reply::Value::NotAvailable_454) )
	{
		// starttls rejected
		throw TlsError( reply.errorText() ) ;
	}
	else if( m_protocol.state == State::StartTls && reply.is(Reply::Value::Internal_secure) )
	{
		// tls session established -- send hello again
		if( m_protocol.is_lmtp )
		{
			m_protocol.state = State::SentTlsLhlo ;
			sendLhlo() ;
		}
		else
		{
			m_protocol.state = State::SentTlsEhlo ;
			sendEhlo() ;
		}
	}
	else if( m_protocol.state == State::Auth && reply.is(Reply::Value::Challenge_334) &&
		( reply.text() == "=" || G::Base64::valid(reply.text()) || m_session.auth_mechanism == "PLAIN" ) )
	{
		// authentication challenge -- send the response
		std::string challenge = G::Base64::valid(reply.text()) ? G::Base64::decode(reply.text()) : std::string() ;
		GAuth::SaslClient::Response rsp = m_sasl->response( m_session.auth_mechanism , challenge ) ;
		if( rsp.error )
			send( "*\r\n"_sv ) ; // expect 501
		else
			sendRsp( rsp ) ;
	}
	else if( m_protocol.state == State::Auth && reply.is(Reply::Value::Challenge_334) )
	{
		// invalid authentication challenge -- send cancel (RFC-4954 p5)
		send( "*\r\n"_sv ) ; // expect 501
	}
	else if( m_protocol.state == State::Auth && reply.positive()/*235*/ )
	{
		// authenticated -- proceed to first message
		m_session.authenticated_with_server = true ;
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
		send( "AUTH "_sv , m_session.auth_mechanism , initialResponse(*m_sasl) , "\r\n"_sv ) ;
	}
	else if( m_protocol.state == State::Auth && !reply.positive() && m_config.must_authenticate )
	{
		// authentication failed and mandatory and no more mechanisms -- abort
		throw AuthError( *m_sasl , reply ) ;
	}
	else if( m_protocol.state == State::Auth && !reply.positive() )
	{
		// authentication failed, but optional -- continue and expect submission errors
		G_ASSERT( !m_session.authenticated_with_server ) ;
		G_WARNING( "GSmtp::ClientProtocol::applyEvent: " << AuthError(*m_sasl,reply).str() << ": continuing" ) ;
		m_protocol.state = State::Filtering ;
		startFiltering() ;
	}
	else if( m_protocol.state == State::Filtering && reply.is(Reply::Value::Internal_filter_abandon) )
	{
		// filter failed with 'abandon' -- finish
		m_protocol.state = State::MessageDone ;
		raiseDoneSignal( -1 , std::string() ) ;
	}
	else if( m_protocol.state == State::Filtering && reply.is(Reply::Value::Internal_filter_error) )
	{
		// filter failed with 'error' -- finish
		m_protocol.state = State::MessageDone ;
		raiseDoneSignal( -2 , reply.errorText() , reply.errorReason() ) ;
	}
	else if( m_protocol.state == State::Filtering && reply.is(Reply::Value::Internal_filter_ok) )
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
	else if( m_protocol.state == State::SentMail && reply.is(Reply::Value::Ok_250) )
	{
		// got reponse to MAIL-FROM -- send first RCPT-TO
		m_protocol.state = State::SentRcpt ;
		sendRcptTo() ;
	}
	else if( m_protocol.state == State::SentRcpt && m_message.to_index < message()->toCount() )
	{
		// got reponse to RCTP-TO and more recipients to go -- send next RCPT-TO
		bool accepted = m_protocol.is_lmtp ? reply.positiveCompletion() : reply.positive() ;
		if( accepted )
			m_message.to_accepted++ ;
		else
			m_message.to_rejected.push_back( message()->to(m_message.to_index-1U) ) ;
		sendRcptTo() ;
	}
	else if( m_protocol.state == State::SentRcpt )
	{
		// got reponse to the last RCTP-TO -- send DATA or BDAT command

		bool accepted = m_protocol.is_lmtp ? reply.positiveCompletion() : reply.positive() ;
		if( accepted )
			m_message.to_accepted++ ;
		else
			m_message.to_rejected.push_back( message()->to(m_message.to_index-1U) ) ;

		if( ( m_config.must_accept_all_recipients && m_message.to_accepted < message()->toCount() ) || m_message.to_accepted == 0U )
		{
			m_protocol.state = State::SentDataStub ;
			send( "RSET\r\n"_sv ) ;
		}
		else if( ( message()->bodyType() == BodyType::BinaryMime || G::Test::enabled("smtp-client-prefer-bdat") ) &&
			m_session.server.has_binarymime && m_session.server.has_chunking )
		{
			// RFC-3030
			m_message.content_size = message()->contentSize() ;
			std::string content_size_str = std::to_string( m_message.content_size ) ;

			bool one_chunk = (m_message.content_size+5U) <= m_config.bdat_chunk_size ; // 5 for " LAST"
			if( one_chunk )
			{
				m_protocol.state = State::SentBdatLast ;
				sendBdatAndChunk( m_message.content_size , content_size_str , true ) ;
			}
			else
			{
				m_protocol.state = State::SentBdatMore ;

				m_message.chunk_data_size = m_config.bdat_chunk_size ;
				m_message.chunk_data_size_str = std::to_string(m_message.chunk_data_size) ;

				bool last = sendBdatAndChunk( m_message.chunk_data_size , m_message.chunk_data_size_str , false ) ;
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
	else if( m_protocol.state == State::SentData && reply.is(Reply::Value::OkForData_354) )
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
		raiseDoneSignal( reply.value() , how_many + " recipients rejected" ) ;
	}
	else if( m_protocol.state == State::SentDot && m_protocol.is_lmtp )
	{
		if( !reply.positive() )
		{
			m_protocol.state = State::MessageDone ;
			raiseDoneSignal( reply.value() , reply.errorText() ) ;
		}
		else
		{
			G_ASSERT( m_message.to_accepted != 0U ) ;
			m_message.to_lmtp_delivered.clear() ;
			m_protocol.state = State::Lmtp ;
		}
	}
	else if( m_protocol.state == State::SentBdatMore )
	{
		// got response to BDAT chunk -- send the next chunk
		if( reply.positive() )
		{
			bool last = sendBdatAndChunk( m_message.chunk_data_size , m_message.chunk_data_size_str , false ) ;
			if( last )
				m_protocol.state = State::SentBdatLast ;
		}
		else
		{
			raiseDoneSignal( reply.value() , reply.errorText() ) ;
		}
	}
	else if( m_protocol.state == State::SentDot || m_protocol.state == State::SentBdatLast )
	{
		// got response to DATA EOT or BDAT LAST -- finish
		m_protocol.state = State::MessageDone ;
		m_message_line.clear() ;
		m_message_buffer.clear() ;
		if( reply.positive() && m_message.to_accepted < message()->toCount() )
			raiseDoneSignal( 0 , "one or more recipients rejected" ) ;
		else
			raiseDoneSignal( reply.value() , reply.errorText() ) ;
	}
	else if( m_protocol.state == State::Lmtp )
	{
		m_message.to_lmtp_delivered.push_back( reply.positiveCompletion() ) ;
		if( m_message.to_lmtp_delivered.size() == m_message.to_accepted )
		{
			m_protocol.state = State::MessageDone ;
			raiseDoneSignal( 0 , std::string() ) ;
		}
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

std::shared_ptr<GSmtp::StoredMessage> GSmtp::ClientProtocol::message()
{
	G_ASSERT( !m_message.ptr.expired() ) ;
	if( m_message.ptr.expired() )
		return std::make_shared<StoredMessageStub>() ;

	return m_message.ptr.lock() ;
}

std::string GSmtp::ClientProtocol::initialResponse( const GAuth::SaslClient & sasl )
{
	std::string rsp = sasl.initialResponse( 450U ) ; // RFC-2821 total command line length of 512
	return rsp.empty() ? rsp : ( " " + G::Base64::encode(rsp) ) ;
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

void GSmtp::ClientProtocol::raiseDoneSignal( int response_code , const std::string & response ,
	const std::string & reason )
{
	if( !response.empty() && response_code == 0 )
		G_WARNING( "GSmtp::ClientProtocol: smtp client protocol: " << response ) ;

	cancelTimer() ;
	m_done_signal.emit( response_code , std::string(response) , std::string(reason) , G::StringArray(m_message.to_rejected) ) ;
}

bool GSmtp::ClientProtocol::endOfContent()
{
	return !message()->contentStream().good() ;
}

std::string GSmtp::ClientProtocol::checkSendable()
{
	const bool eightbitmime_mismatch =
		message()->bodyType() == BodyType::EightBitMime &&
		!m_session.server.has_8bitmime ;

	const bool utf8_mismatch =
		message()->utf8Mailboxes() &&
		!m_session.server.has_smtputf8 ;

	const bool binarymime_mismatch =
		message()->bodyType() == BodyType::BinaryMime &&
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
	else if( utf8_mismatch && m_config.utf8_strict )
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
	std::string mail_from_tail = message()->from() ;
	mail_from_tail.append( 1U , '>' ) ;

	if( message()->bodyType() == BodyType::SevenBit )
	{
		if( m_session.server.has_8bitmime )
			mail_from_tail.append( " BODY=7BIT" ) ; // RFC-6152
	}
	else if( message()->bodyType() == BodyType::EightBitMime )
	{
		if( m_session.server.has_8bitmime )
			mail_from_tail.append( " BODY=8BITMIME" ) ; // RFC-6152
	}
	else if( message()->bodyType() == BodyType::BinaryMime )
	{
		if( m_session.server.has_binarymime && m_session.server.has_chunking )
		{
			mail_from_tail.append( " BODY=BINARYMIME" ) ; // RFC-3030
			use_bdat = true ;
		}
		else if( m_session.server.has_8bitmime )
		{
			mail_from_tail.append( " BODY=8BITMIME" ) ; // hmm TODO revisit
		}
	}

	if( m_session.server.has_smtputf8 && message()->utf8Mailboxes() )
	{
		mail_from_tail.append( " SMTPUTF8" ) ; // RFC-6531 3.4
	}

	if( m_session.authenticated_with_server && message()->fromAuthOut().empty() && !m_sasl->id().empty() )
	{
		// default policy is to use the session authentication id, although
		// this is not strictly conforming with RFC-2554/RFC-4954
		mail_from_tail.append( " AUTH=" ) ;
		mail_from_tail.append( G::Xtext::encode(m_sasl->id()) ) ;
	}
	else if( m_session.authenticated_with_server && G::Xtext::valid(message()->fromAuthOut()) )
	{
		mail_from_tail.append( " AUTH=" ) ;
		mail_from_tail.append( message()->fromAuthOut() ) ;
	}
	else if( m_session.authenticated_with_server )
	{
		mail_from_tail.append( " AUTH=<>" ) ;
	}
	mail_from_tail.append( "\r\n" , 2U ) ;

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
		commands.append("MAIL FROM:<").append(mail_from_tail) ;
		const std::size_t n = message()->toCount() ;
		for( std::size_t i = 0U ; i < n ; i++ )
			commands.append("RCPT TO:<").append(message()->to(i)).append(">\r\n",3U) ;
		m_message.to_index = 0 ;
		sendCommandLines( commands ) ;
	}
	else
	{
		send( "MAIL FROM:<"_sv , mail_from_tail ) ;
	}
	return use_bdat ;
}

void GSmtp::ClientProtocol::sendRcptTo()
{
	if( m_config.pipelining && m_session.server.has_pipelining )
	{
		m_message.to_index++ ;
	}
	else
	{
		G_ASSERT( m_message.to_index < message()->toCount() ) ;
		std::string to = message()->to( m_message.to_index++ ) ;
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
	G_ASSERT( !line.empty() && line.at(0) == '.' ) ;
	line.erase( 1U ) ; // leave "."

	bool ok = false ;
	if( message()->contentStream().good() )
	{
		std::istream & stream = message()->contentStream() ;
		const bool pre_erase = false ;
		G::Str::readLineFrom( stream , "\n"_sv , line , pre_erase ) ;
		G_ASSERT( line.size() >= 1U && line.at(0U) == '.' ) ;

		if( !stream.fail() )
		{
			// read file wrt. lf -- send with cr-lf
			if( !line.empty() && line.at(line.size()-1U) != '\r' )
				line.append( 1U , '\r' ) ; // moot
			line.append( 1U , '\n' ) ;

			ok = sendContentLineImp( line , line.at(1U) == '.' ? 0U : 1U ) ;
		}
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

void GSmtp::ClientProtocol::sendLhlo()
{
	send( "LHLO "_sv , m_config.thishost_name , "\r\n"_sv ) ;
}

void GSmtp::ClientProtocol::sendEot()
{
	sendImp( ".\r\n"_sv , false ) ;
}

void GSmtp::ClientProtocol::sendRsp( const GAuth::SaslClient::Response & rsp )
{
	std::string s = G::Base64::encode(rsp.data).append("\r\n",2U) ;
	sendImp( {s.data(),s.size()} , rsp.sensitive ) ;
}

void GSmtp::ClientProtocol::sendCommandLines( const std::string & lines )
{
	sendImp( {lines.data(),lines.size()} , false ) ;
}

void GSmtp::ClientProtocol::send( G::string_view s )
{
	sendImp( s , false ) ;
}

void GSmtp::ClientProtocol::send( G::string_view s0 , const std::string & s1 , G::string_view s2 )
{
	std::string line = std::string(s0.data(),s0.size()).append(s1).append(s2.data(),s2.size()) ;
	sendImp( {line.data(),line.size()} , false ) ;
}

void GSmtp::ClientProtocol::send( G::string_view s0 , const std::string & s1 , const std::string & s2 , G::string_view s3 )
{
	std::string line = std::string(s0.data(),s0.size()).append(s1).append(s2).append(s3.data(),s3.size()) ;
	sendImp( {line.data(),line.size()} , false ) ;
}

void GSmtp::ClientProtocol::send( G::string_view s0 , G::string_view s1 , G::string_view s2 , G::string_view s3 )
{
	std::string line = std::string(s0.data(),s0.size()).append(s1.data(),s1.size()).append(s2.data(),s2.size()).append(s3.data(),s3.size()) ;
	sendImp( {line.data(),line.size()} , false ) ;
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

	std::memcpy( out , "BDAT " , 5U ) ;
	std::memcpy( out+5U , size_str.data() , size_str.size() ) ;
	if( last )
		std::memcpy( out+5U+size_str.size() , " LAST" , 5U ) ;
	std::memcpy( out+eolpos , "\r\n" , 2U ) ;

	G_ASSERT( buffer_size > datapos ) ;
	G_ASSERT( (out+datapos) < (&m_message_buffer[0]+m_message_buffer.size()) ) ;
	message()->contentStream().read( out+datapos , buffer_size-datapos ) ;
	std::streamsize gcount = message()->contentStream().gcount() ;

	G_ASSERT( gcount >= 0 ) ;
	static_assert( sizeof(std::streamsize) == sizeof(std::size_t) , "" ) ;
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
		std::memcpy( out , "BDAT " , 5U ) ;
		std::memcpy( out+5U , n.data() , n.size() ) ;
		std::memcpy( out+5U+n.size(), " LAST\r\n" , 7U ) ;
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

	if( G::LogOutput::instance() && G::LogOutput::instance()->at(G::Log::Severity::InfoVerbose) )
	{
		std::size_t pos = sv.find( "\r\n"_sv ) ;
		G::string_view cmd = G::Str::head( sv , pos , {p,std::size_t(0U)} ) ;
		G::StringTokenT<G::string_view> t( cmd , " "_sv ) ;
		G::string_view count = t.next()() ;
		G::string_view end = count.size() == 1U && count[0] == '1' ? "]"_sv : "s]"_sv ;
		G_LOG( "GSmtp::ClientProtocol: tx>>: \"" << cmd << "\" [" << count << " byte" << end ) ;
	}

	m_sender.protocolSend( sv , 0U , false ) ;
}

bool GSmtp::ClientProtocol::sendContentLineImp( const std::string & line , std::size_t offset )
{
	bool all_sent = m_sender.protocolSend( {line.data(),line.size()} , offset , false ) ;
	if( !all_sent && m_config.response_timeout != 0U )
		startTimer( m_config.response_timeout ) ; // response timer while blocked by flow-control
	return all_sent ;
}

bool GSmtp::ClientProtocol::sendImp( G::string_view line , bool sensitive )
{
	if( m_protocol.state == State::Quitting )
		startTimer( 1U ) ;
	else if( m_config.response_timeout != 0U )
		startTimer( m_config.response_timeout ) ; // response timer on every smtp command

	if( G::LogOutput::instance() && G::LogOutput::instance()->at(G::Log::Severity::InfoVerbose) )
	{
		if( sensitive )
		{
			G_LOG( "GSmtp::ClientProtocol: tx>>: [response not logged]" ) ;
		}
		else if( line.find("\r\n",0U,2U) != std::string::npos )
		{
			for( G::StringFieldT<G::string_view> f(line,"\r\n",2U) ; f && !f.last() ; ++f )
			{
				G_LOG( "GSmtp::ClientProtocol: tx>>: "
					"\"" << G::Str::printable(G::string_view(f.data(),f.size())) << "\"" ) ;
			}
		}
		else
		{
			G_LOG( "GSmtp::ClientProtocol: tx>>: \"" << G::Str::printable(line) << "\"" ) ;
		}
	}

	return m_sender.protocolSend( line , 0U , false ) ;
}

// ==

GSmtp::ClientProtocolImp::EhloReply::EhloReply( const ClientProtocol::Reply & reply ) :
	m_reply(reply)
{
	G_ASSERT( reply.is(ClientProtocol::Reply::Value::Ok_250) ) ;
}

bool GSmtp::ClientProtocolImp::EhloReply::has( const std::string & option ) const
{
	return m_reply.text().find("\n"+option) != std::string::npos ; // (eg. "hello\nPIPELINE\n")
}

G::StringArray GSmtp::ClientProtocolImp::EhloReply::values( const std::string & option ) const
{
	G::StringArray result ;
	std::string text = m_reply.text() ; // (eg. "hello\nAUTH FOO\n")
	std::size_t start_pos = text.find( "\n" + option + " " ) ;
	std::size_t end_pos = start_pos == std::string::npos ? start_pos : text.find('\n',start_pos+1U) ;
	if( end_pos != std::string::npos )
	{
		result = G::Str::splitIntoTokens( text.substr(start_pos,end_pos-start_pos) , G::Str::ws() ) ;
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
	const GSmtp::ClientProtocol::Reply & reply ) :
		SmtpError( "authentication failed " + sasl.info() + ": [" + G::Str::printable(reply.text()) + "]" )
{
}

std::string GSmtp::ClientProtocolImp::AuthError::str() const
{
	return std::string( what() ) ;
}

