//
// Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gclientprotocol.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gsmtp.h"
#include "glocal.h"
#include "gfile.h"
#include "gsasl.h"
#include "gbase64.h"
#include "gstr.h"
#include "gmemory.h"
#include "gxtext.h"
#include "gclientprotocol.h"
#include "gresolve.h"
#include "glog.h"
#include "gassert.h"

GSmtp::ClientProtocol::ClientProtocol( Sender & sender , const Secrets & secrets ,
	const std::string & thishost_name , unsigned int timeout , bool must_authenticate , bool strict ) :
		m_sender(sender) ,
		m_secrets(secrets) ,
		m_thishost(thishost_name) ,
		m_state(sStart) ,
		m_server_has_8bitmime(false) ,
		m_message_is_8bit(false) ,
		m_authenticated_with_server(false) ,
		m_must_authenticate(must_authenticate) ,
		m_strict(strict) ,
		m_warned(false) ,
		m_timeout(timeout) ,
		m_signalled(false)
{
}

void GSmtp::ClientProtocol::start( const std::string & from , const G::Strings & to , bool eight_bit ,
	std::string authentication , std::string server_name , std::auto_ptr<std::istream> content )
{
	G_DEBUG( "GSmtp::ClientProtocol::start" ) ;

	// reinitialise for the new message & server
	m_signalled = false ;
	m_to = to ;
	m_from = from ;
	m_content = content ;
	m_message_is_8bit = eight_bit ;
	m_message_authentication = authentication ;
	m_reply = Reply() ;
	m_sasl <<= new SaslClient( m_secrets , server_name ) ;
	if( m_state != sStart && m_state != sEnd )
		throw NotReady() ;

	G_ASSERT( m_state == sStart || m_state == sEnd ) ;
	if( m_state == sStart )
	{
		// wait for 220 greeting
		startTimer( 20U ) ; // should be configurable
	}
	else
	{
		m_state = sSentMail ;
		sendMail() ;
	}
}

bool GSmtp::ClientProtocol::done() const
{
	return m_state == sEnd ;
}

void GSmtp::ClientProtocol::sendDone()
{
	if( m_state == sData )
	{
		size_t n = sendLines() ;

		G_LOG( "GSmtp::ClientProtocol: tx>>: [" << n << " line(s) of content]" ) ;
		if( endOfContent() )
		{
			m_state = sDone ;
			send(".",true) ;
		}
	}
}

//static
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

void GSmtp::ClientProtocol::apply( const std::string & rx )
{
	G_LOG( "GSmtp::ClientProtocol: rx<<: \"" << G::Str::toPrintableAscii(rx) << "\"" ) ;

	std::string reason ;
	bool complete = parseReply( m_reply , rx , reason ) ;
	if( complete )
	{
		applyEvent( m_reply ) ;
	}
	else 
	{
		if( reason.length() != 0U )
			send( std::string("550 syntax error: ")+reason ) ;
	}
}

void GSmtp::ClientProtocol::sendEhlo()
{
	send( std::string("EHLO ") + m_thishost ) ;
}

void GSmtp::ClientProtocol::sendHelo()
{
	send( std::string("HELO ") + m_thishost ) ;
}

