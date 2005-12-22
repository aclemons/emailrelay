//
// Copyright (C) 2001-2005 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gpopserverprotocol.cpp
//

#include "gdef.h"
#include "gpop.h"
#include "gpopserverprotocol.h"
#include "gstr.h"
#include "gmemory.h"
#include "gbase64.h"
#include "gassert.h"
#include "glog.h"
#include <sstream>

GPop::ServerProtocol::ServerProtocol( Sender & sender , Store & store , const Secrets & secrets , const Text & text , 
	GNet::Address peer_address , Config ) :
		m_text(text) ,
		m_sender(sender) ,
		m_store(store) ,
		m_store_lock(m_store) ,
		m_secrets(secrets) ,
		m_auth(m_secrets) ,
		m_peer_address(peer_address) ,
		m_fsm(sStart,sEnd,s_Same,s_Any) ,
		m_body_limit(-1L) ,
		m_in_body(false)
{
	// (dont send anything to the peer from this ctor -- the Sender object is not fuly constructed)

	m_fsm.addTransition( eStat , sActive , sActive , &GPop::ServerProtocol::doStat ) ;
	m_fsm.addTransition( eList , sActive , sActive , &GPop::ServerProtocol::doList ) ;
	m_fsm.addTransition( eRetr , sActive , sData , &GPop::ServerProtocol::doRetr , sActive ) ;
	m_fsm.addTransition( eTop , sActive , sData , &GPop::ServerProtocol::doTop , sActive ) ;
	m_fsm.addTransition( eDele , sActive , sActive , &GPop::ServerProtocol::doDele ) ;
	m_fsm.addTransition( eNoop , sActive , sActive , &GPop::ServerProtocol::doNoop ) ;
	m_fsm.addTransition( eRset , sActive , sActive , &GPop::ServerProtocol::doRset ) ;
	m_fsm.addTransition( eUidl , sActive , sActive , &GPop::ServerProtocol::doUidl ) ;
	m_fsm.addTransition( eSent , sData , sActive , &GPop::ServerProtocol::doNothing ) ;
	m_fsm.addTransition( eUser , sStart , sStart , &GPop::ServerProtocol::doUser ) ;
	m_fsm.addTransition( ePass , sStart , sActive , &GPop::ServerProtocol::doPass , sStart ) ;
	m_fsm.addTransition( eApop , sStart , sActive , &GPop::ServerProtocol::doApop , sStart ) ;
	m_fsm.addTransition( eApop , sStart , sEnd , &GPop::ServerProtocol::doQuitEarly ) ;
	m_fsm.addTransition( eCapa , sStart , sStart , &GPop::ServerProtocol::doCapa ) ;
	m_fsm.addTransition( eAuth , sStart , sAuth , &GPop::ServerProtocol::doAuth , sStart ) ;
	m_fsm.addTransition( eAuthData , sAuth , sActive , &GPop::ServerProtocol::doAuthData , sStart ) ;
	m_fsm.addTransition( eCapa , sActive , sActive , &GPop::ServerProtocol::doCapa ) ;
	m_fsm.addTransition( eQuit , sActive , sEnd , &GPop::ServerProtocol::doQuit ) ;
}

void GPop::ServerProtocol::init()
{
	sendInit() ;
}

GPop::ServerProtocol::~ServerProtocol()
{
}

void GPop::ServerProtocol::sendInit()
{
	std::string greeting = std::string() + "+OK " + m_text.greeting() ;
	std::string apop_challenge = m_auth.challenge() ;
	if( ! apop_challenge.empty() )
	{
		greeting.append( " " ) ;
		greeting.append( apop_challenge ) ;
	}
	send( greeting ) ;
}

void GPop::ServerProtocol::sendOk()
{
	send( "+OK" ) ;
}

void GPop::ServerProtocol::sendError()
{
	send( "-ERR" ) ;
}

