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

namespace G
{
	namespace PidFileImp
	{
		Process::Id read( SignalSafe , const char * path ) ;
		bool cleanup( const Cleanup::Arg & ) noexcept ;
		bool cleanup( const Path & ) noexcept ;
	}
}

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
	static_assert( noexcept(m_path.cstr()) , "" ) ;
	static_assert( noexcept(m_path.empty()) , "" ) ;
	static_assert( noexcept(PidFileImp::cleanup(m_path)), "" ) ;
	static_assert( noexcept(Root::atExit()) , "" ) ;
	if( !m_path.empty() )
	{
		bool done = PidFileImp::cleanup( m_path ) ;
		if( !done )
		{
			Root::atExit() ;
			PidFileImp::cleanup( m_path ) ;
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

		Cleanup::add( PidFileImp::cleanup , Cleanup::arg(pid_file) ) ;
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

// --

bool G::PidFileImp::cleanup( const Path & path ) noexcept
{
	try
	{
		int fd = File::open( path , File::InOutAppend::In ) ;
		if( fd < 0 )
			return false ;

		constexpr std::size_t buffer_size = 11U ;
		std::array<char,buffer_size> buffer {} ;
		buffer[0U] = '\0' ;

		ssize_t rc = File::read( fd , buffer.data() , buffer_size-1U ) ;
		File::close( fd ) ;
		if( rc <= 0 )
			return false ;

		Process::Id file_pid( buffer.data() , buffer.data()+static_cast<std::size_t>(rc) ) ;

		Process::Id this_pid ;
		if( this_pid != file_pid )
			return false ; // not our pid file -- false => try again

		return File::remove( path , std::nothrow ) ;
	}
	catch(...)
	{
		return false ;
	}
}

bool G::PidFileImp::cleanup( const Cleanup::Arg & arg ) noexcept
{
	try
	{
		return cleanup( Path(arg.str()) ) ;
	}
	catch(...)
	{
		return false ;
	}
}

