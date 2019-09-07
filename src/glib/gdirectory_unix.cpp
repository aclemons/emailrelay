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
// gdirectory_unix.cpp
//

#include "gdef.h"
#include "gdirectory.h"
#include "gprocess.h"
#include "gdatetime.h"
#include "gfile.h"
#include "glog.h"
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>

namespace G
{
	class DirectoryIteratorImp ;
}

/// \class G::DirectoryIteratorImp
/// A pimple-pattern implementation class for DirectoryIterator using
/// opendir()/readdir().
///
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

private:
	DirectoryIteratorImp( const DirectoryIteratorImp & ) g__eq_delete ;
	void operator=( const DirectoryIteratorImp & ) g__eq_delete ;

private:
	DIR * m_d ;
	struct dirent * m_dp ;
	Directory m_dir ;
	bool m_error ;
} ;

//

bool G::Directory::valid( bool for_creation ) const
{
	bool rc = true ;
	struct stat statbuf ;
	if( ::stat( m_path.str().c_str() , &statbuf ) )
	{
		rc = false ; // doesnt exist
	}
	else if( !(statbuf.st_mode & S_IFDIR) )
	{
		rc = false ; // not a directory
	}
	else
	{
		DIR * p = ::opendir( m_path.str().c_str() ) ;
		if( p == nullptr )
			rc = false ; // cant open directory for reading
		else
			::closedir( p ) ;
	}

	if( rc && for_creation )
	{
		// (not definitive -- see also GNU/Linux ::euidaccess())
		if( 0 != ::access( m_path.str().c_str() , W_OK ) )
			rc = false ;
	}
	return rc ;
}

bool G::Directory::writeable( std::string filename ) const
{
	// use open(2) so we can use O_EXCL, ie. fail if it already exists
	Path path( m_path , filename.empty() ? tmp() : filename ) ;
	int fd = ::open( path.str().c_str() , O_WRONLY | O_CREAT | O_EXCL , S_IRWXU ) ;
	if( fd == -1 )
		return false ;

	::close( fd ) ;
	return 0 == std::remove( path.str().c_str() ) ;
}

// ===

G::DirectoryIterator::DirectoryIterator( const Directory & dir ) :
	m_imp( new DirectoryIteratorImp(dir) )
{
}

G::DirectoryIterator::~DirectoryIterator()
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

// ===

G::DirectoryIteratorImp::DirectoryIteratorImp( const Directory & dir ) :
	m_d(nullptr) ,
	m_dp(nullptr) ,
	m_dir(dir) ,
	m_error(true)
{
	m_d = ::opendir( dir.path().str().c_str() ) ;
	m_error = m_d == nullptr ;
}

bool G::DirectoryIteratorImp::error() const
{
	return m_error ;
}

bool G::DirectoryIteratorImp::more()
{
	while( !m_error )
	{
		m_dp = ::readdir( m_d ) ;
		m_error = m_dp == nullptr ;
		bool special = !m_error && ( fileName() == "." || fileName() == ".." ) ;
		if( !special )
			break ;
	}
	return !m_error ;
}

G::Path G::DirectoryIteratorImp::filePath() const
{
	return m_dir.path() + fileName() ;
}

std::string G::DirectoryIteratorImp::fileName() const
{
	return m_dp == nullptr ? std::string() : std::string(m_dp->d_name) ;
}

bool G::DirectoryIteratorImp::isDir() const
{
	struct stat statbuf ;
	return ::stat( filePath().str().c_str() , &statbuf ) == 0 && (statbuf.st_mode & S_IFDIR) ;
}

G::DirectoryIteratorImp::~DirectoryIteratorImp()
{
	if( m_d != nullptr )
		::closedir( m_d ) ;
}

std::string G::DirectoryIteratorImp::sizeString() const
{
	std::string s = File::sizeString( filePath() ) ;
	return s.empty() ? std::string("0") : s ;
}

