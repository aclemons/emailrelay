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
/// \file gsmtpserversend.cpp
///

#include "gdef.h"
#include "gsmtpserversend.h"
#include "gstringfield.h"
#include "gstringview.h"
#include "gbase64.h"
#include "gstr.h"

GSmtp::ServerSend::ServerSend( ServerSender * sender ) :
	m_sender(sender)
{
}

void GSmtp::ServerSend::setSender( ServerSender * sender )
{
	m_sender = sender ;
}

void GSmtp::ServerSend::sendChallenge( const std::string & challenge )
{
	send( "334 " + G::Base64::encode(challenge) ) ;
}

void GSmtp::ServerSend::sendGreeting( const std::string & text , bool enabled )
{
	if( enabled )
		send( "220 " + text ) ;
	else
		sendDisabled() ;
}

void GSmtp::ServerSend::sendReadyForTls()
{
	send( "220 ready to start tls" , /*go-secure=*/ true ) ;
}

void GSmtp::ServerSend::sendInvalidArgument()
{
	send( "501 invalid argument" ) ;
}

void GSmtp::ServerSend::sendAuthenticationCancelled()
{
	send( "501 authentication cancelled" ) ;
}

void GSmtp::ServerSend::sendInsecureAuth( bool with_starttls_help )
{
	if( with_starttls_help )
		send( "504 unsupported authentication mechanism: use starttls" ) ;
	else
		send( "504 unsupported authentication mechanism" ) ;
}

void GSmtp::ServerSend::sendBadMechanism( const std::string & preferred )
{
	if( preferred.empty() )
		send( "504 unsupported authentication mechanism" ) ;
	else
		send( "432 " + G::Str::upper(preferred) + " password transition needed" ) ; // RFC-4954 6
}

void GSmtp::ServerSend::sendAuthDone( bool ok )
{
	if( ok )
		send( "235 authentication successful" ) ;
	else
		send( "535 authentication failed" ) ;
}

void GSmtp::ServerSend::sendBadDataOutOfSequence()
{
	send( "503 invalid data command with binarymime -- use RSET to resynchronise" ) ;
}

void GSmtp::ServerSend::sendOutOfSequence()
{
	send( "503 command out of sequence -- use RSET to resynchronise" ) ;
}

void GSmtp::ServerSend::sendMissingParameter()
{
	send( "501 parameter required" ) ;
}

void GSmtp::ServerSend::sendQuitOk()
{
	send( "221 OK" ) ;
}

void GSmtp::ServerSend::sendVerified( const std::string & user )
{
	send( "250 " + user ) ;
}

void GSmtp::ServerSend::sendCannotVerify()
{
	send( "252 cannot vrfy" ) ; // RFC-5321 7.3
}

void GSmtp::ServerSend::sendNotVerified( const std::string & response , bool temporary )
{
	send( (temporary?"450":"550") + std::string(1U,' ') + response ) ;
}

void GSmtp::ServerSend::sendWillAccept( const std::string & user )
{
	send( "252 cannot verify but will accept: " + user ) ;
}

void GSmtp::ServerSend::sendUnrecognised( const std::string & )
{
	send( "500 command unrecognized" ) ;
}

void GSmtp::ServerSend::sendNotImplemented()
{
	send( "502 command not implemented" ) ;
}

void GSmtp::ServerSend::sendAuthRequired( bool with_starttls_help )
{
	if( with_starttls_help )
		send( "530 authentication required: use starttls" ) ;
	else
		send( "530 authentication required" ) ;
}

void GSmtp::ServerSend::sendDisabled()
{
	send( "421 service not available" ) ;
}

void GSmtp::ServerSend::sendEncryptionRequired( bool with_starttls_help )
{
	if( with_starttls_help )
		send( "530 encryption required: use starttls" ) ; // 530 sic
	else
		send( "530 encryption required" ) ;
}

void GSmtp::ServerSend::sendNoRecipients()
{
	send( "554 no valid recipients" ) ;
}

void GSmtp::ServerSend::sendTooBig()
{
	send( "552 message size exceeds fixed maximum message size" ) ; // RFC-1427
}

void GSmtp::ServerSend::sendDataReply()
{
	send( "354 start mail input -- end with <CRLF>.<CRLF>" ) ;
}

