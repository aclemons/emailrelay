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
// gserverprotocol.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gsaslserverfactory.h"
#include "gsocketprotocol.h"
#include "gserverprotocol.h"
#include "gxtext.h"
#include "gbase64.h"
#include "gdate.h"
#include "gtime.h"
#include "gdatetime.h"
#include "gstr.h"
#include "glog.h"
#include "gassert.h"
#include <string>

GSmtp::ServerProtocol::ServerProtocol( GNet::ExceptionHandler & eh , Sender & sender ,
	Verifier & verifier , ProtocolMessage & pmessage , const GAuth::Secrets & secrets , Text & text ,
	GNet::Address peer_address , Config config ) :
		GNet::TimerBase(eh) ,
		m_sender(sender) ,
		m_verifier(verifier) ,
		m_text(text) ,
		m_message(pmessage) ,
		m_sasl(GAuth::SaslServerFactory::newSaslServer(secrets,false/*apop*/)) ,
		m_config(config) ,
		m_fsm(sStart,sEnd,s_Same,s_Any) ,
		m_with_starttls(false) ,
		m_peer_address(peer_address) ,
		m_secure(false) ,
		m_bad_client_count(0U) ,
		m_bad_client_limit(8U) ,
		m_session_authenticated(false)
{
	m_message.doneSignal().connect( G::Slot::slot(*this,&ServerProtocol::processDone) ) ;
	verifier.doneSignal().connect( G::Slot::slot(*this,&ServerProtocol::verifyDone) ) ;

	// (dont send anything to the peer from this ctor -- the Sender object is not fuly constructed)

	m_fsm.addTransition( eQuit        , s_Any       , sEnd        , &GSmtp::ServerProtocol::doQuit ) ;
	m_fsm.addTransition( eUnknown     , sProcessing , s_Same      , &GSmtp::ServerProtocol::doIgnore ) ;
	m_fsm.addTransition( eUnknown     , s_Any       , s_Same      , &GSmtp::ServerProtocol::doUnknown ) ;
	m_fsm.addTransition( eRset        , sStart      , s_Same      , &GSmtp::ServerProtocol::doNoop ) ;
	m_fsm.addTransition( eRset        , s_Any       , sIdle       , &GSmtp::ServerProtocol::doRset ) ;
	m_fsm.addTransition( eNoop        , s_Any       , s_Same      , &GSmtp::ServerProtocol::doNoop ) ;
	m_fsm.addTransition( eHelp        , s_Any       , s_Same      , &GSmtp::ServerProtocol::doHelp ) ;
	m_fsm.addTransition( eExpn        , s_Any       , s_Same      , &GSmtp::ServerProtocol::doExpn ) ;
	m_fsm.addTransition( eVrfy        , sStart      , sVrfyStart  , &GSmtp::ServerProtocol::doVrfy , s_Same ) ;
	m_fsm.addTransition( eVrfyReply   , sVrfyStart  , sStart      , &GSmtp::ServerProtocol::doVrfyReply ) ;
	m_fsm.addTransition( eVrfy        , sIdle       , sVrfyIdle   , &GSmtp::ServerProtocol::doVrfy , s_Same ) ;
	m_fsm.addTransition( eVrfyReply   , sVrfyIdle   , sIdle       , &GSmtp::ServerProtocol::doVrfyReply ) ;
	m_fsm.addTransition( eVrfy        , sGotMail    , sVrfyGotMail, &GSmtp::ServerProtocol::doVrfy , s_Same ) ;
	m_fsm.addTransition( eVrfyReply   , sVrfyGotMail, sGotMail    , &GSmtp::ServerProtocol::doVrfyReply ) ;
	m_fsm.addTransition( eVrfy        , sGotRcpt    , sVrfyGotRcpt, &GSmtp::ServerProtocol::doVrfy , s_Same ) ;
	m_fsm.addTransition( eVrfyReply   , sVrfyGotRcpt, sGotRcpt    , &GSmtp::ServerProtocol::doVrfyReply ) ;
	m_fsm.addTransition( eEhlo        , s_Any       , sIdle       , &GSmtp::ServerProtocol::doEhlo , s_Same ) ;
	m_fsm.addTransition( eHelo        , s_Any       , sIdle       , &GSmtp::ServerProtocol::doHelo , s_Same ) ;
	m_fsm.addTransition( eMail        , sIdle       , sGotMail    , &GSmtp::ServerProtocol::doMail , sIdle ) ;
	m_fsm.addTransition( eRcpt        , sGotMail    , sVrfyTo1    , &GSmtp::ServerProtocol::doRcpt , s_Same ) ;
	m_fsm.addTransition( eVrfyReply   , sVrfyTo1    , sGotRcpt    , &GSmtp::ServerProtocol::doVrfyToReply , sGotMail ) ;
	m_fsm.addTransition( eRcpt        , sGotRcpt    , sVrfyTo2    , &GSmtp::ServerProtocol::doRcpt , s_Same ) ;
	m_fsm.addTransition( eVrfyReply   , sVrfyTo2    , sGotRcpt    , &GSmtp::ServerProtocol::doVrfyToReply ) ;
	m_fsm.addTransition( eData        , sGotMail    , sIdle       , &GSmtp::ServerProtocol::doNoRecipients ) ;
	m_fsm.addTransition( eData        , sGotRcpt    , sData       , &GSmtp::ServerProtocol::doData ) ;
	m_fsm.addTransition( eContent     , sData       , sData       , &GSmtp::ServerProtocol::doContent , sDiscarding ) ;
	m_fsm.addTransition( eEot         , sData       , sProcessing , &GSmtp::ServerProtocol::doEot ) ;
	m_fsm.addTransition( eDone        , sProcessing , sIdle       , &GSmtp::ServerProtocol::doComplete ) ;
	m_fsm.addTransition( eTimeout     , sProcessing , sIdle       , &GSmtp::ServerProtocol::doComplete ) ;
	m_fsm.addTransition( eContent     , sDiscarding , sDiscarding , &GSmtp::ServerProtocol::doDiscard ) ;
	m_fsm.addTransition( eEot         , sDiscarding , sIdle       , &GSmtp::ServerProtocol::doDiscarded ) ;

	if( m_sasl->active() )
	{
		m_fsm.addTransition( eAuth    , sIdle   , sAuth    , &GSmtp::ServerProtocol::doAuth , sIdle ) ;
		m_fsm.addTransition( eAuthData, sAuth   , sAuth    , &GSmtp::ServerProtocol::doAuthData , sIdle ) ;
	}

	m_with_starttls = GNet::SocketProtocol::secureAcceptCapable() && m_config.advertise_tls_if_possible ;
	if( m_with_starttls )
	{
		m_fsm.addTransition( eStartTls , sIdle , sStartingTls , &GSmtp::ServerProtocol::doStartTls , sIdle ) ;
		m_fsm.addTransition( eSecure   , sStartingTls , sIdle , &GSmtp::ServerProtocol::doSecure ) ;
	}
}

