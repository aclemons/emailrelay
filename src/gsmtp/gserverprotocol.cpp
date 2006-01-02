//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// gserverprotocol.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gserverprotocol.h"
#include "gbase64.h"
#include "gdate.h"
#include "gtime.h"
#include "gdatetime.h"
#include "gstr.h"
#include "glog.h"
#include "gassert.h"
#include <string>

GSmtp::ServerProtocol::ServerProtocol( Sender & sender , Verifier & verifier , ProtocolMessage & pmessage ,
	const Secrets & secrets , Text & text , GNet::Address peer_address , Config config ) :
		m_sender(sender) ,
		m_verifier(verifier) ,
		m_pmessage(pmessage) ,
		m_text(text) ,
		m_peer_address(peer_address) ,
		m_fsm(sStart,sEnd,s_Same,s_Any) ,
		m_authenticated(false) ,
		m_sasl(secrets,false,true) ,
		m_with_vrfy(config.with_vrfy) ,
		m_preprocessor_timeout(config.preprocessor_timeout)
{
	m_pmessage.doneSignal().connect( G::slot(*this,&ServerProtocol::processDone) ) ;
	m_pmessage.preparedSignal().connect( G::slot(*this,&ServerProtocol::prepareDone) ) ;

	// (dont send anything to the peer from this ctor -- the Sender 
	// object is not fuly constructed)

	m_fsm.addTransition( eQuit    , s_Any   , sEnd     , &GSmtp::ServerProtocol::doQuit ) ;
	m_fsm.addTransition( eUnknown , s_Any   , s_Same   , &GSmtp::ServerProtocol::doUnknown ) ;
	m_fsm.addTransition( eRset    , s_Any   , sIdle    , &GSmtp::ServerProtocol::doRset ) ;
	m_fsm.addTransition( eNoop    , s_Any   , s_Same   , &GSmtp::ServerProtocol::doNoop ) ;
	m_fsm.addTransition( eHelp    , s_Any   , s_Same   , &GSmtp::ServerProtocol::doHelp ) ;
	m_fsm.addTransition( eExpn    , s_Any   , s_Same   , &GSmtp::ServerProtocol::doExpn ) ;
	m_fsm.addTransition( eVrfy    , s_Any   , s_Same   , &GSmtp::ServerProtocol::doVrfy ) ;
	m_fsm.addTransition( eEhlo    , s_Any   , sIdle    , &GSmtp::ServerProtocol::doEhlo , s_Same ) ;
	m_fsm.addTransition( eHelo    , s_Any   , sIdle    , &GSmtp::ServerProtocol::doHelo , s_Same ) ;
	m_fsm.addTransition( eMail    , sIdle   , sPrepare , &GSmtp::ServerProtocol::doMailPrepare , sIdle ) ;
	m_fsm.addTransition( ePrepared, sPrepare, sGotMail , &GSmtp::ServerProtocol::doMail , sIdle ) ;
	m_fsm.addTransition( eRcpt    , sGotMail, sGotRcpt , &GSmtp::ServerProtocol::doRcpt , sGotMail ) ;
	m_fsm.addTransition( eRcpt    , sGotRcpt, sGotRcpt , &GSmtp::ServerProtocol::doRcpt ) ;
	m_fsm.addTransition( eData    , sGotMail, sIdle    , &GSmtp::ServerProtocol::doNoRecipients ) ;
	m_fsm.addTransition( eData    , sGotRcpt, sData    , &GSmtp::ServerProtocol::doData ) ;
	m_fsm.addTransition( eContent , sData   , sData    , &GSmtp::ServerProtocol::doContent ) ;
	m_fsm.addTransition( eEot     , sData , sProcessing, &GSmtp::ServerProtocol::doEot ) ;

	if( m_sasl.active() )
	{
		m_fsm.addTransition( eAuth    , sStart  , sAuth    , &GSmtp::ServerProtocol::doAuth , sIdle ) ;
		m_fsm.addTransition( eAuth    , sIdle   , sAuth    , &GSmtp::ServerProtocol::doAuth , sIdle ) ;
		m_fsm.addTransition( eAuthData, sAuth   , sAuth    , &GSmtp::ServerProtocol::doAuthData , sIdle ) ;
	}
}

void GSmtp::ServerProtocol::init()
{
	sendGreeting( m_text.greeting() ) ;
}

GSmtp::ServerProtocol::~ServerProtocol()
{
	m_pmessage.doneSignal().disconnect() ;
	m_pmessage.preparedSignal().disconnect() ;
}

