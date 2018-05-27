//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gbufferedserverpeer.h
///

#ifndef G_NET_BUFFERED_SERVER_PEER__H
#define G_NET_BUFFERED_SERVER_PEER__H

#include "gdef.h"
#include "gserver.h"
#include "gsocketprotocol.h"
#include "glinebuffer.h"
#include "gexception.h"
#include <string>

namespace GNet
{
	class BufferedServerPeer ;
}

/// \class GNet::BufferedServerPeer
/// A ServerPeer that does line-buffering on input.
///
class GNet::BufferedServerPeer : public ServerPeer
{
public:
	explicit BufferedServerPeer( Server::PeerInfo , LineBufferConfig ) ;
		///< Constructor.

	virtual ~BufferedServerPeer() ;
		///< Destructor.

	std::string lineBufferEndOfLine() const ;
		///< Returns the line buffer end-of-line string. Returns
		///< the empty string if auto-detecting and not yet
		///< auto-detected.

	void expect( size_t n ) ;
		///< Temporarily suspends line buffering so that the next 'n' bytes
		///< are accumulated without regard to line terminators.

protected:
	virtual bool onReceive( const char * line_data , size_t line_size , size_t eolsize ) = 0 ;
		///< Called when a complete line is received from the peer. Returns
		///< false if no more lines should be delivered.

	virtual void onData( const char * , ServerPeer::size_type ) override ;
		///< Override from GNet::SocketProtocolSink.

private:
	BufferedServerPeer( const BufferedServerPeer & ) ;
	void operator=( const BufferedServerPeer & ) ;

private:
	LineBuffer m_line_buffer ;
	size_t m_expect ;
	std::string m_expect_data ;
} ;

#endif
