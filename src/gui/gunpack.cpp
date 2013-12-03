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
// gunpack.cpp
//

#include "gdef.h"
#include "gunpack.h"
#include "gfile.h"
#include "gstr.h"
#include "glog.h"
#include <fstream>
#include <stdexcept>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <list>

static std::list<std::string> error_stack ;
struct ErrorHandler 
{ 
	ErrorHandler()
	{
		error_stack.push_back( std::string() ) ;
	}
	~ErrorHandler()
	{
		error_stack.pop_back() ;
	}
	std::string result() const
	{
		return error_stack.back() ;
	}
	std::string error() const
	{
		return result().empty() ? std::string("error") : result() ;
	}
	void throw_()
	{
		throw G::Unpack::PackingError(result()) ;
	}
	void check()
	{
		if( !error_stack.back().empty() )
			throw G::Unpack::PackingError( error_stack.back() ) ;
	}
} ;
static void error_handler( const char * e )
{
	if( !error_stack.empty() && e )
		error_stack.back() = std::string( e ) ;
}

// ==

bool G::Unpack::isPacked( Path path )
{
	ErrorHandler eh ;
	::Unpack * m = unpack_new( path.str().c_str() , error_handler ) ;
	if( m )
	{
		unpack_delete( m ) ;
		return true ;
	}
	else
	{
		return false ;
	}
}

int G::Unpack::fileCount( Path path )
{
	int n = 0 ;
	ErrorHandler eh ;
	::Unpack * m = unpack_new( path.str().c_str() , error_handler ) ;
	if( m )
	{
		n = unpack_count( m ) ;
		unpack_delete( m ) ;
	}
	return n ;
}

G::Unpack::Unpack( Path path ) :
	m_path(path) ,
	m_imp(NULL)
{
	ErrorHandler eh ;
	m_imp = unpack_new( path.str().c_str() , error_handler ) ;
	if( m_imp == NULL )
		eh.throw_() ;
}

G::Unpack::Unpack( Path path , NoThrow ) :
	m_path(path) ,
	m_imp(NULL)
{
	ErrorHandler eh ;
	m_imp = unpack_new( path.str().c_str() , error_handler ) ;
}

G::Unpack::~Unpack()
{
	if( m_imp != NULL )
		unpack_delete( m_imp ) ;
}

G::Path G::Unpack::path() const
{
	return m_path ;
}

G::Strings G::Unpack::names() const
{
	Strings result ;
	ErrorHandler eh ;
	if( m_imp != NULL )
	{
		int n = unpack_count( m_imp ) ;
		for( int i = 0 ; i < n ; i++ )
		{
			char * p = unpack_name( m_imp , i ) ;
			result.push_back( std::string(p) ) ;
			unpack_free( p ) ;
		}
	}
	eh.check() ;
	return result ;
}

std::string G::Unpack::flags( const std::string & name ) const
{
	std::string result ;
	ErrorHandler eh ;
	if( m_imp != NULL )
	{
		bool done = false ;
		int n = unpack_count( m_imp ) ;
		for( int i = 0 ; i < n && !done ; i++ )
		{
			char * p = unpack_name( m_imp , i ) ;
			if( p != NULL && name == std::string(p) )
			{
				done = true ;
				char * f = unpack_flags( m_imp , i ) ;
				if( f != NULL )
					result = std::string( f ) ;
				unpack_free( f ) ;
			}
			unpack_free( p ) ;
		}
		if( !done )
			throw NoSuchFile( name ) ;
	}
	eh.check() ;
	return result ;
}

void G::Unpack::unpack( const Path & to_dir )
{
	if( m_imp != NULL )
	{
		ErrorHandler eh ;
		if( ! unpack_all( m_imp , to_dir.str().c_str() ) )
			eh.throw_() ;
	}
}

void G::Unpack::unpack( const Path & to_dir , const std::string & name )
{
	if( m_imp != NULL )
	{
		ErrorHandler eh ;
		flags( name ) ; // check the name and throw NoSuchFile
		if( ! unpack_file( m_imp , to_dir.str().c_str() , name.c_str() ) )
			eh.throw_() ;
	}
}

void G::Unpack::unpack( const std::string & name , const Path & dst )
{
	if( m_imp != NULL )
	{
		ErrorHandler eh ;
		flags( name ) ; // check the name and throw NoSuchFile
		if( ! unpack_file_to( m_imp , name.c_str() , dst.str().c_str() ) )
			eh.throw_() ;
	}
}

void G::Unpack::unpackOriginal( const Path & dst )
{
	ErrorHandler eh ;
	unsigned long original_size = unpack_original_size( m_imp ) ;
	bool is_packed = names().size() != 0U ;
	if( original_size != 0UL && is_packed )
	{
		if( ! unpack_original_file( m_imp , dst.str().c_str() ) )
			eh.throw_() ;
	}
	eh.check() ;
}

std::string G::Unpack::unpackOriginal( const Path & dst , NoThrow )
{
	ErrorHandler eh ;
	unsigned long original_size = unpack_original_size( m_imp ) ;
	bool is_packed = names().size() != 0U ;
	if( original_size != 0UL && is_packed )
	{
		if( ! unpack_original_file( m_imp , dst.str().c_str() ) )
			eh.throw_() ;
	}
	return eh.result() ;
}

/// \file gunpack.cpp