void GSmtp::ServerProtocol::sendGreeting( const std::string & text )
{
	send( std::string("220 ") + text ) ;
}

void GSmtp::ServerProtocol::apply( const std::string & line )
{
	Event event = eUnknown ;
	State state = m_fsm.state() ;
	const std::string * event_data = &line ;
	if( state == sData && isEndOfText(line) )
	{
		event = eEot ;
	}
	else if( state == sData )
	{
		event = eContent ;
	}
	else if( state == sAuth )
	{
		event = eAuthData ;
	}
	else
	{
		G_LOG( "GSmtp::ServerProtocol: rx<<: \"" << G::Str::toPrintableAscii(line) << "\"" ) ;
		event = commandEvent( commandWord(line) ) ;
		m_buffer = commandLine(line) ;
		event_data = &m_buffer ;
	}

	State new_state = m_fsm.apply( *this , event , *event_data ) ;
	const bool protocol_error = new_state == s_Any ;
	if( protocol_error )
		sendOutOfSequence( line ) ;
}

void GSmtp::ServerProtocol::doContent( const std::string & line , bool & )
{
	if( isEscaped(line) ) 
		m_pmessage.addText( line.substr(1U) ) ; // temporary string constructed, but rare
	else
		m_pmessage.addText( line ) ;
}

void GSmtp::ServerProtocol::doEot( const std::string & line , bool & )
{
	G_LOG( "GSmtp::ServerProtocol: rx<<: [message content not logged]" ) ;
	G_LOG( "GSmtp::ServerProtocol: rx<<: \"" << G::Str::toPrintableAscii(line) << "\"" ) ;
	if( m_preprocessor_timeout != 0U ) startTimer( m_preprocessor_timeout ) ;
	m_pmessage.process( m_sasl.id() , m_peer_address.displayString(false) ) ;
}

void GSmtp::ServerProtocol::processDone( bool success , unsigned long , std::string reason )
{
	G_DEBUG( "GSmtp::ServerProtocol::processDone: " << success << ", \"" << reason << "\"" ) ;
	G_ASSERT( m_fsm.state() == sProcessing ) ; // all exit paths from sProcessing call reset()
	if( m_fsm.state() != sProcessing ) return ;

	m_fsm.reset( sIdle ) ;
	reset() ;
	sendCompletionReply( success , reason ) ;
}

void GSmtp::ServerProtocol::onTimeout()
{
	G_WARNING( "GSmtp::ServerProtocol::onTimeout: message processing timed out" ) ;
	G_ASSERT( m_fsm.state() == sProcessing ) ; // all exit paths from sProcessing call reset()
	if( m_fsm.state() != sProcessing ) return ;

	m_fsm.reset( sIdle ) ;
	reset() ;
	sendCompletionReply( false , "message processing timed out" ) ;
}

void GSmtp::ServerProtocol::doQuit( const std::string & , bool & )
{
	reset() ;
	sendClosing() ; // never deletes this
	throw ProtocolDone() ;
}

void GSmtp::ServerProtocol::doNoop( const std::string & , bool & )
{
	sendOk() ;
}

void GSmtp::ServerProtocol::doExpn( const std::string & , bool & )
{
	sendNotImplemented() ;
}

void GSmtp::ServerProtocol::doHelp( const std::string & , bool & )
{
	sendNotImplemented() ;
}

void GSmtp::ServerProtocol::doVrfy( const std::string & line , bool & )
{
	if( m_with_vrfy )
	{
		std::string mbox = parseMailbox( line ) ;
		Verifier::Status rc = verify( mbox , "" ) ;
		if( rc.is_valid && rc.is_local )
			sendVerified( rc.full_name ) ; // 250
		else if( rc.is_valid )
			sendWillAccept( mbox ) ; // 252
		else
			sendNotVerified( mbox , rc.temporary ) ; // 550 or 450
	}
	else
	{
		sendNotImplemented() ;
	}
} 

GSmtp::Verifier::Status GSmtp::ServerProtocol::verify( const std::string & to , const std::string & from ) const
{
	std::string mechanism = m_sasl.active() ? m_sasl.mechanism() : std::string() ;
	std::string id = m_sasl.active() ? m_sasl.id() : std::string() ;
	if( m_sasl.active() && !m_authenticated )
		mechanism = "NONE" ;
	return m_verifier.verify( to , from , m_peer_address , mechanism , id ) ;
}

