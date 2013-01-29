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
// gdirectory.cpp
//

#include "gdef.h"
#include "gdirectory.h"
#include "gfs.h"
#include "gstr.h"
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

void G::DirectoryList::readAll( const G::Path & dir )
{
	readType( dir , std::string() ) ;
}

void G::DirectoryList::readType( const G::Path & dir , const std::string & suffix , unsigned int limit )
{
	// we do our own filename matching here so as to reduce
	// our dependency on the glob()bing DirectoryIterator

	Directory directory( dir ) ;
	DirectoryIterator iter( directory ) ;
	for( unsigned int i = 0U ; iter.more() && !iter.error() ; ++i )
	{
		if( suffix.empty() || Str::tailMatch(iter.fileName().str(),suffix) )
		{
			if( limit == 0U || m_path.size() < limit )
			{
				m_is_dir.push_back( iter.isDir() ) ;
				m_path.push_back( iter.filePath() ) ;
				m_name.push_back( iter.fileName() ) ;
			}
			if( m_path.size() == limit )
				break ;
		}
	}
}

bool G::DirectoryList::more()
{
	bool more = false ;
	if( m_first )
	{
		m_first = false ;
		more = ! m_is_dir.empty() ;
	}
	else
	{
		m_index++ ;
		more = m_index < m_is_dir.size() ;
	}
	return more ;
}

bool G::DirectoryList::isDir() const
{
	return !! m_is_dir[m_index] ;
}

G::Path G::DirectoryList::filePath() const
{
	return m_path[m_index] ;
}

G::Path G::DirectoryList::fileName() const
{
	return m_name[m_index] ;
}

/// \file gdirectory.cpp
