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
//	gpath.cpp
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
	m_dot(NULL)
{
	validate( "d-ctor" ) ;
}

G::Path::~Path()
{
}

G::Path::Path( const std::string & path )
{
	set( path ) ;
	validate( "c-ctor" ) ;
}

G::Path::Path( const char * path )
{
	G_ASSERT( path != NULL ) ;
	set( std::string(path) ) ;
	validate( "ctor(cstr)" ) ;
}

G::Path::Path( const Path & other )
{
	set( other.str() ) ;
	validate( "ctor(Path)" ) ;
}

void G::Path::set( const std::string & path )
{
	clear() ;
	m_str = path ;
	normalise() ;
}

void G::Path::clear()
{
	m_extension = "" ;
	m_str = "" ;
	m_dot = NULL ;
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
	bool has_leading_double_slash = 
		FileSystem::leadingDoubleSlash() &&
		m_str.find( ss ) == 0U ;

	// normalise double slashes
	Str::replaceAll( m_str , ss , s ) ;

	// normalise funny characters
	Str::replaceAll( m_str , "\t" , "" ) ;
	Str::replaceAll( m_str , "\n" , "" ) ;
	Str::replaceAll( m_str , "\r" , "" ) ;

	// normalise case
	if( !FileSystem::caseSensitive() )
		Str::toLower(m_str) ;

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
	const char *slash = std::strrchr( m_str.c_str() , FileSystem::slash() ) ;
	m_dot = std::strrchr( m_str.c_str() , '.' ) ;
	if( m_dot != NULL && slash != NULL && m_dot < slash ) // ie. if "foo.bar/bletch"
		m_dot = NULL ;

	// make a copy of the extension
	if( m_dot != NULL )
	{
		m_extension = std::string(m_dot+1U) ;
	}
}

bool G::Path::validPath() const
{
	const char *slash = std::strrchr( m_str.c_str() , FileSystem::slash() ) ;
	const char *dot = std::strrchr( m_str.c_str() , '.' ) ;
	if( dot && slash && dot < slash )
		dot = NULL ;

	return m_dot == dot ;
}

const char *G::Path::pathCstr() const
{
	validate("pathCstr") ;
	return m_str.c_str() ;
}

std::string G::Path::str() const
{
	validate("str") ;
	return m_str ;
}

void G::Path::streamOut( std::ostream & stream ) const
{
	stream << str() ;
}

void G::Path::validate( const char * /* where */ ) const
{
	//if( !validPath() ) G_ERROR( "G::Path::validate: " << where << ": \"" << m_str << "\"" ) ;
	G_ASSERT( validPath() ) ;
}

bool G::Path::simple() const
{
	return dirname().str().empty() ;
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

void G::Path::setExtension( const std::string & extension )
{
	bool dotted = extension.length() && extension.at(0U) == '.' ;

	Path copy = *this ;
	copy.removeExtension() ;

	std::string s( copy.str() ) ;
	s.append( 1U , '.' ) ;
	s.append( dotted ? extension.substr(1U) : extension ) ;

	set( s ) ;
	validate( "setExtension" ) ;
}

std::string G::Path::basename() const
{
	// for consistency compare m_str with dirname() and return the
	// difference, excluding any leading slash

	std::string result( m_str ) ;
	std::string head( dirname().str() ) ;
	if( head.length() == 0 )
	{
	}
	else
	{
		result.erase( 0U , head.length() ) ;
		if( result.at(0) == FileSystem::slash() )
			result.erase(0U,1U) ;
	}

	return result ;
}

G::Path G::Path::dirname() const
{
	validate("dirname") ;

	std::string result ;

	if( FileSystem::usesDriveLetters() && 
		m_str.size() >= 2 && m_str.at(1) == ':' )
	{
		if( noSlash() )
		{
			if( m_str.size() > 2 )
				result = driveString() ;
			else
				result = "" ;
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
					result = noTail() ;
					if( result.length() == 2 )
						result.append( slashString().c_str() ) ;
				}
			}
			else
			{
				result = noTail() ;
			}
		}
	}
	else if( FileSystem::leadingDoubleSlash() &&
		m_str.size() >= 2U &&
		m_str.substr(0U,2U) == doubleSlashString() )
	{
		size_t slash_count = 0U ;
		for( const char * p = m_str.c_str() ; *p ; p++ )
			if( *p == FileSystem::slash() )
				slash_count++ ;

		if( slash_count > 3U )
			result = noTail() ;
		else
			result = "" ;
	}
	else
	{
		if( noSlash() || m_str.size() == 1U )
		{
			result = "" ;
		}
		else
		{
			result = noTail() ;
			if( result.length() == 0 )
				result = slashString() ;
		}
	}

	return Path(result) ;
}

std::string G::Path::noTail() const
{
	G_ASSERT( !noSlash() ) ;
	return m_str.substr( 0 , m_str.rfind(slashString()) ) ;
}

bool G::Path::noSlash() const
{
	return m_str.find( slashString() ) == std::string::npos ;
}

size_t G::Path::slashAt() const
{
	size_t position = m_str.find( slashString() ) ;
	G_ASSERT( position != std::string::npos ) ;
	return position ;
}

std::string G::Path::slashString()
{
	return std::string ( 1U , FileSystem::slash() ) ;
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
	if( m_dot != NULL )
	{
		m_str.resize( m_str.size() - std::strlen(m_dot) ) ;
		m_dot = NULL ;
		normalise() ; // in case of dir/foo.bar.bletch
	}

	validate("removeExtension") ;
}

void G::Path::setDirectory( const std::string & dir )
{
	std::string temp( basename() ) ;
	set( dir ) ;
	pathAppend( temp ) ;
	validate("setDirectory") ;
}

void G::Path::pathAppend( const std::string & tail )
{
	// if empty or root or just a drive letter...
	if( m_str.size() == 0U || m_str == slashString() || 
		( hasDriveLetter() && m_str == driveString() ) )
	{
		; // no-op
	}
	else
	{
		m_str.append( slashString().c_str() ) ;
	}

	m_str.append( tail ) ;
	normalise() ;
	validate("pathAppend") ;
}

std::string G::Path::extension() const
{
	return m_extension ;
}

G::Strings G::Path::split( bool no_dot ) const
{
	Path path( *this ) ;
	Strings list ;
	for( unsigned int part = 0U ;; part++ )
	{
		std::string front = path.dirname().str() ;
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

G::Path &G::Path::operator=( const Path & other ) 
{ 
	if( &other != this ) 
		set( other.str() ) ;
	return *this ; 
}


