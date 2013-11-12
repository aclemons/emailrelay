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
// gdirectory_unix_glob.cpp
//

#include "gdef.h"
#include "gdirectory.h"
#include "gfile.h"
#include "gdebug.h"
#include "glog.h"
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <glob.h>
#include <fcntl.h>

namespace G
{
	class DirectoryIteratorImp ;
}

// ===

/// \class G::DirectoryIteratorImp
/// A pimple-pattern implementation class for DirectoryIterator.
/// 
class G::DirectoryIteratorImp 
{
private:
	glob_t m_glob ;
	Directory m_dir ;
	bool m_first ;
	size_t m_index ; // 'int' on some systems
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

G::DirectoryIterator::DirectoryIterator( const Directory & dir ) :
	m_imp( new DirectoryIteratorImp(dir,std::string()) )
{
}

G::DirectoryIterator::DirectoryIterator( const Directory & dir , const std::string & wc ) :
	m_imp( new DirectoryIteratorImp(dir,wc) )
{
	// note that this overload is rarely used -- if glob() is 
	// unavailable and this overload is never called then the
	// DirectoryIteratorImp class can be rewritten with
	// just a default constructor that uses readdir()
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

extern "C"
{
	int gdirectory_unix_on_error_( const char * , int ) ;
	int gdirectory_unix_on_error_( const char * , int )
	{
		const int abort = 1 ;
		return abort ;
	}
}

G::DirectoryIteratorImp::DirectoryIteratorImp( const Directory & dir , const std::string & wildcard ) :
	m_dir(dir) ,
	m_first(true) ,
	m_index(0) ,
	m_error(false)
{
	m_glob.gl_pathc = 0 ;
	m_glob.gl_pathv = NULL ;

	Path wild_path( dir.path() , wildcard.empty() ? std::string("*") : wildcard ) ;

	int flags = 0 | GLOB_ERR ;
	int error = ::glob( wild_path.str().c_str() , flags , gdirectory_unix_on_error_ , &m_glob ) ;
	if( error || m_glob.gl_pathv == NULL )
		m_error = true ;
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

	if( m_index >= static_cast<size_t>(m_glob.gl_pathc) )
		return false ;

	return true ;
}

G::Path G::DirectoryIteratorImp::filePath() const
{
	const bool sane =
		m_index < static_cast<size_t>(m_glob.gl_pathc) &&
		m_glob.gl_pathv != NULL &&
		m_glob.gl_pathv[m_index] != NULL ;
	const char * file_path = sane ? m_glob.gl_pathv[m_index] : "" ;
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

