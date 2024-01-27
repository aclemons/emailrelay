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
/// \file gpopserverprotocol.cpp
///

#include "gdef.h"
#include "gpopserverprotocol.h"
#include "gsaslserverfactory.h"
#include "gstr.h"
#include "gstringtoken.h"
#include "gtest.h"
#include "gbase64.h"
#include "gassert.h"
#include "glog.h"
#include <sstream>
#include <algorithm>

GPop::ServerProtocol::ServerProtocol( Sender & sender , Security & security , Store & store ,
	const GAuth::SaslServerSecrets & server_secrets , const std::string & sasl_server_config ,
	const Text & text , const GNet::Address & peer_address , const Config & config ) :
		m_text(text) ,
		m_sender(sender) ,
		m_security(security) ,
		m_store(store) ,
		m_config(config) ,
		m_sasl(GAuth::SaslServerFactory::newSaslServer(server_secrets,true,sasl_server_config,config.sasl_server_challenge_domain)) ,
		m_peer_address(peer_address) ,
		m_fsm(State::sStart,State::sEnd,State::s_Same,State::s_Any)
{
	// (dont send anything to the peer from this ctor -- the Sender object is not fuly constructed)

	m_fsm( Event::eStat , State::sActive , State::sActive , &GPop::ServerProtocol::doStat ) ;
	m_fsm( Event::eList , State::sActive , State::sActive , &GPop::ServerProtocol::doList ) ;
	m_fsm( Event::eRetr , State::sActive , State::sData , &GPop::ServerProtocol::doRetr , State::sActive ) ;
	m_fsm( Event::eTop , State::sActive , State::sData , &GPop::ServerProtocol::doTop , State::sActive ) ;
	m_fsm( Event::eDele , State::sActive , State::sActive , &GPop::ServerProtocol::doDele ) ;
	m_fsm( Event::eNoop , State::sActive , State::sActive , &GPop::ServerProtocol::doNoop ) ;
	m_fsm( Event::eRset , State::sActive , State::sActive , &GPop::ServerProtocol::doRset ) ;
	m_fsm( Event::eUidl , State::sActive , State::sActive , &GPop::ServerProtocol::doUidl ) ;
	m_fsm( Event::eSent , State::sData , State::sActive , &GPop::ServerProtocol::doNothing ) ;
	m_fsm( Event::eUser , State::sStart , State::sStart , &GPop::ServerProtocol::doUser ) ;
	m_fsm( Event::ePass , State::sStart , State::sActive , &GPop::ServerProtocol::doPass , State::sStart ) ;
	m_fsm( Event::eApop , State::sStart , State::sActive , &GPop::ServerProtocol::doApop , State::sStart ) ;
	m_fsm( Event::eQuit , State::sStart , State::sEnd , &GPop::ServerProtocol::doQuitEarly ) ;
	m_fsm( Event::eCapa , State::sStart , State::sStart , &GPop::ServerProtocol::doCapa ) ;
	m_fsm( Event::eCapa , State::sActive , State::sActive , &GPop::ServerProtocol::doCapa ) ;
	if( m_security.securityEnabled() )
		m_fsm( Event::eStls , State::sStart , State::sStart , &GPop::ServerProtocol::doStls , State::sStart ) ;
	m_fsm( Event::eAuth , State::sStart , State::sAuth , &GPop::ServerProtocol::doAuth , State::sStart ) ;
	m_fsm( Event::eAuthData , State::sAuth , State::sAuth , &GPop::ServerProtocol::doAuthData , State::sStart ) ;
	m_fsm( Event::eAuthComplete , State::sAuth , State::sActive , &GPop::ServerProtocol::doAuthComplete ) ;
	m_fsm( Event::eCapa , State::sActive , State::sActive , &GPop::ServerProtocol::doCapa ) ;
	m_fsm( Event::eQuit , State::sActive , State::sEnd , &GPop::ServerProtocol::doQuit ) ;
}

void GPop::ServerProtocol::init()
{
	sendInit() ;
}

void GPop::ServerProtocol::sendInit()
{
	std::string greeting = std::string("+OK ",4U).append(m_text.greeting()) ;
	if( m_sasl->init( m_secure , "APOP" ) )
	{
		m_sasl_init_apop = true ;
		std::string apop_challenge = m_sasl->initialChallenge() ;
		if( !apop_challenge.empty() )
		{
			greeting.append( 1U , ' ' ) ;
			greeting.append( apop_challenge ) ;
		}
	}
	sendLine( std::move(greeting) ) ;
}

