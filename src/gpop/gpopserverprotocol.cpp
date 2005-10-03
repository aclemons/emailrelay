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
#include "gassert.h"
#include "glog.h"
#include <sstream>

GPop::ServerProtocol::ServerProtocol( Sender & sender , Store & store , const Secrets & secrets , Text & , 
	GNet::Address peer_address , Config ) :
		m_sender(sender) ,
		m_store(store) ,
		m_store_lock(m_store) ,
		m_secrets(secrets) ,
		m_auth(m_secrets) ,
		m_peer_address(peer_address) ,
		m_fsm(sStart,sEnd,s_Same,s_Any)
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
	send( std::string() + "+OK POP3 server ready " + m_auth.challenge() ) ;
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
	G_LOG( "GPop::ServerProtocol: rx<<: \"" << G::Str::toPrintableAscii(line) << "\"" ) ;

	State new_state = m_fsm.apply( *this , commandEvent(commandWord(line)) , line ) ;
	const bool protocol_error = new_state == s_Any ;
	if( protocol_error )
		sendError() ;

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
		m_content.reset() ; // free up resources
		m_fsm.apply( *this , eSent , "" ) ; // sData -> sActive
	}
}

void GPop::ServerProtocol::resume()
{
	G_LOG_S( "GPop::ServerProtocol::resume" ) ;
	sendContent() ;
}

bool GPop::ServerProtocol::sendContentLine( std::string & line , bool & done )
{
	G_ASSERT( m_content.get() != NULL ) ;

	bool limited = m_content_limit == 0L ;
	if( m_content_limit > 0L )
		m_content_limit-- ;

	line.erase( 1U ) ; // leave "."
	G::Str::readLineFrom( *(m_content.get()) , crlf() , line , false ) ;

	bool eof = ! m_content->good() ;
	//G_DEBUG( "ServerProtocol::sendContentLine: " << (line.length()-1U) << ", " << eof << ", " << limited ) ;
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
	bool all_sent = m_sender.protocolSend( line , offset ) ;

	done = ( limited || eof ) && all_sent ;
	bool more = ! ( limited || eof ) ;
	return more ;
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

	return eUnknown ;
}

void GPop::ServerProtocol::doQuitEarly( const std::string & , bool & )
{
	cancelTimer() ;
	send( "+OK signing off" ) ;
	throw ProtocolDone() ;
}

void GPop::ServerProtocol::doQuit( const std::string & , bool & )
{
	cancelTimer() ;
	m_store_lock.commit() ;
	send( "+OK signing off" ) ;
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

	GPop::StoreLock::List list = m_store_lock.list( id ) ;
	std::ostringstream ss ;
	ss << "+OK " << list.size() << " message(s)" << crlf() ;
	for( GPop::StoreLock::List::iterator p = list.begin() ; p != list.end() ; ++p )
	{
		ss << (*p).id << " " ;
		if( uidl ) ss << (*p).uidl ;
		if( !uidl ) ss << (*p).size ;
		ss << crlf() ;
	}
	ss << "." ;
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
		m_content_limit = -1L ;

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
		m_content_limit = n ;
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

void GPop::ServerProtocol::doUser( const std::string & line , bool & )
{
	m_user = commandParameter(line) ;
	send( std::string() + "+OK user: " + commandParameter(line) ) ;
}

void GPop::ServerProtocol::doPass( const std::string & line , bool & ok )
{
	ok = m_auth.valid() && m_auth.init("LOGIN") && m_auth.authenticated(m_user,commandParameter(line)) ;
	if( ok )
	{
		m_store_lock.lock( m_user ) ;
		sendOk() ;
	}
	else
	{
		sendError() ;
	}
}

void GPop::ServerProtocol::doApop( const std::string & line , bool & ok )
{
	std::string m_user = commandParameter(line,1) ;
	ok = m_auth.valid() && m_auth.authenticated(m_user+" "+commandParameter(line,2),std::string()) ;
	if( ok )
	{
		m_store_lock.lock( m_user ) ;
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
	G_LOG( "GPop::ServerProtocol: tx>>: \"" << line << "\"" ) ;
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

std::string GPop::ServerProtocolText::dummy() const
{
	return "" ;
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

