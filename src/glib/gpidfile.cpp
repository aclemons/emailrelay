//
// Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gpidfile.cpp
//

#include "gdef.h"
#include "gpidfile.h"
#include "gprocess.h"
#include "gcleanup.h"
#include "gfile.h"
#include "gdebug.h"
#include <cstdlib> // std::malloc()
#include <cstring> // std::strdup()
#include <fstream>
#include <fcntl.h>

namespace
{
	// strdup() not in std:: ?
	char * strdup_( const char * p )
	{
		p = p ? p : "" ;
		const size_t n = std::strlen(p) ;
		char * buffer = static_cast<char*>( std::malloc(n+1U) ) ;
		if( buffer != NULL )
		{
			std::strncpy( buffer , p , n ) ;
			buffer[n] = '\0' ;
		}
		return buffer ;
	}
}

//static
void G::PidFile::create( const Path & pid_file )
{
	if( pid_file != Path() )
	{
		G_DEBUG( "G::PidFile::create: \"" << pid_file << "\"" ) ;
		Process::Umask readable(Process::Umask::Readable) ;
		std::ofstream file( pid_file.str().c_str() ) ;
		Process::Id pid ;
		file << pid.str() << std::endl ;
		if( !file.good() )
			throw Error(std::string("cannot create file: ")+pid_file.str()) ;
		Cleanup::add( cleanup , strdup_(pid_file.str().c_str()) ) ; // (leaks)
	}
}

//static
bool G::PidFile::mine( const char * path )
{
	// reentrant implementation...
	Process::Id this_pid ;
	Process::Id file_pid( path ) ;
	return this_pid == file_pid ;
}

//static
void G::PidFile::cleanup( const char * path )
{
	// reentrant implementation...
	try
	{
		if( path && *path && mine(path) )
			std::remove( path ) ;
	}
	catch(...)
	{
	}
}

// ===

G::PidFile::PidFile()
{
}

G::PidFile::~PidFile()
{
	if( valid() )
		cleanup( m_path.pathCstr() ) ;
}

G::PidFile::PidFile( const Path & path ) :
	m_path(path)
{
}

void G::PidFile::init( const Path & path )
{
	m_path = path ;
}

void G::PidFile::check()
{
	if( valid() && ! m_path.isAbsolute() )
		throw Error(std::string("must be an absolute path: ")+m_path.str()) ;
}

void G::PidFile::commit()
{
	if( valid() )
		create( m_path ) ;
}

G::Path G::PidFile::path() const
{
	return m_path ;
}

bool G::PidFile::valid() const
{
	return m_path != Path() ;
}

