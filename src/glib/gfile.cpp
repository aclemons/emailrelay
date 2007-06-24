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
//
// gfile.cpp
//
	
#include "gdef.h"
#include "gfile.h"
#include "gprocess.h"
#include "glog.h"
#include <iostream>
#include <cstdio>

bool G::File::remove( const Path & path , const G::File::NoThrow & )
{
	bool rc = 0 == std::remove( path.pathCstr() ) ;
	G_DEBUG( "G::File::remove: \"" << path << "\": success=" << rc ) ;
	return rc ;
}

void G::File::remove( const Path & path )
{
	if( 0 != std::remove( path.pathCstr() ) )
	{
		//int error = G::Process::errno_() ;
		throw CannotRemove( path.str() ) ;
	}
	G_DEBUG( "G::File::remove: \"" << path << "\"" ) ;
}

bool G::File::rename( const Path & from , const Path & to , const NoThrow & )
{
	bool rc = 0 == std::rename( from.pathCstr() , to.pathCstr() ) ;
	G_DEBUG( "G::File::rename: \"" << from << "\" -> \"" << to << "\": success=" << rc ) ;
	return rc ;
}

void G::File::rename( const Path & from , const Path & to )
{
	if( 0 != std::rename( from.pathCstr() , to.pathCstr() ) )
	{
		//int error = G::Process::errno_() ;
		throw CannotRename( from.str() ) ;
	}
	G_DEBUG( "G::File::rename: \"" << from << "\" -> \"" << to << "\"" ) ;
}

void G::File::copy( const Path & from , const Path & to )
{
	if( !copy(from,to,NoThrow()) )
		throw CannotCopy( from.str() ) ;
}

bool G::File::copy( const Path & from , const Path & to , const NoThrow & )
{
	std::ifstream in( from.str().c_str() , std::ios::binary | std::ios::in ) ;
	if( !in.good() )
		return false ;

	std::ofstream out( to.str().c_str() , std::ios::binary | std::ios::out | std::ios::trunc ) ;
	if( !out.good() )
		return false ;

	copy( from , to ) ;
	out.flush() ;

	return !in.fail() && !in.bad() && out.good() ;
}

void G::File::copy( std::istream & in , std::ostream & out , std::streamsize limit , std::string::size_type block )
{
	// cf. "out<<in.rdbuf()"

	block = block ? block : 102400U ;
	std::vector<char> buffer ;
	buffer.reserve( block ) ;

	const std::streamsize b = static_cast<std::streamsize>(block) ;
	std::streamsize size = 0U ;
	while( size < limit && in.good() && out.good() )
	{
		std::streamsize request = limit == 0U || (limit-size) > b ? b : (limit-size) ;
		std::streamsize result = in.readsome( &buffer[0] , request ) ;
		if( result == 0U ) break ;
		out.write( &buffer[0] , result ) ;
		size += result ;
	}
}

void G::File::mkdir( const Path & dir )
{
	if( ! mkdir( dir , NoThrow() ) )
		throw CannotMkdir( dir.str() ) ;
}

bool G::File::exists( const Path & path )
{
	return exists( path , false , true ) ;
}

bool G::File::exists( const Path & path , const NoThrow & )
{
	return exists( path , false , false ) ;
}

bool G::File::exists( const Path & path , bool on_error , bool do_throw )
{
	bool enoent = false ;
	bool rc = exists( path.pathCstr() , enoent ) ; // o/s-specific
	if( !rc && enoent )
	{
		return false ;
	}
	else if( !rc && do_throw )
	{
		throw StatError( path.str() ) ;
	}
	else if( !rc )
	{
		return on_error ;
	}
	return true ;
}

bool G::File::chmodx( const Path & path , const NoThrow & )
{
	return chmodx(path,false) ;
}

void G::File::chmodx( const Path & path )
{
	chmodx(path,true) ;
}

bool G::File::mkdirs( const Path & path , const NoThrow & , int limit )
{
	// (recursive)
	G_DEBUG( "File::mkdirs: " << path ) ;
	if( limit == 0 ) return false ;
	if( exists(path) ) return true ;
	if( path.str().empty() ) return true ;
	if( ! mkdirs( path.dirname() , NoThrow() , limit-1 ) ) return false ;
	bool ok = mkdir( path , NoThrow() ) ;
	if( ok ) chmodx( path , NoThrow() ) ;
	return ok ;
}

void G::File::mkdirs( const Path & path , int limit )
{
	if( ! mkdirs(path,NoThrow(),limit) )
		throw CannotMkdir(path.str()) ;
}

/// \file gfile.cpp
