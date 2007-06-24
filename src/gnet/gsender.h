//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
///
/// \file gsender.h
///

#ifndef G_SENDER_H 
#define G_SENDER_H

#include "gdef.h"
#include "gnet.h"
#include "gserver.h"
#include <string>

/// \namespace GNet
namespace GNet
{
	class Sender ;
}

/// \class GNet::Sender
/// A class that does buffered sending of data down a socket 
/// with flow-control.
///
class GNet::Sender 
{
public:
	explicit Sender( EventHandler & handler ) ;
		///< Constructor with an event handler.

	virtual ~Sender() ;
		///< Destructor.

	bool send( Socket & socket , const std::string & data , std::string::size_type offset = 0U ) ;
		///< Sends data down the socket. 
		///<
		///< Returns true if all the data is sent successfully.
		///<
		///< If flow control is asserted and the event handler
		///< constructor was used then the residue is saved 
		///< internally, a write-event handler is installed on 
		///< the socket on bahalf of the event handler 
		///< and false is returned. When the socket's write-event 
		///< is triggered the event handler is expected to call 
		///< resumeSending() so that the residue can be sent.
		///<
		///< If there was any other error, or if flow control
		///< was asserted after default construction, then false
		///< is returned and failed() will return true.

	bool resumeSending( Socket & ) ;
		///< To be called from the event-handler's write
		///< handler after flow-control has been released.
		///< If all residual data is sent then the socket's
		///< write handler is removed and true is returned.

	bool failed() const ;
		///< Returns true after a fatal error when send()ing data
		///< down the socket.

	bool busy() const ;
		///< Returns true if there is data queued up in the sender.

private:
	Sender( const Sender & ) ;
	void operator=( const Sender & ) ;

private:
	EventHandler & m_handler ;
	std::string m_residue ;
	bool m_failed ;
	unsigned long m_n ;
} ;

#endif
