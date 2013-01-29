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
// gpath.cpp
//

#include "gdef.h"
#include "gpath.h"
#include "gfs.h"
#include "gstr.h"
#include "gdebug.h"
#include "glog.h"
#include "gassert.h"
#include <cstring>

G::Path::Path() :
	m_dot(std::string::npos)
{
}

G::Path::~Path()
{
}

G::Path::Path( const std::string & path )
{
	set( path ) ;
}

G::Path::Path( const char * path )
{
	G_ASSERT( path != NULL ) ;
	set( std::string(path) ) ;
}

G::Path::Path( const Path & path , const std::string & tail )
{
	set( path.str() ) ;
	pathAppend( tail ) ;
}

G::Path::Path( const Path & path , const std::string & tail_1 , const std::string & tail_2 )
{
	set( path.str() ) ;
	pathAppend( tail_1 ) ;
	pathAppend( tail_2 ) ;
}

G::Path::Path( const Path & other )
{
	set( other.str() ) ;
}

void G::Path::set( const std::string & path )
{
	clear() ;
	m_str = path ;
	normalise() ;
}

void G::Path::clear()
{
	m_extension.erase() ;
	m_str.erase() ;
	m_dot = std::string::npos ;
}

void G::Path::normalise()
{
	char ns[2] ; char s[2] ; char ss[3] ;
	s[0] = ss[0] = ss[1] = FileSystem::slash() ;
	ns[0] = FileSystem::nonSlash() ;
	ns[1] = s[1] = ss[2] = '\0' ;

	// normalise spaces
	if( !FileSystem::allowsSpaces() )
		Str::replaceAll( m_str , " " , "" ) ;

	// normalise alternate (non) slash characters
	Str::replaceAll( m_str , ns , s ) ;

	// save leading double-slashes
	bool has_leading_double_slash = FileSystem::leadingDoubleSlash() && m_str.find( ss ) == 0U ;

	// normalise double slashes
	Str::replaceAll( m_str , ss , s ) ;

	// normalise funny characters
	Str::removeAll( m_str , '\t' ) ;
	Str::removeAll( m_str , '\n' ) ;
	Str::removeAll( m_str , '\r' ) ;

	// remove trailing slashes where appropriate
	while( ( 
			m_str.size() > 1U && 
			m_str.at(m_str.size()-1) == FileSystem::slash() && 
			!( FileSystem::usesDriveLetters() && 
				m_str.size() == 3U && 
				m_str.at(1) == ':' ) ) )
	{
		m_str.resize( m_str.size()-1U ) ;
	}

	// restore leading double slash
	if( has_leading_double_slash )
		m_str = s + m_str ;

	// prepare a pointer to the extension
	std::string::size_type slash = m_str.rfind( FileSystem::slash() ) ;
	m_dot = m_str.rfind( '.' ) ;
	if( m_dot != std::string::npos && slash != std::string::npos && m_dot < slash ) // ie. if "foo.bar/bletch"
		m_dot = std::string::npos ;

	// make a copy of the extension
	if( m_dot != std::string::npos )
	{
		m_extension = (m_dot+1U) == m_str.length() ? std::string() : m_str.substr(m_dot+1U) ;
	}
}

std::string G::Path::str() const
{
	return m_str ;
}

bool G::Path::simple() const
{
	return dirnameImp().empty() ;
}

bool G::Path::isRelative() const
{
	return !isAbsolute() ;
}

bool G::Path::isAbsolute() const
{
	if( hasNetworkDrive() )
		return true ;

	std::string str(m_str) ;
	if( hasDriveLetter() )
		str.erase( 0U , driveString().length() ) ;

	return str.length() > 0U && str.at(0U) == FileSystem::slash() ;
}

std::string G::Path::basename() const
{
	// for consistency compare m_str with dirname() and return the
	// difference, excluding any leading slash

	std::string head( dirnameImp() ) ;
	if( head.length() == 0 )
	{
		return m_str ;
	}
	else
	{
		std::string result( m_str ) ;
		result.erase( 0U , head.length() ) ;
		if( result.at(0) == FileSystem::slash() )
			result.erase( 0U , 1U ) ;
		return result ;
	}
}

G::Path G::Path::dirname() const
{
	return Path( dirnameImp() ) ;
}

