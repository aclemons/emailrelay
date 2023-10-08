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
/// \file gsmtpserverprotocol.cpp
///

#include "gdef.h"
#include "gsmtpserverprotocol.h"
#include "gsaslserverfactory.h"
#include "gsocketprotocol.h"
#include "gxtext.h"
#include "glocal.h"
#include "gbase64.h"
#include "gdate.h"
#include "gtime.h"
#include "gdatetime.h"
#include "gscope.h"
#include "gstr.h"
#include "gstringfield.h"
#include "gstringtoken.h"
#include "glog.h"
#include "gtest.h"
#include "gassert.h"
#include <string>
#include <tuple>

std::unique_ptr<GAuth::SaslServer> GSmtp::ServerProtocol::newSaslServer( const GAuth::SaslServerSecrets & secrets ,
	const std::string & sasl_config , const std::string & challenge_hostname )
{
	bool with_apop = false ;
	return GAuth::SaslServerFactory::newSaslServer( secrets , with_apop , sasl_config , challenge_hostname ) ;
}

GSmtp::ServerProtocol::ServerProtocol( ServerSender & sender , Verifier & verifier ,
	ProtocolMessage & pm , const GAuth::SaslServerSecrets & secrets ,
	Text & text , const GNet::Address & peer_address , const Config & config ,
	bool enabled ) :
		ServerSend(&sender) ,
		m_sender(&sender) ,
		m_verifier(verifier) ,
		m_text(text) ,
		m_pm(pm) ,
		m_sasl(newSaslServer(secrets,config.sasl_server_config,config.sasl_server_challenge_hostname)) ,
		m_config(config) ,
		m_apply_data(nullptr) ,
		m_apply_more(false) ,
		m_fsm(State::Start,State::End,State::s_Same,State::s_Any) ,
		m_with_starttls(false) ,
		m_peer_address(peer_address) ,
		m_secure(false) ,
		m_client_error_count(0U) ,
		m_session_esmtp(false) ,
		m_bdat_arg(0U) ,
		m_bdat_sum(0U) ,
		m_enabled(enabled)
{
	m_fsm( Event::Quit , State::s_Any , State::End , &ServerProtocol::doQuit ) ;
	m_fsm( Event::Unknown , State::Processing , State::s_Same , &ServerProtocol::doIgnore ) ;
	m_fsm( Event::Unknown , State::s_Any , State::s_Same , &ServerProtocol::doUnknown ) ;
	m_fsm( Event::Rset , State::Start , State::s_Same , &ServerProtocol::doRset ) ;
	m_fsm( Event::Rset , State::s_Any , State::Idle , &ServerProtocol::doRset ) ;
	m_fsm( Event::Noop , State::s_Any , State::s_Same , &ServerProtocol::doNoop ) ;
	m_fsm( Event::Help , State::s_Any , State::s_Same , &ServerProtocol::doHelp ) ;
	m_fsm( Event::Expn , State::s_Any , State::s_Same , &ServerProtocol::doExpn ) ;
	m_fsm( Event::Vrfy , State::Start , State::VrfyStart , &ServerProtocol::doVrfy , State::s_Same ) ;
	m_fsm( Event::VrfyReply , State::VrfyStart , State::Start , &ServerProtocol::doVrfyReply ) ;
	m_fsm( Event::Vrfy , State::Idle , State::VrfyIdle , &ServerProtocol::doVrfy , State::s_Same ) ;
	m_fsm( Event::VrfyReply , State::VrfyIdle , State::Idle , &ServerProtocol::doVrfyReply ) ;
	m_fsm( Event::Vrfy , State::GotMail , State::VrfyGotMail, &ServerProtocol::doVrfy , State::s_Same ) ;
	m_fsm( Event::VrfyReply , State::VrfyGotMail, State::GotMail , &ServerProtocol::doVrfyReply ) ;
	m_fsm( Event::Vrfy , State::GotRcpt , State::VrfyGotRcpt, &ServerProtocol::doVrfy , State::s_Same ) ;
	m_fsm( Event::VrfyReply , State::VrfyGotRcpt, State::GotRcpt , &ServerProtocol::doVrfyReply ) ;
	m_fsm( Event::Ehlo , State::s_Any , State::Idle , &ServerProtocol::doEhlo , State::s_Same ) ;
	m_fsm( Event::Helo , State::s_Any , State::Idle , &ServerProtocol::doHelo , State::s_Same ) ;
	m_fsm( Event::Mail , State::Idle , State::GotMail , &ServerProtocol::doMail , State::Idle ) ;
	m_fsm( Event::Rcpt , State::GotMail , State::RcptTo1 , &ServerProtocol::doRcpt , State::s_Same ) ;
	m_fsm( Event::RcptReply , State::RcptTo1 , State::GotRcpt , &ServerProtocol::doRcptToReply , State::GotMail ) ;
	m_fsm( Event::Rcpt , State::GotRcpt , State::RcptTo2 , &ServerProtocol::doRcpt , State::s_Same ) ;
	m_fsm( Event::RcptReply , State::RcptTo2 , State::GotRcpt , &ServerProtocol::doRcptToReply ) ;
	m_fsm( Event::DataFail , State::GotMail , State::MustReset , &ServerProtocol::doBadDataCommand ) ;
	m_fsm( Event::DataFail , State::GotRcpt , State::MustReset , &ServerProtocol::doBadDataCommand ) ;
	m_fsm( Event::Data , State::GotMail , State::Idle , &ServerProtocol::doNoRecipients ) ;
	m_fsm( Event::Data , State::GotRcpt , State::Data , &ServerProtocol::doData ) ;
	m_fsm( Event::DataContent , State::Data , State::Data , &ServerProtocol::doDataContent ) ;
	m_fsm( Event::Bdat , State::Idle , State::MustReset , &ServerProtocol::doBdatOutOfSequence ) ;
	m_fsm( Event::Bdat , State::GotMail , State::Idle , &ServerProtocol::doNoRecipients ) ; // 1
	m_fsm( Event::BdatLast , State::GotMail , State::Idle , &ServerProtocol::doNoRecipients ) ; // 2
	m_fsm( Event::BdatLastZero , State::GotMail , State::Idle , &ServerProtocol::doNoRecipients ) ; // 3
	m_fsm( Event::Bdat , State::GotRcpt , State::BdatData , &ServerProtocol::doBdatFirst , State::MustReset ) ; // 4
	m_fsm( Event::BdatLast , State::GotRcpt , State::BdatDataLast , &ServerProtocol::doBdatFirstLast , State::MustReset ) ; // 5
	m_fsm( Event::BdatLastZero , State::GotRcpt , State::BdatChecking , &ServerProtocol::doBdatFirstLastZero ) ; // 6
	m_fsm( Event::BdatContent , State::BdatData , State::BdatIdle , &ServerProtocol::doBdatContent , State::BdatData ) ; // 7
	m_fsm( Event::Bdat , State::BdatIdle , State::BdatData , &ServerProtocol::doBdatMore , State::MustReset ) ; // 8
	m_fsm( Event::BdatLast , State::BdatIdle , State::BdatDataLast , &ServerProtocol::doBdatMoreLast , State::MustReset ) ; // 9
	m_fsm( Event::BdatLastZero , State::BdatIdle , State::BdatChecking , &ServerProtocol::doBdatMoreLastZero ) ; // 10
	m_fsm( Event::BdatContent , State::BdatDataLast , State::BdatChecking , &ServerProtocol::doBdatContentLast , State::BdatDataLast ) ;//11
	m_fsm( Event::BdatCheck , State::BdatChecking , State::BdatProcessing , &ServerProtocol::doBdatCheck , State::Idle ) ; //12
	m_fsm( Event::Done , State::BdatProcessing , State::Idle , &ServerProtocol::doBdatComplete ) ; // 13
	m_fsm( Event::Eot , State::Data , State::Processing , &ServerProtocol::doEot , State::Idle ) ;
	m_fsm( Event::Done , State::Processing , State::Idle , &ServerProtocol::doComplete ) ;
	m_fsm( Event::Auth , State::Idle , State::Auth , &ServerProtocol::doAuth , State::Idle ) ;
	m_fsm( Event::AuthData, State::Auth , State::Auth , &ServerProtocol::doAuthData , State::Idle ) ;
	if( m_config.tls_starttls )
	{
		m_with_starttls = true ;
		m_fsm( Event::StartTls , State::Idle , State::StartingTls , &ServerProtocol::doStartTls , State::Idle ) ;
		m_fsm( Event::Secure , State::StartingTls , State::Idle , &ServerProtocol::doSecure ) ;
	}
	else if( m_config.tls_connection )
	{
		m_fsm.reset( State::StartingTls ) ;
		m_fsm( Event::Secure , State::StartingTls , State::Start , &ServerProtocol::doSecureGreeting ) ;
	}
	m_verifier.doneSignal().connect( G::Slot::slot(*this,&ServerProtocol::verifyDone) ) ;
	m_pm.processedSignal().connect( G::Slot::slot(*this,&ServerProtocol::protocolMessageProcessed) ) ;
}