void GPop::ServerProtocol::sendOk()
{
	sendLine( "+OK\r\n"_sv , true ) ;
}

void GPop::ServerProtocol::sendError( const std::string & more )
{
	if( more.empty() )
		sendError() ;
	else
		sendLine( std::string("-ERR ",5U).append(more) ) ;
}

void GPop::ServerProtocol::sendError()
{
	sendLine( "-ERR\r\n"_sv , true ) ;
}

void GPop::ServerProtocol::apply( const std::string & line )
{
	// decode the event
	Event event = m_fsm.state() == State::sAuth ? Event::eAuthData : commandEvent(commandWord(line)) ;

	// log the input
	std::string log_text = G::Str::printable(line) ;
	if( event == Event::ePass )
		log_text = (commandPart(line,0U)+" [password not logged]") ;
	if( event == Event::eAuthData || event == Event::eAuthComplete )
		log_text = "[authentication response not logged]" ;
	if( event == Event::eAuth && !commandPart(line,1U).empty() )
		log_text = commandPart(line,0U) + " " + commandPart(line,1U) ;
	G_LOG( "GPop::ServerProtocol: rx<<: \"" << log_text << "\"" ) ;

	// apply the event to the state machine
	State new_state = m_fsm.apply( *this , event , line ) ;
	const bool protocol_error = new_state == State::s_Any ;
	if( protocol_error )
	{
		G_DEBUG( "GPop::ServerProtocol::apply: protocol error: " << static_cast<int>(event) << " " << static_cast<int>(m_fsm.state()) ) ;
		sendError() ;
	}

	// squirt data down the pipe if appropriate
	if( new_state == State::sData )
		sendContent() ;
}

void GPop::ServerProtocol::sendContent()
{
	// send until no more content or until blocked by flow-control
	std::string line( 200 , '.' ) ;
	std::size_t n = 0 ;
	bool eot = false ;
	while( sendContentLine(line,eot) && !eot )
		n++ ;

	G_LOG( "GPop::ServerProtocol: tx>>: [" << n << " line(s) of content]" ) ;
	if( eot )
	{
		G_LOG( "GPop::ServerProtocol: tx>>: \".\"" ) ;
		m_content.reset() ; // free up resources
		m_fsm.apply( *this , Event::eSent , "" ) ; // State::sData -> State::sActive
	}
}

void GPop::ServerProtocol::resume()
{
	// flow control is not an issue for protocol responses because we
	// always send a complete protocol response in one go -- however,
	// message content is sent in chunks so the resume() has to send
	// the next bit
	G_DEBUG( "GPop::ServerProtocol::resume: flow control released" ) ;
	if( m_fsm.state() == State::sData )
		sendContent() ;
}

bool GPop::ServerProtocol::sendContentLine( std::string & line , bool & eot )
{
	G_ASSERT( m_content != nullptr ) ;

	bool limited = m_in_body && m_body_limit == 0L ;
	if( m_body_limit > 0L && m_in_body )
		m_body_limit-- ;

	line.erase( 1U ) ; // leave "."
	bool eof = !G::Str::readLine( *m_content , line ,
		m_config.crlf_only ? G::Str::Eol::CrLf : G::Str::Eol::Cr_Lf_CrLf ,
		/*pre_erase_result=*/false ) ;

	eot = eof || limited ;
	if( eot ) line.erase( 1U ) ;
	line.append( "\r\n" , 2U ) ;
	std::size_t offset = eot ? 0U : ( line.at(1U) == '.' ? 0U : 1U ) ;

	if( !m_in_body && line.length() == (offset+2U) )
		m_in_body = true ;

	return m_sender.protocolSend( line , offset ) ;
}

int GPop::ServerProtocol::commandNumber( const std::string & line , int default_ , std::size_t index ) const
{
	int number = default_ ;
	try
	{
		number = G::Str::toInt( commandParameter(line,index) ) ;
	}
	catch( G::Str::Overflow & ) // defaulted
	{
	}
	catch( G::Str::InvalidFormat & ) // defaulted
	{
	}
	return number ;
}

std::string GPop::ServerProtocol::commandWord( const std::string & line ) const
{
	return G::Str::upper(commandPart(line,0U)) ;
}

