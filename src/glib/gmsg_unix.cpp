//
// Copyright (C) 2001-2020 Graeme Walker <graeme_walker@users.sourceforge.net>
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
//
// gmsg_unix.cpp
//

#include "gdef.h"
#include "gmsg.h"
#include "gprocess.h"
#include "gassert.h"
#include "gstr.h"
#include <cstring> // std::memcpy()
#include <cerrno> // EINTR etc
#include <stdexcept>
#include <array>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>

ssize_t G::Msg::send( int fd , const void * buffer , std::size_t size , int flags ,
	int fd_to_send )
{
	return sendto( fd , buffer , size , flags , nullptr , 0 , fd_to_send ) ;
}

ssize_t G::Msg::sendto( int fd , const void * buffer , std::size_t size , int flags ,
	const sockaddr * address_p , socklen_t address_n , int fd_to_send )
{
	struct ::msghdr msg {} ;

	msg.msg_name = const_cast<sockaddr*>(address_p) ;
	msg.msg_namelen = address_n ;

	struct ::iovec io {} ;
	io.iov_base = const_cast<void*>(buffer) ;
	io.iov_len = size ;
	msg.msg_iov = &io ;
	msg.msg_iovlen = 1 ;

	std::array<char,CMSG_SPACE(sizeof(int))> control_buffer ; // NOLINT cppcoreguidelines-pro-type-member-init
	if( fd_to_send == -1 )
	{
		msg.msg_control = nullptr ;
		msg.msg_controllen = 0U ;
	}
	else
	{
		std::memset( &control_buffer[0] , 0 , control_buffer.size() ) ;
		msg.msg_control = &control_buffer[0] ;
		msg.msg_controllen = control_buffer.size() ;

		struct ::cmsghdr * cmsg = CMSG_FIRSTHDR( &msg ) ;
		G_ASSERT( cmsg != nullptr ) ;
		cmsg->cmsg_len = CMSG_LEN( sizeof(int) ) ;
		cmsg->cmsg_level = SOL_SOCKET ;
		cmsg->cmsg_type = SCM_RIGHTS ;
		std::memcpy( CMSG_DATA(cmsg) , &fd_to_send , sizeof(int) ) ;
	}

	return ::sendmsg( fd , &msg , flags | MSG_NOSIGNAL ) ;
}

ssize_t G::Msg::recv( int fd , void * buffer , std::size_t size , int flags )
{
	return ::recv( fd , buffer , size , flags ) ;
}

ssize_t G::Msg::recv( int fd , void * buffer , std::size_t size , int flags ,
	int * fd_received_p )
{
	return recvfrom( fd , buffer , size , flags , nullptr , nullptr , fd_received_p ) ;
}

ssize_t G::Msg::recvfrom( int fd , void * buffer , std::size_t size , int flags ,
	sockaddr * address_p , socklen_t * address_np , int * fd_received_p )
{
	struct ::msghdr msg {} ;

	msg.msg_name = address_p ;
	msg.msg_namelen = address_np == nullptr ? socklen_t(0) : *address_np ;

	struct ::iovec io {} ;
	io.iov_base = buffer ;
	io.iov_len = size ;
	msg.msg_iov = &io ;
	msg.msg_iovlen = 1 ;

	std::array<char,CMSG_SPACE(sizeof(int))> control_buffer ; // NOLINT cppcoreguidelines-pro-type-member-init
	msg.msg_control = &control_buffer[0] ;
	msg.msg_controllen = control_buffer.size() ;

	ssize_t rc = ::recvmsg( fd , &msg , flags ) ;
	int e = Process::errno_() ;
	if( rc >= 0 && msg.msg_controllen > 0U && fd_received_p != nullptr )
	{
		struct cmsghdr * cmsg = CMSG_FIRSTHDR( &msg ) ;
		if( cmsg != nullptr && cmsg->cmsg_type == SCM_RIGHTS )
		{
			std::memcpy( fd_received_p , CMSG_DATA(cmsg) , sizeof(int) ) ;
		}
	}
	if( rc >= 0 && address_np != nullptr )
	{
		*address_np = msg.msg_namelen ;
	}
	Process::errno_( SignalSafe() , e ) ;
	return rc ; // with errno
}

bool G::Msg::fatal( int error )
{
	return !(
		error == 0 ||
		error == EAGAIN ||
		error == EINTR ||
		error == EMSGSIZE || // moot
		error == ENOBUFS ||
		error == ENOMEM ||
		false ) ;
}

/// \file gmsg_unix.cpp