void GSmtp::ServerProtocol::init()
{
	sendGreeting( m_text.greeting() ) ;
}

GSmtp::ServerProtocol::~ServerProtocol()
{
	m_message.doneSignal().disconnect() ;
	m_verifier.doneSignal().disconnect() ;
}

void GSmtp::ServerProtocol::secure( const std::string & certificate )
{
	State new_state = m_fsm.apply( *this , eSecure , std::make_pair(certificate.data(),certificate.size()) ) ;
	if( new_state == s_Any )
		throw ProtocolDone( "protocol error" ) ;
}

void GSmtp::ServerProtocol::doSecure( EventData certificate , bool & )
{
	G_DEBUG( "GSmtp::ServerProtocol::doSecure" ) ;
	m_secure = true ;
	m_certificate = std::string(certificate.first,certificate.second) ;
}

void GSmtp::ServerProtocol::doStartTls( EventData , bool & ok )
{
	if( m_secure )
	{
		send( "503 command out of sequence" ) ;
		ok = false ;
	}
	else
	{
		send( "220 ready to start tls" , true ) ;
	}
}

void GSmtp::ServerProtocol::sendGreeting( const std::string & text )
{
	send( std::string("220 ") + text ) ;
}

bool GSmtp::ServerProtocol::apply( const char * line_data , size_t line_size , size_t /*eolsize*/ )
{
	Event event = eUnknown ;
	State state = m_fsm.state() ;
	EventData event_data( line_data , line_size ) ;
	if( (state == sData || state == sDiscarding) && isEndOfText(line_data,line_size) )
	{
		event = eEot ;
	}
	else if( state == sData || state == sDiscarding )
	{
		event = eContent ;
	}
	else if( state == sAuth )
	{
		event = eAuthData ;
	}
	else
	{
		std::string line( line_data , line_size ) ;
		G_LOG( "GSmtp::ServerProtocol: rx<<: \"" << G::Str::printable(line) << "\"" ) ;
		event = commandEvent( commandWord(line) ) ;
		m_buffer = commandLine( line ) ;
		event_data = EventData( m_buffer.data() , m_buffer.size() ) ;
	}

	State new_state = m_fsm.apply( *this , event , event_data ) ;
	if( new_state == s_Any )
		sendOutOfSequence( std::string(line_data,line_size) ) ;

	return true ; // see GNet::LineBuffer::apply()
}

