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
/// \file gdirectory_win32.cpp
///

#include "gdef.h"
#include "gnowide.h"
#include "gdirectory.h"
#include "gfile.h"
#include "gconvert.h"
#include "glog.h"
#include "gassert.h"
#include <iomanip>
#include <cerrno>
#include <fcntl.h>
#include <io.h>
#include <share.h>

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES 0xFFFFFFFF
#endif

namespace G
{
	class DirectoryIteratorImp ;
}

class G::DirectoryIteratorImp
{
public:
	explicit DirectoryIteratorImp( const Directory & dir ) ;
	~DirectoryIteratorImp() ;
	bool isDir() const ;
	bool more() ;
	bool error() const ;
	std::string sizeString() const ;
	Path filePath() const ;
	std::string fileName() const ;

public:
	DirectoryIteratorImp( const DirectoryIteratorImp & ) = delete ;
	DirectoryIteratorImp( DirectoryIteratorImp && ) = delete ;
	DirectoryIteratorImp & operator=( const DirectoryIteratorImp & ) = delete ;
	DirectoryIteratorImp & operator=( DirectoryIteratorImp && ) = delete ;

private:
	static bool oneDot( const wchar_t * p ) noexcept { return p[0] == L'.' && p[1] == 0 ; }
	static bool twoDots( const wchar_t * p ) noexcept { return p[0] == L'.' && p[1] == L'.' && p[2] == 0 ; }
	static bool oneDot( const char * p ) noexcept { return p[0] == '.' && p[1] == 0 ; }
	static bool twoDots( const char * p ) noexcept { return p[0] == '.' && p[1] == '.' && p[2] == 0 ; }

private:
	nowide::FIND_DATA_type m_context ;
	HANDLE m_handle ;
	Directory m_dir ;
	bool m_error ;
	bool m_first ;
} ;

// ==

int G::Directory::usable( bool /*for_creation*/ ) const
{
	DWORD attributes = nowide::getFileAttributes( m_path ) ;
	if( attributes == INVALID_FILE_ATTRIBUTES )
	{
		DWORD e = GetLastError() ;
		if( e == ERROR_ACCESS_DENIED || e == ERROR_NETWORK_ACCESS_DENIED )
			return EACCES ;
		return ENOENT ;
	}
	return ( attributes & FILE_ATTRIBUTE_DIRECTORY ) ? 0 : ENOTDIR ;
}

bool G::Directory::writeable( const std::string & filename ) const
{
	Path path( m_path , filename.empty() ? tmp() : filename ) ;
	return File::probe( path ) ;
}

// ==

G::DirectoryIterator::DirectoryIterator( const Directory & dir ) :
	m_imp(std::make_unique<DirectoryIteratorImp>(dir))
{
}

bool G::DirectoryIterator::error() const
{
	return m_imp->error() ;
}

bool G::DirectoryIterator::more()
{
	return m_imp->more() ;
}

G::Path G::DirectoryIterator::filePath() const
{
	return m_imp->filePath() ;
}

std::string G::DirectoryIterator::fileName() const
{
	return m_imp->fileName() ;
}

bool G::DirectoryIterator::isDir() const
{
	return m_imp->isDir() ;
}

bool G::DirectoryIterator::isLink() const
{
	return false ;
}

std::string G::DirectoryIterator::sizeString() const
{
	return m_imp->sizeString() ;
}

G::DirectoryIterator::~DirectoryIterator()
= default ;

// ===

G::DirectoryIteratorImp::DirectoryIteratorImp( const Directory & dir ) :
	m_dir(dir) ,
	m_error(false) ,
	m_first(true)
{
	m_handle = nowide::findFirstFile( dir.path()/"*" , &m_context ) ;
	if( m_handle == INVALID_HANDLE_VALUE )
	{
		DWORD err = GetLastError() ;
		if( err != ERROR_FILE_NOT_FOUND )
			m_error = true ;
	}
}

bool G::DirectoryIteratorImp::error() const
{
	return m_error ;
}

bool G::DirectoryIteratorImp::more()
{
	if( m_handle == INVALID_HANDLE_VALUE )
		return false ;

	if( m_first )
	{
		m_first = false ;
		if( !oneDot(m_context.cFileName) && !twoDots(m_context.cFileName) )
			return true ;
	}

	for(;;)
	{
		bool rc = nowide::findNextFile( m_handle , &m_context ) != 0 ;
		if( !rc )
		{
			DWORD err = GetLastError() ;
			if( err != ERROR_NO_MORE_FILES )
				m_error = true ;

			FindClose( m_handle ) ;
			m_handle = INVALID_HANDLE_VALUE ;
			return false ;
		}

		// go round again if . or ..
		if( !oneDot(m_context.cFileName) && !twoDots(m_context.cFileName) )
			break ;
	}

	return true ;
}

G::Path G::DirectoryIteratorImp::filePath() const
{
	G_ASSERT( m_handle != INVALID_HANDLE_VALUE ) ;
	return m_dir.path() / fileName() ;
}

std::string G::DirectoryIteratorImp::fileName() const
{
	G_ASSERT( m_handle != INVALID_HANDLE_VALUE ) ;
	return nowide::cFileName( m_context ) ;
}

bool G::DirectoryIteratorImp::isDir() const
{
	return !! ( m_context.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ;
}

G::DirectoryIteratorImp::~DirectoryIteratorImp()
{
	if( m_handle != INVALID_HANDLE_VALUE )
		FindClose( m_handle ) ;
}

std::string G::DirectoryIteratorImp::sizeString() const
{
	const DWORD & hi = m_context.nFileSizeHigh ;
	const DWORD & lo = m_context.nFileSizeLow ;

	__int64 n = hi ;
	n <<= 32 ;
	n |= lo ;
	if( n == 0 )
		return "0" ;
	std::string s ;
	while( n != 0 )
	{
		int i = static_cast<int>( n % 10 ) ;
		char c = static_cast<char>( '0' + i ) ;
		s.insert( 0U , 1U , c ) ;
		n /= 10 ;
	}
	return s ;
}

