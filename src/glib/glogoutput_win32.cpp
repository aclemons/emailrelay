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
// glogoutput_win32.cpp
//

#include "gdef.h"
#include "glogoutput.h"
#include "glimits.h"
#include "genvironment.h"
#include <time.h> // localtime_s

static HANDLE source() ;

static bool simple( const std::string & dir )
{
	const char * map = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-\\.: " ;
	return std::string::npos == dir.find_first_not_of( map ) ;
}

void G::LogOutput::cleanup()
{
	if( m_handle != 0 )
		::DeregisterEventSource( m_handle ) ;
}

void G::LogOutput::rawOutput( std::ostream & std_err , G::Log::Severity severity , const std::string & message )
{
	// standard error
	//
	std_err << message << std::endl ;

	// debugger
	//
	static bool debugger = ! G::Environment::get( "GLOGOUTPUT_DEBUGGER" , std::string() ).empty() ;
	if( debugger )
	{
		::OutputDebugStringA( message.c_str() ) ;
	}

	// file
	//
	static std::string dir = G::Environment::get( "GLOGOUTPUT_DIR" , std::string() ) ;
	if( !dir.empty() && simple(dir) )
	{
		static std::ofstream file( (dir+"\\log.txt").c_str() ) ;
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

		const char * p[] = { message.c_str() , NULL } ;
		G_IGNORE_RETURN(BOOL) ::ReportEventA( m_handle, type, 0, id, NULL, 1, 0, p, NULL ) ;
	}
}

void G::LogOutput::init()
{
	m_handle = ::source() ;
}

static HANDLE source()
{
	// get our executable path
	std::string exe_path ;
	{
		HINSTANCE hinstance = 0 ;
		char buffer[G::limits::path] ;
		size_t size = sizeof(buffer) ;
		*buffer = '\0' ;
		::GetModuleFileNameA( hinstance , buffer , size-1U ) ;
		buffer[size-1U] = '\0' ;
		exe_path = std::string(buffer) ;
	}

	// parse out our executable basename
	std::string exe_name = exe_path ;
	{
		std::string::size_type pos1 = exe_name.find_last_of( "\\" ) ;
		if( pos1 != std::string::npos )
			exe_name = exe_name.substr( pos1+1U ) ;
		std::string::size_type pos2 = exe_name.find_last_of( "." ) ;
		if( pos2 != std::string::npos )
			exe_name.resize( pos2 ) ;
	}

	// build a registry path for our executable
	//
	std::string reg_path_prefix( "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\" ) ;
	std::string reg_path = reg_path_prefix + exe_name ;

	// create a registry entry
	//
	HKEY key = 0 ;
	::RegCreateKeyA( HKEY_LOCAL_MACHINE , reg_path.c_str() , &key ) ;
	bool ok = key != 0 ;

	// add our executable path
	if( ok )
	{
		ok = ! ::RegSetValueExA( key , "EventMessageFile" , 0 , REG_EXPAND_SZ ,
			reinterpret_cast<const BYTE*>(exe_path.c_str()) , exe_path.length()+1U ) ;
	}

	// add our message types
	if( ok )
	{
		DWORD value = 
			EVENTLOG_INFORMATION_TYPE | 
			EVENTLOG_WARNING_TYPE |
			EVENTLOG_ERROR_TYPE ;

		ok = ! ::RegSetValueExA( key , "TypesSupported" , 0 , REG_DWORD ,
			reinterpret_cast<const BYTE*>(&value) , sizeof(value) ) ;
	}

	// close the registry
	//
	if( key != 0 )
		::RegCloseKey( key ) ;

	// if we failed to get access to the registry we can still write
	// to the event log but the associated text will be messed up,
	// so ignore 'ok' here
	//
	return ::RegisterEventSourceA( NULL , exe_name.c_str() ) ;
}

void G::LogOutput::getLocalTime( time_t epoch_time , struct std::tm * broken_down_time_p )
{
	errno_t rc = localtime_s( broken_down_time_p , &epoch_time ) ;
	// ignore errors - it's just logging
}
/// \file glogoutput_win32.cpp
