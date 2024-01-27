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
/// \file gpidfile.cpp
///

#include "gdef.h"
#include "gpidfile.h"
#include "gprocess.h"
#include "groot.h"
#include "gstr.h"
#include "gcleanup.h"
#include "gfile.h"
#include "glog.h"
#include <fstream>
#include <string>
#include <array>

#ifndef G_LIB_SMALL
G::PidFile::PidFile()
= default;
#endif

G::PidFile::PidFile( const Path & path ) :
	m_path((!path.empty()&&path.isRelative())?Path::join(Process::cwd(),path):path)
{
}

G::PidFile::~PidFile()
{
	if( !m_path.empty() )
	{
		bool done = cleanup( SignalSafe() , m_path.cstr() ) ;
		if( !done )
		{
			Root::atExit() ;
			cleanup( SignalSafe() , m_path.cstr() ) ;
		}
	}
}

void G::PidFile::mkdir()
{
	if( !m_path.empty() )
		File::mkdir( m_path.dirname() , std::nothrow ) ;
}

void G::PidFile::create( const Path & pid_file )
{
	if( !pid_file.empty() )
	{
		// (the effective user-id and umask is set by the caller)
		std::ofstream file ;
		File::open( file , pid_file , File::Text() ) ;
		int e = G::Process::errno_() ;
		if( !file.good() )
			throw Error( "cannot create file" , pid_file.str() , G::Process::strerror(e) ) ;
		Process::Id pid ;
		file << pid.str() << std::endl ;
		file.close() ;
		if( file.fail() )
			throw Error( "cannot write file" , pid_file.str() ) ;

		// (leak only if necessary)
		static constexpr std::size_t buffer_size = 60U ; // eg. "/var/run/whatever/whatever.pid"
		static std::array<char,buffer_size> buffer {} ;
		const char * cleanup_arg = buffer.data() ;
		if( buffer[0] == '\0' && pid_file.size() < buffer.size() )
			G::Str::strncpy_s( buffer.data() , buffer.size() , pid_file.cstr() , pid_file.size() ) ;
		else
			cleanup_arg = Cleanup::strdup( pid_file.str() ) ;

		Cleanup::add( cleanup , cleanup_arg ) ;
	}
}

G::Process::Id G::PidFile::read( SignalSafe , const char * path ) noexcept
{
	int fd = File::open( path , File::InOutAppend::In ) ;
	if( fd < 0 )
		return Process::Id::invalid() ;

	constexpr std::size_t buffer_size = 11U ;
	std::array<char,buffer_size> buffer {} ;
	buffer[0U] = '\0' ;

	ssize_t rc = File::read( fd , buffer.data() , buffer_size-1U ) ;
	File::close( fd ) ;
	if( rc <= 0 )
		return Process::Id::invalid() ;

	return Process::Id( buffer.data() , buffer.data()+static_cast<std::size_t>(rc) ) ;
}

bool G::PidFile::cleanup( SignalSafe safe , const char * path ) noexcept
{
	try
	{
		if( path == nullptr || *path == '\0' )
			return true ; // nothing to do

		Process::Id this_pid ;
		Process::Id file_pid = read( safe , path ) ;
		if( this_pid != file_pid )
			return false ; // try again in case we didnt have read permission

		return 0 == std::remove( path ) ;
	}
	catch(...)
	{
		return false ;
	}
}

void G::PidFile::commit()
{
	if( !m_path.empty() )
	{
		create( m_path ) ;
		m_committed = true ;
	}
}

bool G::PidFile::committed() const
{
	return m_committed ;
}

G::Path G::PidFile::path() const
{
	return m_path ;
}

