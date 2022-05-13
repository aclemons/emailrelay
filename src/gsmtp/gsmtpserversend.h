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
/// \file gsmtpserversend.h
///

#ifndef G_SMTP_SERVER_SEND_H
#define G_SMTP_SERVER_SEND_H

#include "gdef.h"
#include "gsmtpserversender.h"
#include "gstringarray.h"
#include <string>
#include <sstream>

namespace GSmtp
{
	class ServerSend ;
}

//| \class GSmtp::ServerSend
/// A mix-in class for GSmtp::ServerProtocol that sends protocol responses.
///
class GSmtp::ServerSend
{
public:
	explicit ServerSend( ServerSender * ) ;
		///< Constructor.

	void setSender( ServerSender * ) ;
		///< Sets the sender interface pointer.

	virtual std::string sendUseStartTls() const = 0 ;
		///< Returns a helpful string that is appended to some responses to
		///< indicate the STARTTLS command should be used. Returns the empty
		///< string if already using TLS or if TLS is not supported.

	virtual bool sendFlush() const = 0 ;
		///< Returns a 'flush' value for GSmtp::ServerSender::protocolSend().

public:
	ServerSend( const ServerSend & ) = delete ;
	ServerSend( ServerSend && ) = delete ;
	void operator=( const ServerSend & ) = delete ;
	void operator=( ServerSend && ) = delete ;

protected:
	struct Advertise
	{
		std::string hello ;
		std::size_t max_size {0U} ;
		G::StringArray mechanisms ;
		bool starttls {false} ;
		bool vrfy {false} ;
		bool chunking {false} ;
		bool pipelining {false} ;
		bool smtputf8 {false} ;
	} ;
	void send( const char * ) ;
	void send( std::string , bool = false ) ;
	void send( const std::ostringstream & ) ;
	void sendReadyForTls() ;
	void sendInsecureAuth() ;
	void sendEncryptionRequired() ;
	void sendBadMechanism( const std::string & = {} ) ;
	void sendBadFrom( const std::string & ) ;
	void sendTooBig() ;
	void sendChallenge( const std::string & ) ;
	void sendBadTo( const std::string & , const std::string & , bool ) ;
	void sendBadDataOutOfSequence() ;
	void sendOutOfSequence() ;
	void sendGreeting( const std::string & ) ;
	void sendQuitOk() ;
	void sendUnrecognised( const std::string & ) ;
	void sendNotImplemented() ;
	void sendHeloReply() ;
	void sendEhloReply( const Advertise & ) ;
	void sendRsetReply() ;
	void sendMailReply( const std::string & from ) ;
	void sendRcptReply( const std::string & to ) ;
	void sendDataReply() ;
	void sendCompletionReply( bool ok , const std::string & ) ;
	void sendFailed() ;
	void sendInvalidArgument() ;
	void sendAuthenticationCancelled() ;
	void sendAuthRequired() ;
	void sendNoRecipients() ;
	void sendMissingParameter() ;
	void sendVerified( const std::string & ) ;
	void sendNotVerified( const std::string & , bool ) ;
	void sendWillAccept( const std::string & ) ;
	void sendAuthDone( bool ok ) ;
	void sendOk( const std::string & ) ;
	void sendOk() ;

private:
	ServerSender * m_sender ;
} ;

#endif
