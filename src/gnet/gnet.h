//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gnet.h
///

#ifndef G_NET_H 
#define G_NET_H

/// Title: gnet.h
/// gnet.h includes o/s-dependent network header files.

#ifdef G_UNIX
	#include <sys/types.h>
	#include <sys/utsname.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <arpa/inet.h> // for inet_ntoa()
	typedef int SOCKET ; // used in gdescriptor.h
#else
	typedef int socklen_t ;
#endif
typedef g_uint16_t g_port_t ; // 'in_port_t' not always available

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff // (should be in netinet/in.h)
#endif

#endif