void GPop::ServerProtocol::apply( const std::string & line )
{
	// decode the event
	Event event = m_fsm.state() == sAuth ? eAuthData : commandEvent(commandWord(line)) ;

	// log the input
	std::string log_text = event == ePass ? 
		(commandPart(line,0U)+" [password not logged]") : G::Str::toPrintableAscii(line) ;
	G_LOG( "GPop::ServerProtocol: rx<<: \"" << log_text << "\"" ) ;

	// apply the event to the state machine
	State new_state = m_fsm.apply( *this , event , line ) ;
	const bool protocol_error = new_state == s_Any ;
	if( protocol_error )
		sendError() ;

	// squirt data down the pipe if appropriate
	if( new_state == sData )
		sendContent() ;
}

void GPop::ServerProtocol::sendContent()
{
	// send until no more content or until blocked by flow-control
	std::string line( 200 , '.' ) ;
	size_t n = 0 ;
	bool end_of_content = false ;
	while( sendContentLine(line,end_of_content) )
		n++ ;

	G_LOG( "GPop::ServerProtocol: tx>>: [" << n << " line(s) of content]" ) ;

	if( end_of_content )
	{
		G_LOG( "GPop::ServerProtocol: tx>>: ." ) ;
		m_content <<= 0 ; // free up resources
		m_fsm.apply( *this , eSent , "" ) ; // sData -> sActive
	}
}

void GPop::ServerProtocol::resume()
{
	G_LOG_S( "GPop::ServerProtocol::resume" ) ;
	sendContent() ;
}

bool GPop::ServerProtocol::sendContentLine( std::string & line , bool & stop )
{
	G_ASSERT( m_content.get() != NULL ) ;

	// maintain the line limit
	bool limited = m_in_body && m_body_limit == 0L ;
	if( m_body_limit > 0L && m_in_body )
		m_body_limit-- ;

	// read the line of text
	line.erase( 1U ) ; // leave "."
	G::Str::readLineFrom( *(m_content.get()) , crlf() , line , false ) ;

	// add crlf and choose an offset
	bool eof = ! m_content->good() ;
	size_t offset = 0U ;
	if( eof || limited )
	{
		if( eof && line.length() > 1 ) 
			G_WARNING( "ServerProtocol::sendContentLine: discarding unterminated line" ) ;
		line.erase( 1U ) ;
		line.append( crlf() ) ;
	}
	else
	{
		line.append( crlf() ) ;
		offset = line.at(1U) == '.' ? 0U : 1U ;
	}

	// maintain the in-body flag
	if( !m_in_body && line.length() == (offset+2U) ) 
		m_in_body = true ;

	// send it
	bool line_fully_sent = m_sender.protocolSend( line , offset ) ;

	// continue to send while not finished or blocked by flow-control
	stop = ( limited || eof ) && line_fully_sent ;
	const bool pause = limited || eof || ! line_fully_sent ;
	return !pause ;
}

int GPop::ServerProtocol::commandNumber( const std::string & line , int default_ , size_t index ) const
{
	int number = default_ ;
	try { number = G::Str::toInt( commandParameter(line,index) ) ; }
	catch( G::Str::Overflow & ) {}
	catch( G::Str::InvalidFormat & ) {}
	return number ;
}

std::string GPop::ServerProtocol::commandWord( const std::string & line ) const
{
	return G::Str::upper(commandPart(line,0U)) ;
}

std::string GPop::ServerProtocol::commandPart( const std::string & line , size_t index ) const
{
	G::Strings part ;
	G::Str::splitIntoTokens( line , part , " \t\r\n" ) ;
	if( index >= part.size() ) return std::string() ;
	G::Strings::iterator p = part.begin() ;
	for( ; index > 0 ; ++p , index-- ) ;
	return *p ;
}

std::string GPop::ServerProtocol::commandParameter( const std::string & line_in , size_t index ) const
{
	return commandPart( line_in , index ) ;
}

