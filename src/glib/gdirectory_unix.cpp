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
// gdirectory_unix.cpp
//

#include "gdef.h"
#include "gdirectory.h"
#include "gprocess.h"
#include "gdatetime.h"
#include "gdebug.h"
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
		if( p == NULL )
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

std::string G::Directory::tmp()
{
	std::ostringstream ss ;
	ss << "." << DateTime::now() << "." << Process::Id() << ".tmp" ;
	return ss.str() ;
}

bool G::Directory::writeable( std::string tmp_filename ) const
{
	G::Path test_file( m_path ) ;
	if( tmp_filename.empty() ) tmp_filename = tmp() ;
	test_file.pathAppend( tmp_filename ) ;

	int fd = ::open( test_file.str().c_str() , O_WRONLY | O_CREAT | O_EXCL , S_IRWXU ) ;
	if( fd == -1 )
	{
		return false ;
	}
	::close( fd ) ;
	bool ok = 0 == ::unlink( test_file.str().c_str() ) ;
	return ok ;
}

/// \file gdirectory_unix.cpp
