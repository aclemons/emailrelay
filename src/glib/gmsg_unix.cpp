//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gmsg_unix.cpp
///

#include "gdef.h"
#include "gmsg.h"
#include "gprocess.h"
#include "gassert.h"
#include "gstr.h"
#include <cstring> // std::memcpy()
#include <cerrno> // EINTR etc
#include <stdexcept>
#include <array>
#include <type_traits>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>

namespace G
{
	namespace MsgImp
	{
		ssize_t sendmsg( int fd , const iovec * , std::size_t , int flags ,
			const sockaddr * address_p , socklen_t address_n , int fd_to_send ) noexcept ;
	}
}

ssize_t G::Msg::send( int fd , const void * buffer , std::size_t size , int flags ) noexcept
{
	return sendto( fd , buffer , size , flags , nullptr , 0U ) ;
}

ssize_t G::Msg::sendto( int fd , const void * buffer , std::size_t size , int flags ,
	const sockaddr * address_p , socklen_t address_n ) noexcept
{
	return ::sendto( fd , buffer , size , flags|MSG_NOSIGNAL , // NOLINT
		const_cast<sockaddr*>(address_p) , address_n ) ;
}

ssize_t G::Msg::sendto( int fd , const std::vector<string_view> & data , int flags ,
	const sockaddr * address_p , socklen_t address_n )
{
	if( data.empty() )
	{
		return 0 ;
	}
	else if( data.size() == 1U )
	{
		return sendto( fd , data[0].data() , data[0].size() , flags , address_p , address_n ) ;
	}
	else
	{
		#if GCONFIG_HAVE_IOVEC_SIMPLE
			// struct iovec is the same as string_view so we can cast
			const ::iovec * io_p = reinterpret_cast<const ::iovec*>( &data[0] ) ;
			return MsgImp::sendmsg( fd , io_p , data.size() , flags , address_p , address_n , -1 ) ;
 		#else
			std::vector<::iovec> io( data.size() ) ; // TODO optimise
			std::size_t j = 0U ;
			for( std::size_t i = 0U ; i < data.size() ; i++ )
			{
				if( data[i].empty() ) continue ;
				io[j].iov_base = const_cast<char*>(data[i].data()) ;
				io[j++].iov_len = data[i].size() ;
			}
			return MsgImp::sendmsg( fd , &io[0] , j , flags , address_p , address_n , -1 ) ;
		#endif
	}
}

ssize_t G::Msg::sendto( int fd , const void * buffer , std::size_t size , int flags ,
	const sockaddr * address_p , socklen_t address_n , int fd_to_send )
{
	if( fd_to_send == -1 )
	{
		return sendto( fd , buffer , size , flags , address_p , address_n ) ;
	}
	else
	{
		struct ::iovec io {} ;
		io.iov_base = const_cast<void*>(buffer) ;
		io.iov_len = size ;
		return MsgImp::sendmsg( fd , &io , 1U , flags , address_p , address_n , fd_to_send ) ;
	}
}

ssize_t G::MsgImp::sendmsg( int fd , const ::iovec * iovec_p , std::size_t iovec_n , int flags ,
	const sockaddr * address_p , socklen_t address_n , int fd_to_send ) noexcept
{
	struct ::msghdr msg {} ;

	msg.msg_name = const_cast<sockaddr*>(address_p) ;
	msg.msg_namelen = address_n ;
	msg.msg_iov = const_cast<::iovec*>(iovec_p) ;
	msg.msg_iovlen = iovec_n ;

	// TODO CMSG_SPACE() is not a compile-time constant on OSX

	constexpr std::size_t space = CMSG_SPACE( sizeof(int) ) ;
	static_assert( space != 0U , "" ) ;
	std::array<char,space> control_buffer {} ;
	std::memset( &control_buffer[0] , 0 , control_buffer.size() ) ;
	msg.msg_control = &control_buffer[0] ;
	msg.msg_controllen = control_buffer.size() ;

	struct ::cmsghdr * cmsg = CMSG_FIRSTHDR( &msg ) ; /// NOLINT
	G_ASSERT( cmsg != nullptr ) ;
	if( cmsg != nullptr )
	{
		cmsg->cmsg_len = CMSG_LEN( sizeof(int) ) ;
		cmsg->cmsg_level = SOL_SOCKET ;
		cmsg->cmsg_type = SCM_RIGHTS ;
		std::memcpy( CMSG_DATA(cmsg) , &fd_to_send , sizeof(int) ) ;
	}

	return ::sendmsg( fd , &msg , flags | MSG_NOSIGNAL ) ; // NOLINT
}

ssize_t G::Msg::recv( int fd , void * buffer , std::size_t size , int flags ) noexcept
{
	return ::recv( fd , buffer , size , flags ) ;
}

ssize_t G::Msg::recvfrom( int fd , void * buffer , std::size_t size , int flags ,
	sockaddr * address_p , socklen_t * address_np ) noexcept
{
	return ::recvfrom( fd , buffer , size , flags , address_p , address_np ) ;
}

ssize_t G::Msg::recvfrom( int fd , void * buffer , std::size_t size , int flags ,
	sockaddr * address_p , socklen_t * address_np , int * fd_received_p )
{
	if( fd_received_p == nullptr )
		return recvfrom( fd , buffer , size , flags , address_p , address_np ) ;

	struct ::msghdr msg {} ;

	msg.msg_name = address_p ;
	msg.msg_namelen = address_np == nullptr ? socklen_t(0) : *address_np ;

	struct ::iovec io {} ;
	io.iov_base = buffer ;
	io.iov_len = size ;
	msg.msg_iov = &io ;
	msg.msg_iovlen = 1 ;

	std::array<char,CMSG_SPACE(sizeof(int))> control_buffer {} ;
	msg.msg_control = &control_buffer[0] ;
	msg.msg_controllen = control_buffer.size() ;

	ssize_t rc = ::recvmsg( fd , &msg , flags ) ;
	int e = Process::errno_() ;
	if( rc >= 0 && msg.msg_controllen > 0U && fd_received_p != nullptr )
	{
		struct cmsghdr * cmsg = CMSG_FIRSTHDR( &msg ) ; /// NOLINT
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

bool G::Msg::fatal( int error ) noexcept
{
	return !(
		error == 0 ||
		error == EAGAIN ||
		error == EINTR ||
		error == EMSGSIZE || // moot
		error == ENOBUFS ||
		error == ENOMEM ) ;
}