GPop::ServerProtocol::Event GPop::ServerProtocol::commandEvent( const std::string & command ) const
{
	if( command == "QUIT" ) return eQuit ;
	if( command == "STAT" ) return eStat ;
	if( command == "LIST" ) return eList ;
	if( command == "RETR" ) return eRetr ;
	if( command == "DELE" ) return eDele ;
	if( command == "NOOP" ) return eNoop ;
	if( command == "RSET" ) return eRset ;
	//
	if( command == "TOP" ) return eTop ;
	if( command == "UIDL" ) return eUidl ;
	if( command == "USER" ) return eUser ;
	if( command == "PASS" ) return ePass ;
	if( command == "APOP" ) return eApop ;
	if( command == "AUTH" ) return eAuth ;
	if( command == "CAPA" ) return eCapa ;

	return eUnknown ;
}

void GPop::ServerProtocol::doQuitEarly( const std::string & , bool & )
{
	cancelTimer() ;
	send( std::string() + "+OK " + m_text.quit() ) ;
	throw ProtocolDone() ;
}

void GPop::ServerProtocol::doQuit( const std::string & , bool & )
{
	cancelTimer() ;
	m_store_lock.commit() ;
	send( std::string() + "+OK " + m_text.quit() ) ;
	throw ProtocolDone() ;
}

void GPop::ServerProtocol::doStat( const std::string & , bool & )
{
	std::ostringstream ss ;
	ss << "+OK " << m_store_lock.messageCount() << " " << m_store_lock.totalByteCount() ;
	send( ss.str() ) ;
}

void GPop::ServerProtocol::doUidl( const std::string & line , bool & )
{
	sendList( line , true ) ;
}

void GPop::ServerProtocol::doList( const std::string & line , bool & )
{
	sendList( line , false ) ;
}

void GPop::ServerProtocol::sendList( const std::string & line , bool uidl )
{
	std::string id_string = commandParameter( line ) ;

	// parse and check the id if supplied
	int id = -1 ;
	if( ! id_string.empty() )
	{
		id = commandNumber( line , -1 ) ;
		if( !m_store_lock.valid(id) )
		{
			sendError() ;
			return ;
		}
	}

	// send back the list with sizes or uidls
	bool multi_line = id == -1 ;
	GPop::StoreLock::List list = m_store_lock.list( id ) ;
	std::ostringstream ss ;
	ss << "+OK " ;
	if( multi_line ) ss << list.size() << " message(s)" << crlf() ;
	for( GPop::StoreLock::List::iterator p = list.begin() ; p != list.end() ; ++p )
	{
		ss << (*p).id << " " ;
		if( uidl ) ss << (*p).uidl ;
		if( !uidl ) ss << (*p).size ;
		if( multi_line ) ss << crlf() ;
	}
	if( multi_line ) ss << "." ;
	send( ss.str() ) ;
}

void GPop::ServerProtocol::doRetr( const std::string & line , bool & more )
{
	int id = commandNumber( line , -1 ) ;
	if( id == -1 || ! m_store_lock.valid(id) )
	{
		more = false ; // stay in the same state
		sendError() ;
	}
	else
	{
		m_content = m_store_lock.get( id ) ;
		m_body_limit = -1L ;

		std::ostringstream ss ;
		ss << "+OK " << m_store_lock.byteCount(id) << " octets" ;
		send( ss.str() ) ;
	}
}

void GPop::ServerProtocol::doTop( const std::string & line , bool & more )
{
	int id = commandNumber( line , -1 , 1U ) ;
	int n = commandNumber( line , -1 , 2U ) ;
	G_DEBUG( "ServerProtocol::doTop: " << id << ", " << n ) ;
	if( id == -1 || ! m_store_lock.valid(id) || n < 0 )
	{
		more = false ; // stay in the same state
		sendError() ;
	}
	else
	{
		m_content = m_store_lock.get( id ) ;
		m_body_limit = n ;
		m_in_body = false ;
		sendOk() ;
	}
}

void GPop::ServerProtocol::doDele( const std::string & line , bool & )
{
	int id = commandNumber( line , -1 ) ;
	if( id == -1 || ! m_store_lock.valid(id) )
	{
		sendError() ;
	}
	else
	{
		m_store_lock.remove( id ) ;
		sendOk() ;
	}
}

void GPop::ServerProtocol::doRset( const std::string & , bool & )
{
	m_store_lock.rollback() ;
	sendOk() ;
}

