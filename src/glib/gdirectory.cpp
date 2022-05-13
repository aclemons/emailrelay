//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gdirectory.cpp
///

#include "gdef.h"
#include "gdirectory.h"
#include "gfile.h"
#include "gpath.h"
#include "gprocess.h"
#include "gstr.h"
#include <algorithm> // std::sort
#include <functional>
#include <iterator> // std::distance

G::Directory::Directory() :
	m_path(".")
{
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

G::Path G::Directory::path() const
{
	return m_path ;
}

std::string G::Directory::tmp()
{
	std::ostringstream ss ;
	static int sequence = 1 ;
	ss << "." << SystemTime::now() << "." << sequence++ << "." << Process::Id() << ".tmp" ;
	return ss.str() ;
}

bool G::Directory::valid( bool for_creation ) const
{
	return 0 == usable( for_creation ) ;
}

// ==

G::DirectoryList::DirectoryList()
= default;

void G::DirectoryList::readAll( const G::Path & dir , std::vector<G::DirectoryList::Item> & out , bool sorted )
{
	DirectoryList list ;
	list.readAll( dir ) ;
	if( sorted ) list.sort() ;
	list.m_list.swap( out ) ;
}

void G::DirectoryList::readAll( const G::Path & dir )
{
	readType( dir , std::string() ) ;
}

void G::DirectoryList::readType( const G::Path & dir , const std::string & suffix , unsigned int limit )
{
	Directory directory( dir ) ;
	DirectoryIterator iter( directory ) ;
	for( unsigned int i = 0U ; iter.more() && !iter.error() ; ++i )
	{
		// (we do our own filename matching here to avoid glob())
		if( suffix.empty() || Str::tailMatch(iter.fileName(),suffix) )
		{
			if( limit == 0U || m_list.size() < limit )
			{
				Item item ;
				item.m_is_dir = iter.isDir() ;
				item.m_is_link = iter.isLink() ;
				item.m_path = iter.filePath() ;
				item.m_name = iter.fileName() ;
				m_list.push_back( item ) ;
			}
			if( m_list.size() == limit )
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
		more = ! m_list.empty() ;
	}
	else
	{
		m_index++ ;
		more = m_index < m_list.size() ;
	}
	return more ;
}

bool G::DirectoryList::isLink() const
{
	return m_list.at(m_index).m_is_link ;
}

bool G::DirectoryList::isDir() const
{
	return m_list.at(m_index).m_is_dir ;
}

G::Path G::DirectoryList::filePath() const
{
	return m_list.at(m_index).m_path ;
}

std::string G::DirectoryList::fileName() const
{
	return m_list.at(m_index).m_name ;
}

bool G::DirectoryList::compare( const Item & a , const Item & b )
{
	return a.m_name.compare( b.m_name ) < 0 ;
}

void G::DirectoryList::sort()
{
	std::sort( m_list.begin() , m_list.end() , compare ) ;
}