std::string GPop::ServerProtocol::commandPart( const std::string & line , std::size_t index ) const
{
	G::string_view line_sv( line ) ;
	G::StringTokenView t( line_sv , G::Str::ws() ) ;
	for( std::size_t i = 0 ; i < index ; ++t , i++ )
		{;}
	return t.valid() ? G::sv_to_string(t()) : std::string() ;
}

std::string GPop::ServerProtocol::commandParameter( const std::string & line_in , std::size_t index ) const
{
	return commandPart( line_in , index ) ;
}

GPop::ServerProtocol::Event GPop::ServerProtocol::commandEvent( G::string_view command ) const
{
	if( command == "QUIT"_sv ) return Event::eQuit ;
	if( command == "STAT"_sv ) return Event::eStat ;
	if( command == "LIST"_sv ) return Event::eList ;
	if( command == "RETR"_sv ) return Event::eRetr ;
	if( command == "DELE"_sv ) return Event::eDele ;
	if( command == "NOOP"_sv ) return Event::eNoop ;
	if( command == "RSET"_sv ) return Event::eRset ;
	//
	if( command == "TOP"_sv ) return Event::eTop ;
	if( command == "UIDL"_sv ) return Event::eUidl ;
	if( command == "USER"_sv ) return Event::eUser ;
	if( command == "PASS"_sv ) return Event::ePass ;
	if( command == "APOP"_sv ) return Event::eApop ;
	if( command == "AUTH"_sv ) return Event::eAuth ;
	if( command == "CAPA"_sv ) return Event::eCapa ;
	if( command == "STLS"_sv ) return Event::eStls ;

	return Event::eUnknown ;
}

void GPop::ServerProtocol::doQuitEarly( const std::string & , bool & )
{
	sendLine( std::string("+OK ",4).append(m_text.quit()) ) ;
	throw ProtocolDone() ;
}

void GPop::ServerProtocol::doQuit( const std::string & , bool & )
{
	m_store_list.commit() ;
	sendLine( std::string("+OK ",4U).append(m_text.quit()) ) ;
	throw ProtocolDone() ;
}

void GPop::ServerProtocol::doStat( const std::string & , bool & )
{
	sendLine( std::string("+OK ",4U)
		.append(std::to_string(m_store_list.messageCount()))
		.append(1U,' ')
 		.append(std::to_string(m_store_list.totalByteCount())) ) ;
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
	if( !id_string.empty() )
	{
		id = commandNumber( line , -1 ) ;
		if( !m_store_list.valid(id) )
		{
			sendError( "invalid id" ) ;
			return ;
		}
	}

	// send back the list with sizes or uidls
	std::ostringstream ss ;
	ss << "+OK " ;
	bool multi_line = id == -1 ;
	if( multi_line )
	{
		ss << m_store_list.messageCount() << " message(s)" << "\r\n" ;
		std::size_t i = 1 ;
		for( auto item_p = m_store_list.cbegin() ; item_p != m_store_list.cend() ; ++item_p , ++i )
		{
			const auto & item = *item_p ;
			if( !item.deleted )
			{
				ss << i << " " ;
				if( uidl ) ss << item.uidl() ;
				if( !uidl ) ss << item.size ;
				ss << "\r\n" ;
			}
		}
		ss << "." ;
		sendLines( ss ) ;
	}
	else
	{
		auto item = m_store_list.get( id ) ;
		ss << id << " " ;
		if( uidl ) ss << item.uidl() ;
		if( !uidl ) ss << item.size ;
		sendLine( ss.str() ) ;
	}
}

void GPop::ServerProtocol::doRetr( const std::string & line , bool & more )
{
	int id = commandNumber( line , -1 ) ;
	if( id == -1 || !m_store_list.valid(id) )
	{
		more = false ; // stay in the same state
		sendError() ;
	}
	else
	{
		m_content = m_store_list.content(id) ;
		m_body_limit = -1L ;

		std::ostringstream ss ;
		ss << "+OK " << m_store_list.byteCount(id) << " octets" ;
		sendLine( ss.str() ) ;
	}
}

void GPop::ServerProtocol::doTop( const std::string & line , bool & more )
{
	int id = commandNumber( line , -1 , 1U ) ;
	int n = commandNumber( line , -1 , 2U ) ;
	G_DEBUG( "ServerProtocol::doTop: " << id << ", " << n ) ;
	if( id == -1 || !m_store_list.valid(id) || n < 0 )
	{
		more = false ; // stay in the same state
		sendError() ;
	}
	else
	{
		m_content = m_store_list.content( id ) ;
		m_body_limit = n ;
		m_in_body = false ;
		sendOk() ;
	}
}

