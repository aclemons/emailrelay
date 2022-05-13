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
/// \file gsmtpserverbuffer.h
///

#ifndef G_SMTP_SERVER_BUFFER_H
#define G_SMTP_SERVER_BUFFER_H

#include "gdef.h"
#include "gsmtpserverprotocol.h"
#include "gsmtpserversender.h"
#include "gexceptionsink.h"
#include "glimits.h"
#include "glinebuffer.h"
#include "gexception.h"
#include "gtimer.h"

namespace GSmtp
{
	class ServerBuffer ;
}

//| \class GSmtp::ServerBuffer
/// A helper for GSmtp::ServerProtocol that does line buffering
/// on input and RFC-2920 batching and flow-control buffering
/// on output.
///
class GSmtp::ServerBuffer : private ServerSender
{
public:
	G_EXCEPTION( Overflow , tx("server protocol error") ) ;

	ServerBuffer( GNet::ExceptionSink , ServerProtocol & , ServerSender & ,
		std::size_t line_buffer_limit = static_cast<std::size_t>(G::Limits<>::net_buffer*10) ,
		std::size_t pipelining_buffer_limit = 0U ,
		bool enable_batching = true ) ;
			///< Constructor.
			///<
			///< The supplied sender interface will be called with slightly
			///< modified semantics: the protocolSend() callback will have
			///< 'flush' always true and its return value will be used to
			///< indicate whether the send operation succeeded or was blocked
			///< by flow control (see sendComplete()).
			///<
			///< By design pipelining should not result in enormous buffering
			///< requirements because output batches are limited by the size of
			///< an incoming TPDU (ie. Limits::net_buffer or 16K for TLS).
			///< However, DoS protection against long lines with no CRLF is
			///< still required.

	void apply( const char * , std::size_t ) ;
		///< Called when raw data is received from the peer. Line
		///< buffering is performed and complete lines are
		///< apply()ed to the ServerProtocol. If the ServerProtocol
		///< cannot accept everything apply()d then the residue
		///< is retained in the line buffer and re-apply()ed
		///< transparently. Throws Done at the end of the
		///< protocol.

	void sendComplete() ;
		///< To be called once a protocolSend() callback that
		///< returned false has now completed.

	std::string head() const ;
		///< Returns the line buffer head.

public:
	~ServerBuffer() override = default ;
	ServerBuffer( const ServerBuffer & ) = delete ;
	ServerBuffer( ServerBuffer && ) = delete ;
	ServerBuffer & operator=( const ServerBuffer & ) = delete ;
	ServerBuffer & operator=( ServerBuffer && ) = delete ;

private: // overrides
	bool protocolSend( const std::string & , bool ) override ; // GSmtp::ServerSender
	void protocolSecure() override ; // GSmtp::ServerSender
	void protocolShutdown( int ) override ; // GSmtp::ServerSender
	void protocolExpect( std::size_t ) override ; // GSmtp::ServerSender

private:
	void onTimeout() ;
	void check( std::size_t ) ;
	void doOverflow( const std::string & ) ;

private:
	GNet::Timer<ServerBuffer> m_timer ;
	ServerProtocol & m_protocol ;
	ServerSender & m_sender ;
	GNet::LineBuffer m_line_buffer ;
	std::size_t m_line_buffer_limit {0U} ;
	std::size_t m_pipelining_buffer_limit {0U} ;
	std::string m_batch ;
	bool m_enable_batching {true} ;
	bool m_blocked {false} ;
} ;

#endif