void GSmtp::ServerProtocol::doContent( EventData event_data , bool & ok )
{
	if( isEscaped(event_data.first,event_data.second) )
		ok = m_message.addText( event_data.first+1 , event_data.second-1U ) ;
	else
		ok = m_message.addText( event_data.first , event_data.second ) ;

	// moves to discard state if not ok - discard state throws if so configured
	if( !ok && m_config.disconnect_on_max_size )
		sendTooBig( true ) ;
}

void GSmtp::ServerProtocol::doEot( EventData event_data , bool & )
{
	std::string line( event_data.first , event_data.second ) ;
	G_LOG( "GSmtp::ServerProtocol: rx<<: [message content not logged]" ) ;
	G_LOG( "GSmtp::ServerProtocol: rx<<: \"" << G::Str::printable(line) << "\"" ) ;
	if( m_config.filter_timeout != 0U )
	{
		G_DEBUG( "GSmtp::ServerProtocol: starting filter timer: " << m_config.filter_timeout ) ;
		startTimer( m_config.filter_timeout ) ;
	}
	m_message.process( m_sasl->id() , m_peer_address.hostPartString() , m_certificate ) ;
}

void GSmtp::ServerProtocol::processDone( bool success , unsigned long id , std::string response , std::string reason )
{
	G_IGNORE_PARAMETER( unsigned long , id ) ;
	G_IGNORE_PARAMETER( std::string , reason ) ;
	G_DEBUG( "GSmtp::ServerProtocol::processDone: " << (success?1:0) << " " << id << " [" << response << "] [" << reason << "]" ) ;
	G_ASSERT( success == response.empty() ) ;

	State new_state = m_fsm.apply( *this , eDone , std::make_pair(response.data(),response.size()) ) ;
	if( new_state == s_Any )
		throw ProtocolDone( "protocol error" ) ;
}

void GSmtp::ServerProtocol::onTimeout()
{
	G_WARNING( "GSmtp::ServerProtocol::onTimeout: message filter timed out after " << m_config.filter_timeout << "s" ) ;
	std::string reason = "timed out" ;
	State new_state = m_fsm.apply( *this , eTimeout , std::make_pair(reason.data(),reason.size()) ) ;
	if( new_state == s_Any )
		throw ProtocolDone( "protocol error" ) ;
}

void GSmtp::ServerProtocol::doComplete( EventData event_data , bool & )
{
	reset() ;
	const bool empty = event_data.second == 0U ;
	sendCompletionReply( empty , std::string(event_data.first,event_data.second) ) ;
}

void GSmtp::ServerProtocol::doQuit( EventData , bool & )
{
	reset() ;
	sendClosing() ; // never deletes this
	throw ProtocolDone() ;
}

