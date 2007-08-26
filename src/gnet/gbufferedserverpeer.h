//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#ifndef G_BUFFERED_SERVER_PEER_H 
#define G_BUFFERED_SERVER_PEER_H 

#include "gdef.h"
#include "gnet.h"
#include "gserver.h"
#include "gsender.h"
#include "glinebuffer.h"
#include "gexception.h"
#include <string>

/// \namespace GNet
namespace GNet
{
	class BufferedServerPeer ;
}

/// \class GNet::BufferedServerPeer
/// A ServerPeer that does buffered sending with flow-control
/// and line-buffered input.
///
class GNet::BufferedServerPeer : public GNet::ServerPeer 
{
public:
	G_EXCEPTION( SendError , "peer disconnected" ) ;

	BufferedServerPeer( Server::PeerInfo , const std::string & eol , bool throw_on_flow_control = false ) ;
		///< Constructor. If the 'throw' parameter is true then
		///< a failure to send resulting from flow-control results 
		///< in a SendError exception, rather than the 
		///< return-false-and-on-resume mechanism.

	virtual ~BufferedServerPeer() ;
		///< Destructor.

	bool send( const std::string & data , std::string::size_type offset = 0U ) ;
		///< Sends data down the socket to the peer. Returns true 
		///< if all the data is sent. If not all the data is sent
		///< because of flow control then onSendComplete() will 
		///< be called when it has. Throws on error, or if
		///< flow control was asserted and the constructor's
		///< throw parameter was true.

	virtual void writeEvent() ; 
		///< Final override from EventHandler.

protected:
	virtual void onSendComplete() = 0 ;
		///< Called after flow-control has been released and all 
		///< residual data sent.
		///<
		///< If an exception is thrown in the override then this 
		///< object catches it and deletes iteself by calling 
		///< doDelete().

	virtual bool onReceive( const std::string & ) = 0 ;
		///< Called when a complete line is received from the peer.
		///< Returns false if no more lines should be delivered.

	virtual void onData( const char * , ServerPeer::size_type ) ; 
		///< Final override from ServerPeer.

private:
	BufferedServerPeer( const BufferedServerPeer & ) ;
	void operator=( const BufferedServerPeer & ) ;
	void logFlowControlAsserted() const ;
	void logFlowControlReleased() const ;
	void logFlowControlReasserted() const ;

private:
	Sender m_sender ;
	LineBuffer m_line_buffer ;
	bool m_throw ;
	std::string m_residue ;
	unsigned long m_n ;
} ;

#endif