void GPop::ServerProtocol::doDele( const std::string & line , bool & )
{
	int id = commandNumber( line , -1 ) ;
	if( id == -1 || !m_store_list.valid(id) )
	{
		sendError() ;
	}
	else
	{
		m_store_list.remove( id ) ;
		sendOk() ;
	}
}

void GPop::ServerProtocol::doRset( const std::string & , bool & )
{
	m_store_list.rollback() ;
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

	if( mechanism.empty() )
	{
		// non-standard, but some clients expect a list of mechanisms
		ok = false ; // => stay in start state
		std::string list = mechanisms() ;
		G::Str::replaceAll( list , " "_sv , "\r\n"_sv ) ;
		std::ostringstream ss ;
		ss << "+OK\r\n" ;
		if( !list.empty() )
			ss << list << "\r\n" ;
		ss << "." ;
		sendLines( ss ) ;
	}
	else if( mechanisms().empty() )
	{
		ok = false ;
		sendError( "must use STLS before authentication" ) ;
	}
	else
	{
		std::string initial_response = commandParameter(line,2) ;
		if( initial_response == "=" )
			initial_response.clear() ; // RFC-5034

		m_sasl_init_apop = false ;
		if( !m_sasl->init( m_secure , mechanism ) )
		{
			ok = false ;
			sendError( "invalid mechanism" ) ;
		}
		else if( m_sasl->mustChallenge() && !initial_response.empty() )
		{
			ok = false ;
			sendError( "invalid initial response" ) ;
		}
		else if( !initial_response.empty() )
		{
			m_fsm.apply( *this , Event::eAuthData , initial_response ) ;
		}
		else
		{
			std::string initial_challenge = m_sasl->initialChallenge() ;
			sendLine( std::string("+ ",2U).append(G::Base64::encode(initial_challenge)) ) ;
		}
	}
}

void GPop::ServerProtocol::doAuthData( const std::string & line , bool & ok )
{
	if( line == "*" )
	{
		ok = false ;
		sendError() ;
		return ;
	}

	bool done = false ;
	std::string challenge = m_sasl->apply( G::Base64::decode(line) , done ) ;
	if( done && m_sasl->authenticated() )
	{
		m_fsm.apply( *this , Event::eAuthComplete , "" ) ;
	}
	else if( done )
	{
		ok = false ; // => start
		sendError() ;
	}
	else
	{
		sendLine( std::string("+ ",2U).append(G::Base64::encode(challenge)) ) ;
	}
}

void GPop::ServerProtocol::doAuthComplete( const std::string & , bool & )
{
	G_LOG_S( "GPop::ServerProtocol: pop authentication of " << m_sasl->id() << " connected from " << m_peer_address.displayString() ) ;
	m_user = m_sasl->id() ;
	readStore( m_user ) ;
	sendOk() ;
}

void GPop::ServerProtocol::readStore( const std::string & user )
{
	m_store_user = std::make_unique<StoreUser>( m_store , user ) ;
	m_store_list = StoreList( *m_store_user , m_store.allowDelete() ) ;
}

void GPop::ServerProtocol::doStls( const std::string & , bool & )
{
	G_ASSERT( m_security.securityEnabled() ) ;
	sendOk() ; // "please start tls"
	m_security.securityStart() ;
}

void GPop::ServerProtocol::secure()
{
	m_secure = true ;
	sendOk() ; // "hello (again)"
}

bool GPop::ServerProtocol::mechanismsIncludePlain() const
{
	return mechanisms().find("PLAIN") != std::string::npos ;
}

std::string GPop::ServerProtocol::mechanisms() const
{
	G::StringArray m = m_sasl->mechanisms( m_secure ) ;
	m.erase( std::remove( m.begin() , m.end() , "APOP" ) , m.end() ) ;
	return G::Str::join( " " , m ) ;
}

void GPop::ServerProtocol::doCapa( const std::string & , bool & )
{
	std::ostringstream ss ;
	ss << "+OK " << m_text.capa() << "\r\n" ;

	// USER/PASS POP3 authentication uses the PLAIN SASL mechanism
	// so only advertise it if it is available
	if( mechanismsIncludePlain() )
		ss << "USER\r\n" ;

	ss << "CAPA\r\nTOP\r\nUIDL\r\n" ;

	if( m_security.securityEnabled() )
		ss << "STLS\r\n" ;

	if( !mechanisms().empty() )
		ss << "SASL " << mechanisms() << "\r\n" ;

	ss << "." ;
	sendLines( ss ) ;
}