void GSmtp::ServerProtocol::doDiscard( EventData , bool & )
{
	if( m_config.disconnect_on_max_size )
	{
		reset() ;
		sendClosing() ; // never deletes this
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
	std::string line( event_data.first , event_data.second ) ;
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

void GSmtp::ServerProtocol::verifyDone( std::string mbox , VerifierStatus status )
{
	if( status.abort )
		throw ProtocolDone( "verifier abort" ) ;

	std::string status_str = status.str( mbox ) ;
	State new_state = m_fsm.apply( *this , eVrfyReply , std::make_pair(status_str.data(),status_str.size()) ) ;
	if( new_state == s_Any )
		throw ProtocolDone( "protocol error" ) ;
}

void GSmtp::ServerProtocol::doVrfyReply( EventData event_data , bool & )
{
	std::string line( event_data.first , event_data.second ) ;
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
	size_t pos = line.find_first_of( " \t" ) ;
	if( pos != std::string::npos )
		to = line.substr(pos) ;

	G::Str::trim( to , " \t" ) ;
	return to ;
}

void GSmtp::ServerProtocol::doEhlo( EventData event_data , bool & predicate )
{
	std::string line( event_data.first , event_data.second ) ;
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
	std::string line( event_data.first , event_data.second ) ;
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

void GSmtp::ServerProtocol::doAuth( EventData event_data , bool & predicate )
{
	G::StringArray word_array ;
	G::Str::splitIntoTokens( std::string(event_data.first,event_data.second) , word_array , " \t" ) ;

	std::string mechanism = word_array.size() > 1U ? word_array[1U] : std::string() ;
	G::Str::toUpper( mechanism ) ;
	std::string initial_response = word_array.size() > 2U ? word_array[2U] : std::string() ;
	bool got_initial_response = word_array.size() > 2U ;

	G_DEBUG( "ServerProtocol::doAuth: [" << mechanism << "], [" << initial_response << "]" ) ;

	if( !m_secure && authenticationRequiresEncryption() )
	{
		G_WARNING( "GSmtp::ServerProtocol: rejecting authentication attempt without encryption" ) ;
		predicate = false ; // => idle
		send( "504 Unsupported authentication mechanism" ) ; // since none until encryption
	}
	else if( m_session_authenticated )
	{
		G_WARNING( "GSmtp::ServerProtocol: too many AUTH requests" ) ;
		predicate = false ; // => idle
		sendOutOfSequence(std::string(event_data.first,event_data.second)) ; // see RFC-2554 "Restrictions"
	}
	else if( ! m_sasl->init(mechanism) )
	{
		G_WARNING( "GSmtp::ServerProtocol: request for unsupported server AUTH mechanism: " << mechanism ) ;
		predicate = false ; // => idle
		send( "504 Unsupported authentication mechanism" ) ;
	}
	else if( got_initial_response && ! G::Base64::valid(initial_response) )
	{
		G_WARNING( "GSmtp::ServerProtocol: invalid base64 encoding of AUTH parameter" ) ;
		predicate = false ; // => idle
		send( "501 Invalid argument" ) ;
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

void GSmtp::ServerProtocol::sendAuthDone( bool ok )
{
	if( ok )
		send( "235 Authentication sucessful" ) ;
	else
		send( "535 Authentication failed" ) ;
}

void GSmtp::ServerProtocol::doAuthData( EventData event_data , bool & predicate )
{
	G_LOG( "GSmtp::ServerProtocol: rx<<: [authentication response not logged]" ) ;
	std::string line( event_data.first , event_data.second ) ;
	if( line == "*" )
	{
		predicate = false ; // => idle
		send( "501 authentication cancelled" ) ;
	}
	else if( ! G::Base64::valid(line) )
	{
		G_WARNING( "GSmtp::ServerProtocol: invalid base64 encoding of authentication response" ) ;
		predicate = false ; // => idle
		sendAuthDone( false ) ;
	}
	else
	{
		bool done = false ;
		std::string next_challenge = m_sasl->apply( G::Base64::decode(line) , done ) ;
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
}

void GSmtp::ServerProtocol::sendChallenge( const std::string & challenge )
{
	send( std::string("334 ") + G::Base64::encode(challenge,std::string()) ) ;
}

void GSmtp::ServerProtocol::doMail( EventData event_data , bool & predicate )
{
	std::string line( event_data.first , event_data.second ) ;
	if( !m_session_authenticated && m_sasl->active() && !m_sasl->trusted(m_peer_address) )
	{
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
		std::pair<std::string,std::string> from_pair = parseMailFrom( line ) ;
		bool ok = from_pair.second.empty() && m_message.setFrom( from_pair.first , parseMailAuth(line) ) ;
		predicate = ok ;
		if( ok )
		{
			sendMailReply() ;
		}
		else
		{
			sendBadFrom( from_pair.second ) ;
		}
	}
}

void GSmtp::ServerProtocol::doRcpt( EventData event_data , bool & predicate )
{
	std::pair<std::string,std::string> to_pair = parseRcptTo( std::string(event_data.first,event_data.second) ) ;
	std::string reason = to_pair.second ;
	bool ok = reason.empty() ;

	if( ok )
	{
		verify( to_pair.first , m_message.from() ) ;
	}
	else
	{
		predicate = false ;
		sendBadTo( reason , false ) ;
	}
}

void GSmtp::ServerProtocol::doVrfyToReply( EventData event_data , bool & predicate )
{
	std::string to ;
	VerifierStatus status = VerifierStatus::parse( std::string(event_data.first,event_data.second) , to ) ;

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
	sendUnrecognised( std::string(event_data.first,event_data.second) ) ;
}

void GSmtp::ServerProtocol::reset()
{
	// cancel the current message transaction -- ehlo/quit session unaffected
	cancelTimer() ;
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
	std::string received_line = m_text.received( m_session_peer_name , m_session_authenticated , m_secure ) ;
	if( received_line.length() )
		m_message.addReceived( received_line ) ;

	sendDataReply() ;
}

void GSmtp::ServerProtocol::sendOutOfSequence( const std::string & )
{
	send( "503 command out of sequence -- use RSET to resynchronise" ) ;
	badClientEvent() ;
}

void GSmtp::ServerProtocol::sendMissingParameter()
{
	send( "501 parameter required" ) ;
}

bool GSmtp::ServerProtocol::isEndOfText( const char * line , size_t line_size ) const
{
	return line_size == 1U && *line == '.' ;
}

bool GSmtp::ServerProtocol::isEscaped( const char * line , size_t line_size ) const
{
	return line_size > 1U && *line == '.' ;
}

std::string GSmtp::ServerProtocol::commandWord( const std::string & line_in ) const
{
	std::string line( line_in ) ;
	G::Str::trimLeft( line , " \t" ) ;

	size_t pos = line.find_first_of( " \t" ) ;
	std::string command = line.substr( 0U , pos ) ;

	G::Str::toUpper( command ) ;
	return command ;
}

std::string GSmtp::ServerProtocol::commandLine( const std::string & line_in ) const
{
	std::string line( line_in ) ;
	G::Str::trimLeft( line , " \t" ) ;
	return line ;
}

GSmtp::ServerProtocol::Event GSmtp::ServerProtocol::commandEvent( const std::string & command ) const
{
	if( command == "QUIT" ) return eQuit ;
	if( command == "HELO" ) return eHelo ;
	if( command == "EHLO" ) return eEhlo ;
	if( command == "RSET" ) return eRset ;
	if( command == "DATA" ) return eData ;
	if( command == "RCPT" ) return eRcpt ;
	if( command == "MAIL" ) return eMail ;
	if( command == "VRFY" ) return eVrfy ;
	if( command == "NOOP" ) return eNoop ;
	if( command == "EXPN" ) return eExpn ;
	if( command == "HELP" ) return eHelp ;
	if( command == "STARTTLS" ) return eStartTls ;
	if( m_sasl->active() && command == "AUTH" ) return eAuth ;
	return eUnknown ;
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

void GSmtp::ServerProtocol::sendUnrecognised( const std::string & line )
{
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
	send( "530 encryption required: use starttls" ) ;
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

void GSmtp::ServerProtocol::sendCompletionReply( bool ok , const std::string & reason )
{
	if( ok )
		sendOk() ;
	else
		send( "452 " + reason ) ;
}

void GSmtp::ServerProtocol::sendRcptReply()
{
	sendOk() ;
}

void GSmtp::ServerProtocol::sendBadFrom( std::string reason )
{
	std::string msg = "553 mailbox name not allowed" ;
	if( ! reason.empty() )
	{
		msg.append( ": " ) ;
		msg.append( reason ) ;
	}
	send( msg ) ;
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
		ss << "250-AUTH " << m_sasl->mechanisms() << crlf() ;

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

const std::string & GSmtp::ServerProtocol::crlf()
{
	static const std::string s( "\015\012" ) ;
	return s ;
}

void GSmtp::ServerProtocol::send( std::string line , bool go_secure )
{
	G_LOG( "GSmtp::ServerProtocol: tx>>: \"" << G::Str::printable(line) << "\"" ) ;
	line.append( crlf() ) ;
	m_sender.protocolSend( line , go_secure ) ;
}

size_t GSmtp::ServerProtocol::parseMailSize( const std::string & line ) const
{
	std::string parameter = parseMailParameter( line , "SIZE=" ) ;
	if( parameter.empty() || !G::Str::isULong(parameter) )
		return 0U ;
	else
		return static_cast<size_t>( G::Str::toULong(parameter,G::Str::Limited()) ) ;
}

std::string GSmtp::ServerProtocol::parseMailAuth( const std::string & line ) const
{
	return parseMailParameter( line , "AUTH=" ) ;
}

std::string GSmtp::ServerProtocol::parseMailParameter( const std::string & line , const std::string & key ) const
{
	std::string result ;
	size_t end = line.find( '>' ) ;
	if( end != std::string::npos )
	{
		G::StringArray parameters = G::Str::splitIntoTokens( line.substr(end) , " " ) ;
		for( G::StringArray::iterator p = parameters.begin() ; p != parameters.end() ; ++p )
		{
			size_t pos = G::Str::upper(*p).find( key ) ;
			if( pos == 0U && (*p).length() > key.size() )
			{
				result = G::Xtext::encode( G::Xtext::decode( (*p).substr(key.size()) ) ) ; // ensure valid xtext
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
	size_t start = line.find( '<' ) ;
	size_t end = line.find( '>' ) ;
	if( start == std::string::npos || end == std::string::npos || end < start )
	{
		std::string reason( "missing or invalid angle brackets in mailbox name" ) ;
		return std::make_pair(std::string(),reason) ;
	}

	std::string s = line.substr( start + 1U , end - start - 1U ) ;
	G::Str::trim( s , " \t" ) ;

	// strip source route
	if( s.length() > 0U && s.at(0U) == '@' )
	{
		size_t colon_pos = s.find( ':' ) ;
		if( colon_pos == std::string::npos )
		{
			std::string reason( "invalid mailbox name: no colon after leading at character" ) ;
			return std::make_pair(std::string(),reason) ;
		}
		s = s.substr( colon_pos + 1U ) ;
	}

	return std::make_pair(s,std::string()) ;
}

std::string GSmtp::ServerProtocol::parsePeerName( const std::string & line ) const
{
	size_t pos = line.find_first_of( " \t" ) ;
	if( pos == std::string::npos )
		return std::string() ;

	std::string smtp_peer_name = line.substr( pos + 1U ) ;
	G::Str::trim( smtp_peer_name , " \t" ) ;
	return smtp_peer_name ;
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
	bool authenticated , bool secure ) const
{
	return receivedLine( smtp_peer_name , m_peer_address.hostPartString() , m_thishost , authenticated , secure ) ;
}

std::string GSmtp::ServerProtocolText::receivedLine( const std::string & smtp_peer_name ,
	const std::string & peer_address , const std::string & thishost ,
	bool authenticated , bool secure )
{
	const G::EpochTime t = G::DateTime::now() ;
	const G::DateTime::BrokenDownTime tm = G::DateTime::local(t) ;
	const std::string zone = G::DateTime::offsetString(G::DateTime::offset(t)) ;
	const G::Date date( tm ) ;
	const G::Time time( tm ) ;
	const std::string esmtp = std::string("ESMTP") + (secure?"S":"") + (authenticated?"A":"") ; // RFC-3848

	std::ostringstream ss ;
	ss
		<< "Received: from " << smtp_peer_name
		<< " ("
			<< "[" << peer_address << "]"
		<< ") by " << thishost << " with " << esmtp << " ; "
		<< date.weekdayName(true) << ", "
		<< date.monthday() << " "
		<< date.monthName(true) << " "
		<< date.yyyy() << " "
		<< time.hhmmss(":") << " "
		<< zone ;
	return ss.str() ;
}

// ===

GSmtp::ServerProtocol::Text::~Text()
{
}

// ===

GSmtp::ServerProtocol::Sender::~Sender()
{
}

// ===

GSmtp::ServerProtocol::Config::Config( bool with_vrfy_ , unsigned int filter_timeout_ ,
	size_t max_size_ , bool authentication_requires_encryption_ , bool mail_requires_encryption_ ,
	bool advertise_tls_if_possible_ ) :
		with_vrfy(with_vrfy_) ,
		filter_timeout(filter_timeout_) ,
		max_size(max_size_) ,
		authentication_requires_encryption(authentication_requires_encryption_) ,
		mail_requires_encryption(mail_requires_encryption_) ,
		disconnect_on_max_size(false) ,
		advertise_tls_if_possible(advertise_tls_if_possible_)
{
}

/// \file gserverprotocol.cpp