GSmtp::ServerProtocol::~ServerProtocol()
{
	m_pm.processedSignal().disconnect() ;
	m_verifier.doneSignal().disconnect() ;
}

G::Slot::Signal<> & GSmtp::ServerProtocol::changeSignal() noexcept
{
	return m_change_signal ;
}

bool GSmtp::ServerProtocol::inBusyState() const
{
	// return true if waiting for an asynchronous filter
	// or verifier completion event
	return
		// states expecting Event::Done...
		m_fsm.state() == State::Processing ||
		// states expecting Event::VrfyReply...
		m_fsm.state() == State::VrfyStart ||
		m_fsm.state() == State::VrfyIdle ||
		m_fsm.state() == State::VrfyGotMail ||
		m_fsm.state() == State::VrfyGotRcpt ||
		m_fsm.state() == State::RcptTo1 ||
		m_fsm.state() == State::RcptTo2 ;
}

#ifndef G_LIB_SMALL
bool GSmtp::ServerProtocol::rcptState() const
{
	return
		m_fsm.state() == State::RcptTo1 ||
		m_fsm.state() == State::RcptTo2 ;
}
#endif

bool GSmtp::ServerProtocol::sendFlush() const
{
	// the return value is currently ignored by GSmtp::ServerPeer::protocolSend() ...

	// always flush if no pipelining
	if( !m_session_esmtp || !m_config.with_pipelining )
		return true ;

	// flush at the end of the input batch
	if( !m_apply_more )
		return true ;

	// don't flush after RSET, MAIL-FROM, RCPT-TO, <EOT>, BDAT[!last]
	// RFC-2920 (pipelining) 3.2 (2) (5) (6)
	// RFC-3030 (chunking) 4.2
	Event e = m_fsm.event() ;
	bool batch =
		e == Event::Rset ||
		e == Event::Rcpt ||
		e == Event::RcptReply ||
		e == Event::Mail ||
		e == Event::Done ||
		e == Event::Bdat ;
	return !batch ;
}

