//
// Copyright (C) 2001-2021 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gsaslserverfactory.h"
#include "gsocketprotocol.h"
#include "gsmtpserverprotocol.h"
#include "gxtext.h"
#include "gbase64.h"
#include "gdate.h"
#include "gtime.h"
#include "gdatetime.h"
#include "gstr.h"
#include "glog.h"
#include "gtest.h"
#include "gassert.h"
#include <string>
#include <tuple>

GSmtp::ServerProtocol::ServerProtocol( Sender & sender , Verifier & verifier ,
	ProtocolMessage & pmessage , const GAuth::Secrets & secrets ,
	const std::string & sasl_server_config , Text & text ,
	const GNet::Address & peer_address , const Config & config ) :
		m_sender(sender) ,
		m_verifier(verifier) ,
		m_text(text) ,
		m_message(pmessage) ,
		m_sasl(GAuth::SaslServerFactory::newSaslServer(secrets,sasl_server_config,false/*apop*/)) ,
		m_config(config) ,
		m_fsm(State::sStart,State::sEnd,State::s_Same,State::s_Any) ,
		m_with_starttls(false) ,
		m_peer_address(peer_address) ,
		m_secure(false) ,
		m_bad_client_count(0U) ,
		m_bad_client_limit(8U) ,
		m_session_authenticated(false)
{
	m_message.doneSignal().connect( G::Slot::slot(*this,&ServerProtocol::processDone) ) ;
	m_verifier.doneSignal().connect( G::Slot::slot(*this,&ServerProtocol::verifyDone) ) ;

	// (dont send anything to the peer from this ctor -- the Sender object is not fuly constructed)

	m_fsm( Event::eQuit , State::sProcessing , State::s_Same , &ServerProtocol::doEagerQuit ) ;
	m_fsm( Event::eQuit , State::s_Any , State::sEnd , &ServerProtocol::doQuit ) ;
	m_fsm( Event::eUnknown , State::sProcessing , State::s_Same , &ServerProtocol::doIgnore ) ;
	m_fsm( Event::eUnknown , State::s_Any , State::s_Same , &ServerProtocol::doUnknown ) ;
	m_fsm( Event::eRset , State::sStart , State::s_Same , &ServerProtocol::doNoop ) ;
	m_fsm( Event::eRset , State::s_Any , State::sIdle , &ServerProtocol::doRset ) ;
	m_fsm( Event::eNoop , State::s_Any , State::s_Same , &ServerProtocol::doNoop ) ;
	m_fsm( Event::eHelp , State::s_Any , State::s_Same , &ServerProtocol::doHelp ) ;
	m_fsm( Event::eExpn , State::s_Any , State::s_Same , &ServerProtocol::doExpn ) ;
	m_fsm( Event::eVrfy , State::sStart , State::sVrfyStart , &ServerProtocol::doVrfy , State::s_Same ) ;
	m_fsm( Event::eVrfyReply , State::sVrfyStart , State::sStart , &ServerProtocol::doVrfyReply ) ;
	m_fsm( Event::eVrfy , State::sIdle , State::sVrfyIdle , &ServerProtocol::doVrfy , State::s_Same ) ;
	m_fsm( Event::eVrfyReply , State::sVrfyIdle , State::sIdle , &ServerProtocol::doVrfyReply ) ;
	m_fsm( Event::eVrfy , State::sGotMail , State::sVrfyGotMail, &ServerProtocol::doVrfy , State::s_Same ) ;
	m_fsm( Event::eVrfyReply , State::sVrfyGotMail, State::sGotMail , &ServerProtocol::doVrfyReply ) ;
	m_fsm( Event::eVrfy , State::sGotRcpt , State::sVrfyGotRcpt, &ServerProtocol::doVrfy , State::s_Same ) ;
	m_fsm( Event::eVrfyReply , State::sVrfyGotRcpt, State::sGotRcpt , &ServerProtocol::doVrfyReply ) ;
	m_fsm( Event::eEhlo , State::s_Any , State::sIdle , &ServerProtocol::doEhlo , State::s_Same ) ;
	m_fsm( Event::eHelo , State::s_Any , State::sIdle , &ServerProtocol::doHelo , State::s_Same ) ;
	m_fsm( Event::eMail , State::sIdle , State::sGotMail , &ServerProtocol::doMail , State::sIdle ) ;
	m_fsm( Event::eRcpt , State::sGotMail , State::sVrfyTo1 , &ServerProtocol::doRcpt , State::s_Same ) ;
	m_fsm( Event::eVrfyReply , State::sVrfyTo1 , State::sGotRcpt , &ServerProtocol::doVrfyToReply , State::sGotMail ) ;
	m_fsm( Event::eRcpt , State::sGotRcpt , State::sVrfyTo2 , &ServerProtocol::doRcpt , State::s_Same ) ;
	m_fsm( Event::eVrfyReply , State::sVrfyTo2 , State::sGotRcpt , &ServerProtocol::doVrfyToReply ) ;
	m_fsm( Event::eData , State::sGotMail , State::sIdle , &ServerProtocol::doNoRecipients ) ;
	m_fsm( Event::eData , State::sGotRcpt , State::sData , &ServerProtocol::doData ) ;
	m_fsm( Event::eContent , State::sData , State::sData , &ServerProtocol::doContent , State::sDiscarding ) ;
	m_fsm( Event::eEot , State::sData , State::sProcessing , &ServerProtocol::doEot ) ;
	m_fsm( Event::eDone , State::sProcessing , State::sIdle , &ServerProtocol::doComplete ) ;
	m_fsm( Event::eContent , State::sDiscarding , State::sDiscarding , &ServerProtocol::doDiscard ) ;
	m_fsm( Event::eEot , State::sDiscarding , State::sIdle , &ServerProtocol::doDiscarded ) ;

	if( m_sasl->active() )
	{
		m_fsm( Event::eAuth , State::sIdle , State::sAuth , &ServerProtocol::doAuth , State::sIdle ) ;
		m_fsm( Event::eAuthData, State::sAuth , State::sAuth , &ServerProtocol::doAuthData , State::sIdle ) ;
	}
	else
	{
		m_fsm( Event::eAuth , State::sIdle , State::sIdle , &ServerProtocol::doAuthInvalid ) ;
	}

	if( m_config.tls_starttls && GNet::SocketProtocol::secureAcceptCapable() )
	{
		m_with_starttls = true ;
		m_fsm( Event::eStartTls , State::sIdle , State::sStartingTls , &ServerProtocol::doStartTls , State::sIdle ) ;
		m_fsm( Event::eSecure , State::sStartingTls , State::sIdle , &ServerProtocol::doSecure ) ;
	}
	else if( m_config.tls_connection )
	{
		m_fsm.reset( State::sStartingTls ) ;
		m_fsm( Event::eSecure , State::sStartingTls , State::sStart , &ServerProtocol::doSecureGreeting ) ;
	}
}

