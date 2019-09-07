//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gdirectory_win32.cpp
//

#include "gdef.h"
#include "gdirectory.h"
#include "gfile.h"
#include "glog.h"
#include "gassert.h"
#include <iomanip>
#include <fcntl.h>
#include <io.h>
#include <share.h>

namespace G
{
	class DirectoryIteratorImp ;
}

bool G::Directory::valid( bool for_creation ) const
{
	DWORD attributes = ::GetFileAttributesA( m_path.str().c_str() ) ;
	if( attributes == 0xFFFFFFFF )
	{
		DWORD e = ::GetLastError() ; G_IGNORE_VARIABLE(DWORD,e) ;
		return false ;
	}
	return ( attributes & FILE_ATTRIBUTE_DIRECTORY ) != 0 ;
}

bool G::Directory::writeable( std::string filename ) const
{
	Path path( m_path , filename.empty() ? tmp() : filename ) ;
	int fd = -1 ;
	errno_t e = _sopen_s( &fd , path.str().c_str() , _O_WRONLY | _O_CREAT | _O_EXCL | _O_TEMPORARY , _SH_DENYNO , _S_IWRITE ) ;
	return e == 0 && fd != -1 && 0 == _close( fd ) ; // close and delete
}

// ===

/// \class G::DirectoryIteratorImp
/// A pimple-pattern implementation class for DirectoryIterator.
///
class G::DirectoryIteratorImp
{
public:
	explicit DirectoryIteratorImp( const Directory &dir ) ;
	~DirectoryIteratorImp() ;
	bool isDir() const ;
	bool more() ;
	bool error() const ;
	std::string sizeString() const ;
	Path filePath() const ;
	std::string fileName() const ;

private:
	DirectoryIteratorImp( const DirectoryIteratorImp & ) g__eq_delete ;
	void operator=( const DirectoryIteratorImp & ) g__eq_delete ;

private:
	WIN32_FIND_DATAA m_context ;
	HANDLE m_handle ;
	Directory m_dir ;
	bool m_error ;
	bool m_first ;
} ;

// ===

G::DirectoryIterator::DirectoryIterator( const Directory & dir ) :
	m_imp( new DirectoryIteratorImp(dir) )
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

std::string G::DirectoryIterator::sizeString() const
{
	return m_imp->sizeString() ;
}

G::DirectoryIterator::~DirectoryIterator()
{
}

// ===

G::DirectoryIteratorImp::DirectoryIteratorImp( const Directory & dir ) :
	m_dir(dir) ,
	m_error(false) ,
	m_first(true)
{
	m_handle = ::FindFirstFileA( (dir.path()+"*").str().c_str() , &m_context ) ;
	if( m_handle == INVALID_HANDLE_VALUE )
	{
		DWORD err = ::GetLastError() ;
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
		if( std::string(m_context.cFileName) != "." && std::string(m_context.cFileName) != ".." )
			return true ;
	}

	for(;;)
	{
		bool rc = ::FindNextFileA( m_handle , &m_context ) != 0 ;
		if( !rc )
		{
			DWORD err = ::GetLastError() ;
			if( err != ERROR_NO_MORE_FILES )
				m_error = true ;

			::FindClose( m_handle ) ;
			m_handle = INVALID_HANDLE_VALUE ;
			return false ;
		}

		// go round again if . or ..
		if( std::string(m_context.cFileName) != "." && std::string(m_context.cFileName) != ".." )
			break ;
	}

	return true ;
}

G::Path G::DirectoryIteratorImp::filePath() const
{
	G_ASSERT( m_handle != INVALID_HANDLE_VALUE ) ;
	return m_dir.path() + m_context.cFileName ;
}

std::string G::DirectoryIteratorImp::fileName() const
{
	G_ASSERT( m_handle != INVALID_HANDLE_VALUE ) ;
	return m_context.cFileName ;
}

bool G::DirectoryIteratorImp::isDir() const
{
	return !! ( m_context.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ;
}

G::DirectoryIteratorImp::~DirectoryIteratorImp()
{
	if( m_handle != INVALID_HANDLE_VALUE )
		::FindClose( m_handle ) ;
}

std::string G::DirectoryIteratorImp::sizeString() const
{
	const DWORD & hi = m_context.nFileSizeHigh ;
	const DWORD & lo = m_context.nFileSizeLow ;

	return File::sizeString( hi , lo ) ;
}

