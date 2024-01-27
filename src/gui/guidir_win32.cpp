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
/// \file guidir_win32.cpp
///

#include "gdef.h"
#include "gconvert.h"
#include "guidir.h"
#include "gfile.h"
#include "gpath.h"
#include "genvironment.h"
#include "glog.h"
#include <stdexcept>
#include <vector>
#include <shlwapi.h>
#include <shlobj.h>

#ifndef SHGFP_TYPE_CURRENT
#define SHGFP_TYPE_CURRENT 0
#endif
#ifndef CSIDL_PROGRAM_FILES
#define CSIDL_PROGRAM_FILES 38
#endif
#ifndef CSIDL_PROGRAM_FILESX86
#define CSIDL_PROGRAM_FILESX86 42
#endif

namespace Gui
{
	namespace DirImp
	{
		int special_id( const std::string & type ) ;
		G::Path special( const std::string & type ) ;
		G::Path envPath( const std::string & key , const G::Path & default_ ) ;
		G::Path home() ;
	}
}

G::Path Gui::Dir::install()
{
	return DirImp::special("programs") + "E-MailRelay" ;
}

G::Path Gui::Dir::config()
{
	return DirImp::special("data") + "E-MailRelay" ;
}

G::Path Gui::Dir::spool()
{
	return DirImp::special("data") + "E-MailRelay" + "spool" ;
}

G::Path Gui::Dir::pid( const G::Path & )
{
	return DirImp::special("data") + "E-MailRelay" ;
}

G::Path Gui::Dir::home()
{
	return DirImp::envPath( "USERPROFILE" , DirImp::envPath( "HOME" , desktop() ) ) ;
}

G::Path Gui::Dir::desktop()
{
	return DirImp::special( "desktop" ) ;
}

G::Path Gui::Dir::autostart()
{
	return DirImp::special( "autostart" ) ;
}

G::Path Gui::Dir::menu()
{
	return DirImp::special( "menu" ) ;
}

// ==

G::Path Gui::DirImp::special( const std::string & type )
{
	// this is not quite right when running with UAC administrator rights because
	// it gets the administrator's user directories for the desktop etc links and not
	// the user's -- and there is no reasonable way to get the user's access token
	std::vector<char> buffer( MAX_PATH+1U ) ;
	buffer.at(0) = '\0' ;
	HANDLE user_token = HNULL ; // TODO original user's paths when run-as administrator
	bool ok = S_OK == SHGetFolderPathA( HNULL , special_id(type) , user_token , SHGFP_TYPE_CURRENT , &buffer[0] ) ;
	buffer.at(buffer.size()-1U) = '\0' ;
	return ok ? G::Path(&buffer[0]) : G::Path("c:/") ;
}

int Gui::DirImp::special_id( const std::string & type )
{
	if( type == "desktop" ) return CSIDL_DESKTOPDIRECTORY ; // "c:/users/<username>/desktop"
	if( type == "menu" ) return CSIDL_PROGRAMS ; // "c:/users/<username>/appdata/roaming/microsoft/windows/start menu/programs"
	if( type == "autostart" ) return CSIDL_STARTUP ; // "c:/users/<username>/appdata/roaming/microsoft/windows/start menu/startup/programs"
	if( type == "programs" ) return sizeof(void*) == 4 ? CSIDL_PROGRAM_FILESX86 : CSIDL_PROGRAM_FILES ; // "c:/program files"
	if( type == "data" ) return CSIDL_COMMON_APPDATA ; // "c:/programdata"
	throw std::runtime_error("internal error") ;
}

G::Path Gui::DirImp::envPath( const std::string & key , const G::Path & default_ )
{
	return G::Path( G::Environment::get( key , default_.str() ) ) ;
}