void GSmtp::ClientProtocol::sendMail()
{
	const bool dodgy = m_message_is_8bit && !m_server_has_8bitmime ;
	if( dodgy && m_strict )
	{
		std::string reason = "cannot send 8-bit message to 7-bit server" ;
		G_WARNING( "GSmtp::ClientProtocol: " << reason ) ;
		m_state = sEnd ;
		raiseDoneSignal( false , false , reason ) ;
	}
	else
	{
		if( dodgy && !m_warned )
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
	std::string mail_from = std::string("MAIL FROM:<") + m_from + ">" ;
	if( m_server_has_8bitmime )
	{
		mail_from.append( " BODY=8BITMIME" ) ;
	}
	if( m_authenticated_with_server && !m_message_authentication.empty() )
	{
		mail_from.append( std::string(" AUTH=") + Xtext::encode(m_message_authentication) ) ;
	}
	else if( m_authenticated_with_server )
	{
		mail_from.append( " AUTH=<>" ) ;
	}
	send( mail_from ) ;
}

void GSmtp::ClientProtocol::applyEvent( const Reply & reply )
{
	cancelTimer() ;

	if( m_state == sStart && reply.is(Reply::ServiceReady_220) )
	{
		m_state = sSentEhlo ;
		sendEhlo() ;
	}
	else if( m_state == sSentEhlo && (
		reply.is(Reply::SyntaxError_500) ||
		reply.is(Reply::SyntaxError_501) ||
		reply.is(Reply::NotImplemented_502) ) )
	{
		// it didn't like EHLO so fall back to HELO
		m_state = sSentHelo ;
		sendHelo() ;
	}
	else if( (m_state==sSentEhlo || m_state==sSentHelo) && reply.is(Reply::Ok_250) )
	{
		G_ASSERT( m_sasl.get() != NULL ) ;
		G_DEBUG( "GSmtp::ClientProtocol::applyEvent: ehlo reply \""
			<< G::Str::toPrintableAscii(reply.text()) << "\"" ) ;

		m_auth_mechanism = m_sasl->preferred( serverAuthMechanisms(reply) ) ;
		m_server_has_8bitmime = m_state == sSentEhlo && reply.textContains("\n8BITMIME") ;

		if( m_sasl->active() && !m_auth_mechanism.empty() )
		{
			m_state = sAuth1 ;
			send( std::string("AUTH ") + m_auth_mechanism ) ;
		}
		else if( m_sasl->active() && m_must_authenticate )
		{
			std::string reason = "cannot do mandatory authentication" ; // eg. no suitable mechanism
			G_WARNING( "GSmtp::ClientProtocol: " << reason ) ;
			m_state = sEnd ;
			raiseDoneSignal( false , true , reason ) ;
		}
		else
		{
			m_state = sSentMail ;
			sendMail() ;
		}
	}
	else if( m_state == sAuth1 && reply.is(Reply::Challenge_334) && Base64::valid(reply.text()) )
	{
		bool done = true ;
		bool error = false ;
		std::string rsp = m_sasl->response( m_auth_mechanism , Base64::decode(reply.text()) , done , error ) ;
		if( error )
		{
			m_state = sAuth2 ;
			send( "*" ) ; // ie. cancel authentication
		}
		else
		{
			m_state = done ? sAuth2 : m_state ;
			send( Base64::encode(rsp,std::string()) ) ;
		}
	}
	else if( m_state == sAuth2 )
	{
		m_authenticated_with_server = reply.is(Reply::Authenticated_235) ;

		if( !m_authenticated_with_server && m_must_authenticate )
		{
			m_state = sEnd ;
			raiseDoneSignal( false , true , "mandatory authentication failed" ) ;
		}
		else
		{
			m_state = sSentMail ;
			sendMail() ; // (with or without sucessful authentication)
		}
	}
	else if( m_state == sSentMail && reply.is(Reply::Ok_250) )
	{
		if( m_to.size() == 0U )
		{
			// should never get here -- messages with no remote recipients
			// are filtered out by the message store
			throw NoRecipients() ; 
		}

		m_state = sSentRcpt ;
		send( std::string("RCPT TO:<") + m_to.front() + std::string(">") ) ;
		m_to.pop_front() ;
	}
	else if( m_state == sSentRcpt && m_to.size() != 0U && reply.positive() )
	{
		send( std::string("RCPT TO:<") + m_to.front() + std::string(">") ) ;
		m_to.pop_front() ;
	}
	else if( m_state == sSentRcpt && reply.positive() )
	{
		m_state = sSentData ;
		send( std::string("DATA") ) ;
	}
	else if( m_state == sSentRcpt )
	{
		G_WARNING( "GSmtp::ClientProtocol: recipient rejected" ) ;
		m_state = sEnd ;
		raiseDoneSignal( false , false , reply.text() ) ;
	}
	else if( m_state == sSentData && reply.is(Reply::OkForData_354) )
	{
		m_state = sData ;

		size_t n = sendLines() ;

		const bool log_content = false ; // set true here and in sendLine() if debugging
		if( !log_content )
			G_LOG( "GSmtp::ClientProtocol: tx>>: [" << n << " line(s) of content]" ) ;

		if( endOfContent() )
		{
			m_state = sDone ;
			send( "." , true , log_content ) ;
		}
	}
	else if( m_state == sDone )
	{
		const bool ok = reply.is(Reply::Ok_250) ;
		m_state = sEnd ;
		raiseDoneSignal( ok , false , ok ? std::string() : reply.text() ) ;
	}
	else
	{
		G_WARNING( "GSmtp::ClientProtocol: failure in client protocol: " << static_cast<int>(m_state) ) ;
		m_state = sEnd ;
		raiseDoneSignal( false , true , std::string("unexpected response: ")+reply.text() ) ;
	}
}

void GSmtp::ClientProtocol::onTimeout()
{
	if( m_state == sStart )
	{
		// no 220 greeting seen -- go on regardless
		G_WARNING( "GSmtp::ClientProtocol: timeout: no greeting" ) ;
		m_state = sSentEhlo ;
		sendEhlo() ;
	}
	else
	{
		G_WARNING( "GSmtp::ClientProtocol: timeout" ) ;
		m_state = sEnd ;
		raiseDoneSignal( false , false , "timeout" ) ;
	}
}
 
G::Strings GSmtp::ClientProtocol::serverAuthMechanisms( const ClientProtocolReply & reply ) const
{
	G::Strings result ;
	std::string auth_line = reply.textLine("AUTH ") ; // trailing space to avoid "AUTH="
	if( ! auth_line.empty() )
	{
		G::Str::splitIntoTokens( auth_line , result , " " ) ;
		if( result.size() )
			result.pop_front() ; // remove "AUTH" ;
	}
	return result ;
}

void GSmtp::ClientProtocol::raiseDoneSignal( bool ok , bool abort , const std::string & reason )
{
	G_DEBUG( "GSmtp::ClientProtocol::raiseDoneSignal: " << ok << ": \"" << reason << "\"" ) ;
	cancelTimer() ;
	m_content <<= 0 ;
	if( ! m_signalled )
	{
		m_signalled = true ;
		m_signal.emit( ok , abort , reason ) ;
	}
}

bool GSmtp::ClientProtocol::endOfContent() const
{
	return !m_content->good() ;
}

size_t GSmtp::ClientProtocol::sendLines()
{
	cancelTimer() ; // no response expected during data transfer

	// the read buffer -- capacity grows to longest line, but start with something reasonable
	std::string line( 200U , '.' ) ; 

	size_t n = 0U ;
	while( sendLine(line) )
		n++ ;
	return n ;
}

bool GSmtp::ClientProtocol::sendLine( std::string & line )
{
	line.erase( 1U ) ; // leave "."

	const bool pre_erase = false ;
	G::Str::readLineFrom( *(m_content.get()) , crlf() , line , pre_erase ) ;
	G_ASSERT( line.length() >= 1U && line.at(0U) == '.' ) ;

	if( m_content->good() )
	{
		line.append( crlf() ) ;
		bool all_sent = m_sender.protocolSend( line , line.at(1U) == '.' ? 0U : 1U ) ;
		if( !all_sent && m_timeout != 0U )
			startTimer( m_timeout ) ; // use response timer for when flow-control asserted
		return all_sent ;
	}
	else
	{
		return false ;
	}
}

bool GSmtp::ClientProtocol::send( const std::string & line , bool eot , bool log )
{
	if( m_timeout != 0U )
		startTimer( m_timeout ) ;

	std::string prefix( !eot && line.length() && line.at(0U) == '.' ? "." : "" ) ;
	if( log )
		G_LOG( "GSmtp::ClientProtocol: tx>>: \"" << prefix << G::Str::toPrintableAscii(line) << "\"" ) ;

	return m_sender.protocolSend( prefix + line + crlf() , 0U ) ;
}

//static
const std::string & GSmtp::ClientProtocol::crlf()
{
	static std::string s("\015\012") ;
	return s ;
}

G::Signal3<bool,bool,std::string> & GSmtp::ClientProtocol::doneSignal()
{
	return m_signal ;
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
		m_value = G::Str::toUInt( line.substr(0U,3U) ) ;
		if( line.length() > 4U )
		{
			m_text = line.substr(4U) ;
			G::Str::trimLeft( m_text , " \t" ) ;
			G::Str::replaceAll( m_text , "\t" , " " ) ;
		}
	}
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
	return m_valid && m_value < 400U ;
}

unsigned int GSmtp::ClientProtocolReply::value() const
{
	return m_valid ? m_value : 0 ;
}

bool GSmtp::ClientProtocolReply::is( Value v ) const
{
	return value() == static_cast<unsigned int>( v ) ;
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

//static
bool GSmtp::ClientProtocolReply::is_digit( char c )
{
	return c >= '0' && c <= '9' ;
}

GSmtp::ClientProtocolReply::Type GSmtp::ClientProtocolReply::type() const
{
	G_ASSERT( m_valid && (m_value/100U) >= 1U && (m_value/100U) <= 5U ) ;
	return static_cast<Type>( m_value / 100U ) ;
}

GSmtp::ClientProtocolReply::SubType GSmtp::ClientProtocolReply::subType() const
{
	unsigned int n = ( m_value / 10U ) % 10U ;
	if( n < 4U )
		return static_cast<SubType>( n ) ;
	else
		return Invalid_SubType ;
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


