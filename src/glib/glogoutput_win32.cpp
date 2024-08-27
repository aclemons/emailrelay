//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gnowide.h"
#include "glogoutput.h"
#include "gprocess.h"
#include "gconvert.h"
#include "genvironment.h"
#include "gfile.h"
#include <stdexcept>
#include <fstream>

namespace G
{
	namespace LogOutputWindowsImp
	{
		bool oldWindows() noexcept
		{
			return !IsWindowsVistaOrGreater() ;
		}
	}
}

void G::LogOutput::osoutput( int fd , Severity severity , char * message , std::size_t n )
{
	// event log
	//
	if( m_config.m_use_syslog &&
		severity != Severity::Debug &&
		severity != Severity::InfoVerbose &&
		m_handle != HNULL )
	{
		DWORD id = 0x400003E9L ; // 1001
		WORD type = EVENTLOG_INFORMATION_TYPE ;
		if( severity == Severity::Warning )
		{
			id = 0x800003EAL ; // 1002
			type = EVENTLOG_WARNING_TYPE ;
		}
		else if( severity == Severity::Error || severity == Severity::Assertion )
		{
			id = 0xC00003EBL ; // 1003
			type = EVENTLOG_ERROR_TYPE ;
		}

		// very old windowses do not seem to recognise "!S!" format specifiers so
		// as a workround (fwiw) use additional entries in messages.mc (1011, etc)
		if( LogOutputWindowsImp::oldWindows() )
			id += 10 ;

		message[n] = '\0' ;
		nowide::reportEvent( m_handle , id , type , message ) ;
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
		Path this_exe = G::Process::exe() ;
		if( !this_exe.empty() )
		{
			G::LogOutput::register_( this_exe ) ;

			std::string this_name = this_exe.withoutExtension().basename() ;
			m_handle = nowide::registerEventSource( this_name ) ;
			if( m_handle == HNULL && !m_config.m_allow_bad_syslog )
				throw EventLogError() ;
		}
	}
}

void G::LogOutput::register_( const Path & exe_path )
{
	// this method will normally fail because of access rights so it
	// should also be run as part of the install process

	std::string reg_path = std::string("SYSTEM/CurrentControlSet/services/eventlog/Application/")
		.append( exe_path.withoutExtension().basename() ) ;

	HKEY key = 0 ;
	LONG e = nowide::regCreateKey( reg_path , &key ) ;
	if( e == ERROR_SUCCESS && key != 0 )
	{
		DWORD types = EVENTLOG_INFORMATION_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_ERROR_TYPE ;
		nowide::regSetValue( key , "EventMessageFile" , exe_path.str() ) ;
		nowide::regSetValue( key , "CategoryCount" , DWORD(1) ) ;
		nowide::regSetValue( key , "CategoryMessageFile" , exe_path.str() ) ;
		nowide::regSetValue( key , "TypesSupported" , types ) ;
	}
	if( key != 0 )
		RegCloseKey( key ) ;
}

void G::LogOutput::oscleanup() const noexcept
{
	if( m_handle != HNULL )
		DeregisterEventSource( m_handle ) ;
}

