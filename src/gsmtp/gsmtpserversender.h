//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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

//| \class GSmtp::ServerSender
/// An interface used by ServerProtocol to send protocol responses.
///
/// The RFC-2920 PIPELINING extension defines how SMTP input requests
/// and output responses should be batched up. At this interface that
/// means that protocolSend() has a 'flush' parameter to mark the
/// end of an output batch.
///
class GSmtp::ServerSender
{
public:
	virtual void protocolSend( const std::string & s , bool flush ) = 0 ;
		///< Called when the server protocol class wants to send data
		///< down the socket. The data should be batched up if
		///< 'flush' is false. The 'flush' parameter will always be
		///< true if the server protocol is not using pipelining.
		///<
		///< If the server protocol is using pipelining then calls
		///< to protocolSend() might come in quick succession, so
		///< the implementation must queue up the output if the
		///< socket applies flow control. There is no need to
		///< tell the protocol when flow control is released.

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
