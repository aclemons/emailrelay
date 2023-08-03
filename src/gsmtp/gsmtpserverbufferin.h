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
/// \file gsmtpserverbufferin.h
///

#ifndef G_SMTP_SERVER_BUFFER_IN_H
#define G_SMTP_SERVER_BUFFER_IN_H

#include "gdef.h"
#include "gsmtpserverprotocol.h"
#include "gexceptionsink.h"
#include "glinebuffer.h"
#include "gtimer.h"
#include "gexception.h"
#include "gslot.h"

namespace GSmtp
{
	class ServerBufferIn ;
}

//| \class GSmtp::ServerBufferIn
/// A helper class for GSmtp::ServerProtocol that does buffering of data
/// received from the remote peer and apply()s it to the server protocol.
///
/// The original SMTP protocol has a simple request/response setup phase
/// followed by a streaming data transfer phase, so a GNet::LineBuffer
/// can be used with no risk of overflow. The RFC-2920 PIPELINING extension
/// develops this by defining request/response batches with a well-defined
/// batch boundary before the data transfer phase.
///
/// RFC-2920 PIPELINING tries to define a size limit for an input batch,
/// but only in terms of the network layer PDU size -- which is useless
/// in practice.
///
/// Unfortunately the RFC-3030 CHUNKING ("BDAT") extension is underspecified
/// so there is no batch boundary prior to the data transfer phase. That
/// means that in the worst case the remote client can start the data
/// transfer before the setup commands have been fully processed and blow
/// up the input buffer with megabytes of message body data. Therefore we
/// have to use network flow control to limit the amount of buffering:
///
/// \code
/// Server::Server() : m_buffer(...)
/// {
///   m_buffer.flowSignal().connect( slot(*this,&Server::onFlow) ) ;
/// }
/// void Server::onData( p , n )
/// {
///   m_buffer.apply(p,n) ;
/// }
/// void Server::onFlow( bool on )
/// {
///    on ? addReadHandler(m_fd) : dropReadHandler(m_fd) ;
/// }
/// \endcode
///
class GSmtp::ServerBufferIn
{
public:
	G_EXCEPTION( Overflow , tx("server protocol overflow") ) ;

	struct Config /// A configuration structure for GSmtp::ServerBufferIn.
	{
		std::size_t input_buffer_soft_limit {G::Limits<>::net_buffer} ; // threshold to apply flow control
		std::size_t input_buffer_hard_limit {G::Limits<>::net_buffer*4U} ; // threshold to fail
		Config & set_input_buffer_soft_limit( std::size_t ) noexcept ;
		Config & set_input_buffer_hard_limit( std::size_t ) noexcept ;
	} ;

	ServerBufferIn( GNet::ExceptionSink , ServerProtocol & , const Config & ) ;
		///< Constructor.

	~ServerBufferIn() ;
		///< Destructor.

	void apply( const char * , std::size_t ) ;
		///< Called when raw data is received from the peer. Line
		///< buffering is performed and complete lines are
		///< apply()ed to the ServerProtocol. If the ServerProtocol
		///< cannot accept everything then the residue is queued
		///< and re-apply()d transparently.
		///<
		///< Throws Done at the end of the protocol.

	void expect( std::size_t ) ;
		///< Forwards to GNet::LineBuffer::expect().

	std::string head() const ;
		///< Returns GNet::LineBufferState::head().

	G::Slot::Signal<bool> & flowSignal() noexcept ;
		///< Returns a signal that should be connected to a function
		///< that controls network flow control, typically by adding
		///< and removing the socket file descriptor from the event
		///< loop.

public:
	ServerBufferIn( const ServerBufferIn & ) = delete ;
	ServerBufferIn( ServerBufferIn && ) = delete ;
	ServerBufferIn & operator=( const ServerBufferIn & ) = delete ;
	ServerBufferIn & operator=( ServerBufferIn && ) = delete ;

private:
	void applySome( const char * , std::size_t ) ;
	void onTimeout() ;
	void onProtocolChange() ;
	bool overLimit() const ;
	bool overHardLimit() const ;
	void flowOn() ;
	void flowOff() ;

private:
	ServerProtocol & m_protocol ;
	Config m_config ;
	GNet::LineBuffer m_line_buffer ;
	GNet::Timer<ServerBufferIn> m_timer ;
	G::Slot::Signal<bool> m_flow_signal ;
	bool m_flow_on {true} ;
} ;

inline GSmtp::ServerBufferIn::Config & GSmtp::ServerBufferIn::Config::set_input_buffer_soft_limit( std::size_t n ) noexcept { input_buffer_soft_limit = n ; return *this ; }
inline GSmtp::ServerBufferIn::Config & GSmtp::ServerBufferIn::Config::set_input_buffer_hard_limit( std::size_t n ) noexcept { input_buffer_hard_limit = n ; return *this ; }

#endif
