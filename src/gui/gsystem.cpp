//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gsystem.cpp
//

#include "gsystem.h"

#ifndef G_SPOOLDIR
	#define G_SPOOLDIR ""
#endif

#ifndef G_SYSCONFDIR
	#define G_SYSCONFDIR ""
#endif

#ifndef G_DESTDIR
	#define G_DESTDIR ""
#endif

G::Path GSystem::spool()
{
        std::string spooldir( G_SPOOLDIR ) ;
        if( spooldir.empty() )
                spooldir = "/var/spool/emailrelay" ;
        return G::Path( spooldir ) ;
}

#ifdef _WIN32
G::Path GSystem::install()
{
	return "c:\\program files\\emailrelay" ;
}
G::Path GSystem::config()
{
	char buffer[MAX_PATH+20U] = { '\0' } ;
	unsigned int n = sizeof(buffer) ;
	::GetWindowsDirectory( buffer , n-1U ) ;
	buffer[n-1U] = '\0' ;
	return G::Path(buffer) ;
}
G::Path GSystem::cwd()
{
	char buffer[10000] = { '\0' } ;
	unsigned long n = ::GetCurrentDirectory( sizeof(buffer)-1U , buffer ) ;
	buffer[sizeof(buffer)-1U] = '\0' ;
	std::string s = n ? std::string(buffer) : std::string(".") ;
	return G::Path( s ) ;
}
#else
#include <unistd.h>
G::Path GSystem::install()
{
	std::string s( G_DESTDIR ) ;
	if( s.empty() )
		s = "/usr/local/emailrelay" ;
	return G::Path( s ) ;
}
G::Path GSystem::config()
{
	std::string s( G_SYSCONFDIR ) ;
	if( s.empty() )
		s = "/etc" ;
	return G::Path( s ) ;
}
G::Path GSystem::cwd()
{
	char buffer[10000] = { '\0' } ;
	const char * p = getcwd( buffer , sizeof(buffer)-1U ) ;
	buffer[sizeof(buffer)-1U] = '\0' ;
	std::string s = p ? std::string(buffer) : std::string(".") ;
	return G::Path( s ) ;
}
#endif

