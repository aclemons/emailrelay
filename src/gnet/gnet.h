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
// gnet.h
//

#ifndef G_NET_H 
#define G_NET_H

// Title: gnet.h
// Description: gnet.h includes o/s-dependent network header files.

#ifdef G_UNIX
	#include <sys/types.h>
	#include <sys/utsname.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <arpa/inet.h> // for inet_ntoa()
	typedef int SOCKET ; // used in gdescriptor.h
#else
	//#include <winsock.h> // winsock.h comes via windows.h
	typedef int socklen_t ;
#endif

typedef unsigned short g_port_t ; // ('in_port_t' is not defined on many systems)

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff // (should be in netinet/in.h)
#endif

#endif

