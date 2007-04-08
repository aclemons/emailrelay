//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
///
/// \file package.h
///

#ifndef PACKAGE_H__
#define PACKAGE_H__

#include "unpack.h"
#include "gpath.h"
#include <string>
#include <stdexcept>

/// \class Package
/// A simple inline 'c++' wrapper to the 'c' unpack interface.
///
class Package 
{
public:
	explicit Package( G::Path exe ) ;
		///< Constructor.

	~Package() ;
		///< Destructor.

	int count() const ;
		///< Returns the number of packaged files.

	std::string name( int i ) ;
		///< Returns the i'th file name or relative path.

	void unpack( G::Path base_dir , const std::string & name ) ;
		///< Unpacks the specified file. The target directory
		///< (not just the base directory) must exist.

private:
	Package( const Package & ) ;
	void operator=( const Package & ) ;

private:
	Unpack * m_p ;
} ;

inline
Package::Package( G::Path exe )
{
	m_p = unpack_new( exe.str().c_str() , 0 ) ;
}

inline
Package::~Package()
{
	unpack_delete( m_p ) ;
}

inline
int Package::count() const
{
	return unpack_count(m_p) ;
}

inline
std::string Package::name( int i )
{
	char * p = unpack_name(m_p,i) ;
	std::string s( p ) ;
	unpack_free( p ) ;
	return s ;
}

inline
void Package::unpack( G::Path base_dir , const std::string & name )
{
	bool ok = !! unpack_file( m_p , base_dir.str().c_str() , name.c_str() ) ;
	if( ! ok )
		throw std::runtime_error( std::string() + "cannot unpack \"" + name + "\" into \"" + base_dir.str() + "\"" ) ;
}

#endif
