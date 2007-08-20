//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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

// ==

G::DirectoryList::DirectoryList() :
	m_first(true) ,
	m_index(0U)
{
}

void G::DirectoryList::init( const G::Path & dir , const std::string & wildcard )
{
	Directory directory( dir ) ;
	DirectoryIterator iter( directory , wildcard ) ;
	while( iter.more() && !iter.error() )
	{
		m_is_dir.push_back( iter.isDir() ) ;
		m_path.push_back( iter.filePath() ) ;
		m_name.push_back( iter.fileName() ) ;
	}
}

bool G::DirectoryList::more()
{
	if( m_first )
	{
		m_first = false ;
		return ! m_is_dir.empty() ;
	}
	else
	{
		m_index++ ;
		return m_index < m_is_dir.size() ;
	}
}

bool G::DirectoryList::isDir() const
{
	return !! m_is_dir.at(m_index) ;
}

G::Path G::DirectoryList::filePath() const
{
	return m_path.at(m_index) ;
}

G::Path G::DirectoryList::fileName() const
{
	return m_name.at(m_index) ;
}

/// \file gdirectory.cpp
