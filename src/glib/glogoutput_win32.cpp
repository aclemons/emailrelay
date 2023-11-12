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
/// \file glogoutput_win32.cpp
///

#include "gdef.h"
#include "glogoutput.h"
#include "genvironment.h"
#include "gfile.h"
#include <stdexcept>
#include <fstream>

namespace G
{
	namespace LogOutputWindowsImp
	{
		std::string thisExe()
		{
			// same code is in G::Process:exe()...
			std::vector<char> buffer ;
			std::size_t sizes[] = { 80U , 1024U , 32768U , 0U } ; // documented limit of 32k
			for( std::size_t * size_p = sizes ; *size_p ; ++size_p )
			{
				buffer.resize( *size_p+1U , '\0' ) ;
				DWORD size = static_cast<DWORD>( buffer.size() ) ;
				HINSTANCE hinstance = HNULL ;
				DWORD rc = GetModuleFileNameA( hinstance , &buffer[0] , size ) ;
				if( rc == 0 ) break ;
				if( rc < size )
					return std::string( &buffer[0] , rc ) ;
			}
			return std::string() ;
		}
		std::string basename( std::string s )
		{
			std::string::size_type pos1 = s.find_last_of( "\\" ) ;
			if( pos1 != std::string::npos )
				s = s.substr( pos1+1U ) ;
			std::string::size_type pos2 = s.find_last_of( "." ) ;
			if( pos2 != std::string::npos )
				s.resize( pos2 ) ;
			return s ;
		}
		bool oldWindows()
		{
			static bool old_windows_set = false ;
			static bool old_windows = false ;
			if( !old_windows_set )
			{
				old_windows_set = true ;
				old_windows = !IsWindowsVistaOrGreater() ;
			}
			return old_windows ;
		}
	}
}

void G::LogOutput::osoutput( int fd , G::Log::Severity severity , char * message , std::size_t n )
{
	// event log
	//
	if( m_config.m_use_syslog &&
		severity != Log::Severity::Debug &&
		severity != Log::Severity::InfoVerbose &&
		m_handle != HNULL )
	{
		DWORD id = 0x400003E9L ; // 1001
		WORD type = EVENTLOG_INFORMATION_TYPE ;
		if( severity == Log::Severity::Warning )
		{
			id = 0x800003EAL ; // 1002
			type = EVENTLOG_WARNING_TYPE ;
		}
		else if( severity == Log::Severity::Error || severity == Log::Severity::Assertion )
		{
			id = 0xC00003EBL ; // 1003
			type = EVENTLOG_ERROR_TYPE ;
		}

		// very old windowses do not seem to recognise "!S!" format specifiers so
		// as a workround (fwiw) use additional entries in messages.mc (1011, etc)
		if( LogOutputWindowsImp::oldWindows() )
			id += 10 ;

		message[n] = '\0' ;
		const char * p[] = { message , nullptr } ;
		BOOL rc = ReportEventA( m_handle , type , 0 , id , nullptr , 1 , 0 , p , nullptr ) ;
		GDEF_IGNORE_VARIABLE( rc ) ;
	}

	// standard error or log file -- note that stderr is not accessible if a gui
	// build -- stderr will be text mode whereas a log file will be binary
	//
	if( fd > 2 ) message[n++] = '\r' ;
	message[n++] = '\n' ;
	G::File::write( fd , message , n ) ;
}

void G::LogOutput::osinit()
{
	if( m_config.m_use_syslog )
	{
		std::string this_exe = LogOutputWindowsImp::thisExe() ;
		if( !this_exe.empty() )
		{
			std::string this_name = LogOutputWindowsImp::basename( this_exe ) ;
			G::LogOutput::register_( this_exe ) ;
			m_handle = RegisterEventSourceA( nullptr , this_name.c_str() ) ;
			if( m_handle == HNULL && !m_config.m_allow_bad_syslog )
				throw EventLogError() ;
		}
	}
}

void G::LogOutput::register_( const std::string & exe_path )
{
	// this method will normally fail because of access rights so it
	// should also be run as part of the install process

	std::string reg_path =
		"SYSTEM\\CurrentControlSet\\services\\eventlog\\Application\\" +
		LogOutputWindowsImp::basename(exe_path) ;

	HKEY key = 0 ;
	int sam = KEY_WRITE ;
	LONG e = RegCreateKeyExA( HKEY_LOCAL_MACHINE , reg_path.c_str() , 0 , NULL , 0 , sam , NULL , &key , NULL ) ;
	if( e == ERROR_SUCCESS && key != 0 )
	{
		DWORD one = 1 ;
		DWORD types = EVENTLOG_INFORMATION_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_ERROR_TYPE ;
		RegSetValueExA( key , "EventMessageFile" , 0 , REG_SZ ,
			reinterpret_cast<const BYTE*>(exe_path.c_str()) ,
			static_cast<DWORD>(exe_path.length())+1U ) ;
		RegSetValueExA( key , "CategoryCount" , 0 , REG_DWORD ,
			reinterpret_cast<const BYTE*>(&one) , sizeof(one) ) ;
		RegSetValueExA( key , "CategoryMessageFile" , 0 , REG_SZ ,
			reinterpret_cast<const BYTE*>(exe_path.c_str()) ,
			static_cast<DWORD>(exe_path.length())+1U ) ;
		RegSetValueExA( key , "TypesSupported" , 0 , REG_DWORD ,
			reinterpret_cast<const BYTE*>(&types) , sizeof(types) ) ;
	}
	if( key != 0 )
		RegCloseKey( key ) ;
}

void G::LogOutput::oscleanup() const noexcept
{
	if( m_handle != HNULL )
		DeregisterEventSource( m_handle ) ;
}

