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
// gdirectory_unix_noglob.cpp
//

#include "gdef.h"
#include "gdirectory.h"
#include "gfile.h"
#include "gdebug.h"
#include "glog.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <dirent.h>
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
	DIR * m_d ;
	struct dirent * m_dp ;
	Directory m_dir ;
	bool m_error ;

public:
	explicit DirectoryIteratorImp( const Directory & dir ) ;
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

G::DirectoryIteratorImp::DirectoryIteratorImp( const Directory & dir ) :
	m_d(NULL) ,
	m_dp(NULL) ,
	m_dir(dir) ,
	m_error(true)
{
	m_d = ::opendir( dir.path().str().c_str() ) ;
	m_error = m_d == NULL ;
}

bool G::DirectoryIteratorImp::error() const
{
	return m_error ;
}

bool G::DirectoryIteratorImp::more()
{
	if( !m_error )
	{
		m_dp = ::readdir( m_d ) ;
		m_error = m_dp == NULL ;
	}
	return !m_error ;
}

G::Path G::DirectoryIteratorImp::filePath() const
{
	return m_dir.path() + fileName().str() ;
}

G::Path G::DirectoryIteratorImp::fileName() const
{
	return Path( m_dp == NULL ? "" : m_dp->d_name ) ;
}

bool G::DirectoryIteratorImp::isDir() const
{
	struct stat statbuf ;
	return ::stat( filePath().str().c_str() , &statbuf ) == 0 && (statbuf.st_mode & S_IFDIR) ;
}

G::DirectoryIteratorImp::~DirectoryIteratorImp() 
{
	if( m_d != NULL )
		::closedir( m_d ) ;
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

