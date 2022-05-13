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
/// \file gsmtpserversender.h
///

#ifndef G_SMTP_SERVER_SENDER_H
#define G_SMTP_SERVER_SENDER_H

#include "gdef.h"
#include <string>

namespace GSmtp
{
	class ServerSender ;
}

class GSmtp::ServerSender /// An interface used by ServerProtocol to send protocol replies.
{
public:
	virtual bool protocolSend( const std::string & s , bool flush ) = 0 ;
		///< Called when the protocol class wants to send data
		///< down the socket. The data should be batched up if
		///< 'flush' is false. The 'flush' parameter will always be
		///< true if pipelining is disabled.
		///<
		///< The return value is ignored, but is included in the
		///< signature for the benefit of GSmtp::ServerBuffer.
		///<
		///< If pipelining is enabled then calls to protocolSend()
		///< might come in quick succession, so the implementation
		///< must allow for flow control being asserted from a
		///< non-blocking socket.
		///<
		///< If there are any pipelined input requests pending then
		///< this callback can be used as a trigger for them to
		///< be re-apply()ed.

	virtual void protocolSecure() = 0 ;
		///< Called when the protocol class wants a secure
		///< connection to be initiated. ServerProtocol::secure()
		///< should be called when complete.

	virtual void protocolShutdown( int how ) = 0 ;
		///< Called on receipt of a quit command after the quit
		///< response has been sent. The implementation should
		///< normally do a socket shutdown if the parameter is
		///< 0, 1 or 2. See also Socket::shutdown().

	virtual void protocolExpect( std::size_t n ) = 0 ;
		///< Requests that the next call to ServerProtocol::apply()
		///< carries exactly 'n' bytes of binary data rather than a
		///< line of text. This only called if the protocol config
		///< item 'with_chunking' is true.

	virtual ~ServerSender() = default ;
		///< Destructor.
} ;

#endif