std::string GSmtp::ServerProtocol::parseMailbox( const std::string & line ) const
{
	std::string user ;
	size_t pos = line.find_first_of( " \t" ) ;
	if( pos != std::string::npos )
		user = line.substr(pos) ;

	G::Str::trim( user , " \t" ) ;
	return user ;
}

void GSmtp::ServerProtocol::doEhlo( const std::string & line , bool & predicate )
{
	std::string peer_name = parsePeerName( line ) ;
	if( peer_name.empty() )
	{
		predicate = false ;
		sendMissingParameter() ;
	}
	else
	{
		m_peer_name = peer_name ;
		reset() ;
		sendEhloReply() ;
	}
}

void GSmtp::ServerProtocol::doHelo( const std::string & line , bool & predicate )
{
	std::string peer_name = parsePeerName( line ) ;
	if( peer_name.empty() )
	{
		predicate = false ;
		sendMissingParameter() ;
	}
	else
	{
		m_peer_name = peer_name ;
		reset() ;
		sendHeloReply() ;
	}
}

void GSmtp::ServerProtocol::doAuth( const std::string & line , bool & predicate )
{
	G::StringArray word_array ;
	G::Str::splitIntoTokens( line , word_array , " \t" ) ;

	std::string mechanism = word_array.size() > 1U ? word_array[1U] : std::string() ;
	G::Str::toUpper( mechanism ) ;
	std::string initial_response = word_array.size() > 2U ? word_array[2U] : std::string() ;
	bool got_initial_response = word_array.size() > 2U ;

	G_DEBUG( "ServerProtocol::doAuth: [" << mechanism << "], [" << initial_response << "]" ) ;

	if( m_authenticated )
	{
		G_WARNING( "GSmtp::ServerProtocol: too many AUTHs" ) ;
		predicate = false ; // => idle
		sendOutOfSequence(line) ; // see RFC2554 "Restrictions"
	}
	else if( ! m_sasl.init(mechanism) )
	{
		G_WARNING( "GSmtp::ServerProtocol: request for unsupported AUTH mechanism: " << mechanism ) ;
		predicate = false ; // => idle
		send( "504 Unsupported authentication mechanism" ) ;
	}
	else if( got_initial_response && ! Base64::valid(initial_response) )
	{
		G_WARNING( "GSmtp::ServerProtocol: invalid base64 encoding of AUTH parameter" ) ;
		predicate = false ; // => idle
		send( "501 Invalid argument" ) ;
	}
	else if( got_initial_response )
	{
		std::string s = initial_response == "=" ? std::string() : Base64::decode(initial_response) ;
		bool done = false ;
		std::string next_challenge = m_sasl.apply( s , done ) ;
		if( done )
		{
			predicate = false ; // => idle
			m_authenticated = m_sasl.authenticated() ;
			sendAuthDone( m_sasl.authenticated() ) ;
		}
		else
		{
			sendChallenge( next_challenge ) ;
		}
	}
	else
	{
		sendChallenge( m_sasl.initialChallenge() ) ;
	}
}

void GSmtp::ServerProtocol::sendAuthDone( bool ok )
{
	if( ok )
		send( "235 Authentication sucessful" ) ;
	else
		send( "535 Authentication failed" ) ;
}

void GSmtp::ServerProtocol::doAuthData( const std::string & line , bool & predicate )
{
	G_LOG( "GSmtp::ServerProtocol: rx<<: [authentication response not logged]" ) ;
	if( line == "*" )
	{
		predicate = false ; // => idle
		send( "501 authentication cancelled" ) ;
	}
	else if( ! Base64::valid(line) )
	{
		G_WARNING( "GSmtp::ServerProtocol: invalid base64 encoding of authentication response" ) ;
		predicate = false ; // => idle
		sendAuthDone( false ) ;
	}
	else
	{
		bool done = false ;
		std::string next_challenge = m_sasl.apply( Base64::decode(line) , done ) ;
		if( done )
		{
			predicate = false ; // => idle
			m_authenticated = m_sasl.authenticated() ;
			sendAuthDone( m_sasl.authenticated() ) ;
		}
		else
		{
			sendChallenge( next_challenge ) ;
		}
	}
}

void GSmtp::ServerProtocol::sendChallenge( const std::string & s )
{
	send( std::string("334 ") + Base64::encode(s,std::string()) ) ;
}

