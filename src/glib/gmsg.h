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
/// \file gmsg.h
///

#ifndef G_MSG__H
#define G_MSG__H

#include "gdef.h"

namespace G
{
	class Msg ;
}

/// \class G::Msg
/// Wrappers for sendmsg() and recvmsg() that are near drop-in replacements for
/// send()/sendto() and recv()/recvto(), but with SIGPIPE disabled and optional
/// file-descriptor-passing capabilities.
/// \see man unix(7) and man cmsg(3)
///
class G::Msg
{
public:
	static ssize_t send( SOCKET , const void * , size_t , int ,
		int fd_to_send = -1 ) ;
			///< A send() replacement using sendmsg().

	static ssize_t sendto( SOCKET , const void * , size_t , int , const sockaddr * , socklen_t ,
		int fd_to_send = -1 ) ;
			///< A sendto() replacement using sendmsg().

	static ssize_t recv( SOCKET , void * , size_t , int ) ;
		///< A recv() wrapper.

	static ssize_t recv( SOCKET , void * , size_t , int , int * fd_received_p ) ;
		///< A recv() replacement using recvmsg().

	static ssize_t recvfrom( SOCKET , void * , size_t , int , sockaddr * , socklen_t * ,
		int * fd_received_p = nullptr ) ;
			///< A recvfrom() replacement using recvmsg().

	static bool fatal( int error ) ;
		///< Returns true if the error value indicates a permanent
		///< problem with the socket.

private:
	Msg() ;
} ;

#endif
