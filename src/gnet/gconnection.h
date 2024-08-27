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
/// \file gconnection.h
///

#ifndef G_NET_CONNECTION_H
#define G_NET_CONNECTION_H

#include "gdef.h"
#include "gaddress.h"

namespace GNet
{
	class Connection ;
}

//| \class GNet::Connection
/// An abstract interface which provides information about a network
/// connection.
/// \see GNet::Client, GNet::ServerPeer
///
class GNet::Connection
{
public:
	virtual ~Connection() = default ;
		///< Destructor.

	virtual Address localAddress() const = 0 ;
		///< Returns the connection's local address.

	virtual Address peerAddress() const = 0 ;
		///< Returns the connection's peer address.
		///< Throws if a client connection that has not yet connected.

	virtual std::string connectionState() const = 0 ;
		///< Returns the connection state as a display string.
		///< This should be the peerAddress() display string, unless
		///< a client connection that has not yet connected.

	virtual std::string peerCertificate() const = 0 ;
		///< Returns the peer's TLS certificate. Returns the
		///< empty string if none.
} ;

#endif