void GSmtp::ServerProtocol::init()
{
	if( m_config.tls_connection )
		m_sender.protocolSend( std::string() , /*go-secure=*/ true ) ;
	else
		sendGreeting( m_text.greeting() ) ;
}

GSmtp::ServerProtocol::~ServerProtocol()
{
	m_message.doneSignal().disconnect() ;
	m_verifier.doneSignal().disconnect() ;
}

void GSmtp::ServerProtocol::secure( const std::string & certificate ,
	const std::string & protocol , const std::string & cipher )
{
	m_certificate = certificate ;
	m_protocol = protocol ;
	m_cipher = cipher ;

	State new_state = m_fsm.apply( *this , Event::eSecure , EventData("",0U) ) ;
	if( new_state == State::s_Any )
		throw ProtocolDone( "protocol error" ) ;
}

void GSmtp::ServerProtocol::doSecure( EventData , bool & )
{
	G_DEBUG( "GSmtp::ServerProtocol::doSecure" ) ;
	m_secure = true ;
}

void GSmtp::ServerProtocol::doSecureGreeting( EventData , bool & )
{
	m_secure = true ;
	sendGreeting( m_text.greeting() ) ;
}

void GSmtp::ServerProtocol::doStartTls( EventData , bool & ok )
{
	if( m_secure )
	{
		sendOutOfSequence() ;
		ok = false ;
	}
	else
	{
		sendReadyForTls() ;
	}
}

bool GSmtp::ServerProtocol::inDataState() const
{
	return
		m_fsm.state() == State::sData ||
		m_fsm.state() == State::sDiscarding ;
}

bool GSmtp::ServerProtocol::halfDuplexBusy( const char * , std::size_t ) const
{
	return halfDuplexBusy() ;
}

bool GSmtp::ServerProtocol::halfDuplexBusy() const
{
	// return true if the line buffer must stop apply()ing because we are
	// waiting for an asynchronous filter or verifier completion event
	return
		m_config.allow_pipelining && (
			// states expecting eDone...
			m_fsm.state() == State::sProcessing ||
			// states expecting eVrfyReply...
			m_fsm.state() == State::sVrfyStart ||
			m_fsm.state() == State::sVrfyIdle ||
			m_fsm.state() == State::sVrfyGotMail ||
			m_fsm.state() == State::sVrfyGotRcpt ||
			m_fsm.state() == State::sVrfyTo1 ||
			m_fsm.state() == State::sVrfyTo2 ) ;
}

