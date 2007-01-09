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
// dir_win32.cpp
//

#include "dir.h"
#include "gpath.h"
#include "gfile.h"
#include <cstdlib> //getenv
#include <stdexcept>

std::string Dir::env( const std::string & key , const std::string & default_ )
{
	const char * p = getenv( key.c_str() ) ;
	if( p == NULL )
		return default_ ;
	return std::string(p) ;
}

G::Path Dir::windows()
{
	char buffer[MAX_PATH+20U] = { '\0' } ;
	unsigned int n = sizeof(buffer) ;
	::GetWindowsDirectory( buffer , n-1U ) ;
	buffer[n-1U] = '\0' ;
	return G::Path(buffer) ;
}

std::string Dir::dotexe()
{
	return ".exe" ;
}

G::Path Dir::install()
{
	return prefix(special("programs")) + "emailrelay" ;
}

G::Path Dir::config()
{
	return prefix(windows()) ;
}

G::Path Dir::spool()
{
	return prefix(windows()) + "spool" + "emailrelay" ;
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
	return thisdir() ;
}

G::Path Dir::startup()
{
	return prefix(special("login")) ;
}

G::Path Dir::pid()
{
	return windows() ;
}

G::Path Dir::boot()
{
	return windows() ; // not used
}

