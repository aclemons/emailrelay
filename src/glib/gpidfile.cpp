//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gpidfile.cpp
//

#include "gdef.h"
#include "gpidfile.h"
#include "gprocess.h"
#include "groot.h"
#include "gcleanup.h"
#include "gfile.h"
#include "gdebug.h"
#include <fstream>
#include <string>

G::PidFile::PidFile() :
	m_committed(false)
{
}

G::PidFile::~PidFile()
{
	if( valid() )
		cleanup( SignalSafe() , m_path.str().c_str() ) ;
}

G::PidFile::PidFile( const Path & path ) :
	m_path(path) ,
	m_committed(false)
{
}

void G::PidFile::init( const Path & path )
{
	m_path = path ;
}

void G::PidFile::create( const Path & pid_file )
{
	if( pid_file != Path() )
	{
		G_DEBUG( "G::PidFile::create: \"" << pid_file << "\"" ) ;

		std::ofstream file ;
		{
			//Process::Umask umask(Process::Umask::Readable) ; // let the caller do this now
			file.open( pid_file.str().c_str() , std::ios_base::out | std::ios_base::trunc ) ;
		}
		Process::Id pid ;
		file << pid.str() << std::endl ;
		file.close() ;
		if( file.fail() )
			throw Error(std::string("cannot create file: ")+pid_file.str()) ;
		Cleanup::add( cleanup , new_string_ignore_leak(pid_file.str())->c_str() ) ;
	}
}

bool G::PidFile::mine( SignalSafe safe , const char * path )
{
	// signal-safe, reentrant implementation...
	Process::Id this_pid ;
	Process::Id file_pid( safe , path ) ;
	return this_pid == file_pid ;
}

void G::PidFile::cleanup( SignalSafe safe , const char * path )
{
	// signal-safe, reentrant implementation...
	try
	{
		Identity id = Root::start( safe ) ; // claim_root
		if( path && *path && mine(safe,path) )
			std::remove( path ) ;
		Root::stop( safe , id ) ;
	}
	catch(...)
	{
	}
}

void G::PidFile::check()
{
	if( valid() && ! m_path.isAbsolute() )
		throw Error(std::string("must be an absolute path: ")+m_path.str()) ;
}

void G::PidFile::commit()
{
	if( valid() )
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

bool G::PidFile::valid() const
{
	return m_path != Path() ;
}

std::string * G::PidFile::new_string_ignore_leak( const std::string & s )
{
	return new std::string( s ) ; // (ignore leak)
}

/// \file gpidfile.cpp