void GPop::ServerProtocol::doUser( const std::string & line , bool & )
{
	if( mechanismsIncludePlain() )
	{
		m_user = commandParameter(line) ;
		sendLine( std::string("+OK ",4U).append(m_text.user(commandParameter(line))) ) ;
	}
	else
	{
		sendError( "no SASL PLAIN mechanism to do USER/PASS authentication" ) ;
	}
}

void GPop::ServerProtocol::doPass( const std::string & line , bool & ok )
{
	m_sasl_init_apop = false ;
	if( !m_user.empty() && m_sasl->init(m_secure,"PLAIN") ) // (USER/PASS uses SASL PLAIN)
	{
		std::string rsp = m_user + std::string(1U,'\0') + m_user + std::string(1U,'\0') + commandParameter(line) ;
		bool done = false ;
		std::string ignore = m_sasl->apply( rsp , done ) ;
		if( done && m_sasl->authenticated() )
		{
			readStore( m_user ) ;
			sendOk() ;
		}
		else
		{
			ok = false ;
			sendError() ;
		}
	}
	else
	{
		ok = false ;
		sendError() ;
	}
}

void GPop::ServerProtocol::doApop( const std::string & line , bool & ok )
{
	if( m_sasl_init_apop )
	{
		std::string rsp = commandParameter(line,1) + " " + commandParameter(line,2) ;
		bool done = false ;
		std::string ignore = m_sasl->apply( rsp , done ) ;
		if( done && m_sasl->authenticated() )
		{
			m_user = m_sasl->id() ;
			readStore( m_user ) ;
			sendOk() ;
		}
		else
		{
			ok = false ;
			sendError() ;
		}
	}
	else
	{
		ok = false ;
		sendError() ;
	}
}

void GPop::ServerProtocol::sendLine( G::string_view line , bool has_crlf )
{
	if( has_crlf )
	{
		G_LOG( "GPop::ServerProtocol: tx>>: \"" << G::Str::printable(G::Str::trimmedView(line,"\r\n"_sv)) << "\"" ) ;
		m_sender.protocolSend( line , 0U ) ;
	}
	else
	{
		G_LOG( "GPop::ServerProtocol: tx>>: \"" << G::Str::printable(line) << "\"" ) ;
		m_sender.protocolSend( std::string(line.data(),line.size()).append("\r\n",2U) , 0U ) ;
	}
}

void GPop::ServerProtocol::sendLine( std::string && line )
{
	G_LOG( "GPop::ServerProtocol: tx>>: \"" << G::Str::printable(line) << "\"" ) ;
	m_sender.protocolSend( line.append("\r\n",2U) , 0U ) ;
}

void GPop::ServerProtocol::sendLines( std::ostringstream & ss )
{
	ss << "\r\n" ;
	const std::string s = ss.str() ;
	if( G::Log::atVerbose() )
	{
		std::size_t lines = std::count( s.begin() , s.end() , '\n' ) ;
		const std::size_t npos = std::string::npos ;
		std::size_t p0 = 0U ;
		std::size_t p1 = s.find( '\n' ) ;
		for( std::size_t i = 0U ; i < lines ; i++ , p0 = p1+1U , p1 = s.find('\n',p0+1U) )
		{
			G_ASSERT( p0 != npos && p0 < s.size() ) ;
			std::size_t n = p1 == npos ? (s.size()-p0) : (p1-p0) ;
			if( n && p1 && p1 != npos && s.at(p1-1U) == '\r' ) --n ;
			if( lines <= 7U || i < 4U || i > (lines-3U) )
				G_LOG( "GPop::ServerProtocol: tx>>: \"" << G::Str::printable(s.substr(p0,n)) << "\"" ) ;
			else if( i == 4U )
				G_LOG( "GPop::ServerProtocol: tx>>: [" << (lines-6U) << " lines]" ) ;
			if( p1 == npos || (p1+1U) == s.size() )
				break ;
		}
	}
	m_sender.protocolSend( s , 0U ) ;
}

// ===

GPop::ServerProtocolText::ServerProtocolText( const GNet::Address & )
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
	return std::string("user: ",6U).append(id) ;
}

// ===

GPop::ServerProtocol::Config::Config()
= default ;

