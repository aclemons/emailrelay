//
// Copyright (C) 2001-2005 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gconnection.h
//

#ifndef G_NET_CONNECTION_H
#define G_NET_CONNECTION_H

#include "gdef.h"
#include "gnet.h"
#include "gaddress.h"

namespace GNet
{
	class Connection ;
}

// Class: GNet::Connection
// Description: An interface which provides address information 
// for a network connection.
// See also: GNet::Client, GNet::ServerPeer
//
class GNet::Connection 
{
public:
	virtual std::pair<bool,Address> localAddress() const = 0 ;
		// Returns the connection's local address.
		// Pair.first is false if none.

	virtual std::pair<bool,Address> peerAddress() const = 0 ;
		// Returns the connection's peer address.
		// Pair.first is false if none.

	virtual ~Connection() ;
		// Destructor.

private:
	void operator=( const Connection & ) ; // not implemented
} ;

#endif
