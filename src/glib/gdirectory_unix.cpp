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
// gdirectory_unix.cpp
//

#include "gdef.h"
#include "gdirectory.h"
#include "gfs.h"
#include "gfile.h"
#include "gdebug.h"
#include "glog.h"
#include <unistd.h>
#include <dirent.h>
#include <glob.h>

namespace G
{
	class DirectoryIteratorImp ;
}

bool G::Directory::valid( bool for_creation ) const
{
	bool rc = true ;
	struct stat statbuf ;
	if( ::stat( m_path.pathCstr() , &statbuf ) )
	{
		rc = false ;
	}
	else if( !(statbuf.st_mode & S_IFDIR) )
	{
		rc = false ;
	}
	else
	{
		DIR * p = ::opendir( m_path.pathCstr() ) ;
		if( p == NULL )
			rc = false ;
		else
			::closedir( p ) ;
	}

	if( rc && for_creation )
	{
		// (see also GNU/Linux ::euidaccess())
		if( 0 != ::access( m_path.pathCstr() , W_OK ) )
			rc = false ;
	}

	G_DEBUG( "G::Directory::valid: \"" << m_path.str() << "\" is " << (rc?"":"not ") << "a directory" ) ;
	return rc ;
}

// ===

// Class: G::DirectoryIteratorImp
// Description: A pimple-pattern implementation class for DirectoryIterator.
//
class G::DirectoryIteratorImp 
{
private:
	glob_t m_glob ;
	Directory m_dir ;
	bool m_first ;
	size_t m_index ;
	bool m_error ;

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

extern "C" int gdirectory_unix_on_error_( const char * , int )
{
	const int abort = 1 ;
	return abort ;
}

G::DirectoryIteratorImp::DirectoryIteratorImp( const Directory &dir , 
	const std::string & wildcard ) :
		m_dir(dir) ,
		m_first(true) ,
		m_index(0U) ,
		m_error(false)
{
	m_glob.gl_pathc = 0 ;
	m_glob.gl_pathv = NULL ;

	Path wild_path( m_dir.path() ) ;
	wild_path.pathAppend( wildcard.empty() ? std::string("*") : wildcard ) ;

	G_DEBUG( "G::DirectoryIteratorImp::ctor: glob(\"" << wild_path << "\")" ) ;

	int flags = 0 | GLOB_ERR ;
	int error  = ::glob( wild_path.pathCstr() , flags , gdirectory_unix_on_error_ , &m_glob ) ;
	if( error || m_glob.gl_pathv == NULL )
	{
		G_DEBUG( "G::DirectoryIteratorImp::ctor: glob() error: " << error ) ;
		m_error = true ;
	}
	else
	{
		G_ASSERT( m_glob.gl_pathc == 0 || m_glob.gl_pathv[0] != NULL ) ;
	}
}

bool G::DirectoryIteratorImp::error() const
{
	return m_error ;
}

bool G::DirectoryIteratorImp::more()
{
	if( m_error ) 
		return false ;

	if( ! m_first )
		m_index++ ;
	m_first = false ;

	if( m_index >= m_glob.gl_pathc )
		return false ;

	return true ;
}

G::Path G::DirectoryIteratorImp::filePath() const
{
	G_ASSERT( m_index < m_glob.gl_pathc ) ;
	G_ASSERT( m_glob.gl_pathv != NULL ) ;
	G_ASSERT( m_glob.gl_pathv[m_index] != NULL ) ;
	const char * file_path = m_glob.gl_pathv[m_index] ;
	return Path( file_path ) ;
}

G::Path G::DirectoryIteratorImp::fileName() const
{
	return Path( filePath().basename() ) ;
}

bool G::DirectoryIteratorImp::isDir() const
{
	Directory dir( filePath().str() ) ;
	return dir.valid() ;
}

G::DirectoryIteratorImp::~DirectoryIteratorImp() 
{
	if( ! m_error )
		::globfree( &m_glob ) ;
}

std::string G::DirectoryIteratorImp::modificationTimeString() const
{
	return std::string() ; // for now
}

std::string G::DirectoryIteratorImp::sizeString() const
{
	std::string s = G::File::sizeString( filePath() ) ;
	return s.empty() ? std::string("0") : s ;
}