void GSmtp::ServerProtocol::doMailPrepare( const std::string & line , bool & predicate )
{
	if( m_sasl.active() && !m_sasl.trusted(m_peer_address) && !m_authenticated )
	{
		predicate = false ;
		sendAuthRequired() ;
	}
	else
	{
		m_pmessage.clear() ;
		std::pair<std::string,std::string> from_pair = parseFrom( line ) ;
		bool ok = from_pair.second.empty() && m_pmessage.setFrom( from_pair.first ) ;
		predicate = ok ;
		if( ok )
		{
			bool async_prepare = m_pmessage.prepare() ;
			if( ! async_prepare )
				m_fsm.apply( *this , ePrepared , "" ) ; // re-entrancy ok
		}
		else
		{
			sendBadFrom( from_pair.second ) ;
		}
	}
}

void GSmtp::ServerProtocol::prepareDone( bool success , bool temporary_fault , std::string reason )
{
	G_DEBUG( "GSmtp::ServerProtocol::prepareDone: " << success << ", " 
		<< temporary_fault << ", \"" << reason << "\"" ) ;

	// as a kludge mark temporary failures by prepending a space -- see sendMailError()
	if( !success && temporary_fault )
		reason = std::string(" ")+reason ;

	m_fsm.apply( *this , ePrepared , reason ) ;
}

void GSmtp::ServerProtocol::doMail( const std::string & line , bool & predicate )
{
	// here 'line' comes from prepareDone(), or empty if no preparation stage
	G_DEBUG( "GSmtp::ServerProtocol::doMail: \"" << line << "\"" ) ;
	if( line.empty() )
	{
		sendMailReply() ;
	}
	else
	{
		predicate = false ;
		bool temporary = line.at(0U) == ' ' ;
		sendMailError( line.substr(temporary?1U:0U) , temporary ) ;
	}
}

void GSmtp::ServerProtocol::doRcpt( const std::string & line , bool & predicate )
{
	std::pair<std::string,std::string> to_pair = parseTo( line ) ;
	std::string reason = to_pair.second ;
	bool ok = reason.empty() ;
	bool temporary = false ;
	if( ok )
	{
		Verifier::Status status = verify( to_pair.first , m_pmessage.from() ) ;
		ok = m_pmessage.addTo( to_pair.first , status ) ;
		if( !ok )
		{
			reason = G::Str::toPrintableAscii(status.reason) ;
			temporary = status.temporary ;
		}
	}

	predicate = ok ;
	if( ok )
		sendRcptReply() ;
	else
		sendBadTo( reason , temporary ) ;
}

void GSmtp::ServerProtocol::doUnknown( const std::string & line , bool & )
{
	sendUnrecognised( line ) ;
}

void GSmtp::ServerProtocol::reset()
{
	cancelTimer() ;
	m_pmessage.clear() ;
}

void GSmtp::ServerProtocol::doRset( const std::string & , bool & )
{
	reset() ;
	sendRsetReply() ;
}

void GSmtp::ServerProtocol::doNoRecipients( const std::string & , bool & )
{
	sendNoRecipients() ;
}

void GSmtp::ServerProtocol::doData( const std::string & , bool & )
{
	std::string received_line = m_text.received(m_peer_name) ;
	if( received_line.length() )
		m_pmessage.addReceived( received_line ) ;

	sendDataReply() ;
}

void GSmtp::ServerProtocol::sendOutOfSequence( const std::string & )
{
	send( "503 command out of sequence -- use RSET to resynchronise" ) ;
}

void GSmtp::ServerProtocol::sendMissingParameter()
{
	send( "501 parameter required" ) ;
}

bool GSmtp::ServerProtocol::isEndOfText( const std::string & line ) const
{
	return line.length() == 1U && line.at(0U) == '.' ;
}

bool GSmtp::ServerProtocol::isEscaped( const std::string & line ) const
{
	return line.length() > 1U && line.at(0U) == '.' ;
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
	if( m_sasl.active() && command == "AUTH" ) return eAuth ;
	return eUnknown ;
}

void GSmtp::ServerProtocol::sendClosing()
{
	send( "221 closing connection" ) ;
}

void GSmtp::ServerProtocol::sendVerified( const std::string & user )
{
	send( std::string("250 ") + user ) ;
}

void GSmtp::ServerProtocol::sendNotVerified( const std::string & user , bool temporary )
{
	send( std::string() + (temporary?"450":"550") + " no such mailbox: " + user ) ;
}

void GSmtp::ServerProtocol::sendWillAccept( const std::string & user )
{
	send( std::string("252 cannot verify but will accept: ") + user ) ;
}

void GSmtp::ServerProtocol::sendUnrecognised( const std::string & line )
{
	send( "500 command unrecognized: \"" + line + std::string("\"") ) ;
}