bool GSmtp::ServerProtocol::inDataState() const
{
	return
		m_fsm.state() == State::Data ||
		m_fsm.state() == State::BdatData ||
		m_fsm.state() == State::BdatDataLast ;
}

#ifndef G_LIB_SMALL
void GSmtp::ServerProtocol::setSender( ServerSender & sender )
{
	ServerSend::setSender( &sender ) ;
	m_sender = &sender ;
}
#endif

void GSmtp::ServerProtocol::init()
{
	if( m_config.tls_connection )
		m_sender->protocolSecure() ;
	else
		sendGreeting( m_text.greeting() , m_enabled ) ;
}

void GSmtp::ServerProtocol::applyEvent( Event event , EventData event_data )
{
	State new_state = m_fsm.apply( *this , event , event_data ) ;
	if( new_state == State::s_Any )
		throw Done( "protocol error" ) ;
}

void GSmtp::ServerProtocol::secure( const std::string & certificate ,
	const std::string & protocol , const std::string & cipher )
{
	m_certificate = certificate ;
	m_protocol = protocol ;
	m_cipher = cipher ;

	applyEvent( Event::Secure ) ;
}

void GSmtp::ServerProtocol::doSecure( EventData , bool & )
{
	G_DEBUG( "GSmtp::ServerProtocol::doSecure" ) ;
	m_secure = true ;
}

void GSmtp::ServerProtocol::doSecureGreeting( EventData , bool & )
{
	m_secure = true ;
	sendGreeting( m_text.greeting() , m_enabled ) ;
}

void GSmtp::ServerProtocol::doStartTls( EventData , bool & ok )
{
	if( m_secure )
	{
		sendOutOfSequence() ;
		badClientEvent() ;
		ok = false ;
	}
	else
	{
		sendReadyForTls() ;
	}
}

bool GSmtp::ServerProtocol::apply( const ApplyArgsTuple & args )
{
	G_ASSERT( std::get<2>(args) == 2U || ( inDataState() && std::get<2>(args) == 0U ) ) ; // eolsize 0 or 2
	G_DEBUG( "GSmtp::ServerProtocol::apply: apply "
		"[" << G::Str::printable(G::sv_substr(G::string_view(std::get<0>(args),std::get<1>(args)),0U,10U))
		<< (std::get<1>(args)>10U?"...":"") << "] "
		"state=" << static_cast<int>(m_fsm.state()) << " "
		"more=" << std::get<5>(args) << " "
		"busy=" << inBusyState() ) ;

	// refuse if we are currently busy with asynchronous work
	if( inBusyState() )
		throw Busy() ;

	// squirrel away the line buffer state
	m_apply_data = &args ;
	m_apply_more = std::get<5>( args ) ;
	G::ScopeExit clear_on_return( [&](){ m_apply_data = nullptr ; } ) ;

	// always emit a change signal
	G::ScopeExit emit_on_return( [&](){ m_change_signal.emit() ; } ) ;

	// the event data passed via the state machine is a string-view
	// pointing at the apply()ed data -- this is converted to a
	// string only if it is known to be a SMTP command
	EventData event_data( std::get<0>(args) , std::get<1>(args) ) ;

	// parse the command into an event enum
	Event event = Event::Unknown ;
	State state = m_fsm.state() ;
	if( state == State::Data && isEndOfText(args) )
	{
		event = Event::Eot ;
	}
	else if( state == State::Data )
	{
		event = Event::DataContent ;
	}
	else if( state == State::BdatData || state == State::BdatDataLast )
	{
		event = Event::BdatContent ;
	}
	else if( state == State::Auth )
	{
		event = Event::AuthData ;
	}
	else
	{
		G_LOG( "GSmtp::ServerProtocol: rx<<: \"" << G::Str::printable(str(event_data)) << "\"" ) ;
		event = commandEvent( event_data ) ;
	}

	// apply the event to the state-machine
	State new_state = m_fsm.apply( *this , event , event_data ) ;
	if( new_state == State::s_Any )
	{
		sendOutOfSequence() ;
		badClientEvent() ;
	}

	// return false if we are now busy with asynchronous work
	return !inBusyState() ;
}

