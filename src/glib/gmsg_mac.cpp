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
/// \file gmsg_mac.cpp
///

#include "gdef.h"
#include "gmsg.h"
#include <cerrno> // EINTR etc
#include <sys/types.h>
#include <sys/socket.h>

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

ssize_t G::Msg::recv( int fd , void * buffer , std::size_t size , int flags ) noexcept
{
	return ::recv( fd , buffer , size , flags ) ;
}

ssize_t G::Msg::recvfrom( int fd , void * buffer , std::size_t size , int flags ,
	sockaddr * address_p , socklen_t * address_np ) noexcept
{
	return ::recvfrom( fd , buffer , size , flags , address_p , address_np ) ;
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

