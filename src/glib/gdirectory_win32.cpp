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
// gdirectory_win32.cpp
//

#include "gdef.h"
#include "gdirectory.h"
#include "gfs.h"
#include "gfile.h"
#include "gdebug.h"
#include "glog.h"
#include <iomanip>

namespace G
{
	class DirectoryIteratorImp ;
} ;

bool G::Directory::valid( bool for_creation ) const
{
	DWORD attributes = ::GetFileAttributesA( m_path.str().c_str() ) ;
	if( attributes == 0xFFFFFFFF )
	{
		G_IGNORE_RETURN(DWORD) ::GetLastError() ;
		return false ;
	}
	return ( attributes & FILE_ATTRIBUTE_DIRECTORY ) != 0 ;
}

std::string G::Directory::tmp()
{
	return std::string() ;
}

bool G::Directory::writeable( std::string ) const
{
	return true ; // not implemented
}

// ===

/// \class G::DirectoryIteratorImp
/// A pimple-pattern implementation class for DirectoryIterator.
/// 
class G::DirectoryIteratorImp 
{
private:
	WIN32_FIND_DATAA m_context ;
	HANDLE m_handle ;
	Directory m_dir ;
	bool m_error ;
	bool m_first ;
	bool m_special ; // if wc = ".." -- FindFirstFile returns ".."'s proper name -- this class returns ".."

public:
	DirectoryIteratorImp( const Directory &dir , const std::string & wildcard ) ;
	~DirectoryIteratorImp() ;
	bool isDir() const ;
	bool more() ;
	bool error() const ;
	std::string modificationTimeString() const ;
	std::string sizeString() const ;
	Path filePath() const ;
	Path fileName() const ;

private:
	void operator=( const DirectoryIteratorImp & ) ;
	DirectoryIteratorImp( const DirectoryIteratorImp & ) ;
} ;

// ===

G::DirectoryIterator::DirectoryIterator( const Directory &dir )
{
	m_imp = new DirectoryIteratorImp( dir , std::string() ) ;
}

G::DirectoryIterator::DirectoryIterator( const Directory &dir , const std::string & wc )
{
	m_imp = new DirectoryIteratorImp( dir , wc ) ;
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

G::Path G::DirectoryIterator::fileName() const
{
	return m_imp->fileName() ;
}

bool G::DirectoryIterator::isDir() const
{
	return m_imp->isDir() ;
}

std::string G::DirectoryIterator::modificationTimeString() const
{
	return m_imp->modificationTimeString() ;
}

std::string G::DirectoryIterator::sizeString() const
{
	return m_imp->sizeString() ;
}

G::DirectoryIterator::~DirectoryIterator()
{
	delete m_imp ;
}

// ===

G::DirectoryIteratorImp::DirectoryIteratorImp( const Directory & dir , 
	const std::string & wildcard ) :
		m_dir(dir) ,
		m_error(false) ,
		m_first(true) ,
		m_special(false)
{
	if( std::string(wildcard) == ".." )
	{
		G_DEBUG( "DirectoryIteratorImp: special work-round for .." ) ;
		m_special = true ;
		return ;
	}

	Path wild_path( m_dir.path() ) ;
	wild_path.pathAppend( wildcard.empty() ? std::string("*.*") : wildcard ) ;

	G_DEBUG( "G::DirectoryIteratorImp::ctor: FindFirstFile(\"" << wild_path << "\")" ) ;

	m_handle = ::FindFirstFileA( wild_path.str().c_str() , &m_context ) ;
	if( m_handle == INVALID_HANDLE_VALUE )
	{
		DWORD err = ::GetLastError() ;
		if( err == ERROR_FILE_NOT_FOUND )
		{
			G_DEBUG( "G::DirectoryIteratorImp::ctor: none" ) ;
		}
		else
		{
			m_error = true ;
			G_DEBUG( "G::DirectoryIteratorImp::ctor: error " << err << " for " << wild_path ) ;
		}
	}
	else
	{
		G_DEBUG( "G::DirectoryIteratorImp::ctor: first \"" << m_context.cFileName << "\"" ) ;
	}
}

bool G::DirectoryIteratorImp::error() const
{
	return m_error ;
}

bool G::DirectoryIteratorImp::more()
{
	if( m_special )
	{
		bool rc = m_first ;
		m_first = false ;
		return rc ;
	}

	if( m_handle == INVALID_HANDLE_VALUE )
		return false ;

	if( m_first )
	{
		m_first = false ;
		if( std::string(m_context.cFileName) != "." && std::string(m_context.cFileName) != ".." )
			return true ;

		G_DEBUG( "G::DirectoryIteratorImp::more: ignoring " << m_context.cFileName);
	}

	for(;;)
	{
		bool rc = ::FindNextFileA( m_handle , &m_context ) != 0 ;
		if( !rc )
		{
			DWORD err = ::GetLastError() ;
			if( err == ERROR_NO_MORE_FILES )
			{
				G_DEBUG( "G::DirectoryIteratorImp::more: no more" ) ;
			}
			else
			{
				G_DEBUG( "G::DirectoryIteratorImp::more: error" ) ;
				m_error = true ;
			}
			::FindClose( m_handle ) ;
			m_handle = INVALID_HANDLE_VALUE ;
			return false ;
		}

		// go round again if . or ..
		if( std::string(m_context.cFileName) != "." &&
			std::string(m_context.cFileName) != ".." )
		{
			G_DEBUG( "G::DirectoryIteratorImp::more: " << m_context.cFileName ) ;
			break ;
		}
		else
		{
			G_DEBUG( "G::DirectoryIteratorImp::more: ignoring " << m_context.cFileName ) ;
		}
	}

	return true ;
}

G::Path G::DirectoryIteratorImp::filePath() const
{
	Path file_path( m_dir.path() ) ;
	if( m_special )
	{
		file_path.pathAppend( ".." ) ;
	}
	else
	{
		G_ASSERT( m_handle != INVALID_HANDLE_VALUE ) ;
		file_path.pathAppend( m_context.cFileName ) ;
	}
	return file_path ;
}

G::Path G::DirectoryIteratorImp::fileName() const
{
	if( m_special )
	{
		return Path("..") ;
	}
	else
	{
		G_ASSERT( m_handle != INVALID_HANDLE_VALUE ) ;
		return Path(m_context.cFileName) ;
	}
}

bool G::DirectoryIteratorImp::isDir() const
{
	return m_special || m_context.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ;
}

G::DirectoryIteratorImp::~DirectoryIteratorImp() 
{
	if( m_handle != INVALID_HANDLE_VALUE )
		::FindClose( m_handle ) ;
}

std::string G::DirectoryIteratorImp::modificationTimeString() const
{
	if( m_special )
	{
		// ??
		G_ASSERT( !"modificationTimeString() not fully implemented for .." ) ;
		return std::string() ;
	}

	const DWORD & hi = m_context.ftLastWriteTime.dwHighDateTime ;
	const DWORD & lo = m_context.ftLastWriteTime.dwLowDateTime ;

	std::ostringstream ss ;
	ss << hi << std::setw(8) << std::setfill('0') << lo ;
	return ss.str() ;
}

std::string G::DirectoryIteratorImp::sizeString() const
{
	if( m_special )
	{
		return std::string( "0" ) ;
	}

	const DWORD & hi = m_context.nFileSizeHigh ;
	const DWORD & lo = m_context.nFileSizeLow ;

	return G::File::sizeString( hi , lo ) ;
}

