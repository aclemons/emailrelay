//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gdirectory_unix.cpp
///

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

//| \class G::DirectoryIteratorImp
/// A pimple-pattern implementation class for DirectoryIterator using
/// opendir()/readdir().
///
class G::DirectoryIteratorImp
{
public:
	explicit DirectoryIteratorImp( const Directory & dir ) ;
	~DirectoryIteratorImp() ;
	bool isDir() const ;
	bool isLink() const ;
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
	DIR * m_d ;
	struct dirent * m_dp ;
	Directory m_dir ;
	bool m_error ;
	bool m_is_dir ;
	bool m_is_link ;
} ;

//

int G::Directory::usable( bool for_creation ) const
{
	if( m_path.empty() )
		return ENOTDIR ;

	// use opendir("foo/.") rather than opendir("foo") to verify
	// that any contained files can be stat()ed -- ie. that all
	// directory parts in the m_path have '--x'
	std::string path_dot = m_path.str() + (m_path.str()=="/"?"":"/") + "." ;

	DIR * p = ::opendir( path_dot.c_str() ) ;
	int error = p == nullptr ? Process::errno_() : 0 ;
	if( p != nullptr )
		::closedir( p ) ;

	if( error == 0 && for_creation )
	{
		// (not definitive -- see also GNU/Linux ::euidaccess())
		int rc = ::access( m_path.cstr() , W_OK ) ;
		error = rc == 0 ? 0 : Process::errno_() ;
	}
	return error ;
}

bool G::Directory::writeable( const std::string & filename ) const
{
	Path path( m_path , filename.empty() ? tmp() : filename ) ;
	return File::probe( path.cstr() ) ;
}

// ===

G::DirectoryIterator::DirectoryIterator( const Directory & dir ) :
	m_imp(std::make_unique<DirectoryIteratorImp>(dir))
{
}

G::DirectoryIterator::~DirectoryIterator()
= default ;

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

bool G::DirectoryIterator::isLink() const
{
	return m_imp->isLink() ;
}

bool G::DirectoryIterator::isDir() const
{
	return m_imp->isDir() ;
}

#ifndef G_LIB_SMALL
std::string G::DirectoryIterator::sizeString() const
{
	return m_imp->sizeString() ;
}
#endif

// ===

G::DirectoryIteratorImp::DirectoryIteratorImp( const Directory & dir ) :
	m_d(nullptr) ,
	m_dp(nullptr) ,
	m_dir(dir) ,
	m_error(true) ,
	m_is_dir(false) ,
	m_is_link(false)
{
	m_d = ::opendir( dir.path().cstr() ) ;
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
		if( m_error )
			break ;

		static_assert( sizeof(dirent::d_name) > 2U , "" ) ;
		bool special = m_dp->d_name[0] == '.' && (
			( m_dp->d_name[1] == '\0' ) ||
			( m_dp->d_name[1] == '.' && m_dp->d_name[2] == '\0' ) ) ;

		if( special )
		{
			m_is_dir = true ;
			m_is_link = false ;
		}
		#if GCONFIG_HAVE_DIRENT_D_TYPE
		else if( m_dp->d_type != DT_UNKNOWN )
		{
			m_is_dir = m_dp->d_type == DT_DIR ;
			m_is_link = m_dp->d_type == DT_LNK ;
			if( m_is_link )
				m_is_dir = File::isDirectory(filePath(),std::nothrow) ;
		}
		#endif
		else
		{
			m_is_dir = File::isDirectory(filePath(),std::nothrow) ;
			m_is_link = File::isLink(filePath(),std::nothrow) ;
		}
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
	return m_is_dir ;
}

bool G::DirectoryIteratorImp::isLink() const
{
	return m_is_link ;
}

G::DirectoryIteratorImp::~DirectoryIteratorImp()
{
	if( m_d != nullptr )
		::closedir( m_d ) ;
}

std::string G::DirectoryIteratorImp::sizeString() const
{
	std::string s = File::sizeString( filePath() ) ;
	return s.empty() ? std::string(1U,'0') : s ;
}