void GSmtp::ServerProtocol::doDataContent( EventData , bool & )
{
	G_ASSERT( m_apply_data != nullptr ) ;
	if( m_apply_data == nullptr ) throw Done( "protocol error" ) ;

	const char * ptr = std::get<0>( *m_apply_data ) ;
	std::size_t size = std::get<1>( *m_apply_data ) ;
	std::size_t eolsize = std::get<2>( *m_apply_data ) ;

	if( isEscaped(*m_apply_data) )
		m_pm.addContent( ptr+1 , size+eolsize-1U ) ;
	else
		m_pm.addContent( ptr , size+eolsize ) ;

	// ignore addContent() errors here -- use addContent(0) at the end to check
}

void GSmtp::ServerProtocol::doEot( EventData , bool & ok )
{
	G_LOG( "GSmtp::ServerProtocol: rx<<: [message content not logged]" ) ;
	G_LOG( "GSmtp::ServerProtocol: rx<<: \".\"" ) ;

	if( messageAddContentFailed() )
	{
		ok = false ;
		clear() ;
		sendFailed() ;
	}
	else if( messageAddContentTooBig() )
	{
		ok = false ;
		clear() ;
		sendTooBig() ;
	}
	else
	{
		m_pm.process( m_sasl->id() , m_peer_address.hostPartString() , m_certificate ) ;
	}
}

void GSmtp::ServerProtocol::protocolMessageProcessed( const ProtocolMessage::ProcessedInfo & info )
{
	G_ASSERT( info.response.find_first_of("\r\t",0U,2U) == std::string::npos ) ;
	G_ASSERT( info.success == info.response.empty() ) ;
	G_ASSERT( info.response_code >= 0 ) ;
	G_DEBUG( "GSmtp::ServerProtocol::protocolMessageProcessed: ok=" << (info.success?1:0) << " msgid=" << info.id.str()
		<< " rc=" << info.response_code << " rsp=[" << info.response << "] reason=[" << info.reason << "]" ) ;

	std::string response = info.response ;
	G::Str::replace( response , '\n' , ' ' ) ;
	if( !info.success )
		response
			.append(1U,'\t')
			.append(G::Str::fromInt((info.response_code<400||info.response_code>=600)?452:info.response_code)) ;

	applyEvent( Event::Done , {response.data(),response.size()} ) ;
	m_change_signal.emit() ;
}

void GSmtp::ServerProtocol::doComplete( EventData event_data , bool & )
{
	clear() ;
	sendCompletionReply( event_data.empty() , code(event_data) , str(event_data) ) ;
}

void GSmtp::ServerProtocol::doQuit( EventData , bool & )
{
	clear() ;
	sendQuitOk() ;
	m_sender->protocolShutdown( m_config.shutdown_how_on_quit ) ;
	throw Done() ;
}

void GSmtp::ServerProtocol::doBadDataCommand( EventData , bool & )
{
	sendBadDataOutOfSequence() ; // RFC-3030 p6
	badClientEvent() ;
}

void GSmtp::ServerProtocol::doBdatOutOfSequence( EventData , bool & )
{
	sendOutOfSequence() ; // RFC-3030 p4 para 2
	badClientEvent() ;
}

void GSmtp::ServerProtocol::doBdatFirst( EventData event_data , bool & ok )
{
	doBdatImp( event_data , ok , true , false , false ) ;
}

void GSmtp::ServerProtocol::doBdatFirstLast( EventData event_data , bool & ok )
{
	doBdatImp( event_data , ok , true , true , false ) ;
}

void GSmtp::ServerProtocol::doBdatFirstLastZero( EventData event_data , bool & ok )
{
	doBdatImp( event_data , ok , true , true , true ) ;
}

void GSmtp::ServerProtocol::doBdatMore( EventData event_data , bool & ok )
{
	doBdatImp( event_data , ok , false , false , false ) ;
}

void GSmtp::ServerProtocol::doBdatMoreLast( EventData event_data , bool & ok )
{
	doBdatImp( event_data , ok , false , true , false ) ;
}

void GSmtp::ServerProtocol::doBdatMoreLastZero( EventData event_data , bool & ok )
{
	doBdatImp( event_data , ok , false , true , true ) ;
}

void GSmtp::ServerProtocol::doBdatImp( G::string_view bdat_line , bool & ok , bool first , bool last , bool zero )
{
	G_ASSERT( !zero || last ) ;
	if( first )
	{
		std::string received_line = m_text.received( m_session_peer_name , m_sasl->authenticated() ,
			m_secure , m_protocol , m_cipher ) ;
		if( received_line.length() )
			m_pm.addReceived( received_line ) ;
	}

	if( last && zero )
	{
		if( messageAddContentFailed() )
		{
			ok = false ;
			clear() ;
			sendFailed() ;
		}
		else if( messageAddContentTooBig() )
		{
			ok = false ;
			clear() ;
			sendTooBig() ;
		}
		else
		{
			applyEvent( Event::BdatCheck ) ;
		}
	}
	else
	{
		auto pair = parseBdatSize( bdat_line ) ;
		std::size_t bdat_size = pair.first ;
		bool bdat_ok = pair.second ;
		if( !bdat_ok || ( bdat_size == 0U && !last ) )
		{
			G_DEBUG( "GSmtp::ServerProtocol::doBdatImp: bad bdat command: ok=" << bdat_ok << " size=" << bdat_size << " last=" << last ) ;
			ok = false ;
			sendInvalidArgument() ;
		}
		else
		{
			m_bdat_arg = bdat_size ;
			m_bdat_sum = 0U ;
			m_sender->protocolExpect( m_bdat_arg ) ;
		}
	}
}