void GSmtp::ServerSend::sendRsetReply()
{
	send( "250 state reset" ) ;
}

void GSmtp::ServerSend::sendMailReply( const std::string & from )
{
	sendOk( "sender <" + from + "> OK" ) ; // RFC-2920 3.2 (10)
}

void GSmtp::ServerSend::sendCompletionReply( bool ok , int response_code , const std::string & response )
{
	if( ok )
		sendOk( "message processed" ) ;
	else if( response_code >= 400 && response_code < 600 )
		send( G::Str::fromInt(response_code).append(1U,' ').append(response) ) ;
	else
		send( "452 " + response ) ; // 452=>"action not taken"
}

void GSmtp::ServerSend::sendFailed()
{
	send( "554 transaction failed" ) ;
}

void GSmtp::ServerSend::sendRcptReply( const std::string & to , bool is_local )
{
	// RFC-2920 3.2 (10)
	sendOk( std::string("recipient <").append(to).append(is_local?">  ":"> ").append("OK") ) ;
}

void GSmtp::ServerSend::sendBadFrom( const std::string & response_extra )
{
	std::string response = "553 mailbox name not allowed" ;
	if( !response_extra.empty() )
	{
		response.append( ": " ) ;
		response.append( response_extra ) ;
	}
	send( response ) ;
}

void GSmtp::ServerSend::sendBadTo( const std::string & to , const std::string & text , bool temporary )
{
	send( G::Str::join( " " ,
		std::string(temporary?"450":"550") ,
		to.empty() || G::Str::isPrintableAscii(to) ? std::string() : ("recipient <" + to + ">") ,
		text ) ) ;
}

void GSmtp::ServerSend::sendEhloReply( const Advertise & advertise )
{
	static constexpr G::string_view crlf( "\015\012" , 2U ) ;

	std::ostringstream ss ;
		ss << "250-" << G::Str::printable(advertise.hello) << crlf ;

	if( advertise.max_size != 0U )
		ss << "250-SIZE " << advertise.max_size << crlf ; // RFC-1427

	if( !advertise.mechanisms.empty() )
		ss << "250-AUTH " << G::Str::join(" ",advertise.mechanisms) << crlf ;

	if( advertise.starttls )
		ss << "250-STARTTLS" << crlf ;

	if( advertise.vrfy )
		ss << "250-VRFY" << crlf ; // RFC-2821 3.5.2

	if( advertise.chunking )
		ss << "250-CHUNKING" << crlf ; // RFC-3030

	if( advertise.binarymime )
		ss << "250-BINARYMIME" << crlf ; // RFC-3030

	if( advertise.pipelining )
		ss << "250-PIPELINING" << crlf ; // RFC-2920

	if( advertise.smtputf8 )
		ss << "250-SMTPUTF8" << crlf ; // RFC-6531

	ss << "250 8BITMIME" ;

	send( ss ) ;
}

void GSmtp::ServerSend::sendHeloReply()
{
	sendOk( "hello" ) ;
}

void GSmtp::ServerSend::sendOk( const std::string & text )
{
	send( std::string("250 ").append(text) ) ;
}

#ifndef G_LIB_SMALL
void GSmtp::ServerSend::sendOk()
{
	send( "250 OK" ) ;
}
#endif

void GSmtp::ServerSend::send( const char * line )
{
	send( std::string(line) ) ;
}

void GSmtp::ServerSend::send( std::string line_in , bool go_secure )
{
	G_LOG( "GSmtp::ServerSend: tx>>: \"" << G::Str::printable(line_in) << "\"" ) ;
	static const char * crlf = "\015\012" ;
	m_sender->protocolSend( G::Str::printable(line_in).append(crlf,2U) , go_secure || sendFlush() ) ;
	if( go_secure )
		m_sender->protocolSecure() ;
}

void GSmtp::ServerSend::send( const std::ostringstream & ss )
{
	std::string s = ss.str() ;
	for( G::StringField f(s,"\r\n",2U) ; f ; ++f )
		G_LOG( "GSmtp::ServerSend: tx>>: \"" << f() << "\"" ) ;
	static const char * crlf = "\015\012" ;
	m_sender->protocolSend( s.append(crlf,2U) , sendFlush() ) ;
}

