//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "genvironment.h"
#include <time.h> // localtime_s
#include <stdexcept>
#include <fstream>

void G::LogOutput::cleanup()
{
	if( m_handle != 0 )
		::DeregisterEventSource( m_handle ) ;
}

void G::LogOutput::open( std::ofstream & file , const std::string & path )
{
	#if GCONFIG_HAVE_FSOPEN
		file.open( path.c_str() , std::ios_base::out | std::ios_base::app , _SH_DENYNO ) ;
	#else
		file.open( path.c_str() , std::ios_base::out | std::ios_base::app ) ;
	#endif
}

void G::LogOutput::rawOutput( std::ostream & std_err , G::Log::Severity severity , const std::string & message )
{
	// standard error
	//
	std_err << message << std::endl ;

	// debugger
	//
	static bool debugger = ! Environment::get( "GLOGOUTPUT_DEBUGGER" , std::string() ).empty() ;
	if( debugger )
	{
		::OutputDebugStringA( message.c_str() ) ;
		::OutputDebugStringA( "\r\n" ) ;
	}

	// event log
	//
	if( m_syslog && severity != Log::Severity::s_Debug && m_handle != 0 )
	{
		// (assume suitable string resources of 1001..1003)

		DWORD id = 0x400003E9L ;
		WORD type = EVENTLOG_INFORMATION_TYPE ;
		if( severity == Log::Severity::s_Warning )
		{
			id = 0x800003EAL ;
			type = EVENTLOG_WARNING_TYPE ;
		}
		else if( severity == Log::Severity::s_Error || severity == Log::Severity::s_Assertion )
		{
			id = 0xC00003EBL ;
			type = EVENTLOG_ERROR_TYPE ;
		}

		const char * p[] = { message.c_str() , NULL } ;
		BOOL rc = ::ReportEventA( m_handle, type, 0, id, NULL, 1, 0, p, NULL ) ; G_IGNORE_VARIABLE(BOOL,rc) ;
	}
}

namespace
{
	std::string thisExe()
	{
		// same code is in G::Process:exe()...
		std::vector<char> buffer ;
		size_t sizes[] = { 80U , 1024U , 32768U , 0U } ; // documented limit of 32k
		for( size_t * size_p = sizes ; *size_p ; ++size_p )
		{
			buffer.resize( *size_p+1U , '\0' ) ;
			DWORD size = static_cast<DWORD>( buffer.size() ) ;
			HINSTANCE hinstance = NULL ;
			DWORD rc = ::GetModuleFileNameA( hinstance , &buffer[0] , size ) ;
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
}

void G::LogOutput::init()
{
	if( m_syslog )
	{
		std::string this_exe = thisExe() ;
		if( !this_exe.empty() )
		{
			std::string this_name = basename( this_exe ) ;
			G::LogOutput::register_( this_exe ) ;
			m_handle = ::RegisterEventSourceA( NULL , this_name.c_str() ) ;
			if( m_handle == NULL && !m_std_err.good() ) // complain if no other mechanism
				throw std::runtime_error( "cannot access the system event log" ) ;
		}
	}
}

void G::LogOutput::register_( const std::string & exe_path )
{
	std::string reg_path =
		"SYSTEM\\CurrentControlSet\\services\\eventlog\\Application\\" +
		basename(exe_path) ;

	HKEY key = 0 ;
	LONG e = ::RegCreateKeyA( HKEY_LOCAL_MACHINE , reg_path.c_str() , &key ) ;
	if( e == ERROR_SUCCESS && key != 0 )
	{
		DWORD one = 1 ;
		DWORD types = EVENTLOG_INFORMATION_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_ERROR_TYPE ;
		! ::RegSetValueExA( key , "EventMessageFile" , 0 , REG_SZ ,
			reinterpret_cast<const BYTE*>(exe_path.c_str()) ,
			static_cast<DWORD>(exe_path.length())+1U ) &&
		! ::RegSetValueExA( key , "CategoryCount" , 0 , REG_DWORD ,
			reinterpret_cast<const BYTE*>(&one) , sizeof(one) ) &&
		! ::RegSetValueExA( key , "CategoryMessageFile" , 0 , REG_SZ ,
			reinterpret_cast<const BYTE*>(exe_path.c_str()) ,
			static_cast<DWORD>(exe_path.length())+1U ) &&
		! ::RegSetValueExA( key , "TypesSupported" , 0 , REG_DWORD ,
			reinterpret_cast<const BYTE*>(&types) , sizeof(types) ) ;
	}
	if( key != 0 )
		::RegCloseKey( key ) ;
}

void G::LogOutput::getLocalTime( time_t epoch_time , struct std::tm * broken_down_time_p )
{
	errno_t rc = localtime_s( broken_down_time_p , &epoch_time ) ;
	G_IGNORE_VARIABLE(errno_t,rc) ; // ignore errors - it's just for logging
}
/// \file glogoutput_win32.cpp