void GSmtp::ServerProtocol::doBdatContent( EventData , bool & complete )
{
	G_ASSERT( m_apply_data != nullptr ) ;
	if( m_apply_data == nullptr ) throw Done( "protocol error" ) ;

	const char * ptr = std::get<0>( *m_apply_data ) ;
	std::size_t size = std::get<1>( *m_apply_data ) ;
	std::size_t eolsize = std::get<2>( *m_apply_data ) ;

	G_ASSERT( eolsize == 0U ) ; // GNet::LineBuffer::expect()
	G_ASSERT( (m_bdat_sum+size+eolsize) <= m_bdat_arg ) ;

	std::size_t fullsize = size + eolsize ;
	m_bdat_sum += fullsize ;
	complete = m_bdat_sum >= m_bdat_arg ;

	G_DEBUG( "GSmtp::ServerProtocol: rx<<: [" << fullsize << " bytes (" << m_bdat_sum << "/" << m_bdat_arg << ")]" ) ;
	m_pm.addContent( ptr , fullsize ) ;

	if( complete )
	{
		std::ostringstream ss ;
		ss << m_bdat_sum << " bytes received" ;
		sendOk( ss.str() ) ;
	}
}

void GSmtp::ServerProtocol::doBdatContentLast( EventData , bool & complete )
{
	G_ASSERT( m_apply_data != nullptr ) ;
	if( m_apply_data == nullptr ) throw Done( "protocol error" ) ;

	const char * ptr = std::get<0>( *m_apply_data ) ;
	std::size_t size = std::get<1>( *m_apply_data ) ;
	std::size_t eolsize = std::get<2>( *m_apply_data ) ;

	G_ASSERT( (m_bdat_sum+size+eolsize) <= m_bdat_arg ) ;

	std::size_t fullsize = size + eolsize ;
	m_bdat_sum += fullsize ;
	complete = m_bdat_sum >= m_bdat_arg ;

	G_DEBUG( "GSmtp::ServerProtocol: rx<<: [" << fullsize << " bytes (" << m_bdat_sum << "/" << m_bdat_arg << ")]" ) ;
	m_pm.addContent( ptr , fullsize ) ;

	if( complete )
	{
		applyEvent( Event::BdatCheck ) ;
	}
}

void GSmtp::ServerProtocol::doBdatCheck( EventData , bool & ok )
{
	if( messageAddContentFailed() )
	{
		ok = false ;
		clear() ;
		sendFailed() ;
	}
	else if( messageAddContentTooBig() )
	{
		ok = false ;
		clear() ;
		sendTooBig() ;
	}
	else
	{
		m_pm.process( m_sasl->id() , m_peer_address.hostPartString() , m_certificate ) ;
	}
}

bool GSmtp::ServerProtocol::messageAddContentFailed()
{
	bool failed = m_pm.addContent( nullptr , 0U ) == GStore::NewMessage::Status::Error ;
	if( failed )
		G_WARNING( "GSmtp::ServerProtocol::messageAddContentFailed: failed to save message content" ) ;
	return failed ;
}

bool GSmtp::ServerProtocol::messageAddContentTooBig()
{
	bool too_big = m_pm.addContent( nullptr , 0U ) == GStore::NewMessage::Status::TooBig ;
	if( too_big )
		G_WARNING( "GSmtp::ServerProtocol::messageAddContentTooBig: message content too big" ) ;
	return too_big ;
}

void GSmtp::ServerProtocol::doBdatComplete( EventData event_data , bool & )
{
	clear() ;
	sendCompletionReply( event_data.empty() , code(event_data) , str(event_data) ) ;
}

void GSmtp::ServerProtocol::doIgnore( EventData , bool & )
{
}

void GSmtp::ServerProtocol::doNoop( EventData , bool & )
{
	sendOk( "noop" ) ;
}

#ifndef G_LIB_SMALL
void GSmtp::ServerProtocol::doNothing( EventData , bool & )
{
}
#endif

void GSmtp::ServerProtocol::doExpn( EventData , bool & )
{
	sendNotImplemented() ;
}

void GSmtp::ServerProtocol::doHelp( EventData , bool & )
{
	sendNotImplemented() ;
}

void GSmtp::ServerProtocol::doVrfy( EventData event_data , bool & predicate )
{
	if( !m_config.with_vrfy )
	{
		predicate = false ;
		sendCannotVerify() ;
	}
	else if( m_config.mail_requires_authentication &&
		!m_sasl->authenticated() &&
		!m_sasl->trusted(m_peer_address.wildcards(),m_peer_address.hostPartString()) )
	{
		predicate = false ;
		sendAuthRequired( m_config.mail_requires_encryption && !m_secure && m_with_starttls ) ;
	}
	else if( m_config.mail_requires_encryption && !m_secure )
	{
		predicate = false ;
		sendEncryptionRequired( m_with_starttls ) ;
	}
	else
	{
		std::string to = parseVrfy( str(event_data) ) ;
		if( to.empty() )
		{
			predicate = false ;
			sendNotVerified( "invalid mailbox" , false ) ;
		}
		else
		{
			verify( Verifier::Command::VRFY , to ) ;
		}
	}
}