bool GSmtp::ServerProtocol::apply( const char * line_data , std::size_t line_data_size ,
	std::size_t eolsize , std::size_t linesize , char c0 )
{
	G_ASSERT( eolsize == 2U || ( inDataState() && eolsize == 0U ) ) ;

	// bundle the incoming data into a convenient structure
	EventData event_data( line_data , line_data_size , eolsize , linesize , c0 ) ;

	// parse the command into an event enum
	Event event = Event::eUnknown ;
	State state = m_fsm.state() ;
	if( (state == State::sData || state == State::sDiscarding) && isEndOfText(event_data) )
	{
		event = Event::eEot ;
	}
	else if( state == State::sData || state == State::sDiscarding )
	{
		event = Event::eContent ;
	}
	else if( state == State::sAuth )
	{
		event = Event::eAuthData ;
	}
	else
	{
		std::string line( line_data , line_data_size ) ;
		G_LOG( "GSmtp::ServerProtocol: rx<<: \"" << G::Str::printable(line) << "\"" ) ;
		event = commandEvent( commandWord(line) ) ;
		m_buffer = commandLine( line ) ;
		event_data = EventData( m_buffer.data() , m_buffer.size() ) ;
	}

	// apply the event to the state-machine
	State new_state = m_fsm.apply( *this , event , event_data ) ;
	if( new_state == State::s_Any )
		sendOutOfSequence() ;

	// tell the network code to stop apply()ing us if we are now
	// busy -- see GNet::LineBuffer::apply()
	return !halfDuplexBusy() ;
}

void GSmtp::ServerProtocol::doContent( EventData event_data , bool & ok )
{
	if( isEscaped(event_data) )
		ok = m_message.addText( event_data.ptr+1 , event_data.size+event_data.eolsize-1U ) ;
	else
		ok = m_message.addText( event_data.ptr , event_data.size+event_data.eolsize ) ;

	// moves to discard state if not ok - discard state throws if so configured
	if( !ok && m_config.disconnect_on_max_size )
		sendTooBig( true ) ;
}

void GSmtp::ServerProtocol::doEot( EventData , bool & )
{
	G_LOG( "GSmtp::ServerProtocol: rx<<: [message content not logged]" ) ;
	G_LOG( "GSmtp::ServerProtocol: rx<<: \".\"" ) ;
	m_message.process( m_sasl->id() , m_peer_address.hostPartString() , m_certificate ) ;
}

void GSmtp::ServerProtocol::processDone( bool success , const MessageId & id ,
	const std::string & response , const std::string & reason )
{
	GDEF_IGNORE_PARAMS( success , id , reason ) ;
	G_DEBUG( "GSmtp::ServerProtocol::processDone: " << (success?1:0) << " " << id.str()
		<< " [" << response << "] [" << reason << "]" ) ;
	G_ASSERT( success == response.empty() ) ;

	State new_state = m_fsm.apply( *this , Event::eDone , EventData(response.data(),response.size()) ) ;
	if( new_state == State::s_Any )
		throw ProtocolDone( "protocol error" ) ;
}

void GSmtp::ServerProtocol::doComplete( EventData event_data , bool & )
{
	reset() ;
	const bool empty = event_data.size == 0U ;
	sendCompletionReply( empty , std::string(event_data.ptr,event_data.size) ) ;
}

void GSmtp::ServerProtocol::doEagerQuit( EventData , bool & )
{
	// broken client has sent "." and then immediatedly "QUIT" and we
	// have not been saved by the half-duplex queueing -- if so
	// configured we just ignore the quit with a warning
	if( m_config.ignore_eager_quit )
		G_WARNING( "GSmtp::ServerProtocol::doEagerQuit: ignoring out-of-sequence quit" ) ;
	else
		throw ProtocolDone( "protocol error: out-of-sequence quit" ) ;
}

void GSmtp::ServerProtocol::doQuit( EventData , bool & )
{
	reset() ;
	sendQuitOk() ;
	throw ProtocolDone() ;
}

void GSmtp::ServerProtocol::doDiscard( EventData , bool & )
{
	if( m_config.disconnect_on_max_size )
	{
		reset() ;
		sendClosing() ;
		throw ProtocolDone() ;
	}
}

void GSmtp::ServerProtocol::doIgnore( EventData , bool & )
{
}

void GSmtp::ServerProtocol::doNoop( EventData , bool & )
{
	sendOk() ;
}

void GSmtp::ServerProtocol::doNothing( EventData , bool & )
{
}

void GSmtp::ServerProtocol::doDiscarded( EventData , bool & )
{
	reset() ;
	sendTooBig() ;
}

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
	std::string line( event_data.ptr , event_data.size ) ;
	if( m_config.with_vrfy )
	{
		std::string to = parseRcptParameter( line ) ;
		if( to.empty() )
		{
			predicate = false ;
			sendNotVerified( "invalid mailbox" , false ) ;
		}
		else
		{
			verify( to , "" ) ;
		}
	}
	else
	{
		predicate = false ;
		sendNotImplemented() ;
	}
}

