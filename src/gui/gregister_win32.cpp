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
//
// gregister_win32.cpp
//

#include "gdef.h"
#include "gregister.h"

void GRegister::server( const G::Path & path )
{
	// see also glogoutput_win32.cpp

	std::string reg_path = "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\" ;
	G::Path basename = path.basename() ; basename.removeExtension() ;
	reg_path.append( basename.str() ) ;
	HKEY key = 0 ;
	DWORD type_value = EVENTLOG_INFORMATION_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_ERROR_TYPE ;
	::RegCreateKeyA( HKEY_LOCAL_MACHINE , reg_path.c_str() , &key ) ;
	if( key != 0 )
	{
		::RegSetValueExA( key , "EventMessageFile" , 0 , REG_EXPAND_SZ , reinterpret_cast<const BYTE*>(path.str().c_str()) , path.str().length()+1U ) ;
		::RegSetValueExA( key , "TypesSupported" , 0 , REG_DWORD , reinterpret_cast<const BYTE*>(&type_value) , sizeof(type_value) ) ;
		::RegCloseKey( key ) ;
	}
}

/// \file gregister_win32.cpp
