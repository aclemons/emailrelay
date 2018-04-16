//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
		throw CannotRemove( path.str() ) ;
	}
	G_DEBUG( "G::File::remove: \"" << path << "\"" ) ;
}

bool G::File::rename( const Path & from , const Path & to , const NoThrow & )
{
	bool is_missing = false ;
	bool ok = rename( from.str().c_str() , to.str().c_str() , is_missing ) ;
	G_DEBUG( "G::File::rename: \"" << from << "\" -> \"" << to << "\": success=" << ok ) ;
	return ok ;
}

void G::File::rename( const Path & from , const Path & to , bool ignore_missing )
{
	bool is_missing = false ;
	bool ok = rename( from.str().c_str() , to.str().c_str() , is_missing ) ;
	if( !ok && !(is_missing && ignore_missing) )
	{
		throw CannotRename( std::string() + "[" + from.str() + "] to [" + to.str() + "]" ) ;
	}
	G_DEBUG( "G::File::rename: \"" << from << "\" -> \"" << to << "\": success=" << ok ) ;
}

bool G::File::rename( const char * from , const char * to , bool & enoent )
{
	bool ok = 0 == std::rename( from , to ) ;
	int error = G::Process::errno_() ;
	enoent = !ok && error == ENOENT ;
	return ok ;
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

	if( in.fail() )
		return "read error" ;

	in.close() ;
	out.close() ;

	if( out.fail() )
		return "write error" ;

	return std::string() ;
}

void G::File::copy( std::istream & in , std::ostream & out , std::streamsize limit , std::string::size_type block )
{
	std::ios_base::iostate in_state = in.rdstate() ;

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

	// restore the input failbit because it might have been set by us reading an incomplete block at eof
	in.clear( (in.rdstate() & ~std::ios_base::failbit) | (in_state & std::ios_base::failbit) ) ;
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
	bool eaccess = false ;
	bool rc = exists( path.str().c_str() , enoent , eaccess ) ; // o/s-specific implementation
	if( !rc && enoent )
	{
		return false ;
	}
	else if( !rc && do_throw )
	{
		throw StatError( path.str() , eaccess?"permission denied":"" ) ;
	}
	else if( !rc )
	{
		return on_error ;
	}
	return true ;
}

bool G::File::chmodx( const Path & path , const NoThrow & )
{
	return chmodx( path , false ) ;
}

void G::File::chmodx( const Path & path )
{
	chmodx( path , true ) ;
}

bool G::File::mkdirs( const Path & path , const NoThrow & , int limit )
{
	// (recursive)

	G_DEBUG( "File::mkdirs: " << path ) ;
	if( limit == 0 )
		return false ;

	// use a trial mkdir() on the our way towards the root to avoid
	// problems with the windows "virtual store" mis-feature
	const bool mkdir_trial = true ;
	if( mkdir_trial )
	{
		if( mkdir(path,NoThrow()) )
		{
			G_DEBUG( "File::mkdirs: mkdir(" << path << ") -> ok" ) ;
			chmodx( path , NoThrow() ) ;
			return true ;
		}
	}

	if( exists(path) )
		return true ;

	if( path.str().empty() )
		return true ;

	if( ! mkdirs( path.dirname() , NoThrow() , limit-1 ) ) // (recursion)
		return false ;

	G_DEBUG( "File::mkdirs: mkdir(" << path << ")" ) ;
	bool ok = mkdir( path , NoThrow() ) ;
	if( ok )
		chmodx( path , NoThrow() ) ;

	G_DEBUG( "File::mkdirs: mkdir(" << path << ") -> " << (ok?"ok":"failed") ) ;
	return ok ;
}

void G::File::mkdirs( const Path & path , int limit )
{
	if( ! mkdirs(path,NoThrow(),limit) )
		throw CannotMkdir(path.str()) ;
}

void G::File::create( const Path & path )
{
	std::ofstream f( path.str().c_str() , std::ios_base::out | std::ios_base::app ) ;
	f.close() ;
	if( !exists(path) ) // race
		throw CannotCreate( path.str() ) ;
}

/// \file gfile.cpp