void GSmtp::ServerProtocol::sendNotImplemented()
{
	send( "502 command not implemented" ) ;
}

void GSmtp::ServerProtocol::sendAuthRequired()
{
	send( "530 authentication required" ) ;
}

void GSmtp::ServerProtocol::sendNoRecipients()
{
	send( "554 no valid recipients" ) ;
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

void GSmtp::ServerProtocol::sendMailError( const std::string & reason , bool temporary )
{
	std::string number( temporary ? "452" : "550" ) ;
	send( number + " " + reason ) ;
}

void GSmtp::ServerProtocol::sendCompletionReply( bool ok , const std::string & reason )
{
	if( ok )
		sendOk() ;
	else
		send( std::string("452 message processing failed: ") + reason ) ;
}

void GSmtp::ServerProtocol::sendRcptReply()
{
	sendOk() ;
}

void GSmtp::ServerProtocol::sendBadFrom( std::string reason )
{
	std::string msg("553 mailbox name not allowed") ;
	if( ! reason.empty() )
	{
		msg.append( ": " ) ;
		msg.append( reason ) ;
	}
	send( msg ) ;
}

void GSmtp::ServerProtocol::sendBadTo( const std::string & text , bool temporary )
{
	send( std::string() + (temporary?"450":"550") + " mailbox unavailable: " + text ) ;
}

void GSmtp::ServerProtocol::sendEhloReply()
{
	std::ostringstream ss ;
		ss << "250-" << m_text.hello(m_peer_name) << crlf() ;
	if( m_sasl.active() )
		ss << "250-AUTH " << m_sasl.mechanisms() << crlf() ;
	if( m_with_vrfy )
		ss << "250-VRFY" << crlf() ; // see RFC2821-3.5.2
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

//static
std::string GSmtp::ServerProtocol::crlf()
{
	return std::string( "\015\012" ) ;
}

void GSmtp::ServerProtocol::send( std::string line )
{
	G_LOG( "GSmtp::ServerProtocol: tx>>: \"" << line << "\"" ) ;
	line.append( crlf() ) ;
	m_sender.protocolSend( line ) ;
}

std::pair<std::string,std::string> GSmtp::ServerProtocol::parseFrom( const std::string & line ) const
{
	// eg. MAIL FROM:<me@localhost>
	return parse( line ) ;
}

std::pair<std::string,std::string> GSmtp::ServerProtocol::parseTo( const std::string & line ) const
{
	// eg. RCPT TO:<@first.co.uk,@second.co.uk:you@final.co.uk>
	// eg. RCPT TO:<Postmaster>
	return parse( line ) ;
}

std::pair<std::string,std::string> GSmtp::ServerProtocol::parse( const std::string & line ) const
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
			std::string reason( "missing colon" ) ;
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

	std::string peer_name = line.substr( pos + 1U ) ;
	G::Str::trim( peer_name , " \t" ) ;
	return peer_name ;
}

// ===

GSmtp::ServerProtocolText::ServerProtocolText( const std::string & ident , const std::string & thishost ,
	const GNet::Address & peer_address ) :
		m_ident(ident) ,
		m_thishost(thishost) ,
		m_peer_address(peer_address)
{
}

std::string GSmtp::ServerProtocolText::greeting() const
{
	return m_thishost + " -- " + m_ident + " -- Service ready" ;
}

std::string GSmtp::ServerProtocolText::hello( const std::string & ) const
{
	return m_thishost + " says hello" ;
}

std::string GSmtp::ServerProtocolText::received( const std::string & peer_name ) const
{
	return receivedLine( peer_name , m_peer_address.displayString(false) , m_thishost ) ;
}

//static
std::string GSmtp::ServerProtocolText::receivedLine( const std::string & peer_name , 
	const std::string & peer_address , const std::string & thishost )
{
	G::DateTime::EpochTime t = G::DateTime::now() ;
	G::DateTime::BrokenDownTime tm = G::DateTime::local(t) ;
	std::string zone = G::DateTime::offsetString(G::DateTime::offset(t)) ;
	G::Date date( tm ) ;
	G::Time time( tm ) ;

	std::ostringstream ss ;
	ss 
		<< "Received: "
		<< "FROM " << peer_name << " "
		<< "([" << peer_address << "]) "
		<< "BY " << thishost << " "
		<< "WITH ESMTP "
		<< "; "
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

GSmtp::ServerProtocol::Config::Config( bool b , unsigned int i ) :
	with_vrfy(b) ,
	preprocessor_timeout(i)
{
}