void GSmtp::ServerProtocol::verify( Verifier::Command command , const std::string & to , const std::string & from )
{
	Verifier::Info info ;
	info.client_ip = m_peer_address ;
	info.mail_from_parameter = from ;
	info.auth_mechanism = m_sasl->authenticated() ? m_sasl->mechanism() : std::string("NONE") ;
	info.auth_extra = m_sasl->id() ;
	m_verifier.verify( command , to , info ) ;
}

void GSmtp::ServerProtocol::verifyDone( Verifier::Command command , const VerifierStatus & status )
{
	G_DEBUG( "GSmtp::ServerProtocol::verifyDone: verify done: [" << status.str() << "]" ) ;
	if( status.abort )
		throw Done( "address verifier abort" ) ;

	Event event = command == Verifier::Command::RCPT ? Event::RcptReply : Event::VrfyReply ;

	// pass the verification result through the state machine as a single string
	std::string status_str = status.str() ;
	State new_state = m_fsm.apply( *this , event , {status_str.data(),status_str.size()} ) ;
	if( new_state == State::s_Any )
		throw Done( "protocol error" ) ;

	m_change_signal.emit() ;
}

void GSmtp::ServerProtocol::doVrfyReply( EventData event_data , bool & )
{
	VerifierStatus status = VerifierStatus::parse( str(event_data) ) ;

	if( status.is_valid && status.is_local )
		sendVerified( status.full_name ) ; // 250
	else if( status.is_valid )
		sendWillAccept( status.recipient ) ; // 252
	else
		sendNotVerified( status.response , status.temporary ) ; // 550 or 450
}

void GSmtp::ServerProtocol::doEhlo( EventData event_data , bool & predicate )
{
	std::string smtp_peer_name = parseHeloPeerName( str(event_data) ) ;
	if( smtp_peer_name.empty() )
	{
		predicate = false ;
		sendMissingParameter() ;
	}
	else
	{
		m_session_esmtp = true ;
		m_session_peer_name = smtp_peer_name ;
		m_sasl->reset() ;
		clear() ;
		G_ASSERT( !m_sasl->authenticated() ) ;

		ServerSend::Advertise advertise ;
		advertise.hello = m_text.hello( m_session_peer_name ) ;
		advertise.max_size = m_config.max_size ; // see also NewFile::ctor
		advertise.mechanisms = mechanisms() ;
		advertise.starttls = m_with_starttls && !m_secure ;
		advertise.vrfy = m_config.with_vrfy ;
		advertise.chunking = advertise.binarymime = m_config.with_chunking ;
		advertise.pipelining = m_config.with_pipelining ;
		advertise.smtputf8 = m_config.with_smtputf8 ;
		sendEhloReply( advertise ) ;
	}
}

void GSmtp::ServerProtocol::doHelo( EventData event_data , bool & predicate )
{
	std::string smtp_peer_name = parseHeloPeerName( str(event_data) ) ;
	if( smtp_peer_name.empty() )
	{
		predicate = false ;
		sendMissingParameter() ;
	}
	else
	{
		m_session_peer_name = smtp_peer_name ;
		clear() ;
		sendHeloReply() ;
	}
}

void GSmtp::ServerProtocol::doAuth( EventData event_data , bool & predicate )
{
	G::StringTokenView word( event_data , " \t" , 2U ) ;
	std::string mechanism = G::Str::upper( word.next()() ) ;
	std::string initial_response = G::sv_to_string( word.next()() ) ;
	bool got_initial_response = !initial_response.empty() ;

	G_DEBUG( "ServerProtocol::doAuth: [" << mechanism << "], [" << initial_response << "]" ) ;

	if( m_sasl->authenticated() )
	{
		G_WARNING( "GSmtp::ServerProtocol: too many AUTH requests" ) ;
		predicate = false ; // => idle
		sendOutOfSequence() ; // see RFC-2554 "Restrictions"
		badClientEvent() ;
	}
	else if( mechanisms().empty() && !m_secure && !mechanisms(true).empty() )
	{
		G_WARNING( "GSmtp::ServerProtocol: rejecting authentication attempt without encryption" ) ;
		predicate = false ; // => idle
		sendInsecureAuth( m_with_starttls ) ;
	}
	else if( mechanisms().empty() )
	{
		G_WARNING( "GSmtp::ServerProtocol: client protocol error: AUTH requested but not advertised" ) ;
		predicate = false ;
		sendNotImplemented() ;
	}
	else if( !m_sasl->init(m_secure,mechanism) )
	{
		G_WARNING( "GSmtp::ServerProtocol: request for unsupported server AUTH mechanism: " << mechanism ) ;
		predicate = false ; // => idle
		sendBadMechanism( m_sasl->preferredMechanism(m_secure) ) ;
	}
	else if( got_initial_response && m_sasl->mustChallenge() ) // RFC-4954 4
	{
		G_WARNING( "GSmtp::ServerProtocol: unexpected initial-response with a server-first AUTH mechanism" ) ;
		predicate = false ; // => idle
		sendInvalidArgument() ;
	}
	else if( got_initial_response && ( initial_response != "=" && !G::Base64::valid(initial_response) ) )
	{
		G_WARNING( "GSmtp::ServerProtocol: invalid base64 encoding of AUTH parameter" ) ;
		predicate = false ; // => idle
		sendInvalidArgument() ;
	}
	else if( got_initial_response )
	{
		std::string s = initial_response == "=" ? std::string() : G::Base64::decode(initial_response) ;
		bool done = false ;
		std::string next_challenge = m_sasl->apply( s , done ) ;
		if( done )
		{
			predicate = false ; // => idle
			sendAuthDone( m_sasl->authenticated() ) ;
		}
		else
		{
			sendChallenge( next_challenge ) ;
		}
	}
	else
	{
		sendChallenge( m_sasl->initialChallenge() ) ;
	}
}

