//
// Copyright (C) 2001-2004 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// glogoutput_win32.cpp
//

#include "gdef.h"
#include "glogoutput.h"
#include "gpath.h"
#include <cstdlib> // getenv

static HANDLE source() ;

void G::LogOutput::cleanup()
{
	if( m_handle != 0 )
		::DeregisterEventSource( m_handle ) ;
}

void G::LogOutput::rawOutput( G::Log::Severity severity , const char *message )
{
	// standard error
	//
	std::cerr << message << std::endl ;

	// debugger
	//
	static bool debugger = std::getenv("GLOGOUTPUT_DEBUGGER") != NULL ;
	if( debugger )
	{
		::OutputDebugString( message ) ;
	}

	// file
	//
	static const char * key = "GLOGOUTPUT_FILE" ;
	static const char * filename = std::getenv( key ) ;
	if( filename != NULL && *filename != '\0' )
	{
		static std::ofstream file( filename ) ;
		file << message << std::endl ;
	}

	// event log
	//
	if( m_syslog && severity != Log::s_Debug && m_handle != 0 )
	{
		// (assume suitable string resources of 1001..1003)

		DWORD id = 0x400003E9L ;
		WORD type = EVENTLOG_INFORMATION_TYPE ;
		if( severity == Log::s_Warning )
		{
			id = 0x800003EAL ;
			type = EVENTLOG_WARNING_TYPE ;
		}
		else if( severity == Log::s_Error || severity == Log::s_Assertion )
		{
			id = 0xC00003EBL ;
			type = EVENTLOG_ERROR_TYPE ;
		}

		const char * p[] = { message , NULL } ;
		G_IGNORE ::ReportEvent( m_handle, type, 0, id, NULL, 1, 0, p, NULL ) ;
	}
}

void G::LogOutput::init()
{
	m_handle = ::source() ;
}

static HANDLE source()
{
	G::Path exe_path ;
	{
		HINSTANCE hinstance = 0 ;
		char buffer[260U*4U] ; // *4 for luck
		size_t size = sizeof(buffer) ;
		*buffer = '\0' ;
		::GetModuleFileName( hinstance , buffer , size-1U ) ;
		buffer[size-1U] = '\0' ;
		exe_path = G::Path(buffer) ;
	}
	std::string exe_name ;
	{
		G::Path p( exe_path ) ;
		p.removeExtension() ;
		exe_name = p.basename() ;
	}
	std::string reg_path_prefix( "SYSTEM\\CurrentControlSet\\Services\\"
		"EventLog\\Application\\" ) ;
	std::string reg_path = reg_path_prefix + exe_name ;

	HKEY key = 0 ;
	::RegCreateKey( HKEY_LOCAL_MACHINE , reg_path.c_str() , &key ) ;
	bool ok = key != 0 ;

	if( ok )
	{
		std::string value = exe_path.str() ;
		char * value_p = const_cast<char*>(value.c_str()) ;
		ok = ! ::RegSetValueEx( key , "EventMessageFile" , 0 , REG_EXPAND_SZ ,
			reinterpret_cast<LPBYTE>(value_p) , value.length()+1U ) ;
	}

	if( ok )
	{
		DWORD value = 
			EVENTLOG_INFORMATION_TYPE | 
			EVENTLOG_WARNING_TYPE |
			EVENTLOG_ERROR_TYPE ;

		ok = ! ::RegSetValueEx( key , "TypesSupported" , 0 , REG_DWORD ,
			reinterpret_cast<LPBYTE>(&value) , sizeof(value) ) ;

	}

	if( key != 0 )
		::RegCloseKey( key ) ;

	return ok ?
		::RegisterEventSource( NULL , exe_name.c_str() ) :
		0 ;
}

