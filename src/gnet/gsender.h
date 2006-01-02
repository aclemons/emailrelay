//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gsender.h
//

#ifndef G_SENDER_H 
#define G_SENDER_H

#include "gdef.h"
#include "gnet.h"
#include "gserver.h"
#include "gexception.h"
#include <string>

namespace GNet
{
	class Sender ;
}

// Class: GNet::Sender
// Description: A ServerPeer that does sending with flow-control.
//
class GNet::Sender : public GNet::ServerPeer 
{
public:
	G_EXCEPTION( SendError , "socket send error" ) ;

	explicit Sender( Server::PeerInfo , bool throw_on_flow_control = false ) ;
		// Constructor. If the 'throw' parameter is true then
		// a failure to send resulting from flow-control results 
		// in a SendError exception, rather than the 
		// return-false-and-on-resume mechanism.

	virtual ~Sender() ;
		// Destructor.

	bool send( const std::string & data , size_t offset = 0U ) ;
		// Sends data down the socket. Returns true if all
		// the data is sent. Throws on error. 
		//
		// If flow control is asserted then the residue is
		// saved internally, a write-event handler is 
		// installed on the socket, and false is returned. 
		//
		// When the socket's write-event is triggered the 
		// residue is sent. If fully sent then the write-event 
		// handler is removed from the socket and onResume() 
		// is called. If an exception is thrown in the 
		// write-event handler then the object deletes 
		// iteself by calling doDelete().

protected:
	virtual void onResume() = 0 ;
		// Called after flow-control has been released
		// and all residual data sent.

private:
	Sender( const Sender & ) ;
	void operator=( const Sender & ) ;
	virtual void writeEvent() ; // from EventHandler

private:
	bool m_throw ;
	std::string m_residue ;
	unsigned long m_n ;
} ;

#endif
