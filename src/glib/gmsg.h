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
/// \file gmsg.h
///

#ifndef G_MSG_H
#define G_MSG_H

#include "gdef.h"
#include "gstringview.h"
#include <vector>

namespace G
{
	class Msg ;
}

//| \class G::Msg
/// Wrappers for sendmsg() and recvmsg(). These are near drop-in replacements for
/// send()/sendto() and recv()/recvto(), but with SIGPIPE disabled and optional
/// file-descriptor-passing capabilities.
/// \see man unix(7) and man cmsg(3)
///
class G::Msg
{
public:
	static ssize_t send( SOCKET , const void * , std::size_t , int flags ) noexcept ;
		///< A send() wrapper.

	static ssize_t sendto( SOCKET , const void * , std::size_t , int flags , const sockaddr * , socklen_t ) noexcept ;
		///< A sendto() wrapper.

	static ssize_t sendto( SOCKET , const void * , std::size_t , int flags , const sockaddr * , socklen_t ,
		int fd_to_send ) ;
			///< A sendmsg() wrapper. Not always implemented.

	static ssize_t sendto( SOCKET , const std::vector<string_view> & , int flags , const sockaddr * , socklen_t ) ;
		///< A sendto() wrapper with scatter-gather data chunks. Not always implemented.

	static ssize_t recv( SOCKET , void * , std::size_t , int flags ) noexcept ;
		///< A recv() wrapper.

	static ssize_t recvfrom( SOCKET , void * , std::size_t , int , sockaddr * , socklen_t * ) noexcept ;
		///< A recvfrom() wrapper.

	static ssize_t recvfrom( SOCKET , void * , std::size_t , int , sockaddr * , socklen_t * ,
		int * fd_received_p ) ;
			///< A recvmsg() wrapper. The address and file descriptor pointers
			///< can be null independently. Not always implemented.

	static bool fatal( int error ) noexcept ;
		///< Returns true if the error value indicates a permanent
		///< problem with the socket.

public:
	Msg() = delete ;
} ;

#endif
