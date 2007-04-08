//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
///
/// \file dir_winnt.cpp"
///

#include "dir.h"
#include "gpath.h"
#include "gfile.h"
#include <cstdlib> //getenv
#include <stdexcept>

G::Path Dir::special( const std::string & type )
{
	std::string user = env("USER") ;
	G::Path user_profile = windows() + "Profiles" + user ;
	G::Path common_profile = windows() + "Profiles" + "All Users" ;

	if( type == "desktop" ) 
	{
		return user_profile + "Desktop" ;
	}
	if( type == "menu" ) 
	{
		return user_profile + "Start Menu" + "Programs" ;
	}
	if( type == "login" ) 
	{
		return user_profile + "Start Menu" + "Programs" + "Startup" ;
	}
	if( type == "lib" ) 
	{
		return G::Path() ;
	}
	if( type == "programs" )
	{
		G::Path p = env("ProgramFiles") ; // doesnt work on nt :-(
		if( p.str().empty() )
			p = "c:/program files" ;
		return p ;
	}
	if( type == "reskit" )
	{
		G::Path p = env("NTRESKIT") ;
		if( p.str().empty() )
			p = special("programs")+"resource kit";
		return p ;
	}
	return G::Path() ;
}