void GSmtp::ServerProtocol::verify( const std::string & to , const std::string & from )
{
	std::string mechanism = m_sasl->active() ? m_sasl->mechanism() : std::string() ;
	std::string id = m_sasl->active() ? m_sasl->id() : std::string() ;
	if( m_sasl->active() && !m_session_authenticated )
		mechanism = "NONE" ;
	m_verifier.verify( to , from , m_peer_address , mechanism , id ) ;
}

void GSmtp::ServerProtocol::verifyDone( const std::string & mbox , const VerifierStatus & status )
{
	G_DEBUG( "GSmtp::ServerProtocol::verifyDone: verify done: [" << status.str(mbox) << "]" ) ;
	if( status.abort )
		throw ProtocolDone( "address verifier abort" ) ; // denial-of-service countermeasure

	std::string status_str = status.str( mbox ) ;
	State new_state = m_fsm.apply( *this , Event::eVrfyReply , EventData(status_str.data(),status_str.size()) ) ;
	if( new_state == State::s_Any )
		throw ProtocolDone( "protocol error" ) ;
}

void GSmtp::ServerProtocol::doVrfyReply( EventData event_data , bool & )
{
	std::string line( event_data.ptr , event_data.size ) ;
	std::string mbox ;
	VerifierStatus status = VerifierStatus::parse( line , mbox ) ;

	if( status.is_valid && status.is_local )
		sendVerified( status.full_name ) ; // 250
	else if( status.is_valid )
		sendWillAccept( mbox ) ; // 252
	else
		sendNotVerified( status.response , status.temporary ) ; // 550 or 450
}

std::string GSmtp::ServerProtocol::parseRcptParameter( const std::string & line ) const
{
	std::string to ;
	std::size_t pos = line.find_first_of( " \t" ) ;
	if( pos != std::string::npos )
		to = line.substr(pos) ;

	G::Str::trim( to , {" \t",2U} ) ;
	return to ;
}

void GSmtp::ServerProtocol::doEhlo( EventData event_data , bool & predicate )
{
	std::string line( event_data.ptr , event_data.size ) ;
	std::string smtp_peer_name = parsePeerName( line ) ;
	if( smtp_peer_name.empty() )
	{
		predicate = false ;
		sendMissingParameter() ;
	}
	else
	{
		m_session_peer_name = smtp_peer_name ;
		m_session_authenticated = false ;
		reset() ;
		sendEhloReply() ;
	}
}

void GSmtp::ServerProtocol::doHelo( EventData event_data , bool & predicate )
{
	std::string line( event_data.ptr , event_data.size ) ;
	std::string smtp_peer_name = parsePeerName( line ) ;
	if( smtp_peer_name.empty() )
	{
		predicate = false ;
		sendMissingParameter() ;
	}
	else
	{
		m_session_peer_name = smtp_peer_name ;
		reset() ;
		sendHeloReply() ;
	}
}

bool GSmtp::ServerProtocol::authenticationRequiresEncryption() const
{
	bool encryption_required_by_user = m_config.authentication_requires_encryption ;
	bool encryption_required_by_sasl = m_sasl->active() && m_sasl->requiresEncryption() ;
	return encryption_required_by_user || encryption_required_by_sasl ;
}

void GSmtp::ServerProtocol::doAuthInvalid( EventData , bool & )
{
	// (workround with "--server-auth" pointing to an empty file)
	G_WARNING( "GSmtp::ServerProtocol: client protocol error: AUTH requested but not advertised" ) ;
	sendNotImplemented() ;
}