void GSmtp::ServerProtocol::doAuthData( EventData event_data , bool & predicate )
{
	G_LOG( "GSmtp::ServerProtocol: rx<<: [authentication response not logged]" ) ;
	if( event_data.size() == 1U && event_data[0] == '*' )
	{
		predicate = false ; // => idle
		sendAuthenticationCancelled() ;
	}
	else if( !G::Base64::valid(event_data) )
	{
		G_WARNING( "GSmtp::ServerProtocol: invalid base64 encoding of authentication response" ) ;
		predicate = false ; // => idle
		sendAuthDone( false ) ;
	}
	else
	{
		bool done = false ;
		std::string next_challenge = m_sasl->apply( G::Base64::decode(event_data) , done ) ;
		if( done )
		{
			predicate = false ; // => idle
			sendAuthDone( m_sasl->authenticated() ) ;
		}
		else
		{
			sendChallenge( next_challenge ) ;
		}
	}
}

void GSmtp::ServerProtocol::doMail( EventData event_data , bool & predicate )
{
	G::string_view mail_line = event_data ;
	m_pm.clear() ;
	if( !m_enabled )
	{
		predicate = false ;
		sendDisabled() ;
	}
	else if( m_config.mail_requires_authentication &&
		!m_sasl->authenticated() &&
		!m_sasl->trusted(m_peer_address.wildcards(),m_peer_address.hostPartString()) )
	{
		G_LOG( "GSmtp::ServerProtocol::doMail: server authentication enabled "
			"but not a trusted address: " << m_peer_address.hostPartString() ) ;
		predicate = false ;
		sendAuthRequired( m_config.mail_requires_encryption && !m_secure && m_with_starttls ) ;
	}
	else if( m_config.mail_requires_encryption && !m_secure )
	{
		predicate = false ;
		sendEncryptionRequired( m_with_starttls ) ;
	}
	else
	{
		auto mail_command = parseMailFrom( mail_line ) ;
		if( !mail_command.error.empty() )
		{
			predicate = false ;
			sendBadFrom( mail_command.error ) ;
		}
		else if( m_config.max_size && mail_command.size > m_config.max_size )
		{
			// RFC-1427 6.1 (2)
			predicate = false ;
			sendTooBig() ;
		}
		else if( mail_command.utf8address && !mail_command.smtputf8 && m_config.smtputf8_strict )
		{
			predicate = false ;
			sendBadFrom( "invalid character in mailbox name" ) ;
		}
		else
		{
			sendMailReply( mail_command.address ) ;
			ProtocolMessage::FromInfo from_info ;
			from_info.auth = mail_command.auth ;
			from_info.body = mail_command.body ;
			from_info.smtputf8 = mail_command.smtputf8 ;
			from_info.utf8address = mail_command.utf8address ;
			m_pm.setFrom( mail_command.address , from_info ) ;
		}
	}
}

void GSmtp::ServerProtocol::doRcpt( EventData event_data , bool & predicate )
{
	G::string_view rcpt_line = event_data ;
	auto rcpt_command = parseRcptTo( rcpt_line ) ;
	if( !rcpt_command.error.empty() )
	{
		predicate = false ;
		sendBadTo( std::string() , rcpt_command.error , false ) ;
	}
	else if( rcpt_command.utf8address && !m_pm.fromInfo().smtputf8 && m_config.smtputf8_strict )
	{
		predicate = false ;
		sendBadTo( std::string() , "invalid character in mailbox name" , false ) ;
	}
	else
	{
		verify( Verifier::Command::RCPT , rcpt_command.address , m_pm.from() ) ;
	}
}

void GSmtp::ServerProtocol::doRcptToReply( EventData event_data , bool & predicate )
{
	// recover the VerifierStatus from the event-data string
	VerifierStatus status = VerifierStatus::parse( str(event_data) ) ;

	// store the status.address as the recipient address in the envelope
	bool ok = m_pm.addTo( ProtocolMessage::ToInfo(status) ) ;
	G_ASSERT( status.is_valid || !ok ) ;

	// respond with reference the original recipient address
	if( ok )
	{
		sendRcptReply( status.recipient , status.is_local ) ;
	}
	else
	{
		predicate = false ;
		sendBadTo( status.recipient , status.response , status.temporary ) ;
	}
}

void GSmtp::ServerProtocol::doUnknown( EventData event_data , bool & )
{
	sendUnrecognised( str(event_data) ) ;
	badClientEvent() ;
}

