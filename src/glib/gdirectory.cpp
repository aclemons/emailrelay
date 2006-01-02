//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gdirectory.cpp
//

#include "gdef.h"
#include "gdirectory.h"
#include "gfs.h"
#include "glog.h"

G::Directory::Directory() :
	m_path(".")
{
}

G::Directory G::Directory::root()
{
	std::string s ;
	s.append( 1U , G::FileSystem::slash() ) ;
	return Directory( s ) ;
}
		
G::Directory::Directory( const char * path ) :
	m_path(path)
{
}

G::Directory::Directory( const std::string & path ) :
	m_path(path)
{
}

G::Directory::Directory( const Path & path ) :
	m_path(path)
{
}
		
G::Directory::~Directory()
{
}

G::Directory::Directory( const Directory &other ) :
	m_path(other.m_path)
{
}

G::Directory &G::Directory::operator=( const Directory &rhs )
{
	m_path = rhs.m_path ;
	return *this ;
}

G::Path G::Directory::path() const
{
	return m_path ;
}

