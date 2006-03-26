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
// dir.cpp
//

#include "dir.h"
#include "gpath.h"
#include "gfile.h"

#ifndef G_SPOOLDIR
	#define G_SPOOLDIR ""
#endif

#ifndef G_SYSCONFDIR
	#define G_SYSCONFDIR ""
#endif

#ifndef G_LIBEXECDIR
	#define G_LIBEXECDIR ""
#endif

#ifndef G_DESTDIR
	#define G_DESTDIR ""
#endif

#ifdef _WIN32
G::Path Dir::install()
{
	return "c:\\program files\\emailrelay" ;
}
static G::Path windows()
{
	char buffer[MAX_PATH+20U] = { '\0' } ;
	unsigned int n = sizeof(buffer) ;
	::GetWindowsDirectory( buffer , n-1U ) ;
	buffer[n-1U] = '\0' ;
	return G::Path(buffer) ;
}
G::Path Dir::config()
{
	return ::windows() ;
}
G::Path Dir::spool()
{
        std::string spooldir( G_SPOOLDIR ) ;
        if( spooldir.empty() )
                spooldir = windows() + "spool" + "emailrelay" ;
        return G::Path( spooldir ) ;
}
G::Path Dir::cwd()
{
	char buffer[10000] = { '\0' } ;
	unsigned long n = ::GetCurrentDirectory( sizeof(buffer)-1U , buffer ) ;
	buffer[sizeof(buffer)-1U] = '\0' ;
	std::string s = n ? std::string(buffer) : std::string(".") ;
	return G::Path( s ) ;
}
G::Path Dir::tooldir()
{
	return G::Path( "...." ) ; // bogus
}
G::Path Dir::startup()
{
	return G::Path( "...." ) ; // bogus
}
G::Path G::System::pid()
{
	return G::Path( "...." ) ; // bogus
}
#else
#include <unistd.h>
G::Path Dir::install()
{
	std::string s( G_DESTDIR ) ;
	if( s.empty() )
		s = "/usr/local/emailrelay" ;
	return G::Path( s ) ;
}
G::Path Dir::config()
{
	std::string s( G_SYSCONFDIR ) ;
	if( s.empty() )
		s = "/etc" ;
	return G::Path( s ) ;
}
G::Path Dir::spool()
{
        std::string spooldir( G_SPOOLDIR ) ;
        if( spooldir.empty() )
                spooldir = "/var/spool/emailrelay" ;
        return G::Path( spooldir ) ;
}
G::Path Dir::cwd()
{
	char buffer[10000] = { '\0' } ;
	const char * p = getcwd( buffer , sizeof(buffer)-1U ) ;
	buffer[sizeof(buffer)-1U] = '\0' ;
	std::string s = p ? std::string(buffer) : std::string(".") ;
	return G::Path( s ) ;
}
G::Path Dir::tooldir()
{
	std::string s( G_LIBEXECDIR ) ;
	if( s.empty() )
		s = "/usr/lib" ;
	return G::Path( s ) ;
}
G::Path Dir::startup()
{
	return config() + "init.d" ;
}
G::Path Dir::pid()
{
	G::Path var_run( "/var/run" ) ;
	G::Path tmp( "/tmp" ) ;
	return G::File::exists(var_run) ? var_run : tmp ;
}
#endif

G::Path Dir::tooldir( const std::string & argv0 )
{
	// (make some effort to return an absolute path -- not foolproof on windows)
	G::Path exe_dir = G::Path(argv0).dirname() ; 
	return
		( exe_dir.isRelative() && !exe_dir.hasDriveLetter() ) ?
			( cwd() + exe_dir.str() ) :
			exe_dir ;
}