void GSmtp::ServerProtocol::clear()
{
	// cancel the current message transaction -- ehlo/quit session
	// unaffected -- forwarding client connection unaffected --
	// sasl state unaffected
	m_bdat_sum = 0U ;
	m_bdat_arg = 0U ;
	m_pm.clear() ;
	m_verifier.cancel() ;
}

void GSmtp::ServerProtocol::doRset( EventData , bool & )
{
	clear() ;
	m_pm.reset() ; // drop any ProtocolMessage forwarding client connection (moot)

	sendRsetReply() ;
}

void GSmtp::ServerProtocol::doNoRecipients( EventData , bool & )
{
	sendNoRecipients() ;
}

void GSmtp::ServerProtocol::doData( EventData , bool & )
{
	std::string received_line = m_text.received( m_session_peer_name , m_sasl->authenticated() ,
		m_secure , m_protocol , m_cipher ) ;

	if( received_line.length() )
		m_pm.addReceived( received_line ) ;

	sendDataReply() ;
}

bool GSmtp::ServerProtocol::isEndOfText( const ApplyArgsTuple & args ) const
{
	std::size_t eolsize = std::get<2>( args ) ;
	std::size_t linesize = std::get<3>( args ) ;
	char c0 = std::get<4>( args ) ;
	return linesize == 1U && eolsize == 2U && c0 == '.' ;
}

bool GSmtp::ServerProtocol::isEscaped( const ApplyArgsTuple & args ) const
{
	std::size_t size = std::get<1>( args ) ;
	std::size_t linesize = std::get<3>( args ) ;
	char c0 = std::get<4>( args ) ;
	return size > 1U && size == linesize && c0 == '.' ;
}

GSmtp::ServerProtocol::Event GSmtp::ServerProtocol::commandEvent( G::string_view line ) const
{
	G::StringTokenView t( line , " \t"_sv ) ;
	G::string_view word = t() ;
	if( G::Str::imatch(word,"QUIT"_sv) ) return Event::Quit ;
	if( G::Str::imatch(word,"HELO"_sv) ) return Event::Helo ;
	if( G::Str::imatch(word,"EHLO"_sv) ) return Event::Ehlo ;
	if( G::Str::imatch(word,"RSET"_sv) ) return Event::Rset ;
	if( G::Str::imatch(word,"DATA"_sv) ) return dataEvent(line) ;
	if( G::Str::imatch(word,"RCPT"_sv) ) return Event::Rcpt ;
	if( G::Str::imatch(word,"MAIL"_sv) ) return Event::Mail ;
	if( G::Str::imatch(word,"VRFY"_sv) ) return Event::Vrfy ;
	if( G::Str::imatch(word,"NOOP"_sv) ) return Event::Noop ;
	if( G::Str::imatch(word,"EXPN"_sv) ) return Event::Expn ;
	if( G::Str::imatch(word,"HELP"_sv) ) return Event::Help ;
	if( G::Str::imatch(word,"STARTTLS"_sv) && m_with_starttls ) return Event::StartTls ;
	if( G::Str::imatch(word,"AUTH"_sv) ) return Event::Auth ;
	if( G::Str::imatch(word,"BDAT"_sv) && m_config.with_chunking ) return bdatEvent(line) ;
	return Event::Unknown ;
}

GSmtp::ServerProtocol::Event GSmtp::ServerProtocol::dataEvent( G::string_view ) const
{
	// RFC-3030 p6 ("BINARYMIME cannot be used with the DATA command...")
	if( G::Str::imatch( m_pm.bodyType() , "BINARYMIME" ) )
		return Event::DataFail ; // State::MustReset

	return Event::Data ;
}

GSmtp::ServerProtocol::Event GSmtp::ServerProtocol::bdatEvent( G::string_view line ) const
{
	bool last = parseBdatLast(line).second ? parseBdatLast(line).first : false ;
	std::size_t size = parseBdatSize(line).second ? parseBdatSize(line).first : 0L ;
	if( last && size == 0U )
		return Event::BdatLastZero ;
	else if( last )
		return Event::BdatLast ;
	else
		return Event::Bdat ;
}

void GSmtp::ServerProtocol::badClientEvent()
{
	m_client_error_count++ ;
	if( m_config.client_error_limit && m_client_error_count >= m_config.client_error_limit )
	{
		std::string reason = "too many protocol errors from the client" ;
		G_DEBUG( "GSmtp::ServerProtocol::badClientEvent: " << reason << ": dropping the connection" ) ;
		throw Done( reason ) ;
	}
}

int GSmtp::ServerProtocol::code( EventData sv )
{
	return G::Str::toInt( G::Str::tailView(sv,sv.find('\t'),"501"_sv) ) ;
}

std::string GSmtp::ServerProtocol::str( EventData sv )
{
	return G::sv_to_string( G::Str::headView(sv,sv.find('\t'),sv) ) ;
}

G::StringArray GSmtp::ServerProtocol::mechanisms() const
{
	return m_sasl->mechanisms( m_secure ) ;
}

G::StringArray GSmtp::ServerProtocol::mechanisms( bool secure ) const
{
	return m_sasl->mechanisms( secure ) ;
}

// ===

GSmtp::ServerProtocol::Config::Config()
= default;

