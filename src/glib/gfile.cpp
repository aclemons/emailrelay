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
// gfile.cpp
//
	
#include "gdef.h"
#include "glimits.h"
#include "gfile.h"
#include "gprocess.h"
#include "glog.h"
#include <iostream>
#include <cstdio>

bool G::File::remove( const Path & path , const G::File::NoThrow & )
{
	bool rc = 0 == std::remove( path.str().c_str() ) ;
	G_DEBUG( "G::File::remove: \"" << path << "\": success=" << rc ) ;
	return rc ;
}

void G::File::remove( const Path & path )
{
	if( 0 != std::remove( path.str().c_str() ) )
	{
		//int error = G::Process::errno_() ;
		throw CannotRemove( path.str() ) ;
	}
	G_DEBUG( "G::File::remove: \"" << path << "\"" ) ;
}

bool G::File::rename( const Path & from , const Path & to , const NoThrow & )
{
	bool rc = 0 == std::rename( from.str().c_str() , to.str().c_str() ) ;
	G_DEBUG( "G::File::rename: \"" << from << "\" -> \"" << to << "\": success=" << rc ) ;
	return rc ;
}

void G::File::rename( const Path & from , const Path & to )
{
	if( 0 != std::rename( from.str().c_str() , to.str().c_str() ) )
	{
		//int error = G::Process::errno_() ;
		throw CannotRename( std::string() + "[" + from.str() + "] to [" + to.str() + "]" ) ;
	}
	G_DEBUG( "G::File::rename: \"" << from << "\" -> \"" << to << "\"" ) ;
}

void G::File::copy( const Path & from , const Path & to )
{
	std::string reason = copy( from , to , 0 ) ;
	if( !reason.empty() )
		throw CannotCopy( std::string() + "[" + from.str() + "] to [" + to.str() + "]: " + reason ) ;
}

bool G::File::copy( const Path & from , const Path & to , const NoThrow & )
{
	return copy(from,to,0).empty() ;
}

std::string G::File::copy( const Path & from , const Path & to , int )
{
	std::ifstream in( from.str().c_str() , std::ios::binary | std::ios::in ) ;
	if( !in.good() )
		return "cannot open input file" ;

	std::ofstream out( to.str().c_str() , std::ios::binary | std::ios::out | std::ios::trunc ) ;
	if( !out.good() )
		return "cannot open output file" ;

	out << in.rdbuf() ;

	if( in.fail() || in.bad() )
		return "read error" ;

	if( !out.good() )
		return "write error" ;

	in.close() ;
	out.close() ;
	if( sizeString(from) != sizeString(to) )
		return "file size mismatch" ;

	return std::string() ;
}

void G::File::copy( std::istream & in , std::ostream & out , std::streamsize limit , std::string::size_type block )
{
	block = block ? block : static_cast<std::string::size_type>(limits::file_buffer) ;
	std::vector<char> buffer ;
	buffer.reserve( block ) ;

	const std::streamsize b = static_cast<std::streamsize>(block) ;
	std::streamsize size = 0U ;
	while( ( limit == 0U || size < limit ) && in.good() && out.good() )
	{
		std::streamsize request = limit == 0U || (limit-size) > b ? b : (limit-size) ;
		in.read( &buffer[0] , request ) ;
		std::streamsize result = in.gcount() ;
		if( result == 0U ) 
			break ;
		out.write( &buffer[0] , result ) ;
		size += result ;
	}

	out.flush() ;
	in.clear( in.rdstate() & ~std::ios_base::failbit ) ; // failbit set at eof so not useful
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
	bool rc = exists( path.str().c_str() , enoent ) ; // o/s-specific
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

void G::File::create( const Path & path )
{
	std::ofstream f( path.str().c_str() ) ;
	f.close() ;
	if( !exists(path) ) // race
		throw CannotCreate( path.str() ) ;
}

/// \file gfile.cpp
