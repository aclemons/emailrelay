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
/// \file dir_win32.cpp
///

#include "gdef.h"
#include "gconvert.h"
#include "dir.h"
#include "gfile.h"
#include "gpath.h"
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

G::Path Dir::os_install()
{
	return special("programs") + "E-MailRelay" ;
}

G::Path Dir::os_config()
{
	return special("data") + "E-MailRelay" ;
}

G::Path Dir::os_spool()
{
	return special("data") + "E-MailRelay" + "spool" ;
}

G::Path Dir::os_pid( const G::Path & /*config_dir*/ )
{
	return special("data") + "E-MailRelay" ;
}

G::Path Dir::os_boot()
{
	// the default has to be any non-empty string
	return "services" ;
}

namespace
{
	int special_id( const std::string & type )
	{
		if( type == "desktop" ) return CSIDL_DESKTOPDIRECTORY ; // "c:/users/<username>/desktop"
		if( type == "menu" ) return CSIDL_PROGRAMS ; // "c:/users/<username>/appdata/roaming/microsoft/windows/start menu/programs"
		if( type == "autostart" ) return CSIDL_STARTUP ; // "c:/users/<username>/appdata/roaming/microsoft/windows/start menu/startup/programs"
		if( type == "programs" ) return sizeof(void*) == 4 ? CSIDL_PROGRAM_FILESX86 : CSIDL_PROGRAM_FILES ; // "c:/program files"
		if( type == "data" ) return CSIDL_COMMON_APPDATA ; // "c:/programdata"
		throw std::runtime_error("internal error") ;
		return 0 ;
	}
}

G::Path Dir::special( const std::string & type )
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

G::Path Dir::home()
{
	return envPath( "USERPROFILE" , envPath( "HOME" , desktop() ) ) ;
}