void GPop::ServerProtocol::doNoop( const std::string & , bool & )
{
	sendOk() ;
}

void GPop::ServerProtocol::doNothing( const std::string & , bool & )
{
}

void GPop::ServerProtocol::doAuth( const std::string & line , bool & ok )
{
	std::string mechanism = G::Str::upper( commandParameter(line) ) ;

	// reject the LOGIN mechanism here since we did not avertise it
	// and we only want a one-step challenge-response dialogue

	bool supported = mechanism != "LOGIN" && m_auth.init( mechanism ) ;
	if( ! supported )
	{
		ok = false ;
		sendError() ;
	}
	else
	{
		std::string initial_challenge = m_auth.challenge() ;
		send( std::string() + "+ " + GSmtp::Base64::encode(initial_challenge) ) ;
	}
}

void GPop::ServerProtocol::doAuthData( const std::string & line , bool & ok )
{
	// (only one-step sasl authentication supported for now)
	ok = m_auth.authenticated( GSmtp::Base64::decode(line) , std::string() ) ;
	if( ok )
	{
		sendOk() ; 
		m_user = m_auth.id() ;
		lockStore() ;
	}
	else
	{
		sendError() ;
	}
}

void GPop::ServerProtocol::lockStore()
{
	m_store_lock.lock( m_user ) ;
	G_LOG_S( "GPop::ServerProtocol: pop authentication of " << m_user 
		<< " connected from " << m_peer_address.displayString() ) ;
}

void GPop::ServerProtocol::doCapa( const std::string & , bool & )
{
	send( std::string() + "+OK " + m_text.capa() ) ;
	send( "USER" ) ;
	send( "TOP" ) ;
	send( "UIDL" ) ;
	if( ! m_auth.mechanisms().empty() )
		send( std::string() + "SASL " + m_auth.mechanisms() ) ;
	send( "." ) ;
}

void GPop::ServerProtocol::doUser( const std::string & line , bool & )
{
	m_user = commandParameter(line) ;
	send( std::string() + "+OK " + m_text.user(commandParameter(line)) ) ;
}

void GPop::ServerProtocol::doPass( const std::string & line , bool & ok )
{
	ok = m_auth.valid() && m_auth.init("LOGIN") && m_auth.authenticated(m_user,commandParameter(line)) ;
	if( ok )
	{
		lockStore() ;
		sendOk() ;
	}
	else
	{
		sendError() ;
	}
}

void GPop::ServerProtocol::doApop( const std::string & line , bool & ok )
{
	m_user = commandParameter(line,1) ;
	ok = m_auth.valid() && m_auth.authenticated(m_user+" "+commandParameter(line,2),std::string()) ;
	if( ok )
	{
		lockStore() ;
		sendOk() ;
	}
	else
	{
		sendError() ;
	}
}

void GPop::ServerProtocol::onTimeout()
{
	// not used
	G_WARNING( "GPop::ServerProtocol::onTimeout: operation timed out" ) ;
}


void GPop::ServerProtocol::send( std::string line )
{
	G_LOG( "GPop::ServerProtocol: tx>>: \"" << G::Str::toPrintableAscii(line) << "\"" ) ;
	line.append( crlf() ) ;
	m_sender.protocolSend( line , 0U ) ;
}

std::string GPop::ServerProtocol::crlf()
{
	return "\r\n" ;
}

// ===

GPop::ServerProtocolText::ServerProtocolText( GNet::Address )
{
}

std::string GPop::ServerProtocolText::greeting() const
{
	return "POP3 server ready" ;
}

std::string GPop::ServerProtocolText::quit() const
{
	return "signing off" ;
}

std::string GPop::ServerProtocolText::capa() const
{
	return "capability list follows" ;
}

std::string GPop::ServerProtocolText::user( const std::string & id ) const
{
	return std::string() + "user: " + id ;
}

// ===

GPop::ServerProtocol::Text::~Text()
{
}

// ===

GPop::ServerProtocol::Sender::~Sender()
{
}

// ===

GPop::ServerProtocol::Config::Config()
{
}