void GSmtp::ServerProtocol::doAuth( EventData event_data , bool & predicate )
{
	G::StringArray word_array ;
	G::Str::splitIntoTokens( std::string(event_data.ptr,event_data.size) , word_array , " \t" ) ;

	std::string mechanism = word_array.size() > 1U ? G::Str::upper(word_array[1U]) : std::string() ;
	std::string initial_response = word_array.size() > 2U ? word_array[2U] : std::string() ;
	bool got_initial_response = word_array.size() > 2U ;

	G_DEBUG( "ServerProtocol::doAuth: [" << mechanism << "], [" << initial_response << "]" ) ;

	if( !m_secure && authenticationRequiresEncryption() )
	{
		G_WARNING( "GSmtp::ServerProtocol: rejecting authentication attempt without encryption" ) ;
		predicate = false ; // => idle
		sendBadMechanism() ; // since none until encryption
	}
	else if( m_session_authenticated )
	{
		G_WARNING( "GSmtp::ServerProtocol: too many AUTH requests" ) ;
		predicate = false ; // => idle
		sendOutOfSequence() ; // see RFC-2554 "Restrictions"
	}
	else if( ! m_sasl->init(mechanism) )
	{
		G_WARNING( "GSmtp::ServerProtocol: request for unsupported server AUTH mechanism: " << mechanism ) ;
		predicate = false ; // => idle
		sendBadMechanism() ;
	}
	else if( got_initial_response && m_sasl->mustChallenge() ) // RFC-4959 4
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
			m_session_authenticated = m_sasl->authenticated() ;
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
	std::string line( event_data.ptr , event_data.size ) ;
	if( line == "*" )
	{
		predicate = false ; // => idle
		sendAuthenticationCancelled() ;
	}
	else if( !G::Base64::valid(line) )
	{
		G_WARNING( "GSmtp::ServerProtocol: invalid base64 encoding of authentication response" ) ;
		predicate = false ; // => idle
		sendAuthDone( false ) ;
	}
	else
	{
		bool done = false ;
		std::string next_challenge = m_sasl->apply( G::Base64::decode(line) , done ) ;
		if( done && G::Test::enabled("sasl-server-oauth") )
		{
			predicate = false ;
			m_session_authenticated = m_sasl->authenticated() ;
			send( "535-more info at\r\n535 http://example.com" ) ; // testing
		}
		else if( done )
		{
			predicate = false ; // => idle
			m_session_authenticated = m_sasl->authenticated() ;
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
	std::string line( event_data.ptr , event_data.size ) ;
	if( !m_session_authenticated && m_sasl->active() && !m_sasl->trusted(m_peer_address) )
	{
		G_LOG( "GSmtp::ServerProtocol::doMail: server authentication enabled "
			"but not a trusted address: " << m_peer_address.hostPartString() ) ;
		predicate = false ;
		sendAuthRequired() ;
	}
	else if( !m_secure && m_config.mail_requires_encryption )
	{
		predicate = false ;
		sendEncryptionRequired() ;
	}
	else if( m_config.max_size && parseMailSize(line) > m_config.max_size )
	{
		predicate = false ;
		sendTooBig() ;
	}
	else
	{
		m_message.clear() ;
		std::string from_address ;
		std::string from_error_response ;
		std::tie(from_address,from_error_response) = parseMailFrom( line ) ;
		bool ok = from_error_response.empty() ;
		m_message.setFrom( from_address , parseMailAuth(line) ) ;
		predicate = ok ;
		if( ok )
		{
			sendMailReply() ;
		}
		else
		{
			sendBadFrom( from_error_response ) ;
		}
	}
}

void GSmtp::ServerProtocol::doRcpt( EventData event_data , bool & predicate )
{
	std::string to_address ;
	std::string to_error_response ;
	std::tie(to_address,to_error_response) = parseRcptTo( std::string(event_data.ptr,event_data.size) ) ;
	bool ok = to_error_response.empty() ;
	if( ok )
	{
		verify( to_address , m_message.from() ) ;
	}
	else
	{
		predicate = false ;
		sendBadTo( to_error_response , false ) ;
	}
}

void GSmtp::ServerProtocol::doVrfyToReply( EventData event_data , bool & predicate )
{
	std::string to ;
	VerifierStatus status = VerifierStatus::parse( std::string(event_data.ptr,event_data.size) , to ) ;

	bool ok = m_message.addTo( to , status ) ;
	if( ok )
	{
		sendRcptReply() ;
	}
	else
	{
		predicate = false ;
		sendBadTo( G::Str::printable(status.response) , status.temporary ) ;
	}
}

void GSmtp::ServerProtocol::doUnknown( EventData event_data , bool & )
{
	sendUnrecognised( std::string(event_data.ptr,event_data.size) ) ;
}

void GSmtp::ServerProtocol::reset()
{
	// cancel the current message transaction -- ehlo/quit session unaffected
	m_message.clear() ;
	m_verifier.cancel() ;
}

void GSmtp::ServerProtocol::doRset( EventData , bool & )
{
	reset() ;
	m_message.reset() ;
	sendRsetReply() ;
}

void GSmtp::ServerProtocol::doNoRecipients( EventData , bool & )
{
	sendNoRecipients() ;
}

void GSmtp::ServerProtocol::doData( EventData , bool & )
{
	std::string received_line = m_text.received( m_session_peer_name , m_session_authenticated ,
		m_secure , m_protocol , m_cipher ) ;

	if( received_line.length() )
		m_message.addReceived( received_line ) ;

	sendDataReply() ;
}

bool GSmtp::ServerProtocol::isEndOfText( const EventData & e ) const
{
	return e.linesize == 1U && e.eolsize == 2U && e.c0 == '.' ;
}

bool GSmtp::ServerProtocol::isEscaped( const EventData & e ) const
{
	return e.size > 1U && e.size == e.linesize && e.c0 == '.' ;
}

std::string GSmtp::ServerProtocol::commandWord( const std::string & line_in ) const
{
	std::string line( line_in ) ;
	G::Str::trimLeft( line , {" \t",2U} ) ;

	std::size_t pos = line.find_first_of( " \t" ) ;
	std::string command = line.substr( 0U , pos ) ;

	G::Str::toUpper( command ) ;
	return command ;
}

std::string GSmtp::ServerProtocol::commandLine( const std::string & line_in ) const
{
	std::string line( line_in ) ;
	G::Str::trimLeft( line , {" \t",2U} ) ;
	return line ;
}

GSmtp::ServerProtocol::Event GSmtp::ServerProtocol::commandEvent( const std::string & command ) const
{
	if( command == "QUIT" ) return Event::eQuit ;
	if( command == "HELO" ) return Event::eHelo ;
	if( command == "EHLO" ) return Event::eEhlo ;
	if( command == "RSET" ) return Event::eRset ;
	if( command == "DATA" ) return Event::eData ;
	if( command == "RCPT" ) return Event::eRcpt ;
	if( command == "MAIL" ) return Event::eMail ;
	if( command == "VRFY" ) return Event::eVrfy ;
	if( command == "NOOP" ) return Event::eNoop ;
	if( command == "EXPN" ) return Event::eExpn ;
	if( command == "HELP" ) return Event::eHelp ;
	if( command == "STARTTLS" && m_with_starttls ) return Event::eStartTls ;
	if( command == "AUTH" ) return Event::eAuth ;
	return Event::eUnknown ;
}

const std::string & GSmtp::ServerProtocol::crlf()
{
	static const std::string s( "\015\012" ) ;
	return s ;
}

void GSmtp::ServerProtocol::sendChallenge( const std::string & challenge )
{
	send( "334 " + G::Base64::encode(challenge) ) ;
}

void GSmtp::ServerProtocol::sendGreeting( const std::string & text )
{
	send( "220 " + text ) ;
}

void GSmtp::ServerProtocol::sendReadyForTls()
{
	send( "220 ready to start tls" , /*go-secure=*/ true ) ;
}

void GSmtp::ServerProtocol::sendInvalidArgument()
{
	send( "501 invalid argument" ) ;
}

void GSmtp::ServerProtocol::sendAuthenticationCancelled()
{
	send( "501 authentication cancelled" ) ;
}

void GSmtp::ServerProtocol::sendBadMechanism()
{
	send( "504 unsupported authentication mechanism" ) ;
}

void GSmtp::ServerProtocol::sendAuthDone( bool ok )
{
	if( ok )
		send( "235 authentication successful" ) ;
	else
		send( "535 authentication failed" ) ;
}

void GSmtp::ServerProtocol::sendOutOfSequence()
{
	send( "503 command out of sequence -- use RSET to resynchronise" ) ;
	badClientEvent() ;
}

void GSmtp::ServerProtocol::sendMissingParameter()
{
	send( "501 parameter required" ) ;
}

void GSmtp::ServerProtocol::sendQuitOk()
{
	send( "221 OK" ) ;
	m_sender.protocolShutdown() ;
}

void GSmtp::ServerProtocol::sendClosing()
{
	send( "221 closing connection" ) ;
	m_sender.protocolShutdown() ;
}

void GSmtp::ServerProtocol::sendVerified( const std::string & user )
{
	send( "250 " + user ) ;
}

void GSmtp::ServerProtocol::sendNotVerified( const std::string & response , bool temporary )
{
	send( (temporary?"450":"550") + std::string(1U,' ') + response ) ;
}

void GSmtp::ServerProtocol::sendWillAccept( const std::string & user )
{
	send( "252 cannot verify but will accept: " + G::Str::printable(user) ) ;
}

void GSmtp::ServerProtocol::sendUnrecognised( const std::string & line_in )
{
	std::string line = line_in.substr( 0U , 80U ) ;
	if( line.size() >= 80U ) line.append( " ..." ) ;
	send( "500 command unrecognized: \"" + G::Str::printable(line) + std::string(1U,'\"') ) ;
	badClientEvent() ;
}

void GSmtp::ServerProtocol::sendNotImplemented()
{
	send( "502 command not implemented" ) ;
}

void GSmtp::ServerProtocol::sendAuthRequired()
{
	std::string more_help = authenticationRequiresEncryption() && !m_secure ? ": use starttls" : "" ;
	send( "530 authentication required" + more_help ) ;
}

void GSmtp::ServerProtocol::sendEncryptionRequired()
{
	send( "538 encryption required: use starttls" ) ; // was 530 -- 538 in RFC-2554 but deprecated in RFC-4954
}

void GSmtp::ServerProtocol::sendNoRecipients()
{
	send( "554 no valid recipients" ) ;
}

void GSmtp::ServerProtocol::sendTooBig( bool disconnecting )
{
	std::string s = "552 message exceeds fixed maximum message size" ;
	if( disconnecting ) s.append( ", disconnecting" ) ;
	send( s ) ;
}

void GSmtp::ServerProtocol::sendDataReply()
{
	send( "354 start mail input -- end with <CRLF>.<CRLF>" ) ;
}

void GSmtp::ServerProtocol::sendRsetReply()
{
	send( "250 state reset" ) ;
}

void GSmtp::ServerProtocol::sendMailReply()
{
	sendOk() ;
}

void GSmtp::ServerProtocol::sendCompletionReply( bool ok , const std::string & response )
{
	if( ok )
		sendOk() ;
	else
		// 452=>"action not taken", or perhaps 554=>"transaction failed" (so don't try again)
		send( "452 " + response ) ;
}

void GSmtp::ServerProtocol::sendRcptReply()
{
	sendOk() ;
}

void GSmtp::ServerProtocol::sendBadFrom( const std::string & response_extra )
{
	std::string response = "553 mailbox name not allowed" ;
	if( ! response_extra.empty() )
	{
		response.append( ": " ) ;
		response.append( response_extra ) ;
	}
	send( response ) ;
}

void GSmtp::ServerProtocol::sendBadTo( const std::string & text , bool temporary )
{
	send( (temporary?"450":"550") + std::string(text.empty()?"":" ") + text ) ;
}

void GSmtp::ServerProtocol::sendEhloReply()
{
	std::ostringstream ss ;
		ss << "250-" << m_text.hello(m_session_peer_name) << crlf() ;

	if( m_config.max_size != 0U )
		ss << "250-SIZE " << m_config.max_size << crlf() ;

	if( m_sasl->active() && !( authenticationRequiresEncryption() && !m_secure ) )
		ss << "250-AUTH " << m_sasl->mechanisms(' ') << crlf() ;

	if( m_with_starttls && !m_secure )
		ss << "250-STARTTLS" << crlf() ;

	if( m_config.with_vrfy )
		ss << "250-VRFY" << crlf() ; // see RFC-2821 3.5.2

	ss << "250 8BITMIME" ;
	send( ss.str() ) ;
}

void GSmtp::ServerProtocol::sendHeloReply()
{
	sendOk() ;
}

void GSmtp::ServerProtocol::sendOk()
{
	send( "250 OK" ) ;
}

void GSmtp::ServerProtocol::send( const char * line )
{
	send( std::string(line) ) ;
}

void GSmtp::ServerProtocol::send( std::string line , bool go_secure )
{
	G_LOG( "GSmtp::ServerProtocol: tx>>: \"" << G::Str::printable(line) << "\"" ) ;
	line.append( crlf() ) ;
	m_sender.protocolSend( line , go_secure ) ;
}

void GSmtp::ServerProtocol::badClientEvent()
{
	m_bad_client_count++ ;
	if( m_bad_client_limit && m_bad_client_count >= m_bad_client_limit )
	{
		std::string reason = "too many protocol errors from the client" ;
		G_DEBUG( "GSmtp::ServerProtocol::badClientEvent: " << reason << ": dropping the connection" ) ;
		throw ProtocolDone( reason ) ;
	}
}

std::size_t GSmtp::ServerProtocol::parseMailSize( const std::string & line ) const
{
	std::string parameter = parseMailParameter( line , "SIZE=" ) ;
	if( parameter.empty() || !G::Str::isULong(parameter) )
		return 0U ;
	else
		return static_cast<std::size_t>( G::Str::toULong(parameter,G::Str::Limited()) ) ;
}

std::string GSmtp::ServerProtocol::parseMailAuth( const std::string & line ) const
{
	return parseMailParameter( line , "AUTH=" ) ;
}

std::string GSmtp::ServerProtocol::parseMailParameter( const std::string & line , const std::string & key ) const
{
	std::string result ;
	std::size_t end = line.find( '>' ) ;
	if( end != std::string::npos )
	{
		G::StringArray parameters = G::Str::splitIntoTokens( line.substr(end) , " " ) ;
		for( const auto & parameter : parameters )
		{
			std::size_t pos = G::Str::upper(parameter).find( key ) ;
			if( pos == 0U && parameter.length() > key.size() )
			{
				// ensure valid xtext
				result = G::Xtext::encode( G::Xtext::decode( parameter.substr(key.size()) ) ) ;
				break ;
			}
		}
	}
	return result ;
}

std::pair<std::string,std::string> GSmtp::ServerProtocol::parseMailFrom( const std::string & line ) const
{
	// eg. MAIL FROM:<me@localhost>
	return parseAddress( line ) ;
}

std::pair<std::string,std::string> GSmtp::ServerProtocol::parseRcptTo( const std::string & line ) const
{
	// eg. RCPT TO:<@first.net,@second.net:you@last.net>
	// eg. RCPT TO:<Postmaster>
	return parseAddress( line ) ;
}

std::pair<std::string,std::string> GSmtp::ServerProtocol::parseAddress( const std::string & line ) const
{
	std::size_t start = line.find( '<' ) ;
	std::size_t end = line.find( '>' ) ;
	if( start == std::string::npos || end == std::string::npos || end < start )
	{
		std::string response( "missing or invalid angle brackets in mailbox name" ) ;
		return std::make_pair(std::string(),response) ;
	}

	std::string s = line.substr( start + 1U , end - start - 1U ) ;
	G::Str::trim( s , {" \t",2U} ) ;

	// strip source route
	if( s.length() > 0U && s.at(0U) == '@' )
	{
		std::size_t colon_pos = s.find( ':' ) ;
		if( colon_pos == std::string::npos )
		{
			std::string response( "invalid mailbox name: no colon after leading at character" ) ;
			return std::make_pair(std::string(),response) ;
		}
		s = s.substr( colon_pos + 1U ) ;
	}

	return std::make_pair(s,std::string()) ;
}

std::string GSmtp::ServerProtocol::parsePeerName( const std::string & line ) const
{
	std::size_t pos = line.find_first_of( " \t" ) ;
	if( pos == std::string::npos )
		return std::string() ;

	std::string smtp_peer_name = line.substr( pos + 1U ) ;
	G::Str::trim( smtp_peer_name , {" \t",2U} ) ;
	return smtp_peer_name ;
}

// ===

GSmtp::ServerProtocolText::ServerProtocolText( const std::string & code_ident , const std::string & thishost ,
	const GNet::Address & peer_address ) :
		m_code_ident(code_ident) ,
		m_thishost(thishost) ,
		m_peer_address(peer_address)
{
}

std::string GSmtp::ServerProtocolText::greeting() const
{
	return m_thishost + " -- " + m_code_ident + " -- Service ready" ;
}

std::string GSmtp::ServerProtocolText::hello( const std::string & ) const
{
	return m_thishost + " says hello" ;
}

std::string GSmtp::ServerProtocolText::received( const std::string & smtp_peer_name ,
	bool authenticated , bool secure , const std::string & protocol , const std::string & cipher ) const
{
	return receivedLine( smtp_peer_name , m_peer_address.hostPartString() , m_thishost ,
		authenticated , secure , protocol , cipher ) ;
}

std::string GSmtp::ServerProtocolText::receivedLine( const std::string & smtp_peer_name ,
	const std::string & peer_address , const std::string & thishost ,
	bool authenticated , bool secure , const std::string & , const std::string & cipher_in )
{
	const G::SystemTime t = G::SystemTime::now() ;
	const G::BrokenDownTime tm = t.local() ;
	const std::string zone = G::DateTime::offsetString(G::DateTime::offset(t)) ;
	const G::Date date( tm ) ;
	const G::Time time( tm ) ;
	const std::string esmtp = std::string("ESMTP") + (secure?"S":"") + (authenticated?"A":"") ; // RFC-3848
	const std::string peer_name = G::Str::toPrintableAscii(
		G::Str::replaced(smtp_peer_name,' ','-') ) ; // typically alphanumeric with ".-:[]_"
	std::string cipher = secure ?
		G::Str::only( G::sv_to_string(G::Str::alnum())+"_",G::Str::replaced(cipher_in,'-','_')) :
		std::string() ;

	// RFC-5321 4.4
	std::ostringstream ss ;
	ss
		<< "Received: from " << peer_name
		<< " ("
			<< "[" << peer_address << "]"
		<< ") by " << thishost << " with " << esmtp
		<< (cipher.empty()?"":" tls ") << cipher // RFC-8314 4.3 7.4
		<< " ; "
		<< date.weekdayName(true) << ", "
		<< date.monthday() << " "
		<< date.monthName(true) << " "
		<< date.yyyy() << " "
		<< time.hhmmss(":") << " "
		<< zone ;
	return ss.str() ;
}

// ===

GSmtp::ServerProtocol::Config::Config()
= default;

GSmtp::ServerProtocol::Config::Config( bool with_vrfy_in ,
	std::size_t max_size_in , bool authentication_requires_encryption_in ,
	bool mail_requires_encryption_in , bool tls_starttls_in ,
	bool tls_connection_in ) :
		with_vrfy(with_vrfy_in) ,
		max_size(max_size_in) ,
		authentication_requires_encryption(authentication_requires_encryption_in) ,
		mail_requires_encryption(mail_requires_encryption_in) ,
		tls_starttls(tls_starttls_in) ,
		tls_connection(tls_connection_in)
{
}

// ===

GSmtp::ServerProtocol::EventData::EventData( const char * ptr_ , std::size_t size_ ) :
	ptr(ptr_) ,
	size(size_) ,
	eolsize(2U) ,
	linesize(size_) ,
	c0('\0')
{
}

GSmtp::ServerProtocol::EventData::EventData( const char * ptr_ , std::size_t size_ ,
	std::size_t eolsize_ , std::size_t linesize_ , char c0_ ) :
		ptr(ptr_) ,
		size(size_) ,
		eolsize(eolsize_) ,
		linesize(linesize_) ,
		c0(c0_)
{
}