std::string G::Path::dirnameImp() const
{
	std::string result ;
	if( FileSystem::usesDriveLetters() && m_str.size() >= 2 && m_str.at(1) == ':' )
	{
		if( hasNoSlash() )
		{
			result = m_str.size() > 2 ? driveString() : std::string() ;
		}
		else
		{
			if( m_str.rfind(FileSystem::slash()) == 2U )
			{
				if( m_str.size() == 3U )
				{
					result = "" ;
				}
				else
				{
					result = withoutTail() ;
					if( result.length() == 2 )
						result.append( slashString() ) ;
				}
			}
			else
			{
				result = withoutTail() ;
			}
		}
	}
	else if( FileSystem::leadingDoubleSlash() && m_str.size() >= 2U && m_str.substr(0U,2U) == doubleSlashString() )
	{
		size_t slash_count = 0U ;
		for( std::string::const_iterator p = m_str.begin() ; p != m_str.end() ; ++p )
			if( *p == FileSystem::slash() )
				slash_count++ ;

		result = slash_count > 3U ? withoutTail() : std::string() ;
	}
	else if( hasNoSlash() || m_str.size() == 1U )
	{
		;
	}
	else
	{
		result = withoutTail() ;
		if( result.length() == 0U )
			result = slashString() ;
	}
	return result ;
}

std::string G::Path::withoutTail() const
{
	G_ASSERT( !hasNoSlash() ) ;
	return m_str.substr( 0U , m_str.rfind(FileSystem::slash()) ) ;
}

bool G::Path::hasNoSlash() const
{
	return m_str.find(FileSystem::slash()) == std::string::npos ;
}

std::string::size_type G::Path::slashAt() const
{
	std::string::size_type position = m_str.find( FileSystem::slash() ) ;
	G_ASSERT( position != std::string::npos ) ;
	return position ;
}

std::string G::Path::slashString()
{
	return std::string( 1U , FileSystem::slash() ) ;
}

std::string G::Path::doubleSlashString()
{
	return std::string ( 2U , FileSystem::slash() ) ;
}

bool G::Path::hasDriveLetter() const
{
	return
		FileSystem::usesDriveLetters() &&
		m_str.size() >= 2U &&
		m_str.at(1U) == ':' ;
}

bool G::Path::hasNetworkDrive() const
{
	return
		FileSystem::leadingDoubleSlash() &&
		m_str.size() > 2U&&
		m_str.at(0U) == FileSystem::slash() &&
		m_str.at(1U) == FileSystem::slash() ;
}

std::string G::Path::driveString() const
{
	G_ASSERT( m_str.at(1U) == ':' ) ;
	G_ASSERT( FileSystem::usesDriveLetters() ) ;
	return std::string( 1U , m_str.at(0U) ) + std::string(":") ;
}

void G::Path::removeExtension()
{
	if( m_dot != std::string::npos )
	{
		m_str.resize( m_dot ) ;
		m_dot = std::string::npos ;
		normalise() ; // in case of dir/foo.bar.bletch
	}
}

void G::Path::pathAppend( const std::string & tail )
{
	if( !tail.empty() )
	{
		// if empty or root or just a drive letter...
		if( m_str.size() == 0U || m_str == slashString() || 
			( hasDriveLetter() && m_str == driveString() ) )
		{
			; // no-op
		}
		else
		{
			m_str.append( slashString() ) ;
		}
		m_str.append( tail ) ;
		normalise() ;
	}
}

std::string G::Path::extension() const
{
	return m_extension ;
}

G::Path G::Path::join( const G::Path & p1 , const G::Path & p2 )
{
	if( p1 == Path() )
	{
		return p2 ;
	}
	else if( p2 == Path() )
	{
		return p1 ;
	}
	else
	{
		G::Path result( p1 ) ;
		Strings list = p2.split() ;
		for( Strings::iterator p = list.begin() ; p != list.end() ; ++p )
			result.pathAppend( *p ) ;
		return result ;
	}
}

G::Strings G::Path::split( bool no_dot ) const
{
	Path path( *this ) ;
	Strings list ;
	for( unsigned int part = 0U ;; part++ )
	{
		std::string front = path.dirnameImp() ;
		std::string back = path.basename() ;

		// if a dot in the middle or end of the path...
		if( back == std::string(".") && no_dot && front.length() != 0U )
			; // ...ignore it
		else
			list.push_front( back ) ;

		if( front.length() == 0U )
			break ;

		path = Path(front) ;
	}
	return list ;
}

bool G::Path::operator==( const Path & other ) const
{
	return 
		( m_str.empty() && other.m_str.empty() ) ||
		( m_str == other.m_str ) ;
}

bool G::Path::operator!=( const Path & other ) const
{
	return m_str != other.m_str ;
}

G::Path & G::Path::operator=( const Path & other ) 
{ 
	if( &other != this ) 
		set( other.str() ) ;
	return *this ; 
}

/// \file gpath.cpp
