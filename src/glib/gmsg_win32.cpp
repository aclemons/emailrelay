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
/// \file gmsg_win32.cpp
///

#include "gdef.h"
#include "gmsg.h"

ssize_t G::Msg::send( SOCKET fd , const void * buffer , std::size_t size , int flags ) noexcept
{
	return ::send( fd , reinterpret_cast<const char*>(buffer) , static_cast<int>(size) , flags ) ;
}

ssize_t G::Msg::sendto( SOCKET fd , const void * buffer , std::size_t size , int flags ,
	const sockaddr * address_p , socklen_t address_n ) noexcept
{
	return ::sendto( fd , reinterpret_cast<const char*>(buffer) , static_cast<int>(size) ,
		flags , address_p , address_n ) ;
}

ssize_t G::Msg::recv( SOCKET fd , void * buffer , std::size_t size , int flags ) noexcept
{
	return ::recv( fd , reinterpret_cast<char*>(buffer) , static_cast<int>(size) , flags ) ;
}

ssize_t G::Msg::recvfrom( SOCKET fd , void * buffer , std::size_t size , int flags ,
	sockaddr * address_p , socklen_t * address_np ) noexcept
{
	return ::recvfrom( fd , reinterpret_cast<char*>(buffer) , static_cast<int>(size) ,
		flags , address_p , address_np ) ;
}

bool G::Msg::fatal( int error ) noexcept
{
	return !(
		error == 0 ||
		error == WSAEINTR ||
		error == WSAEWOULDBLOCK ||
		error == WSAEINPROGRESS ||
		error == WSAENOBUFS ||
		false ) ;
}

